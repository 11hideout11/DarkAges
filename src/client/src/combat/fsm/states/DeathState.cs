using Godot;
using System;

namespace DarkAges.Combat.FSM.States
{
    /// <summary>
    /// Death state - player has died.
    /// Transitions: None (wait for respawn, handled externally)
    /// </summary>
    [GlobalClass]
    public partial class DeathState : State
    {
        public override void Enter()
        {
            if (AnimTree != null)
            {
                AnimTree.Set("parameters/conditions/dead", true);
                AnimTree.Set("parameters/conditions/idle", false);
            }

            if (Player != null)
            {
                Player.TriggerDeath();
            }

            GD.Print("[DeathState] Player died. Waiting for respawn...");
        }

        public override void Update(double delta)
        {
            // Death state is exited externally when respawn happens
            // The PredictedPlayer or server will trigger respawn
        }

        public override void Exit()
        {
            if (AnimTree != null)
            {
                AnimTree.Set("parameters/conditions/dead", false);
            }

            if (Player != null)
            {
                Player.TriggerRespawn();
            }
        }
    }
}
