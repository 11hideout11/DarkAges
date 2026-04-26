using Godot;
using System;
using System.Collections.Generic;
using System.Linq;
using DarkAges.Networking;
using DarkAges.Combat;

namespace DarkAges
{
    /// <summary>
    /// [CLIENT_AGENT] Client-side predicted player controller
    /// Predicts movement locally, server corrects errors via reconciliation
    /// 
    /// WP-7-2 Implementation Features:
    /// - Input prediction buffer (2-second window, 120 inputs)
    /// - Local physics simulation matching server exactly
    /// - Server reconciliation with smooth error correction
    /// - Position error visualization (server ghost)
    /// - Handles 200ms+ latency gracefully
    /// 
    /// Design:
    /// - All inputs are predicted immediately and stored in a buffer
    /// - Server processes inputs and sends corrections when needed
    /// - On correction: rewind to server state, replay unacknowledged inputs
    /// - Smooth error correction for small drift, snap for large errors (>2m)
    /// - Green ghost shows server-authoritative position for debugging
    /// </summary>
    public partial class PredictedPlayer : CharacterBody3D
    {
        // Movement constants - MUST MATCH SERVER EXACTLY
        [Export] public float MaxSpeed = 6.0f;
        [Export] public float SprintMultiplier = 1.5f;
        [Export] public float Acceleration = 10.0f;
        [Export] public float RotationSpeed = 720.0f;
        [Export] public float JumpVelocity = 8.0f;
        [Export] public float Gravity = 20.0f;
        
        // Correction smoothing - small errors blend smoothly, large errors snap
        [Export] public float CorrectionSmoothing = 0.3f;  // Blend factor for smooth correction (0-1)
        [Export] public float ErrorSnapThreshold = 2.0f;   // Meters - snap instantly above this
        [Export] public float ErrorCorrectThreshold = 0.1f; // Meters - correct below this
        [Export] public float CorrectionSpeed = 10.0f;     // Speed of smooth correction (m/s)
        
        // Input buffer configuration
        [Export] public int MaxInputBufferSize = 120;      // 2 seconds at 60Hz
        [Export] public float FixedDt = 1.0f / 60.0f;      // Fixed timestep for reconciliation
        
        // Latency handling
        [Export] public float MaxExpectedLatency = 0.3f;   // 300ms - buffer size accommodates this
        [Export] public bool EnableLatencyCompensation = true;
        
        // Debug visualization
        [Export] public bool ShowServerGhost = true;
        [Export] public bool ShowDebugLabel = true;
        
        // State
        private uint _inputSequence = 1;
        private Queue<PredictedInput> _inputBuffer = new();
        private Vector3 _serverPosition = Vector3.Zero;
        private Vector3 _serverVelocity = Vector3.Zero;
        private Vector3 _predictedPosition = Vector3.Zero;
        private Vector3 _predictedVelocity = Vector3.Zero;
        private bool _reconciling = false;
        private uint _lastProcessedServerInput = 0;
        private float _predictionError = 0.0f;
        private int _reconciliationCount = 0;
        private uint _lastCorrectionTick = 0;
        
        // Dodge state
        private bool _isDodging = false;
        private double _dodgeTimer = 0.0;
        private const double DODGE_DURATION = 0.4;
        private const float DODGE_FORCE = 12.0f;
        private double _invulnerableTimer = 0.0;
        private const double INVULNERABLE_DURATION = 0.3;
        
        // Combat state
        private bool _isAttacking = false;
        private double _attackTimer = 0.0;
        private const double ATTACK_DURATION = 0.5;
        private bool _isHit = false;
        private double _hitTimer = 0.0;
        private const double HIT_DURATION = 0.3;
        private bool _isDead = false;
        
        // Global Cooldown (GCD) - 1.2s after any action
        private double _lastGlobalCooldownTime = 0.0;
        private const double GLOBAL_COOLDOWN_DURATION = 1.2;
        private bool _isOnGlobalCooldown = false;
        
        // Hitbox/Hurtbox Area3D nodes
        private Area3D _hitbox;
        private Area3D _hurtbox;
        
        // Smooth correction state
        private Vector3 _correctionTargetPosition;
        private Vector3 _correctionStartPosition;
        private float _correctionProgress = 0.0f;
        private bool _isSmoothingCorrection = false;
        private float _correctionDuration = 0.0f;
        
        // Camera
        private Node3D _cameraRig;
        private SpringArm3D _springArm;
        private float _cameraYaw = 0.0f;
        private float _cameraPitch = 0.0f;
        
        // Components
        private AnimationPlayer _animPlayer;
        private AnimationTree _animTree;
        private Dictionary<string, bool> _animParams = new();
        
        // Debug visualization
        private MeshInstance3D _serverGhost;
        private Label3D _debugLabel;
        private float _timeSinceLastCorrection = 0.0f;
        
        // Signals for UI/debug
        [Signal]
        public delegate void PredictionErrorEventHandler(float error, bool isLarge);
        
        [Signal]
        public delegate void ReconciliationEventHandler(int inputCount, float error);
        
        [Signal]
        public delegate void ServerCorrectionReceivedEventHandler(Vector3 serverPos, float error);

        public override void _Ready()
        {
            _cameraRig = GetNode<Node3D>("CameraRig");
            _springArm = GetNode<SpringArm3D>("CameraRig/SpringArm3D");
            _animPlayer = GetNode<AnimationPlayer>("AnimationPlayer");
            _animTree = GetNodeOrNull<AnimationTree>("AnimationTree");

            // Initialize animation parameters for state machine
            // (state machine parameters are auto-created by the scene)

            _predictedPosition = GlobalPosition;
            _correctionTargetPosition = GlobalPosition;
            _correctionStartPosition = GlobalPosition;

            // Hide mouse cursor for gameplay
            Input.MouseMode = Input.MouseModeEnum.Captured;

            // Create debug visualization
            CreateDebugVisualization();

            GD.Print("[PredictedPlayer] Initialized with prediction buffer (WP-7-2)");

            // Initialize Hitbox/Hurtbox Area3D nodes for collision layers
            SetupHitboxHurtbox();

            // Subscribe to combat events for hit reaction and death
            CombatEventSystem.Instance.DamageTaken += OnDamageTaken;
            CombatEventSystem.Instance.LocalPlayerDied += OnLocalPlayerDied;
        }

        public override void _Input(InputEvent @event)
        {
            if (@event is InputEventMouseMotion mouseMotion)
            {
                // Rotate camera with mouse
                _cameraYaw -= mouseMotion.Relative.X * 0.003f;
                _cameraPitch -= mouseMotion.Relative.Y * 0.003f;
                _cameraPitch = Mathf.Clamp(_cameraPitch, -Mathf.Pi / 3, Mathf.Pi / 4);
                
                if (_cameraRig != null)
                {
                    _cameraRig.Rotation = new Vector3(_cameraPitch, _cameraYaw, 0);
                }
            }
            
            if (@event.IsActionPressed("ui_cancel"))
            {
                // Toggle mouse capture
                Input.MouseMode = Input.MouseMode == Input.MouseModeEnum.Captured 
                    ? Input.MouseModeEnum.Visible 
                    : Input.MouseModeEnum.Captured;
            }
            
            // Toggle debug visualization
            if (@event.IsActionPressed("toggle_debug"))
            {
                ToggleDebugVisualization();
            }
        }

        public override void _PhysicsProcess(double delta)
        {
            float dt = (float)delta;
            _timeSinceLastCorrection += dt;
            
            // Update state timers
            if (_isDodging)
            {
                _dodgeTimer -= delta;
                if (_dodgeTimer <= 0)
                {
                    _isDodging = false;
                    GD.Print("[PredictedPlayer] Dodge ended");
                }
            }
            if (_isAttacking)
            {
                _attackTimer -= delta;
                if (_attackTimer <= 0)
                {
                    _isAttacking = false;
                }
            }
            if (_isHit)
            {
                _hitTimer -= delta;
                if (_hitTimer <= 0)
                {
                    _isHit = false;
                }
            }
            if (_invulnerableTimer > 0)
            {
                _invulnerableTimer -= delta;
            }
            
            // Update global cooldown timer
            if (_isOnGlobalCooldown)
            {
                double now = Time.GetTicksMsec() / 1000.0;
                if (now - _lastGlobalCooldownTime >= GLOBAL_COOLDOWN_DURATION)
                {
                    _isOnGlobalCooldown = false;
                    GD.Print("[PredictedPlayer] GCD expired");
                }
            }
            
            // Update smooth correction if active
            if (_isSmoothingCorrection)
            {
                ApplySmoothCorrection(dt);
            }
            
            if (GameState.Instance.CurrentConnectionState != GameState.ConnectionState.Connected)
                return;
            
            // Don't predict while reconciling - server state will be applied
            if (_reconciling)
                return;
            
            // 1. Gather input
            var input = GatherInput();
            
            // 2. Handle dodge trigger
            if (Input.IsKeyPressed(Key.Q) && !_isDodging && !_isAttacking && !_isHit && !_isDead && IsOnFloor())
            {
                TriggerDodge(input);
            }
            
            // 2b. Handle attack trigger (with GCD enforcement)
            if (input.Attack && !_isAttacking && !_isDodging && !_isHit && !_isDead && !_isOnGlobalCooldown)
            {
                _isAttacking = true;
                _attackTimer = ATTACK_DURATION;
                // Start global cooldown - blocks all actions for 1.2s
                _isOnGlobalCooldown = true;
                _lastGlobalCooldownTime = Time.GetTicksMsec() / 1000.0;
                GD.Print("[PredictedPlayer] Attack triggered, GCD started");
            }
            
            // 3. Apply locally immediately (prediction)
            ApplyMovement(input, dt);
            _predictedPosition = GlobalPosition;
            _predictedVelocity = Velocity;
            
            // 3. Store for reconciliation and network sending
            StorePredictedInput(input, _predictedPosition, _predictedVelocity, dt);
            
            // 4. Update animations
            UpdateAnimation(input);
            
            // 5. Update debug stats and visualization
            UpdateDebugStats();
            UpdateDebugVisualization();
        }
        
        /// <summary>
        /// Creates the server ghost and debug label for visualization
        /// </summary>
        private void CreateDebugVisualization()
        {
            // Create ghost mesh showing server position
            _serverGhost = new MeshInstance3D();
            _serverGhost.Name = "ServerGhost";
            
            // Use a capsule mesh similar to player
            var ghostMesh = new CapsuleMesh
            {
                Height = 2.0f,
                Radius = 0.5f
            };
            _serverGhost.Mesh = ghostMesh;
            
            // Create transparent green material
            var ghostMaterial = new StandardMaterial3D
            {
                AlbedoColor = new Color(0, 1, 0, 0.3f),
                Transparency = BaseMaterial3D.TransparencyEnum.Alpha,
                NoDepthTest = true,
                ShadingMode = BaseMaterial3D.ShadingModeEnum.Unshaded
            };
            _serverGhost.MaterialOverride = ghostMaterial;
            
            // Add to scene root so it moves independently
            GetTree().Root.AddChild(_serverGhost);
            _serverGhost.Visible = ShowServerGhost;
            
            // Debug label showing prediction stats
            _debugLabel = new Label3D
            {
                Name = "DebugLabel",
                Billboard = BaseMaterial3D.BillboardModeEnum.Enabled,
                PixelSize = 0.01f,
                Position = new Vector3(0, 2.5f, 0),
                Visible = ShowDebugLabel
            };
            AddChild(_debugLabel);
        }
        
        /// <summary>
        /// Toggle debug visualization on/off
        /// </summary>
        private void ToggleDebugVisualization()
        {
            ShowServerGhost = !ShowServerGhost;
            ShowDebugLabel = !ShowDebugLabel;
            
            if (_serverGhost != null)
                _serverGhost.Visible = ShowServerGhost;
            if (_debugLabel != null)
                _debugLabel.Visible = ShowDebugLabel;
        }
        
        /// <summary>
        /// Updates the debug visualization (ghost position, label text)
        /// </summary>
        private void UpdateDebugVisualization()
        {
            // Update server ghost position
            if (_serverGhost != null && _serverGhost.Visible)
            {
                // Only show ghost if we've received at least one correction
                if (_lastProcessedServerInput > 0)
                {
                    _serverGhost.GlobalPosition = _serverPosition;
                    _serverGhost.Visible = true;
                }
                else
                {
                    _serverGhost.Visible = false;
                }
            }
            
            // Update debug label
            if (_debugLabel != null && _debugLabel.Visible)
            {
                UpdateDebugLabel();
            }
        }
        
        /// <summary>
        /// Updates the debug label text with prediction stats
        /// </summary>
        private void UpdateDebugLabel()
        {
            float error = GlobalPosition.DistanceTo(_serverPosition);
            
            // Determine status
            string status;
            Color statusColor;
            
            if (error < 0.1f)
            {
                status = "SYNCED";
                statusColor = Colors.Green;
            }
            else if (_isSmoothingCorrection)
            {
                status = "CORRECTING";
                statusColor = Colors.Yellow;
            }
            else if (error > ErrorSnapThreshold)
            {
                status = "SNAPPING";
                statusColor = Colors.Red;
            }
            else
            {
                status = "OK";
                statusColor = Colors.Green;
            }
            
            // Format label text
            var rtt = GameState.Instance.LastRttMs;
            _debugLabel.Text = $"{status}\n" +
                              $"Error: {error:F2}m\n" +
                              $"Inputs: {_inputBuffer.Count}\n" +
                              $"RTT: {rtt}ms\n" +
                              $"Recons: {_reconciliationCount}";
            _debugLabel.Modulate = statusColor;
        }

        /// <summary>
        /// Store input in prediction buffer for potential reconciliation
        /// </summary>
        private void StorePredictedInput(InputState input, Vector3 predictedPos, Vector3 predictedVel, float dt)
        {
            // Enforce buffer size limit to prevent unbounded growth
            while (_inputBuffer.Count >= MaxInputBufferSize)
            {
                var removed = _inputBuffer.Dequeue();
                GD.Print($"[PredictedPlayer] Dropping old input seq={removed.Sequence} (buffer full)");
            }
            
            // Calculate input direction from movement keys
            Vector2 inputDir = new Vector2(
                (input.Right ? 1.0f : 0.0f) - (input.Left ? 1.0f : 0.0f),
                (input.Backward ? 1.0f : 0.0f) - (input.Forward ? 1.0f : 0.0f)
            );
            
            if (inputDir.LengthSquared() > 0)
            {
                inputDir = inputDir.Normalized();
            }
            
            var predInput = new PredictedInput
            {
                Sequence = _inputSequence++,
                Timestamp = (uint)Time.GetTicksMsec(),
                InputDir = inputDir,
                Yaw = input.Yaw,
                Pitch = input.Pitch,
                Sprint = input.Sprint,
                Jump = input.Jump,
                Attack = input.Attack,
                Block = input.Block,
                PredictedPosition = predictedPos,
                PredictedVelocity = predictedVel,
                DeltaTime = dt,
                Acknowledged = false
            };
            
            _inputBuffer.Enqueue(predInput);
        }

        /// <summary>
        /// Called when server sends correction
        /// Parses ServerCorrection FlatBuffer and handles reconciliation
        /// </summary>
        public void OnServerCorrection(byte[] correctionData)
        {
            try
            {
                // Parse stub protocol ServerCorrection
                // Server format: [packet_type:1][server_tick:4][pos_x:4f][pos_y:4f][pos_z:4f][vel_x:4f][vel_y:4f][vel_z:4f][last_input:4]
                if (correctionData.Length < 33)
                {
                    GD.PrintErr($"[PredictedPlayer] Invalid correction packet size: {correctionData.Length} (expected >= 33)");
                    return;
                }
                
                int offset = 1;  // Skip packet type byte
                
                uint serverTick = BitConverter.ToUInt32(correctionData, offset);
                offset += 4;
                
                // Parse float position (matches server serializeCorrection)
                float posX = BitConverter.ToSingle(correctionData, offset);
                offset += 4;
                float posY = BitConverter.ToSingle(correctionData, offset);
                offset += 4;
                float posZ = BitConverter.ToSingle(correctionData, offset);
                offset += 4;
                
                // Parse float velocity
                float velX = BitConverter.ToSingle(correctionData, offset);
                offset += 4;
                float velY = BitConverter.ToSingle(correctionData, offset);
                offset += 4;
                float velZ = BitConverter.ToSingle(correctionData, offset);
                offset += 4;
                
                uint lastProcessedInput = BitConverter.ToUInt32(correctionData, offset);
                
                // Apply the correction
                ApplyServerCorrection(
                    serverTick,
                    new Vector3(posX, posY, posZ),
                    new Vector3(velX, velY, velZ),
                    lastProcessedInput
                );
            }
            catch (Exception ex)
            {
                GD.PrintErr($"[PredictedPlayer] Failed to parse correction: {ex.Message}");
            }
        }

        /// <summary>
        /// Apply server correction with appropriate handling based on error magnitude
        /// Implements WP-7-2 smooth error correction requirements
        /// </summary>
        private void ApplyServerCorrection(uint serverTick, Vector3 serverPos, Vector3 serverVel, uint lastProcessedSeq)
        {
            _serverPosition = serverPos;
            _serverVelocity = serverVel;
            _lastProcessedServerInput = lastProcessedSeq;
            _lastCorrectionTick = serverTick;
            _timeSinceLastCorrection = 0.0f;
            
            // Mark acknowledged inputs
            MarkAcknowledgedInputs(lastProcessedSeq);
            
            // Remove acknowledged inputs from buffer (all inputs up to and including lastProcessedSeq)
            int removedCount = 0;
            while (_inputBuffer.Count > 0 && _inputBuffer.Peek().Sequence <= lastProcessedSeq)
            {
                var input = _inputBuffer.Dequeue();
                input.Acknowledged = true;
                input.ServerPositionAtAck = serverPos;
                removedCount++;
            }
            
            // Calculate prediction error at the point of server processing
            // We compare server's position to where we predicted we'd be at that input
            Vector3 predictedPosAtServerTime = GetPositionAtSequence(lastProcessedSeq);
            _predictionError = predictedPosAtServerTime.DistanceTo(serverPos);
            
            // Emit debug signal
            EmitSignal(SignalName.PredictionError, _predictionError, _predictionError > ErrorSnapThreshold);
            EmitSignal(SignalName.ServerCorrectionReceived, serverPos, _predictionError);
            
            // Determine correction strategy based on error magnitude
            if (_predictionError <= ErrorCorrectThreshold)
            {
                // Error is negligible - no correction needed
                GD.Print($"[PredictedPlayer] Small error ({_predictionError:F3}m) - no correction");
                _isSmoothingCorrection = false;
                return;
            }
            else if (_predictionError > ErrorSnapThreshold)
            {
                // Large error - snap instantly (possible cheat, desync, or respawn)
                GD.Print($"[PredictedPlayer] LARGE CORRECTION: {_predictionError:F2}m - snapping to server pos");
                GlobalPosition = serverPos;
                Velocity = serverVel;
                _predictedPosition = serverPos;
                _predictedVelocity = serverVel;
                _isSmoothingCorrection = false;
                
                // Still need to replay unacknowledged inputs
                if (_inputBuffer.Count > 0)
                {
                    ReplayUnacknowledgedInputs(serverPos, serverVel, lastProcessedSeq);
                }
            }
            else
            {
                // Small drift - use smooth correction over time (WP-7-2 requirement)
                // This prevents jarring snaps for minor prediction errors
                GD.Print($"[PredictedPlayer] Smooth correction: {_predictionError:F3}m, replaying {_inputBuffer.Count} inputs");
                
                // Start from server state
                GlobalPosition = serverPos;
                Velocity = serverVel;
                
                // Replay unacknowledged inputs to get target position
                int replayedCount = ReplayUnacknowledgedInputs(serverPos, serverVel, lastProcessedSeq);
                
                // Now initiate smooth correction to the replayed position
                // This handles the case where replay puts us at a slightly different position
                float postReplayError = GlobalPosition.DistanceTo(_predictedPosition);
                
                if (postReplayError > ErrorCorrectThreshold)
                {
                    StartSmoothCorrection(GlobalPosition, _predictedPosition);
                }
                else
                {
                    _isSmoothingCorrection = false;
                }
                
                _predictedPosition = GlobalPosition;
                _predictedVelocity = Velocity;
            }
        }
        
        /// <summary>
        /// Mark inputs as acknowledged without removing them
        /// </summary>
        private void MarkAcknowledgedInputs(uint lastProcessedSeq)
        {
            foreach (var input in _inputBuffer)
            {
                if (input.Sequence <= lastProcessedSeq)
                {
                    input.Acknowledged = true;
                }
            }
        }

        /// <summary>
        /// Get the predicted position at a specific input sequence
        /// </summary>
        private Vector3 GetPositionAtSequence(uint sequence)
        {
            // Find the input with matching sequence
            foreach (var input in _inputBuffer)
            {
                if (input.Sequence == sequence)
                {
                    return input.PredictedPosition;
                }
            }
            
            // If not found, use current position
            return _predictedPosition;
        }

        /// <summary>
        /// Start a smooth correction from startPos to targetPos
        /// WP-7-2: Smooth error correction (no snapping for errors < 2m)
        /// </summary>
        private void StartSmoothCorrection(Vector3 startPos, Vector3 targetPos)
        {
            _correctionStartPosition = startPos;
            _correctionTargetPosition = targetPos;
            _correctionProgress = 0.0f;
            _isSmoothingCorrection = true;
            
            // Calculate duration based on distance and correction speed
            float distance = startPos.DistanceTo(targetPos);
            _correctionDuration = Mathf.Max(distance / CorrectionSpeed, FixedDt * 3); // At least 3 frames
            
            GD.Print($"[PredictedPlayer] Starting smooth correction: {distance:F3}m over {_correctionDuration:F3}s");
        }
        
        /// <summary>
        /// Apply smooth correction over time
        /// </summary>
        private void ApplySmoothCorrection(float dt)
        {
            if (!_isSmoothingCorrection)
                return;
            
            _correctionProgress += dt;
            float t = Mathf.Clamp(_correctionProgress / _correctionDuration, 0.0f, 1.0f);
            
            // Use smoothstep for nicer interpolation
            t = t * t * (3.0f - 2.0f * t);
            
            // Interpolate position
            GlobalPosition = _correctionStartPosition.Lerp(_correctionTargetPosition, t);
            
            // Check if correction is complete
            if (t >= 1.0f)
            {
                _isSmoothingCorrection = false;
                GD.Print("[PredictedPlayer] Smooth correction complete");
            }
        }

        /// <summary>
        /// Reconcile by rewinding to server state and replaying unacknowledged inputs
        /// </summary>
        private void Reconcile(Vector3 serverPos, Vector3 serverVel, uint lastProcessedSeq)
        {
            _reconciling = true;
            _reconciliationCount++;
            
            // Emit signal for debug UI
            EmitSignal(SignalName.Reconciliation, _inputBuffer.Count, _predictionError);
            
            // Start from server state
            GlobalPosition = serverPos;
            Velocity = serverVel;
            
            // Replay all unacknowledged inputs
            int replayedCount = ReplayUnacknowledgedInputs(serverPos, serverVel, lastProcessedSeq);
            
            // Update predicted state
            _predictedPosition = GlobalPosition;
            _predictedVelocity = Velocity;
            
            GD.Print($"[PredictedPlayer] Reconciliation complete: replayed {replayedCount} inputs");
            
            _reconciling = false;
        }

        /// <summary>
        /// Replay unacknowledged inputs after server correction
        /// WP-7-2: Replays inputs to maintain responsive feel during corrections
        /// </summary>
        private int ReplayUnacknowledgedInputs(Vector3 serverPos, Vector3 serverVel, uint lastProcessedSeq)
        {
            int replayedCount = 0;
            
            // Get unacknowledged inputs (those with sequence > lastProcessedSeq)
            var unacknowledged = _inputBuffer.Where(i => i.Sequence > lastProcessedSeq).ToList();
            
            if (unacknowledged.Count == 0)
                return 0;
            
            GD.Print($"[PredictedPlayer] Replaying {unacknowledged.Count} unacknowledged inputs");
            
            foreach (var input in unacknowledged)
            {
                var inputState = new InputState
                {
                    Forward = input.InputDir.Y < -0.1f,
                    Backward = input.InputDir.Y > 0.1f,
                    Left = input.InputDir.X < -0.1f,
                    Right = input.InputDir.X > 0.1f,
                    Sprint = input.Sprint,
                    Jump = input.Jump,
                    Attack = input.Attack,
                    Block = input.Block,
                    Yaw = input.Yaw,
                    Pitch = input.Pitch
                };
                
                // Use fixed timestep for deterministic replay
                ApplyMovement(inputState, FixedDt);
                replayedCount++;
                
                // Store the replayed result back in the input for future error calculation
                input.PredictedPosition = GlobalPosition;
                input.PredictedVelocity = Velocity;
            }
            
            return replayedCount;
        }

        private InputState GatherInput()
        {
            return new InputState
            {
                Forward = Input.IsActionPressed("move_forward"),
                Backward = Input.IsActionPressed("move_backward"),
                Left = Input.IsActionPressed("move_left"),
                Right = Input.IsActionPressed("move_right"),
                Jump = Input.IsActionPressed("jump"),
                Sprint = Input.IsActionPressed("sprint"),
                Attack = Input.IsActionPressed("attack"),
                Block = Input.IsActionPressed("block"),
                Yaw = _cameraYaw,
                Pitch = _cameraPitch
            };
        }

        /// <summary>
        /// Apply movement - MUST MATCH SERVER PHYSICS EXACTLY
        /// WP-7-2: Local physics simulation matching server
        /// </summary>
        private void ApplyMovement(InputState input, float dt)
        {
            // If dead, stop all movement
            if (_isDead)
            {
                Velocity = Vector3.Zero;
                return;
            }

            // Calculate input direction
            Vector3 direction = Vector3.Zero;
            
            if (input.Forward) direction.Z -= 1;
            if (input.Backward) direction.Z += 1;
            if (input.Left) direction.X -= 1;
            if (input.Right) direction.X += 1;
            
            if (direction.LengthSquared() > 0)
            {
                direction = direction.Normalized();
                
                // Rotate by camera yaw
                direction = direction.Rotated(Vector3.Up, input.Yaw);
                
                // Smooth acceleration
                float targetSpeed = MaxSpeed * (input.Sprint ? SprintMultiplier : 1.0f);
                Vector3 targetVel = direction * targetSpeed;
                
                // Preserve Y velocity (gravity/jump), only lerp XZ
                targetVel.Y = Velocity.Y;
                Velocity = Velocity.Lerp(targetVel, Acceleration * dt);
                
                // Rotate player to face movement direction
                float targetRotation = Mathf.Atan2(direction.X, direction.Z);
                Rotation = new Vector3(
                    Rotation.X,
                    Mathf.LerpAngle(Rotation.Y, targetRotation, RotationSpeed * dt * Mathf.Pi / 180.0f),
                    Rotation.Z
                );
            }
            else
            {
                // Decelerate XZ, preserve Y
                Vector3 targetVel = new Vector3(0, Velocity.Y, 0);
                Velocity = Velocity.Lerp(targetVel, Acceleration * dt);
            }
            
            // Jump - only if on floor
            if (input.Jump && IsOnFloor())
            {
                Velocity = new Vector3(Velocity.X, JumpVelocity, Velocity.Z);
            }
            
            // [FIX] Disable gravity for flat terrain demo - server has no gravity
            // Clamp Y to 0 to match server behavior
            Velocity = new Vector3(Velocity.X, 0, Velocity.Z);
            
            // Apply movement
            Velocity = new Vector3(Velocity.X, 0, Velocity.Z);  // Ensure Y stays 0
            MoveAndSlide();
        }
        
        /// <summary>
        /// Trigger a dodge roll in the current movement direction
        /// </summary>
        private void TriggerDodge(InputState input)
        {
            _isDodging = true;
            _dodgeTimer = DODGE_DURATION;
            _invulnerableTimer = INVULNERABLE_DURATION;
            
            // Determine dodge direction from input, default to backward
            Vector3 dodgeDir = Vector3.Zero;
            if (input.Forward) dodgeDir.Z -= 1;
            if (input.Backward) dodgeDir.Z += 1;
            if (input.Left) dodgeDir.X -= 1;
            if (input.Right) dodgeDir.X += 1;
            
            if (dodgeDir.LengthSquared() > 0)
            {
                dodgeDir = dodgeDir.Normalized().Rotated(Vector3.Up, input.Yaw);
            }
            else
            {
                // Default dodge backward relative to facing
                dodgeDir = new Vector3(0, 0, 1).Rotated(Vector3.Up, Rotation.Y);
            }
            
            Velocity += dodgeDir * DODGE_FORCE;
            GD.Print($"[PredictedPlayer] Dodge triggered! Dir={dodgeDir} Force={DODGE_FORCE}");
        }
        
        /// <summary>
        /// Public method to trigger hit reaction (called by CombatEventSystem)
        /// </summary>
        public void TriggerHitReaction()
        {
            if (_isDead) return;
            _isHit = true;
            _hitTimer = HIT_DURATION;
            GD.Print("[PredictedPlayer] Hit reaction triggered");
        }

        /// <summary>
        /// Called when local player takes damage from server event.
        /// Triggers hit reaction (stun) and UI feedback.
        /// </summary>
        private void OnDamageTaken(int damage, bool isCritical)
        {
            if (_isDead) return;
            // Show floating damage number at local player position
            CombatEventSystem.Instance.SpawnDamageNumber(damage, GlobalPosition, isCritical);
            TriggerHitReaction();
        }
        
        /// <summary>
        /// Public method to trigger death state
        /// </summary>
        public void TriggerDeath()
        {
            _isDead = true;
            _isDodging = false;
            _isAttacking = false;
            _isHit = false;
            GD.Print("[PredictedPlayer] Death triggered");
        }

        private void OnLocalPlayerDied(uint killerId)
        {
            TriggerDeath();
        }
        
        /// <summary>
        /// Public method to respawn
        /// </summary>
        public void TriggerRespawn()
        {
            _isDead = false;
            _isHit = false;
            _isDodging = false;
            _isAttacking = false;
            GD.Print("[PredictedPlayer] Respawn triggered");
        }
        
        public bool IsDodging => _isDodging;
        public bool IsAttacking => _isAttacking;
        public bool IsInvulnerable => _invulnerableTimer > 0;
        public bool IsDead => _isDead;
        public bool IsHit => _isHit;

        /// <summary>
        /// Set up Hitbox (Layer 3) and Hurtbox (Layer 4) Area3D nodes.
        /// Hitbox detects when our attacks hit enemies.
        /// Hurtbox detects when enemy attacks hit us.
        /// </summary>
        private void SetupHitboxHurtbox()
        {
            // Hitbox — attack hit detection (Layer 3)
            _hitbox = new Area3D
            {
                Name = "HitboxArea",
                CollisionLayer = 1 << 2,  // Layer 3
                CollisionMask = 1 << 3    // Detect Layer 4 (Hurtbox)
            };
            var hitboxShape = new CollisionShape3D
            {
                Shape = new CylinderShape3D { Radius = 0.8f, Height = 1.5f },
                Position = new Vector3(0, 1.0f, 0.5f)  // Slightly in front
            };
            _hitbox.AddChild(hitboxShape);
            _hitbox.AreaEntered += OnHitboxAreaEntered;
            AddChild(_hitbox);

            // Hurtbox — damage reception area (Layer 4)
            _hurtbox = new Area3D
            {
                Name = "HurtboxArea",
                CollisionLayer = 1 << 3,  // Layer 4
                CollisionMask = 1 << 2    // Detect Layer 3 (Hitbox)
            };
            var hurtboxShape = new CollisionShape3D
            {
                Shape = new CylinderShape3D { Radius = 0.5f, Height = 1.8f },
                Position = new Vector3(0, 0.9f, 0)
            };
            _hurtbox.AddChild(hurtboxShape);
            _hurtbox.AreaEntered += OnHurtboxAreaEntered;
            AddChild(_hurtbox);

            GD.Print("[PredictedPlayer] Hitbox (Layer 3) and Hurtbox (Layer 4) initialized");
        }

        /// <summary>
        /// Called when our hitbox overlaps an enemy hurtbox (we hit them).
        /// Only processed on server authority via RPC; local prediction is visual-only.
        /// </summary>
        private void OnHitboxAreaEntered(Area3D area)
        {
            if (_isDead) return;
            GD.Print($"[PredictedPlayer] Hitbox touched: {area.Name}");
        }

        /// <summary>
        /// Called when an enemy hitbox overlaps our hurtbox (we got hit).
        /// Actual damage application is server-authoritative.
        /// </summary>
        private void OnHurtboxAreaEntered(Area3D area)
        {
            if (_isDead || _isDodging) return;  // Invulnerable during dodge
            GD.Print($"[PredictedPlayer] Hurtbox touched by: {area.Name}");
        }


        private void UpdateAnimation(InputState input)
        {
            // AnimationTree state machine drives all visual state transitions cleanly
            if (_animTree == null) return;

            // Compute derived state booleans
            float velocityLength = Velocity.Length();
            bool isSprinting = input.Sprint && velocityLength > 0.1f;
            bool isWalking = velocityLength > 0.1f && !isSprinting;
            bool isJumping = !IsOnFloor();
            bool hasMovement = isWalking || isSprinting;

            // Set AnimationTree parameters — state machine uses these to transition
            // Parameters are defined in the state machine transition conditions
            _animTree.Set("parameters/Sprinting", isSprinting);
            _animTree.Set("parameters/Jumping", isJumping);
            _animTree.Set("parameters/Attacking", _isAttacking);
            _animTree.Set("parameters/Dodging", _isDodging);
            _animTree.Set("parameters/Hit", _isHit);
            _animTree.Set("parameters/Dead", _isDead);
            _animTree.Set("parameters/IsOnFloor", IsOnFloor());
            _animTree.Set("parameters/VelocityLength", velocityLength);

            // Fallback: keep AnimationPlayer in sync when AnimationTree state machine is not providing transitions
            if (_animPlayer != null)
            {
                string animState = "Idle";
                if (_isDead) animState = "Death";
                else if (_isHit) animState = "Hit";
                else if (_isDodging) animState = "Dodge";
                else if (_isAttacking) animState = "Attack";
                else if (isJumping) animState = "Jump";
                else if (isSprinting) animState = "Sprint";
                else if (isWalking) animState = "Walk";

                // Only switch if different or not playing
                string current = _animPlayer.CurrentAnimation;
                if (current != animState)
                {
                    if (_animPlayer.HasAnimation(animState))
                    {
                        _animPlayer.Play(animState);
                    }
                }
            }
        }
private void UpdateDebugStats()
        {
            // Expose stats for debug UI
            GameState.Instance.CurrentPredictionError = _predictionError;
            GameState.Instance.InputBufferSize = _inputBuffer.Count;
            GameState.Instance.LastProcessedInput = _lastProcessedServerInput;
            GameState.Instance.ReconciliationCount = _reconciliationCount;
        }

        // Public accessors for debug UI
        public float GetPredictionError() => _predictionError;
        public int GetInputBufferSize() => _inputBuffer.Count;
        public uint GetLastProcessedServerInput() => _lastProcessedServerInput;
        public int GetReconciliationCount() => _reconciliationCount;
        public bool IsReconciling() => _reconciling;
        public bool IsSmoothingCorrection() => _isSmoothingCorrection;
        public Vector3 GetServerPosition() => _serverPosition;
        public Vector3 GetPredictedPosition() => _predictedPosition;
        public float GetTimeSinceLastCorrection() => _timeSinceLastCorrection;
        
        /// <summary>
        /// Get the next input sequence number for NetworkManager to send
        /// </summary>
        public uint GetNextInputSequence() => _inputSequence;
        
        /// <summary>
        /// Get all pending inputs for NetworkManager to send
        /// </summary>
        public Queue<PredictedInput> GetPendingInputs() => _inputBuffer;
        
        /// <summary>
        /// Get unacknowledged inputs (for replay after correction)
        /// </summary>
        public List<PredictedInput> GetUnacknowledgedInputs()
        {
            return _inputBuffer.Where(i => !i.Acknowledged).ToList();
        }
        
        public override void _ExitTree()
        {
            // Unsubscribe from combat events
            CombatEventSystem.Instance.DamageTaken -= OnDamageTaken;
            CombatEventSystem.Instance.LocalPlayerDied -= OnLocalPlayerDied;
            // Clean up debug visualization
            if (_serverGhost != null && IsInstanceValid(_serverGhost))
            {
                _serverGhost.QueueFree();
            }
            if (_debugLabel != null && IsInstanceValid(_debugLabel))
            {
                _debugLabel.QueueFree();
            }
        }
    }
}
