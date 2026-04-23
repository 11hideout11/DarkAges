using Godot;
using System;
using System.Collections.Generic;
using DarkAges.Networking;

namespace DarkAges.Combat
{
    /// <summary>
    /// [CLIENT_AGENT] Processes combat events from server
    /// Spawns hit markers, damage numbers, handles death cam
    /// </summary>
    public partial class CombatEventSystem : Node
    {
        public static CombatEventSystem Instance { get; private set; } = null!;
        
        [Signal]
        public delegate void DamageDealtEventHandler(uint targetId, int damage, bool isCritical, Vector3 position);
        
        [Signal]
        public delegate void DamageTakenEventHandler(int damage, bool isCritical);
        
        [Signal]
        public delegate void EntityDiedEventHandler(uint entityId, uint killerId);
        
        [Signal]
        public delegate void LocalPlayerDiedEventHandler(uint killerId);

        // Prefabs
        [Export] public PackedScene? DamageNumberPrefab;
        [Export] public PackedScene? HitMarkerPrefab;
        [Export] public PackedScene? DeathCamPrefab;
        
        // Configuration
        [Export] public float DamageNumberLifetime = 1.5f;
        [Export] public float HitMarkerLifetime = 0.3f;
        
        // State
        private Queue<CombatEvent> pendingEvents = new();
        private HashSet<uint> recentHits = new();  // For hit marker deduplication
        private Timer? hitMarkerResetTimer;

        public override void _EnterTree()
        {
            Instance = this;
        }

        public override void _Ready()
        {
            // Connect to network events
            NetworkManager.Instance.CombatEventReceived += OnCombatEventReceived;
            
            // Setup timer for clearing recent hits
            hitMarkerResetTimer = new Timer();
            hitMarkerResetTimer.WaitTime = 0.1f;
            hitMarkerResetTimer.OneShot = false;
            hitMarkerResetTimer.Timeout += () => recentHits.Clear();
            AddChild(hitMarkerResetTimer);
            hitMarkerResetTimer.Start();
        }

        public override void _ExitTree()
        {
            NetworkManager.Instance.CombatEventReceived -= OnCombatEventReceived;
        }

        /// <summary>
        /// Called when combat event arrives from server
        /// Simple binary format: [packet_type:1=3][subtype:1][attacker_id:4][target_id:4][damage:4][health_pct:1][timestamp:4]
        /// Subtypes: 1=Damage, 2=Death, 3=Heal
        /// </summary>
        private void OnCombatEventReceived(uint eventType, byte[] data)
        {
            // eventType is the subtype from NetworkManager (1=damage, 2=death, 3=heal)
            // data is the full packet bytes
            
            if (data.Length < 19) return;
            
            uint subtype = data[1];
            uint attackerId = BitConverter.ToUInt32(data, 2);
            uint targetId = BitConverter.ToUInt32(data, 6);
            int damage = BitConverter.ToInt32(data, 10);
            byte healthPercent = data[14];
            uint timestamp = BitConverter.ToUInt32(data, 15);
            
            uint localEntityId = GameState.Instance.LocalEntityId;
            
            switch (subtype)
            {
                case 1: // Damage
                    // Determine if we dealt or took damage
                    if (attackerId == localEntityId)
                    {
                        // We dealt damage
                        var targetEntity = GameState.Instance.GetEntity(targetId);
                        Vector3 position = targetEntity?.Position ?? Vector3.Zero;
                        SpawnDamageNumber(damage, position, false);
                        ShowHitMarker(false);
                        EmitSignal(SignalName.DamageDealt, targetId, damage, false, position);
                    }
                    else if (targetId == localEntityId)
                    {
                        // We took damage
                        ShowDamageIndicator(damage, false);
                        EmitSignal(SignalName.DamageTaken, damage, false);
                    }
                    break;
                    
                case 2: // Death
                    EmitSignal(SignalName.EntityDied, targetId, attackerId);
                    if (targetId == localEntityId)
                    {
                        EmitSignal(SignalName.LocalPlayerDied, attackerId);
                        ActivateDeathCam(attackerId);
                    }
                    else if (attackerId == localEntityId)
                    {
                        ShowKillNotification(targetId, attackerId);
                    }
                    break;
                    
                case 3: // Heal
                    // Show heal number
                    var healedEntity = GameState.Instance.GetEntity(targetId);
                    Vector3 healPos = healedEntity?.Position ?? Vector3.Zero;
                    SpawnDamageNumber(damage, healPos, false); // Reuse damage number for heal
                    break;
            }
        }

        private void ParseDamageDealt(byte[] data)
        {
            // Parse: [target_id:4][damage:2][is_critical:1][pos_x:4][pos_y:4][pos_z:4]
            if (data.Length < 19) return;
            
            uint targetId = BitConverter.ToUInt32(data, 0);
            int damage = BitConverter.ToUInt16(data, 4);
            bool isCritical = data[6] != 0;
            float posX = BitConverter.ToSingle(data, 7);
            float posY = BitConverter.ToSingle(data, 11);
            float posZ = BitConverter.ToSingle(data, 15);
            
            Vector3 position = new(posX, posY, posZ);
            
            // Show damage number at hit location
            SpawnDamageNumber(damage, position, isCritical);
            
            // Show hit marker on crosshair
            ShowHitMarker(isCritical);
            
            EmitSignal(SignalName.DamageDealt, targetId, damage, isCritical, position);
        }

        private void ParseDamageTaken(byte[] data)
        {
            // Parse: [damage:2][is_critical:1][attacker_id:4]
            if (data.Length < 7) return;
            
            int damage = BitConverter.ToUInt16(data, 0);
            bool isCritical = data[2] != 0;
            uint attackerId = BitConverter.ToUInt32(data, 3);
            
            // Screen flash/damage indicator
            ShowDamageIndicator(damage, isCritical);
            
            // Update local player health
            UpdateLocalHealth(-damage);
            
            EmitSignal(SignalName.DamageTaken, damage, isCritical);
        }

        private void ParseDeathEvent(byte[] data)
        {
            // Parse: [victim_id:4][killer_id:4]
            if (data.Length < 8) return;
            
            uint victimId = BitConverter.ToUInt32(data, 0);
            uint killerId = BitConverter.ToUInt32(data, 4);
            
            EmitSignal(SignalName.EntityDied, victimId, killerId);
            
            // Check if local player died
            if (victimId == GameState.Instance.LocalEntityId)
            {
                EmitSignal(SignalName.LocalPlayerDied, killerId);
                ActivateDeathCam(killerId);
            }
            else
            {
                // Show kill notification
                ShowKillNotification(victimId, killerId);
            }
        }

        private void ParseRespawnEvent(byte[] data)
        {
            // Parse: [entity_id:4][pos_x:4][pos_y:4][pos_z:4]
            // Deactivate death cam, respawn player
            DeactivateDeathCam();
        }

        /// <summary>
        /// Spawn floating damage number
        /// </summary>
        public void SpawnDamageNumber(int damage, Vector3 worldPosition, bool isCritical)
        {
            if (DamageNumberPrefab == null) return;
            
            var damageNumber = DamageNumberPrefab.Instantiate<DamageNumber>();
            AddChild(damageNumber);
            
            damageNumber.Initialize(damage, worldPosition, isCritical);
        }

        /// <summary>
        /// Show hit marker on crosshair
        /// </summary>
        public void ShowHitMarker(bool isCritical)
        {
            // Find or create hit marker UI
            var hitMarker = GetNodeOrNull<HitMarker>("/root/Main/UI/HitMarker");
            if (hitMarker == null) return;
            
            hitMarker.ShowHit(isCritical);
        }

        /// <summary>
        /// Show damage indicator (directional)
        /// </summary>
        private void ShowDamageIndicator(int damage, bool isCritical)
        {
            var indicator = GetNodeOrNull<DamageIndicator>("/root/Main/UI/DamageIndicator");
            indicator?.ShowDamage(damage, isCritical);
        }

        /// <summary>
        /// Update local player health UI
        /// </summary>
        private void UpdateLocalHealth(int delta)
        {
            // Emit signal for UI to update
            // Health bar update handled by UI controller
        }

        /// <summary>
        /// Activate death camera
        /// </summary>
        private void ActivateDeathCam(uint killerId)
        {
            if (DeathCamPrefab == null) return;
            
            var deathCam = DeathCamPrefab.Instantiate<DeathCamera>();
            AddChild(deathCam);
            
            // Pass killer info for kill cam
            string killerName = GameState.Instance.GetEntity(killerId)?.Name ?? "Unknown";
            deathCam.Activate(killerName);
        }

        private void DeactivateDeathCam()
        {
            var deathCam = GetNodeOrNull<DeathCamera>("DeathCamera");
            deathCam?.Deactivate();
        }

        private void ShowKillNotification(uint victimId, uint killerId)
        {
            // Show "You killed [player]" notification
            var notification = GetNodeOrNull<Label>("/root/Main/UI/KillNotification");
            if (notification == null) return;
            
            string victimName = GameState.Instance.GetEntity(victimId)?.Name ?? "Unknown";
            notification.Text = $"You killed {victimName}!";
            notification.Show();
            
            // Fade out after 3 seconds
            var tween = CreateTween();
            tween.TweenInterval(3.0);
            tween.TweenCallback(Callable.From(() => notification.Hide()));
        }
    }

    /// <summary>
    /// Combat event data structure
    /// </summary>
    public struct CombatEvent
    {
        public uint EventType;
        public uint Timestamp;
        public uint SourceEntity;
        public uint TargetEntity;
        public int Damage;
        public bool IsCritical;
        public Vector3 Position;
    }
}
