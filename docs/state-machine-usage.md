# CombatStateMachine Usage Guide

## Overview

This guide explains how to use the node-based CombatStateMachine for third-person combat in Godot 4.2.

## Adding to a Scene

1. Add `CombatStateMachine.tscn` as a child node of your character:
   ```
   Player (CharacterBody3D)
   └── CombatStateMachine (instantiate CombatStateMachine.tscn)
   ```

2. The CombatStateMachine will automatically find and connect to:
   - `AnimationTree` (at `../AnimationTree`)
   - `AnimationPlayer` (at `../AnimationPlayer`)

## States

The FSM supports 7 combat states:

| State | Description | Can Transition To |
|-------|-------------|-----------------|
| Idle | Standing still | Walk, Run, Attack, Dodge, Hit, Death |
| Walk | Walking (slow) | Idle, Run, Attack, Dodge, Hit, Death |
| Run | Sprinting | Idle, Walk, Attack, Dodge, Hit, Death |
| Attack | Melee attack | Idle, Walk, Run (on complete) |
| Dodge | Rolling dodge | Idle, Hit, Death |
| Hit | Taking damage | Idle, Walk, Run, Death |
| Death | Character dead | Idle (on respawn) |

## Global Cooldown

A 1.2s global cooldown (GCD) prevents attack spam:
- GCD starts when attacking or dodging
- Can transition to Idle during GCD
- Cannot attack again until GCD completes

## Integration with AnimationStateMachine

The CombatStateMachineController syncs with the existing code-based `AnimationStateMachine.cs`:
- State transitions propagate to code FSM
- Code FSM actions (TriggerAttack, etc.) update node FSM

## Customizing in Inspector

Export properties:
- `AttackDuration` (default: 0.5s)
- `DodgeDuration` (default: 0.4s) 
- `HitDuration` (default: 0.3s)
- `GlobalCooldown` (default: 1.2s)
- `BlendTime` (default: 0.1s)

## Signals

Connect to these signals for game logic:

```csharp
// C# example
var fsm = GetNode<CombatStateMachineController>("CombatStateMachine");
fsm.StateEntered += OnStateEntered;
fsm.CooldownCompleted += OnCooldownReady;

void OnStateEntered(string state) {
    GD.Print($"Entered: {state}");
}

void OnCooldownReady() {
    GD.Print("Attack ready!");
}
```

| Signal | Description |
|--------|-------------|
| StateEntered | Emitted when entering a state |
| StateExited | Emitted when exiting a state |
| TransitionRequested | Emitted on transition attempt |
| CooldownStarted | Emitted when GCD starts |
| CooldownCompleted | Emitted when GCD ends |

## Adding New States

To add custom states:

1. Edit `CombatStateMachine.tscn` in Godot editor
2. Add state to AnimationNodeStateMachine
3. Add transitions in the editor
4. Update `CombatStateMachineController.cs` enum and switch cases

---

Last updated: 2026-05-01