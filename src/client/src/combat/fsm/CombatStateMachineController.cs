using Godot;
using System;

namespace DarkAges.Combat.FSM
{
    /// <summary>
    /// [CLIENT_AGENT] Combat State Machine Controller - Node-based FSM for combat.
    /// 
    /// This controller provides a designer-friendly interface for managing combat states
    /// via the AnimationTree's AnimationNodeStateMachine. It integrates with the existing
    /// AnimationStateMachine.cs for state logic while exposing visual configuration.
    /// 
    /// States: Idle, Walk, Run, Attack, Dodge, Hit, Death
    /// Supports global cooldown, hit stop, and proper state blocking rules.
    /// 
    /// Usage:
    /// 1. Add CombatStateMachine.tscn as child of Player.tscn
    /// 2. Configure states visually in Godot editor
    /// 3. Connect signals for state callbacks
    /// </summary>
    [GlobalClass]
    public partial class CombatStateMachineController : Node3D
    {
        // ============================================
        // EXPORT PROPERTIES (Designable in Inspector)
        // ============================================
        
        [Export]
        public float AttackDuration
        {
            get => _attackDuration;
            set => _attackDuration = value;
        }
        
        [Export]
        public float DodgeDuration
        {
            get => _dodgeDuration;
            set => _dodgeDuration = value;
        }
        
        [Export]
        public float HitDuration
        {
            get => _hitDuration;
            set => _hitDuration = value;
        }
        
        [Export]
        public float GlobalCooldown
        {
            get => _globalCooldown;
            set => _globalCooldown = value;
        }
        
        [Export]
        public float BlendTime
        {
            get => _blendTime;
            set => _blendTime = value;
        }
        
        // ============================================
        // SIGNALS
        // ============================================
        
        [Signal]
        public delegate void StateEnteredEventHandler(string stateName);
        
        [Signal]
        public delegate void StateExitedEventHandler(string stateName);
        
        [Signal]
        public delegate void TransitionRequestedEventHandler(string fromState, string toState);
        
        [Signal]
        public delegate void CooldownStartedEventHandler(float duration);
        
        [Signal]
        public delegate void CooldownCompletedEventHandler();
        
        // ============================================
        // STATE DEFINITIONS
        // ============================================
        
        public enum CombatState
        {
            Idle = 0,
            Walk = 1,
            Run = 2,
            Attack = 3,
            Dodge = 4,
            Hit = 5,
            Death = 6
        }
        
        // State instance - accessible for external scripts
        private CombatState _currentState = CombatState.Idle;
        
        public CombatState CurrentState
        {
            get => _currentState;
        }
        
        // Current state as string
        public string CurrentStateName => _currentState.ToString();
        
        // ============================================
        // PRIVATE FIELDS
        // ============================================
        
        // Default durations (seconds)
        private float _attackDuration = 0.5f;
        private float _dodgeDuration = 0.4f;
        private float _hitDuration = 0.3f;
        private float _globalCooldown = 1.2f;
        private float _blendTime = 0.1f;
        
        // AnimationTree reference
        private AnimationTree _animTree;
        private AnimationPlayer _animPlayer;
        
        // State timers
        private double _stateTimer = 0.0;
        private double _cooldownTimer = 0.0;
        
        // Busy flags
        private bool _isBusy = false;
        private bool _isOnCooldown = false;
        
        // Reference to existing code-based FSM
        private AnimationStateMachine _codeFsm;
        
        // ============================================
        // PROPERTIES FOR EXTERNAL SCRIPTS
        // ============================================
        
        public bool IsBusy => _isBusy || _currentState == CombatState.Attack || 
                                  _currentState == CombatState.Dodge || 
                                  _currentState == CombatState.Hit;
        
        public bool IsOnCooldown => _cooldownTimer > 0;
        
        public float CooldownRemaining => (float)_cooldownTimer;
        
        // ============================================
        // LIFECYCLE
        // ============================================
        
        public override void _Ready()
        {
            // Get AnimationTree and AnimationPlayer references
            _animTree = GetNode<AnimationTree>("AnimationTree");
            _animPlayer = GetNode<AnimationPlayer>("AnimationPlayer");
            
            if (_animTree == null)
            {
                GD.PrintErr("[CombatStateMachineController] AnimationTree not found!");
                return;
            }
            
            // Try to get reference to code-based FSM if it exists
            var existingFsm = GetParent()?.GetNode<AnimationStateMachine>("AnimationStateMachine");
            if (existingFsm != null)
            {
                _codeFsm = existingFsm;
                GD.Print("[CombatStateMachineController] Connected to existing AnimationStateMachine");
            }
            
            // Configure AnimationNodeStateMachine with transitions
            ConfigureStateMachine();
            
            GD.Print("[CombatStateMachineController] Ready - Node-based FSM initialized");
        }
        
        public override void _PhysicsProcess(double delta)
        {
            UpdateTimers((float)delta);
        }
        
        // ============================================
        // STATE MANAGEMENT
        // ============================================
        
        /// <summary>
        /// Request state transition with validation
        /// </summary>
        public bool RequestTransition(string targetState)
        {
            // Validate transition is allowed
            if (!CanTransition(_currentState.ToString(), targetState))
            {
                return false;
            }
            
            // Emit transition request signal
            EmitSignal(SignalName.TransitionRequested, _currentState.ToString(), targetState);
            
            // Perform transition
            return TransitionTo(targetState);
        }
        
        /// <summary>
        /// Attempt to transition to target state
        /// </summary>
        private bool TransitionTo(string targetState)
        {
            // Handle cooldown start for attack/dodge
            if (targetState == "Attack" || targetState == "Dodge")
            {
                StartCooldown();
            }
            
            // Convert string to enum
            if (Enum.TryParse<CombatState>(targetState, out var newState))
            {
                // Exit current state
                ExitState(_currentState);
                
                // Update state
                _currentState = newState;
                
                // Enter new state
                EnterState(newState);
                
                // Emit signals
                EmitSignal(SignalName.StateEntered, targetState);
                
                // Sync with code-based FSM if available
                SyncToCodeFsm(newState);
                
                GD.Print($"[CombatStateMachineController] -> {targetState}");
                return true;
            }
            
            return false;
        }
        
        /// <summary>
        /// Check if transition is valid
        /// </summary>
        private bool CanTransition(string fromState, string toState)
        {
            // Block transitions while busy (except Hit and Death which override)
            if (IsBusy && toState != "Hit" && toState != "Death")
            {
                return false;
            }
            
            // Block transitions while on cooldown
            if (_cooldownTimer > 0 && toState == "Attack")
            {
                return false;
            }
            
            // Block attack during dodge, etc.
            if (_currentState == CombatState.Dodge && toState != "Idle")
            {
                return false;
            }
            
            return true;
        }
        
        /// <summary>
        /// Start global cooldown
        /// </summary>
        private void StartCooldown()
        {
            _cooldownTimer = _globalCooldown;
            _isOnCooldown = true;
            EmitSignal(SignalName.CooldownStarted, _globalCooldown);
            
            // Sync with code-based FSM
            _codeFsm?.StartGlobalCooldown();
        }
        
        /// <summary>
        /// Update state timers
        /// </summary>
        private void UpdateTimers(float delta)
        {
            // Update cooldown timer
            if (_cooldownTimer > 0)
            {
                _cooldownTimer -= delta;
                if (_cooldownTimer <= 0)
                {
                    _cooldownTimer = 0;
                    _isOnCooldown = false;
                    EmitSignal(SignalName.CooldownCompleted);
                }
            }
            
            // Update state-specific timers
            switch (_currentState)
            {
                case CombatState.Attack:
                    _stateTimer -= delta;
                    if (_stateTimer <= 0)
                        TransitionTo("Idle");
                    break;
                    
                case CombatState.Dodge:
                    _stateTimer -= delta;
                    if (_stateTimer <= 0)
                        TransitionTo("Idle");
                    break;
                    
                case CombatState.Hit:
                    _stateTimer -= delta;
                    if (_stateTimer <= 0)
                        TransitionTo("Idle");
                    break;
            }
        }
        
        // ============================================
        // ANIMATIONTREE CONFIGURATION
        // ============================================
        
        /// <summary>
        /// Configure the AnimationNodeStateMachine with transitions
        /// </summary>
        private void ConfigureStateMachine()
        {
            if (_animTree == null) return;
            
            var stateMachine = _animTree.TreeRoot as AnimationNodeStateMachine;
            if (stateMachine != null)
            {
                // Set blend time for all transitions
                for (int i = 0; i < stateMachine.GetTransitionCount(); i++)
                {
                    // Note: Direct transition configuration requires runtime API
                    // This is a simplified version showing the concept
                }
                
                GD.Print($"[CombatStateMachineController] Configured transitions (count: {stateMachine.GetTransitionCount()})");
            }
            
            // Set initial parameters
            _animTree.Set("parameters/conditions/idle", true);
        }
        
        // ============================================
        // STATE HANDLERS
        // ============================================
        
        private void ExitState(CombatState state)
        {
            if (_animTree == null) return;
            
            // Set condition to false for current state
            string conditionName = state switch
            {
                CombatState.Idle => "idle",
                CombatState.Walk => "walking",
                CombatState.Run => "sprinting",
                CombatState.Attack => "attacking",
                CombatState.Dodge => "dodging",
                CombatState.Hit => "hit",
                CombatState.Death => "dead",
                _ => ""
            };
            
            if (!string.IsNullOrEmpty(conditionName))
            {
                _animTree.Set($"parameters/conditions/{conditionName}", false);
            }
        }
        
        private void EnterState(CombatState state)
        {
            if (_animTree == null) return;
            
            // Set state parameter
            _animTree.Set("parameters/state/current", (int)state);
            
            // Set condition to true for new state
            string conditionName = state switch
            {
                CombatState.Idle => "idle",
                CombatState.Walk => "walking",
                CombatState.Run => "sprinting",
                CombatState.Attack => "attacking",
                CombatState.Dodge => "dodging",
                CombatState.Hit => "hit",
                CombatState.Death => "dead",
                _ => ""
            };
            
            if (!string.IsNullOrEmpty(conditionName))
            {
                _animTree.Set($"parameters/conditions/{conditionName}", true);
            }
            
            // Set state timer
            _stateTimer = state switch
            {
                CombatState.Attack => _attackDuration,
                CombatState.Dodge => _dodgeDuration,
                CombatState.Hit => _hitDuration,
                _ => 0
            };
            
            // Apply hit stop for Hit state
            if (state == CombatState.Hit)
            {
                ApplyHitStop();
            }
        }
        
        /// <summary>
        /// Sync state with code-based FSM
        /// </summary>
        private void SyncToCodeFsm(CombatState state)
        {
            if (_codeFsm == null) return;
            
            // Sync actions to code FSM
            switch (state)
            {
                case CombatState.Attack:
                    _codeFsm.TriggerAttack();
                    break;
                case CombatState.Dodge:
                    _codeFsm.TriggerDodge();
                    break;
                case CombatState.Hit:
                    _codeFsm.TriggerHit();
                    break;
                case CombatState.Death:
                    _codeFsm.TriggerDeath();
                    break;
            }
        }
        
        /// <summary>
        /// Hit stop effect - brief time freeze
        /// </summary>
        private async void ApplyHitStop()
        {
            if (_animPlayer == null) return;
            
            Engine.TimeScale = 0.1f;
            await ToSignal(GetTree().CreateTimer(0.05f, true), "timeout");
            Engine.TimeScale = 1.0f;
        }
        
        // ============================================
        // PUBLIC API FOR INPUT HANDLING
        // ============================================
        
        /// <summary>
        /// Try to attack - respects cooldown and busy state
        /// </summary>
        public bool TryAttack()
        {
            if (IsBusy || _cooldownTimer > 0 || _currentState == CombatState.Death)
                return false;
            
            return TransitionTo("Attack");
        }
        
        /// <summary>
        /// Try to dodge - respects busy state
        /// </summary>
        public bool TryDodge()
        {
            if (_currentState == CombatState.Attack || 
                _currentState == CombatState.Dodge || 
                _currentState == CombatState.Hit || 
                _currentState == CombatState.Death)
                return false;
            
            return TransitionTo("Dodge");
        }
        
        /// <summary>
        /// Trigger hit reaction
        /// </summary>
        public void TriggerHit()
        {
            if (_currentState == CombatState.Death) return;
            TransitionTo("Hit");
        }
        
        /// <summary>
        /// Trigger death
        /// </summary>
        public void TriggerDeath()
        {
            TransitionTo("Death");
        }
        
        /// <summary>
        /// Trigger respawn
        /// </summary>
        public void TriggerRespawn()
        {
            TransitionTo("Idle");
        }
        
        /// <summary>
        /// Set movement state based on velocity
        /// </summary>
        public void SetMovementState(bool isMoving, bool isSprinting)
        {
            if (IsBusy) return;
            
            if (isMoving && _currentState != CombatState.Walk && !isSprinting)
                TransitionTo("Walk");
            else if (isMoving && isSprinting && _currentState != CombatState.Run)
                TransitionTo("Run");
            else if (!isMoving && _currentState != CombatState.Idle)
                TransitionTo("Idle");
        }
    }
}