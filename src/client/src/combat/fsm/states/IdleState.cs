using Godot;
using System;

namespace DarkAges.Combat.FSM.States
{
    /// <summary>
    /// Idle state - player is standing still, no movement input.
    /// Transitions: Walk (movement input), Attack (attack input), Dodge (dodge input)
    /// </summary>
    [GlobalClass]
    public partial class IdleState : State
    {
        public override void Enter()
        {
            // Set animation parameter
            if (AnimTree != null)
            {
                AnimTree.Set("parameters/conditions/idle", true);
                AnimTree.Set("parameters/conditions/walking", false);
                AnimTree.Set("parameters/conditions/sprinting", false);
            }
        }

        public override void Update(double delta)
        {
            // Check for movement input
            Vector2 inputDir = Input.GetVector("ui_left", "ui_right", "ui_up", "ui_down");
            if (inputDir.Length() > 0.1f)
            {
                // Transition to Walk or Run based on shift key
                bool isSprinting = Input.IsKeyPressed(Key.Shift);
                EmitSignal(SignalName.TransitionRequested, isSprinting ? "Sprint" : "Walk");
                return;
            }

            // Apply gravity if not on floor
            if (Character != null && !Character.IsOnFloor())
            {
                Character.Velocity = new Vector3(
                    Character.Velocity.X,
                    Character.Velocity.Y - 20.0f * (float)delta,
                    Character.Velocity.Z
                );
                Character.MoveAndSlide();
            }
        }

        public override void HandleInput(PredictedInput input)
        {
            // Attack input
            if (input.IsAttacking && Player != null && !Player.IsOnGlobalCoolown())
            {
                EmitSignal(SignalName.TransitionRequested, "Attack");
                return;
            }

            // Dodge input (Q key)
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
                AnimTree.Set("parameters/conditions/idle", false);
            }
        }
    }
}
