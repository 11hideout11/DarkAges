using Godot;
using System;

namespace DarkAges.Combat.FSM.States
{
    /// <summary>
    /// Hit state - player has been hit by an attack.
    /// Transitions: Idle (after hit duration), Death (if health <= 0)
    /// </summary>
    [GlobalClass]
    public partial class HitState : State
    {
        private double _hitTimer = 0.0;
        private const double HIT_DURATION = 0.3;
        private bool _hitComplete = false;

        public override void Enter()
        {
            _hitTimer = 0.0;
            _hitComplete = false;

            if (AnimTree != null)
            {
                AnimTree.Set("parameters/conditions/hit", true);
                AnimTree.Set("parameters/conditions/idle", false);
            }

            if (Player != null)
            {
                Player.TriggerHitReaction();
            }
        }

        public override void Update(double delta)
        {
            _hitTimer += delta;

            if (_hitTimer >= HIT_DURATION && !_hitComplete)
            {
                _hitComplete = true;
                
                // Check if player died
                if (Player != null && Player.GetCurrentHealth() <= 0)
                {
                    EmitSignal(SignalName.TransitionRequested, "Death");
                }
                else
                {
                    EmitSignal(SignalName.TransitionRequested, "Idle");
                }
            }
        }

        public override void Exit()
        {
            if (AnimTree != null)
            {
                AnimTree.Set("parameters/conditions/hit", false);
            }
        }
    }
}
