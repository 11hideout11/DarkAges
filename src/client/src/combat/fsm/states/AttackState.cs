using Godot;
using System;

namespace DarkAges.Combat.FSM.States
{
    /// <summary>
    /// Attack state - player is performing an attack animation.
    /// Transitions: Idle/Walk (after attack duration), Hit (if hit during attack)
    /// </summary>
    [GlobalClass]
    public partial class AttackState : State
    {
        private double _attackTimer = 0.0;
        private const double ATTACK_DURATION = 0.5;
        private bool _attackComplete = false;

        public override void Enter()
        {
            _attackTimer = 0.0;
            _attackComplete = false;

            if (AnimTree != null)
            {
                AnimTree.Set("parameters/conditions/attacking", true);
                AnimTree.Set("parameters/conditions/idle", false);
            }

            // Trigger hit stop effect via CombatEventSystem
            if (Player != null)
            {
                Player.TriggerAttack();
            }
        }

        public override void Update(double delta)
        {
            _attackTimer += delta;

            if (_attackTimer >= ATTACK_DURATION && !_attackComplete)
            {
                _attackComplete = true;
                EmitSignal(SignalName.TransitionRequested, "Idle");
            }
        }

        public override void Exit()
        {
            if (AnimTree != null)
            {
                AnimTree.Set("parameters/conditions/attacking", false);
            }
        }
    }
}
