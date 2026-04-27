using Godot;
using System;
using System.Linq;
using DarkAges.Entities;
using DarkAges.Networking;

namespace DarkAges.Client.UI
{
    /// <summary>
    /// [CLIENT_AGENT] WP-7-4 Target lock-on system with soft-lock and 100m range checking.
    /// </summary>
    public partial class TargetLockSystem : Node
    {
        [Export] public float MaxLockRange = 100.0f;  // P0 requirement: 100m range
        [Export] public float LockAngle = 30.0f;  // Degrees from center for lock-on
        [Export] public float Stickiness = 0.5f;  // How long to keep lock after leaving cone
        [Export] public bool ShowDebugInfo = false;
        
        public uint? CurrentTarget { get; private set; }
        public RemotePlayer TargetPlayer { get; private set; }
        
        // Visual feedback
        private MeshInstance3D _targetReticle;
        private Label3D _targetNameLabel;
        private ProgressBar _targetHealthBar;
        private Panel _targetInfoPanel;
        
        private double _timeSinceLastTargetCheck = 0;
        private const double TargetCheckInterval = 0.1;  // 10Hz check
        private double _timeSinceTargetLost = 0;
        private bool _wasTargetValid = false;
        
        // Target health tracking for UI
        private float _targetCurrentHealth = 100;
        private float _targetMaxHealth = 100;
        private bool _lockRequestPending = false;
        private uint? _pendingTarget = null;
        private uint _lastLockAttemptTimeMs = 0;  // For client-side rate-limit mirroring
        private const uint MIN_LOCK_INTERVAL_MS = 1000;  // Match server rate-limit

        public override void _Ready()
        {
            CreateTargetReticle();
            CreateTargetInfoPanel();
            
            // Subscribe to events
            NetworkManager.Instance.EntityStateReceived += OnEntityStateReceived;
            Combat.CombatEventSystem.Instance.EntityDied += OnEntityDied;
            NetworkManager.Instance.LockOnConfirmed += OnLockOnConfirmed;
            NetworkManager.Instance.LockOnFailed += OnLockOnFailed;
        }

        public override void _ExitTree()
        {
            if (NetworkManager.Instance != null)
            {
                NetworkManager.Instance.EntityStateReceived -= OnEntityStateReceived;
            }
            if (NetworkManager.Instance != null)
            {
                NetworkManager.Instance.LockOnConfirmed -= OnLockOnConfirmed;
                NetworkManager.Instance.LockOnFailed -= OnLockOnFailed;
            }
            if (Combat.CombatEventSystem.Instance != null)
            {
                Combat.CombatEventSystem.Instance.EntityDied -= OnEntityDied;
            }
        }

        /// <summary>
        /// Handler: server confirmed lock-on. Apply target state.
        /// </summary>
        private void OnLockOnConfirmed(uint targetEntity)
        {
            _lockRequestPending = false;
            _pendingTarget = null;
            
            // Look up the remote player for this entity
            var remoteManager = RemotePlayerManager.Instance;
            if (remoteManager == null)
            {
                GD.PrintErr("[TargetLockSystem] OnLockOnConfirmed: RemotePlayerManager not available");
                return;
            }
            
            var targetPlayer = remoteManager.GetAllPlayers()
                .FirstOrDefault(p => IsInstanceValid(p) && p.EntityId == targetEntity);
            
            if (targetPlayer == null)
            {
                GD.PrintErr($"[TargetLockSystem] OnLockOnConfirmed: Entity {targetEntity} not found in remote players");
                return;
            }
            
            SetTarget(targetPlayer);
            GD.Print($"[TargetLockSystem] Lock-on confirmed for {targetPlayer.PlayerName} ({targetEntity})");
        }

        /// <summary>
        /// Handler: server rejected lock-on. Clear pending state, optional feedback.
        /// </summary>
        private void OnLockOnFailed(uint targetEntity, byte reason)
        {
            _lockRequestPending = false;
            _pendingTarget = null;
            
            string reasonStr = reason switch
            {
                0 => "OUT_OF_RANGE",
                1 => "NOT_VISIBLE",  
                2 => "NOT_ALIVE",
                3 => "INVALID_TARGET",
                4 => "BUSY (rate-limited)",
                _ => $"UNKNOWN({reason})"
            };
            
            GD.Print($"[TargetLockSystem] Lock-on failed for entity {targetEntity}: {reasonStr}");
            
            // Optional: brief UI feedback for rate-limit
            if (reason == 4) // BUSY
            {
                // Could flash reticle red or show toast; defer to UI layer
                // For now, diagnostic only
            }
        }

        private void CreateTargetReticle()
        {
            _targetReticle = new MeshInstance3D
            {
                Name = "TargetReticle",
                Visible = false
            };
            
            // Create torus mesh for reticle
            var torus = new TorusMesh
            {
                InnerRadius = 0.8f,
                OuterRadius = 1.0f,
                Rings = 16
            };
            _targetReticle.Mesh = torus;
            
            var material = new StandardMaterial3D
            {
                AlbedoColor = Colors.Red,
                Emission = Colors.Red,
                EmissionEnergyMultiplier = 2.0f,
                Transparency = BaseMaterial3D.TransparencyEnum.Alpha,
                NoDepthTest = true
            };
            _targetReticle.MaterialOverride = material;
            
            GetTree().Root.AddChild(_targetReticle);
        }
        
        private void CreateTargetInfoPanel()
        {
            // Create UI panel for target info (will be positioned in _Process)
            _targetInfoPanel = new Panel
            {
                Name = "TargetInfoPanel",
                Visible = false,
                CustomMinimumSize = new Vector2(200, 60)
            };
            
            // Name label
            _targetNameLabel = new Label3D
            {
                Name = "TargetName",
                Billboard = BaseMaterial3D.BillboardModeEnum.Enabled,
                PixelSize = 0.01f,
                FontSize = 24,
                Modulate = Colors.White
            };
            
            GetTree().Root.AddChild(_targetInfoPanel);
        }

        public override void _Process(double delta)
        {
            _timeSinceLastTargetCheck += delta;
            
            if (_timeSinceLastTargetCheck >= TargetCheckInterval)
            {
                _timeSinceLastTargetCheck = 0;
                UpdateTargetLock();
            }
            
            // Update reticle position
            UpdateReticle();
            
            // Update target info UI
            UpdateTargetInfo();
            
            // Check for manual target selection inputs
            if (Input.IsActionJustPressed("target_next"))
            {
                CycleTarget();
            }
            
            if (Input.IsActionJustPressed("target_lock"))
            {
                ToggleTargetLock();
            }
            
            if (Input.IsActionJustPressed("target_clear"))
            {
                ClearTarget();
            }
        }
        
        private void UpdateTargetLock()
        {
            if (CurrentTarget.HasValue)
            {
                // Check if target is still valid
                if (!IsTargetValid(CurrentTarget.Value))
                {
                    _timeSinceTargetLost += TargetCheckInterval;
                    
                    // Apply stickiness - keep lock for a short time after leaving range/angle
                    if (_timeSinceTargetLost > Stickiness)
                    {
                        ClearTarget();
                    }
                    _wasTargetValid = false;
                }
                else
                {
                    _timeSinceTargetLost = 0;
                    _wasTargetValid = true;
                }
            }
            else
            {
                // Only auto-acquire if not waiting for server confirmation
                if (!_lockRequestPending)
                {
                    TryAutoTarget();
                }
            }
        }
        
        /// <summary>
        /// Try to automatically acquire target under crosshair
        /// </summary>
        private void TryAutoTarget()
        {
            var localPlayer = GetTree().GetNodesInGroup("local_player").FirstOrDefault() as Node3D;
            if (localPlayer == null) return;
            
            var camera = GetViewport().GetCamera3D();
            if (camera == null) return;
            
            var remoteManager = RemotePlayerManager.Instance;
            if (remoteManager == null) return;
            
            RemotePlayer bestTarget = null;
            float bestScore = float.MaxValue;
            
            foreach (var player in remoteManager.GetAllPlayers())
            {
                if (!IsInstanceValid(player)) continue;
                
                // Check range (P0 requirement: 100m)
                float distance = localPlayer.GlobalPosition.DistanceTo(player.GlobalPosition);
                if (distance > MaxLockRange) continue;
                
                // Check if target is in front of player
                Vector3 toTarget = (player.GlobalPosition - localPlayer.GlobalPosition).Normalized();
                float dot = camera.GlobalTransform.Basis.Z.Dot(toTarget);
                if (dot > -0.5f) continue;  // Must be roughly in front
                
                // Get screen position
                Vector2 screenPos = camera.UnprojectPosition(player.GlobalPosition + Vector3.Up * 1.5f);
                Vector2 screenCenter = GetViewport().GetVisibleRect().Size / 2;
                float screenDistance = screenPos.DistanceTo(screenCenter);
                
                // Check if within lock angle (screen space)
                float maxScreenDistance = Mathf.Tan(Mathf.DegToRad(LockAngle)) * screenCenter.Y;
                if (screenDistance > maxScreenDistance * 2) continue;
                
                // Score: prefer closer to center and closer in distance
                float score = screenDistance + distance * 0.1f;
                
                if (score < bestScore)
                {
                    bestScore = score;
                    bestTarget = player;
                }
            }
            
            // Lock threshold: within reasonable distance from center
            float threshold = GetViewport().GetVisibleRect().Size.Length() * 0.15f;
            if (bestTarget != null && bestScore < threshold)
            {
                RequestLockOn(bestTarget.EntityId);
            }
        }
        
        private void SetTarget(RemotePlayer player)
        {
            if (player == null || !IsInstanceValid(player)) return;
            
            uint previousTarget = CurrentTarget ?? 0;
            CurrentTarget = player.EntityId;
            TargetPlayer = player;
            
            _targetReticle.Visible = true;
            if (_targetInfoPanel != null)
            {
                _targetInfoPanel.Visible = true;
            }
            
            // Get initial health
            var entity = GameState.Instance.GetEntity(player.EntityId);
            if (entity != null)
            {
                _targetCurrentHealth = entity.HealthPercent;
                _targetMaxHealth = 100;
            }
            
            if (previousTarget != player.EntityId)
            {
                GD.Print($"[TargetLockSystem] Target locked: {player.PlayerName} ({player.EntityId}) at range {GetTargetRange():F1}m");
            }
        }
        
        /// <summary>
        /// Request lock-on to a target entity from the server.
        /// Sends PACKET_LOCK_ON_REQUEST and sets pending state.
        /// </summary>
        private void RequestLockOn(uint entityId)
        {
            // Rate-limit: prevent spamming requests
            uint nowMs = (uint)Time.GetTicksMsec();
            if (nowMs - _lastLockAttemptTimeMs < MIN_LOCK_INTERVAL_MS)
            {
                // Too soon — rate limit would reject anyway
                GD.Print($"[TargetLock] Lock request too soon (rate-limited), last attempt={_lastLockAttemptTimeMs}, now={nowMs}");
                return;
            }

            // If a previous request is still pending, don't send another
            if (_lockRequestPending)
            {
                GD.Print("[TargetLock] Lock request already pending, skipping");
                return;
            }

            _lockRequestPending = true;
            _pendingTarget = entityId;
            _lastLockAttemptTimeMs = nowMs;

            NetworkManager.Instance.SendLockOnRequest(entityId);
            GD.Print($"[TargetLock] Lock-on request sent for entity {entityId}");
        }

        public void ClearTarget()
        {
            if (CurrentTarget.HasValue)
            {
                GD.Print($"[TargetLockSystem] Target cleared (was {CurrentTarget})");
            }
            
            CurrentTarget = null;
            TargetPlayer = null;
            _targetReticle.Visible = false;
            if (_targetInfoPanel != null)
            {
                _targetInfoPanel.Visible = false;
            }
            
            // Clear pending state
            _lockRequestPending = false;
            _pendingTarget = null;
        }
        
        /// <summary>
        /// Check if target is still valid for keeping lock
        /// </summary>
        private bool IsTargetValid(uint entityId)
        {
            var player = RemotePlayerManager.Instance?.GetPlayer(entityId);
            if (player == null || !IsInstanceValid(player)) return false;
            
            // Check range
            var localPlayer = GetTree().GetNodesInGroup("local_player").FirstOrDefault() as Node3D;
            if (localPlayer == null) return false;
            
            float distance = localPlayer.GlobalPosition.DistanceTo(player.GlobalPosition);
            return distance <= MaxLockRange;
        }
        
        /// <summary>
        /// Get current target range
        /// </summary>
        private float GetTargetRange()
        {
            if (TargetPlayer == null) return 0;
            
            var localPlayer = GetTree().GetNodesInGroup("local_player").FirstOrDefault() as Node3D;
            if (localPlayer == null) return 0;
            
            return localPlayer.GlobalPosition.DistanceTo(TargetPlayer.GlobalPosition);
        }
        
        private void UpdateReticle()
        {
            if (TargetPlayer == null || !IsInstanceValid(TargetPlayer))
            {
                _targetReticle.Visible = false;
                return;
            }
            
            // Position reticle above target
            _targetReticle.GlobalPosition = TargetPlayer.GlobalPosition + Vector3.Up * 2.0f;
            
            // Face camera
            var camera = GetViewport().GetCamera3D();
            if (camera != null)
            {
                _targetReticle.LookAt(camera.GlobalPosition, Vector3.Up);
            }
            
            // Pulse effect
            float pulse = (Mathf.Sin((float)Time.GetTicksMsec() / 200f) + 1) / 2;
            _targetReticle.Scale = Vector3.One * (1.0f + pulse * 0.15f);
            
            // Color based on range
            float range = GetTargetRange();
            var material = _targetReticle.MaterialOverride as StandardMaterial3D;
            if (material != null)
            {
                if (range > MaxLockRange * 0.8f)
                {
                    material.AlbedoColor = Colors.Yellow;  // Warning: near max range
                    material.Emission = Colors.Yellow;
                }
                else
                {
                    material.AlbedoColor = Colors.Red;
                    material.Emission = Colors.Red;
                }
            }
        }
        
        private void UpdateTargetInfo()
        {
            if (TargetPlayer == null || !IsInstanceValid(TargetPlayer))
            {
                if (_targetInfoPanel != null)
                    _targetInfoPanel.Visible = false;
                return;
            }
            
            // Update name label position (above reticle in screen space)
            if (_targetNameLabel != null)
            {
                _targetNameLabel.GlobalPosition = TargetPlayer.GlobalPosition + Vector3.Up * 2.5f;
                _targetNameLabel.Text = TargetPlayer.PlayerName;
            }
            
            // Update health bar
            if (_targetHealthBar != null)
            {
                _targetHealthBar.Value = _targetCurrentHealth;
            }
        }
        
        /// <summary>
        /// Cycle to next available target
        /// </summary>
        private void CycleTarget()
        {
            var remoteManager = RemotePlayerManager.Instance;
            if (remoteManager == null) return;
            
            var players = remoteManager.GetAllPlayers().Where(p => IsInstanceValid(p)).ToList();
            if (players.Count == 0) return;
            
            int currentIndex = -1;
            if (CurrentTarget.HasValue)
            {
                currentIndex = players.FindIndex(p => p.EntityId == CurrentTarget.Value);
            }
            
            // Try to find next valid target
            for (int i = 1; i <= players.Count; i++)
            {
                int nextIndex = (currentIndex + i) % players.Count;
                var candidate = players[nextIndex];
                
                if (IsTargetValid(candidate.EntityId))
                {
                    RequestLockOn(candidate.EntityId);
                    return;
                }
            }
        }
        
        private void ToggleTargetLock()
        {
            if (CurrentTarget.HasValue)
            {
                ClearTarget();
            }
            else
            {
                TryAutoTarget();
            }
        }
        
        private void OnEntityStateReceived(uint entityId, Vector3 position, Vector3 velocity)
        {
            // Update target health if it's our target
            if (CurrentTarget.HasValue && entityId == CurrentTarget.Value)
            {
                var entity = GameState.Instance.GetEntity(entityId);
                if (entity != null)
                {
                    _targetCurrentHealth = entity.HealthPercent;
                }
            }
        }
        
        private void OnEntityDied(uint entityId, uint killerId)
        {
            // Clear target if it died
            if (CurrentTarget.HasValue && entityId == CurrentTarget.Value)
            {
                ClearTarget();
            }
        }
        
        /// <summary>
        /// Check if we have a valid target within range
        /// </summary>
        public bool HasValidTarget()
        {
            return CurrentTarget.HasValue && IsTargetValid(CurrentTarget.Value);
        }
        
        /// <summary>
        /// Get target entity ID (0 if no target)
        /// </summary>
        public uint GetTargetId()
        {
            return CurrentTarget ?? 0;
        }
    }
}
