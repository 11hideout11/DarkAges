using Godot;
using System;
using System.Collections.Generic;

namespace DarkAges.Combat.FSM
{
    /// <summary>
    /// Node-based Finite State Machine for managing player states.
    /// Add as a child node, then add State nodes as children.
    /// Call ChangeState() to transition between states.
    /// </summary>
    [GlobalClass]
    public partial class StateMachine : Node
    {
        /// <summary>
        /// Currently active state.
        /// </summary>
        public State CurrentState { get; private set; }

        /// <summary>
        /// Reference to the CharacterBody3D (set by PredictedPlayer).
        /// </summary>
        public CharacterBody3D Character { get; set; }

        /// <summary>
        /// Reference to the PredictedPlayer.
        /// </summary>
        public PredictedPlayer Player { get; set; }

        /// <summary>
        /// Reference to the AnimationTree.
        /// </summary>
        public AnimationTree AnimTree { get; set; }

        /// <summary>
        /// Dictionary of state names to State nodes.
        /// </summary>
        private Dictionary<string, State> _states = new();

        /// <summary>
        /// Initialize the state machine.
        /// Call after adding all states as children.
        /// </summary>
        public void Initialize()
        {
            // Collect all State children
            foreach (Node child in GetChildren())
            {
                if (child is State state)
                {
                    _states[state.Name] = state;
                    state.Character = Character;
                    state.Player = Player;
                    state.AnimTree = AnimTree;
                    
                    // Connect the transition signal
                    state.TransitionRequested += OnTransitionRequested;
                    
                    // Disable processing by default
                    state.ProcessMode = ProcessModeEnum.Disabled;
                }
            }
        }

        /// <summary>
        /// Change to a new state.
        /// </summary>
        /// <param name="stateName">Name of the state to transition to</param>
        /// <param name="data">Optional data to pass to the new state's Enter()</param>
        public void ChangeState(string stateName, Godot.Collections.Dictionary data = null)
        {
            if (!_states.ContainsKey(stateName))
            {
                GD.PrintErr($"[StateMachine] State '{stateName}' not found!");
                return;
            }

            State previousState = CurrentState;
            State nextState = _states[stateName];

            // Exit current state
            if (previousState != null)
            {
                previousState.Exit();
                previousState.ProcessMode = ProcessModeEnum.Disabled;
            }

            // Enter new state
            CurrentState = nextState;
            CurrentState.ProcessMode = ProcessModeEnum.Inherit;
            CurrentState.Enter();

            GD.Print($"[StateMachine] Transitioned: {previousState?.Name ?? "null"} -> {CurrentState.Name}");
        }

        /// <summary>
        /// Called when a state requests a transition.
        /// </summary>
        private void OnTransitionRequested(string nextStateName, Godot.Collections.Dictionary data)
        {
            ChangeState(nextStateName, data);
        }

        /// <summary>
        /// Physics update - calls CurrentState.Update().
        /// Called by PredictedPlayer._PhysicsProcess().
        /// </summary>
        public void PhysicsUpdate(double delta)
        {
            CurrentState?.Update(delta);
        }

        /// <summary>
        /// Handle input - calls CurrentState.HandleInput().
        /// Called by PredictedPlayer when processing input.
        /// </summary>
        public void HandleInput(PredictedInput input)
        {
            CurrentState?.HandleInput(input);
        }

        /// <summary>
        /// Get the name of the current state.
        /// </summary>
        public string GetCurrentStateName()
        {
            return CurrentState?.Name ?? "None";
        }
    }
}
