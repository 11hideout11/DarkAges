using Godot;
using System;
using DarkAges.Combat.FSM;

namespace DarkAges.Combat
{
    /// <summary>
    /// [CLIENT_AGENT] Procedural Leaning - Velocity-based character tilt.
    /// 
    /// Tilts the character model based on movement velocity to create
    /// a sense of weight and momentum. Features:
    /// - Velocity-based tilt angle
    /// - Banking into turns (lateral acceleration)
    /// - Smooth interpolation for natural feel
    /// - Disabled during combat states
    /// </summary>
    [GlobalClass]
    public partial class ProceduralLeaning : Node
    {
        // ============================================
        // EXPORT PROPERTIES
        // ============================================
        
        [Export]
        public float LeanAngleMax
        {
            get => _leanAngleMax;
            set => _leanAngleMax = value;
        }
        
        [Export]
        public float LeanSpeed
        {
            get => _leanSpeed;
            set => _leanSpeed = value;
        }
        
        [Export]
        public float ReturnSpeed
        {
            get => _returnSpeed;
            set => _returnSpeed = value;
        }
        
        [Export]
        public float TurnBankAngle
        {
            get => _turnBankAngle;
            set => _turnBankAngle = value;
        }
        
        [Export]
        public bool EnableDuringCombat
        {
            get => _enableDuringCombat;
            set => _enableDuringCombat = value;
        }
        
        // ============================================
        // STATE
        // ============================================
        
        public float CurrentLeanAngle => _currentLeanAngle;
        
        // ============================================
        // PRIVATE FIELDS
        // ============================================
        
        private float _leanAngleMax = 15.0f; // Max lean angle in degrees
        private float _leanSpeed = 3.0f;    // How fast lean builds up
        private float _returnSpeed = 5.0f;  // How fast return to center
        private float _turnBankAngle = 8.0f; // Banking into turns
        
        private bool _enableDuringCombat = false;
        
        private CharacterBody3D _player;
        private Node3D _model;
        private CombatStateMachineController _combatFsm;
        
        private Vector3 _velocity;
        private Vector3 _lastVelocity;
        private float _currentLeanAngle = 0.0f;
        private float _targetLeanAngle = 0.0f;
        
        // For turn banking
        private float _currentTurnAmount = 0.0f;
        
        // ============================================
        // LIFECYCLE
        // ============================================
        
        public override void _Ready()
        {
            _player = GetParent<CharacterBody3D>();
            if (_player == null)
            {
                GD.PrintErr("[ProceduralLeaning] Parent must be CharacterBody3D");
                return;
            }
            
            // Find model node
            _model = GetNodeOrNull<Node3D>("Model");
            if (_model == null)
            {
                // Create a pivot if no model
                _model = GetParent<Node3D>();
            }
            
            // Get animation state machine
            _combatFsm = GetNodeOrNull<CombatStateMachineController>("CombatStateMachine");
            
            GD.Print("[ProceduralLeaning] Initialized");
        }
        
        public override void _PhysicsProcess(double delta)
        {
            float dt = (float)delta;
            
            // Get velocity from player
            UpdateVelocity(dt);
            
            // Check if should lean
            if (!ShouldLean())
            {
                // Return to center
                _currentLeanAngle = Mathf.MoveToward(_currentLeanAngle, 0, _returnSpeed * dt);
                _currentTurnAmount = Mathf.MoveToward(_currentTurnAmount, 0, _returnSpeed * dt);
                ApplyLeaning();
                return;
            }
            
            // Calculate target lean from forward speed
            float forwardSpeed = _velocity.Dot(_player.GlobalTransform.Basis.Z);
            float targetForwardLean = -forwardSpeed * _leanAngleMax / 10.0f;
            targetForwardLean = Mathf.Clamp(targetForwardLean, -_leanAngleMax, _leanAngleMax);
            
            // Calculate turn banking from lateral velocity
            float lateralSpeed = _velocity.Dot(_player.GlobalTransform.Basis.X);
            float targetTurnBank = lateralSpeed * _turnBankAngle / 5.0f;
            targetTurnBank = Mathf.Clamp(targetTurnBank, -_turnBankAngle, _turnBankAngle);
            
            // Combine leans
            _targetLeanAngle = targetForwardLean + targetTurnBank;
            
            // Smooth toward target
            if (Mathf.Abs(_targetLeanAngle) > Mathf.Abs(_currentLeanAngle))
            {
                _currentLeanAngle = Mathf.MoveToward(_currentLeanAngle, _targetLeanAngle, _leanSpeed * dt);
            }
            else
            {
                _currentLeanAngle = Mathf.MoveToward(_currentLeanAngle, _targetLeanAngle, _returnSpeed * dt);
            }
            
            ApplyLeaning();
        }
        
        // ============================================
        // PRIVATE HELPERS
        // ============================================
        
        private void UpdateVelocity(float dt)
        {
            _lastVelocity = _velocity;
            _velocity = _player.Velocity;
        }
        
        private bool ShouldLean()
        {
            // Check if on ground (no lean while airborne)
            if (!_player.IsOnFloor())
                return false;
            
            // Check combat states if disabled during combat
            if (!_enableDuringCombat && _combatFsm != null)
            {
                var state = _combatFsm.CurrentState;
                if (state != CombatStateMachineController.CombatState.Idle &&
                    state != CombatStateMachineController.CombatState.Walk &&
                    state != CombatStateMachineController.CombatState.Run)
                {
                    return false;
                }
            }
            
            // Minimum velocity threshold
            return _velocity.Length() > 0.5f;
        }
        
        private void ApplyLeaning()
        {
            if (_model == null) return;
            
            // Apply lean rotation (around Z axis for forward lean, X for turn bank)
            // Combine into a single lean
            float leanRad = Mathf.DegToRad(_currentLeanAngle);
            
            // Get current rotation
            var currentBasis = _model.Basis;
            
            // Apply lean - tilt around local X axis (bank) and Z axis (forward lean)
            // For simplicity, apply as Z rotation (roll)
            _model.Rotation = new Vector3(
                _model.Rotation.X,
                _model.Rotation.Y,
                leanRad
            );
        }
        
        /// <summary>
        /// Add impulse lean (e.g., on hit)
        /// </summary>
        public void AddImpulseLean(float angle)
        {
            _currentLeanAngle = Mathf.Clamp(_currentLeanAngle + angle, -_leanAngleMax, _leanAngleMax);
        }
        
        /// <summary>
        /// Reset lean to center
        /// </summary>
        public void Reset()
        {
            _currentLeanAngle = 0.0f;
            _targetLeanAngle = 0.0f;
            _currentTurnAmount = 0.0f;
        }
    }
}