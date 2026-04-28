using Godot;
using System;

namespace DarkAges.Combat.FSM
{
    /// <summary>
    /// Simplified FSM for managing animation states.
    /// Driven by PredictedPlayer - just handles state transitions and animation parameters.
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

        // State timers
        private double _attackTimer = 0.0;
        private double _dodgeTimer = 0.0;
        private double _hitTimer = 0.0;
        private const double ATTACK_DURATION = 0.5;
        private const double DODGE_DURATION = 0.4;
        private const double HIT_DURATION = 0.3;

        public StateType CurrentState => _currentState;

        public override void _Ready()
        {
            _animTree = GetParent().GetNodeOrNull<AnimationTree>("AnimationTree");
        }

        public override void _PhysicsProcess(double delta)
        {
            // Update state timers
            switch (_currentState)
            {
                case StateType.Attacking:
                    _attackTimer -= delta;
                    if (_attackTimer <= 0)
                    {
                        TransitionTo(StateType.Idle);
                    }
                    break;
                case StateType.Dodging:
                    _dodgeTimer -= delta;
                    if (_dodgeTimer <= 0)
                    {
                        TransitionTo(StateType.Idle);
                    }
                    break;
                case StateType.Hit:
                    _hitTimer -= delta;
                    if (_hitTimer <= 0)
                    {
                        TransitionTo(StateType.Idle);
                    }
                    break;
            }
        }

        public void TransitionTo(StateType newState)
        {
            if (_currentState == newState) return;

            ExitState(_currentState);
            _currentState = newState;
            EnterState(newState);

            GD.Print($"[AnimationStateMachine] {_currentState} -> {newState}");
        }

        private void ExitState(StateType state)
        {
            if (_animTree == null) return;

            switch (state)
            {
                case StateType.Idle:
                    _animTree.Set("parameters/Idle", true);
                    break;
                case StateType.Walking:
                    _animTree.Set("parameters/Walking", false);
                    break;
                case StateType.Sprinting:
                    _animTree.Set("parameters/Sprinting", false);
                    break;
                case StateType.Attacking:
                    _animTree.Set("parameters/Attacking", false);
                    break;
                case StateType.Dodging:
                    _animTree.Set("parameters/Dodging", false);
                    break;
                case StateType.Hit:
                    _animTree.Set("parameters/Hit", false);
                    break;
                case StateType.Dead:
                    _animTree.Set("parameters/Dead", false);
                    break;
            }
        }

        private void EnterState(StateType state)
        {
            if (_animTree == null) return;

            switch (state)
            {
                case StateType.Idle:
                    _animTree.Set("parameters/Idle", true);
                    break;
                case StateType.Walking:
                    _animTree.Set("parameters/Walking", true);
                    _animTree.Set("parameters/Idle", false);
                    break;
                case StateType.Sprinting:
                    _animTree.Set("parameters/Sprinting", true);
                    _animTree.Set("parameters/Walking", false);
                    break;
                case StateType.Attacking:
                    _animTree.Set("parameters/Attacking", true);
                    _attackTimer = ATTACK_DURATION;
                    break;
                case StateType.Dodging:
                    _animTree.Set("parameters/Dodging", true);
                    _dodgeTimer = DODGE_DURATION;
                    break;
                case StateType.Hit:
                    _animTree.Set("parameters/Hit", true);
                    _hitTimer = HIT_DURATION;
                    break;
                case StateType.Dead:
                    _animTree.Set("parameters/Dead", true);
                    break;
            }
        }

        public void SetMovementState(bool isMoving, bool isSprinting)
        {
            if (isMoving && _currentState == StateType.Idle)
            {
                TransitionTo(isSprinting ? StateType.Sprinting : StateType.Walking);
            }
            else if (!isMoving && (_currentState == StateType.Walking || _currentState == StateType.Sprinting))
            {
                TransitionTo(StateType.Idle);
            }
            else if (isMoving && _currentState == StateType.Walking && isSprinting)
            {
                TransitionTo(StateType.Sprinting);
            }
            else if (isMoving && _currentState == StateType.Sprinting && !isSprinting)
            {
                TransitionTo(StateType.Walking);
            }
        }

        public void TriggerAttack()
        {
            if (_currentState != StateType.Dead)
            {
                TransitionTo(StateType.Attacking);
            }
        }

        public void TriggerDodge()
        {
            if (_currentState != StateType.Dead && _currentState != StateType.Attacking)
            {
                TransitionTo(StateType.Dodging);
            }
        }

        public void TriggerHit()
        {
            if (_currentState != StateType.Dead)
            {
                TransitionTo(StateType.Hit);
            }
        }

        public void TriggerDeath()
        {
            TransitionTo(StateType.Dead);
        }

        public void TriggerRespawn()
        {
            TransitionTo(StateType.Idle);
        }
    }
}
