using Godot;
using System;

namespace DarkAges.Combat.FSM.States
{
    /// <summary>
    /// Sprint state - player is running at sprint speed.
    /// Transitions: Walk (shift released or no input), Attack, Dodge
    /// </summary>
    [GlobalClass]
    public partial class SprintState : State
    {
        private Vector3 _velocity = Vector3.Zero;
        private const float SprintSpeed = 8.0f;
        private const float Acceleration = 12.0f;

        public override void Enter()
        {
            if (AnimTree != null)
            {
                AnimTree.Set("parameters/conditions/sprinting", true);
                AnimTree.Set("parameters/conditions/walking", false);
                AnimTree.Set("parameters/conditions/idle", false);
            }
        }

        public override void Update(double delta)
        {
            if (Character == null) return;

            Vector2 inputDir = Input.GetVector("ui_left", "ui_right", "ui_up", "ui_down");

            if (inputDir.Length() < 0.1f)
            {
                EmitSignal(SignalName.TransitionRequested, "Idle");
                return;
            }

            // Check if sprint is released
            if (!Input.IsKeyPressed(Key.Shift))
            {
                EmitSignal(SignalName.TransitionRequested, "Walk");
                return;
            }

            Vector3 forward = Character.GlobalTransform.Basis.Z.Normalized();
            Vector3 right = Character.GlobalTransform.Basis.X.Normalized();
            Vector3 direction = (-forward * inputDir.Y + right * inputDir.X).Normalized();

            Vector3 targetVelocity = direction * SprintSpeed;
            _velocity = _velocity.Lerp(targetVelocity, Acceleration * (float)delta);

            Character.Velocity = _velocity;
            Character.MoveAndSlide();

            if (direction.Length() > 0.1f)
            {
                var targetRot = new Vector3(0, Mathf.Atan2(direction.X, -direction.Z), 0);
                Character.Rotation = new Vector3(
                    0,
                    Mathf.LerpAngle(Character.Rotation.Y, targetRot.Y, 10.0f * (float)delta),
                    0
                );
            }
        }

        public override void HandleInput(PredictedInput input)
        {
            if (input.IsAttacking && Player != null && !Player.IsOnGlobalCoolown())
            {
                EmitSignal(SignalName.TransitionRequested, "Attack");
            }
        }

        public override void Exit()
        {
            if (AnimTree != null)
            {
                AnimTree.Set("parameters/conditions/sprinting", false);
            }
        }
    }
}
