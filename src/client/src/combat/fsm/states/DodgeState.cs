using Godot;
using System;

namespace DarkAges.Combat.FSM.States
{
    /// <summary>
    /// Dodge state - player is performing a dodge roll/dash.
    /// Transitions: Idle (after dodge duration), Hit (if hit during dodge)
    /// Has invulnerability frames during dodge.
    /// </summary>
    [GlobalClass]
    public partial class DodgeState : State
    {
        private double _dodgeTimer = 0.0;
        private const double DODGE_DURATION = 0.4;
        private const float DODGE_FORCE = 12.0f;
        private Vector3 _dodgeDirection = Vector3.Zero;
        private bool _dodgeComplete = false;

        public override void Enter()
        {
            _dodgeTimer = 0.0;
            _dodgeComplete = false;

            if (AnimTree != null)
            {
                AnimTree.Set("parameters/conditions/dodging", true);
            }

            // Calculate dodge direction based on input or facing direction
            Vector2 inputDir = Input.GetVector("ui_left", "ui_right", "ui_up", "ui_down");
            Vector3 forward = Character.GlobalTransform.Basis.Z.Normalized();
            Vector3 right = Character.GlobalTransform.Basis.X.Normalized();
            
            if (inputDir.Length() > 0.1f)
            {
                _dodgeDirection = (-forward * inputDir.Y + right * inputDir.X).Normalized();
            }
            else
            {
                _dodgeDirection = -forward; // Dodge backward relative to facing
            }

            if (Player != null)
            {
                Player.TriggerDodge();
            }
        }

        public override void Update(double delta)
        {
            _dodgeTimer += delta;

            // Apply dodge force
            if (_dodgeTimer < DODGE_DURATION * 0.5f)
            {
                Character.Velocity = _dodgeDirection * DODGE_FORCE;
                Character.MoveAndSlide();
            }

            if (_dodgeTimer >= DODGE_DURATION && !_dodgeComplete)
            {
                _dodgeComplete = true;
                EmitSignal(SignalName.TransitionRequested, "Idle");
            }
        }

        public override void Exit()
        {
            if (AnimTree != null)
            {
                AnimTree.Set("parameters/conditions/dodging", false);
            }
            Character.Velocity = Vector3.Zero;
        }
    }
}
