using Godot;
using System;

namespace DarkAges.Combat.FSM
{
    /// <summary>
    /// [CLIENT_AGENT] Simplified FSM for managing combat animation states.
    /// Replaces the partial/abstract state machine with a single-file concrete implementation.
    /// 
    /// States: Idle, Walking, Sprinting, Attacking, Dodging, Hit, Dead
    /// Drives AnimationTree parameters for blended locomotion and combat states.
    /// Includes global cooldown (GCD), hit stop, and proper state blocking rules.
    /// </summary>
    [GlobalClass]
    public partial class AnimationStateMachine : Node
    {
        public enum StateType
        {
            Idle,
            Walking,
            Sprinting,
            Attacking,
            Dodging,
            Hit,
            Dead
        }

        private StateType _currentState = StateType.Idle;
        private AnimationTree _animTree;
        private AnimationPlayer _animPlayer;

        // State durations (seconds)
        private const double ATTACK_DURATION = 0.5;
        private const double DODGE_DURATION = 0.4;
        private const double HIT_DURATION = 0.3;
        private const double GLOBAL_COOLDOWN_DURATION = 1.2;

        // Timers
        private double _attackTimer = 0.0;
        private double _dodgeTimer = 0.0;
        private double _hitTimer = 0.0;
        private double _gcdTimer = 0.0;

        public StateType CurrentState => _currentState;
        public bool IsBusy => _currentState == StateType.Attacking || 
                            _currentState == StateType.Dodging || 
                            _currentState == StateType.Hit;
        
        // Properties expected by PredictedPlayer.cs
        public bool IsOnGlobalCooldown => _gcdTimer > 0;

        public override void _Ready()
        {
            _animTree = GetParent().GetNode<AnimationTree>("AnimationTree");
            _animPlayer = GetParent().GetNode<AnimationPlayer>("AnimationPlayer");
            GD.Print("[AnimationStateMachine] Ready");
            
            // Configure all state transitions in the AnimationNodeStateMachine resource
            // with 0.1s crossfade for smooth animation blending
            var stateMachine = _animTree.TreeRoot as AnimationNodeStateMachine;
            if (stateMachine != null)
            {
                AddTransition(stateMachine, "Idle", "Walking");
                AddTransition(stateMachine, "Idle", "Sprinting");
                AddTransition(stateMachine, "Idle", "Attacking");
                AddTransition(stateMachine, "Idle", "Dodging");
                AddTransition(stateMachine, "Idle", "Hit");
                AddTransition(stateMachine, "Idle", "Dead");
                
                AddTransition(stateMachine, "Walking", "Idle");
                AddTransition(stateMachine, "Walking", "Sprinting");
                AddTransition(stateMachine, "Walking", "Attacking");
                AddTransition(stateMachine, "Walking", "Dodging");
                AddTransition(stateMachine, "Walking", "Hit");
                AddTransition(stateMachine, "Walking", "Dead");
                
                AddTransition(stateMachine, "Sprinting", "Idle");
                AddTransition(stateMachine, "Sprinting", "Walking");
                AddTransition(stateMachine, "Sprinting", "Attacking");
                AddTransition(stateMachine, "Sprinting", "Dodging");
                AddTransition(stateMachine, "Sprinting", "Hit");
                AddTransition(stateMachine, "Sprinting", "Dead");
                
                AddTransition(stateMachine, "Attacking", "Idle");
                AddTransition(stateMachine, "Attacking", "Hit");
                AddTransition(stateMachine, "Attacking", "Dead");
                
                AddTransition(stateMachine, "Dodging", "Idle");
                AddTransition(stateMachine, "Dodging", "Hit");
                AddTransition(stateMachine, "Dodging", "Dead");
                
                AddTransition(stateMachine, "Hit", "Idle");
                AddTransition(stateMachine, "Hit", "Dead");
                
                AddTransition(stateMachine, "Dead", "Idle"); // Respawn
                
                GD.Print($"[AnimationStateMachine] Configured {stateMachine.GetTransitionCount()} transitions");
            }
        }

        /// <summary>
    /// Helper to add transition between two states in the AnimationNodeStateMachine
    /// </summary>
    private void AddTransition(AnimationNodeStateMachine stateMachine, string from, string to)
    {
        var transition = new AnimationNodeStateMachineTransition();
        stateMachine.AddTransition(from, to, transition);
    }

    public override void _PhysicsProcess(double delta)
        {
            // Update timers
            if (_gcdTimer > 0)
            {
                _gcdTimer -= delta;
                if (_gcdTimer < 0) _gcdTimer = 0;
            }

            switch (_currentState)
            {
                case StateType.Attacking:
                    _attackTimer -= delta;
                    if (_attackTimer <= 0)
                        TransitionTo(StateType.Idle);
                    break;

                case StateType.Dodging:
                    _dodgeTimer -= delta;
                    if (_dodgeTimer <= 0)
                        TransitionTo(StateType.Idle);
                    break;

                case StateType.Hit:
                    _hitTimer -= delta;
                    if (_hitTimer <= 0)
                        TransitionTo(StateType.Idle);
                    break;
            }
        }

        /// <summary>
        /// Set movement state based on current velocity
        /// </summary>
        public void SetMovementState(bool isMoving, bool isSprinting)
        {
            if (IsBusy) return; // Don't change state while busy

            if (isMoving && _currentState != StateType.Walking && !isSprinting)
                TransitionTo(StateType.Walking);
            else if (isMoving && isSprinting && _currentState != StateType.Sprinting)
                TransitionTo(StateType.Sprinting);
            else if (!isMoving && _currentState != StateType.Idle)
                TransitionTo(StateType.Idle);
        }

        /// <summary>
        /// Trigger attack - blocked during busy states and GCD
        /// </summary>
        public bool TryAttack()
        {
            if (IsBusy || _gcdTimer > 0 || _currentState == StateType.Dead)
                return false;

            _gcdTimer = GLOBAL_COOLDOWN_DURATION;
            TransitionTo(StateType.Attacking);
            return true;
        }

        /// <summary>
        /// Trigger dodge - blocked during attack/dodge/hit/dead
        /// </summary>
        public bool TryDodge()
        {
            if (_currentState == StateType.Attacking || 
                _currentState == StateType.Dodging || 
                _currentState == StateType.Hit || 
                _currentState == StateType.Dead)
                return false;

            TransitionTo(StateType.Dodging);
            return true;
        }

        /// <summary>
        /// Trigger hit reaction - blocked during death
        /// </summary>
        public void TriggerHit()
        {
            if (_currentState == StateType.Dead) return;
            TransitionTo(StateType.Hit);
        }

        /// <summary>
        /// Trigger death - enters dead state
        /// </summary>
        public void TriggerDeath()
        {
            TransitionTo(StateType.Dead);
        }

        /// <summary>
        /// Trigger respawn - transitions to idle
        /// </summary>
        public void TriggerRespawn()
        {
            TransitionTo(StateType.Idle);
        }

        /// <summary>
        /// Trigger attack - wrapper for PredictedPlayer.cs compatibility
        /// </summary>
        public void TriggerAttack()
        {
            TryAttack();
        }

        /// <summary>
        /// Start global cooldown - for PredictedPlayer.cs compatibility
        /// </summary>
        public void StartGlobalCooldown()
        {
            _gcdTimer = GLOBAL_COOLDOWN_DURATION;
        }

        /// <summary>
        /// Trigger dodge - wrapper for PredictedPlayer.cs compatibility
        /// </summary>
        public void TriggerDodge()
        {
            TryDodge();
        }

        private void TransitionTo(StateType newState)
        {
            if (_currentState == newState) return;

            // Exit current state
            ExitState(_currentState);

            // Update state
            _currentState = newState;

            // Enter new state
            EnterState(newState);

            GD.Print($"[AnimationStateMachine] -> {newState}");
        }

        private void ExitState(StateType state)
        {
            if (_animTree == null) return;

            // Disable previous state blend
            switch (state)
            {
                case StateType.Idle:
                    _animTree.Set("parameters/conditions/idle", false);
                    break;
                case StateType.Walking:
                    _animTree.Set("parameters/conditions/walking", false);
                    break;
                case StateType.Sprinting:
                    _animTree.Set("parameters/conditions/sprinting", false);
                    break;
                case StateType.Attacking:
                    _animTree.Set("parameters/conditions/attacking", false);
                    break;
                case StateType.Dodging:
                    _animTree.Set("parameters/conditions/dodging", false);
                    break;
                case StateType.Hit:
                    _animTree.Set("parameters/conditions/hit", false);
                    break;
                case StateType.Dead:
                    _animTree.Set("parameters/conditions/dead", false);
                    break;
            }
        }

        private void EnterState(StateType state)
        {
            if (_animTree == null) return;

            // Enable new state blend and set timers
            switch (state)
            {
                case StateType.Idle:
                    _animTree.Set("parameters/conditions/idle", true);
                    break;
                case StateType.Walking:
                    _animTree.Set("parameters/conditions/walking", true);
                    break;
                case StateType.Sprinting:
                    _animTree.Set("parameters/conditions/sprinting", true);
                    break;
                case StateType.Attacking:
                    _animTree.Set("parameters/conditions/attacking", true);
                    _attackTimer = ATTACK_DURATION;
                    break;
                case StateType.Dodging:
                    _animTree.Set("parameters/conditions/dodging", true);
                    _dodgeTimer = DODGE_DURATION;
                    break;
                case StateType.Hit:
                    _animTree.Set("parameters/conditions/hit", true);
                    _hitTimer = HIT_DURATION;
                    ApplyHitStop();
                    break;
                case StateType.Dead:
                    _animTree.Set("parameters/conditions/dead", true);
                    break;
            }

            // Trigger state transition in AnimationTree
            _animTree.Set("parameters/state/current", (int)state);
        }

        /// <summary>
        /// Hit stop - brief time freeze for impact feel
        /// </summary>
        private async void ApplyHitStop()
        {
            if (_animPlayer == null) return;
            
            Engine.TimeScale = 0.1f;
            await ToSignal(GetTree().CreateTimer(0.05f, true), "timeout");
            Engine.TimeScale = 1.0f;
        }
    }
}
