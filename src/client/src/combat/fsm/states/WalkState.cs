using Godot;
using System;

namespace DarkAges.Combat.FSM.States
{
    /// <summary>
    /// Walk state - player is moving at normal speed.
    /// Transitions: Idle (no input), Sprint (shift pressed), Attack, Dodge
    /// </summary>
    [GlobalClass]
    public partial class WalkState : State
    {
        private Vector3 _velocity = Vector3.Zero;
        private const float WalkSpeed = 4.0f;
        private const float Acceleration = 10.0f;

        public override void Enter()
        {
            if (AnimTree != null)
            {
                AnimTree.Set("parameters/conditions/walking", true);
                AnimTree.Set("parameters/conditions/idle", false);
                AnimTree.Set("parameters/conditions/sprinting", false);
            }
        }

        public override void Update(double delta)
        {
            if (Character == null) return;

            // Get input direction
            Vector2 inputDir = Input.GetVector("ui_left", "ui_right", "ui_up", "ui_down");
            
            if (inputDir.Length() < 0.1f)
            {
                // No input - transition to Idle
                EmitSignal(SignalName.TransitionRequested, "Idle");
                return;
            }

            // Check for sprint
            if (Input.IsKeyPressed(Key.Shift))
            {
                EmitSignal(SignalName.TransitionRequested, "Sprint");
                return;
            }

            // Calculate movement direction
            Vector3 forward = Character.GlobalTransform.Basis.Z.Normalized();
            Vector3 right = Character.GlobalTransform.Basis.X.Normalized();
            Vector3 direction = (-forward * inputDir.Y + right * inputDir.X).Normalized();

            // Apply acceleration
            Vector3 targetVelocity = direction * WalkSpeed;
            _velocity = _velocity.Lerp(targetVelocity, Acceleration * (float)delta);
            
            // Apply gravity
            _velocity.Y = -20.0f * (float)delta; // Simplified gravity

            Character.Velocity = _velocity;
            Character.MoveAndSlide();

            // Rotate to face movement direction
            if (direction.Length() > 0.1f)
            {
                var targetRot = new Vector3(0, Mathf.Atan2(direction.X, -direction.Z), 0);
                Character.Rotation = Character.Rotation.Lerp(
                    new Vector3(0, Mathf.LerpAngle(Character.Rotation.Y, targetRot.Y, 10.0f * (float)delta), 0),
                    0.1f
                );
            }
        }

        public override void HandleInput(PredictedInput input)
        {
            if (input.IsAttacking && Player != null && !Player.IsOnGlobalCoolown())
            {
                EmitSignal(SignalName.TransitionRequested, "Attack");
                return;
            }

            if (input.IsDodging && Player != null && !Player.IsOnGlobalCoolown())
            {
                EmitSignal(SignalName.TransitionRequested, "Dodge");
                return;
            }
        }

        public override void Exit()
        {
            if (AnimTree != null)
            {
                AnimTree.Set("parameters/conditions/walking", false);
            }
        }
    }
}
