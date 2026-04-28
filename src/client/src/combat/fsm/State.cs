using Godot;
using System;

namespace DarkAges.Combat.FSM
{
    /// <summary>
    /// Base class for all FSM states.
    /// States are Nodes that can be added to the scene tree.
    /// </summary>
    public abstract partial class State : Node
    {
        /// <summary>
        /// The CharacterBody3D this state operates on.
        /// Set by the StateMachine when entering the state.
        /// </summary>
        public CharacterBody3D Character { get; set; }

        /// <summary>
        /// Reference to the PredictedPlayer for accessing player-specific state.
        /// </summary>
        public PredictedPlayer Player { get; set; }

        /// <summary>
        /// Reference to the AnimationTree for animation control.
        /// </summary>
        public AnimationTree AnimTree { get; set; }

        /// <summary>
        /// Called when entering this state.
        /// Override to implement state entry logic (play animation, set flags, etc).
        /// </summary>
        public virtual void Enter() { }

        /// <summary>
        /// Called when exiting this state.
        /// Override to implement state exit logic (cleanup, reset flags, etc).
        /// </summary>
        public virtual void Exit() { }

        /// <summary>
        /// Called every physics frame while this state is active.
        /// Override to implement state update logic.
        /// </summary>
        /// <param name="delta">Time since last physics frame</param>
        public virtual void Update(double delta) { }

        /// <summary>
        /// Called when an input event is received.
        /// Override to handle input-specific transitions.
        /// </summary>
        /// <param name="input">The predicted input data</param>
        public virtual void HandleInput(PredictedInput input) { }

        /// <summary>
        /// Signal emitted when this state wants to transition to another state.
        /// Connect this in the StateMachine.
        /// </summary>
        [Signal]
        public delegate void TransitionRequestedEventHandler(string nextStateName, Godot.Collections.Dictionary data = null);
    }
}
