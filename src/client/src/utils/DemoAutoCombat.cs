using Godot;
using System;
using System.Linq;
using DarkAges.Networking;

namespace DarkAges.Client.Utils
{
    /// <summary>
    /// [DEMO_AGENT] Automated combat demonstration for showcase
    /// Automatically finds nearest NPC, faces it, and sends attack inputs
    /// </summary>
 public partial class DemoAutoCombat : Node
 {
 [Export] public bool Enabled = true;
 [Export] public float AttackRange = 8.0f;
 [Export] public float AttackInterval = 0.8f;
 [Export] public float MoveSpeed = 4.0f;
 [Export] public bool AutoMoveToTarget = true;
 [Export] public bool UseAbilities = true; // NEW: Cycle through abilities
 
 private PredictedPlayer _player;
 private double _attackTimer = 0.0;
 private uint _lastTargetId = 0;
 private int _abilityIndex = 0; // NEW: Tracks which ability to use next
 private const int AbilityCount = 4; // NEW: 0=melee, 1=fireball, 2=heal, 3=power_strike
        
        public override void _Ready()
        {
            _player = GetNodeOrNull<PredictedPlayer>("../PredictedPlayer");
            if (_player == null)
            {
                _player = GetTree().Root.GetNodeOrNull<PredictedPlayer>("Main/PredictedPlayer");
            }
            
            SetProcess(true);
            
            GD.Print($"[DemoAutoCombat] Initialized, Enabled={Enabled}");
        }
        
        public override void _Process(double delta)
        {
            if (!Enabled || _player == null) return;
            if (GameState.Instance.CurrentConnectionState != GameState.ConnectionState.Connected) return;
            
            try
            {
                // Find nearest NPC entity
                var nearest = FindNearestNPC();
                if (nearest == null)
                {
                    _lastTargetId = 0;
                    return;
                }
                
                _lastTargetId = nearest.Id;
                Vector3 playerPos = _player.GlobalPosition;
                Vector3 targetPos = nearest.Position;
                float distance = playerPos.DistanceTo(targetPos);
                
                // Face target using safe method
                Vector3 direction = (targetPos - playerPos).Normalized();
                direction.Y = 0;
                if (direction.LengthSquared() > 0.01f)
                {
                    // Safe look_at that works even if node tree state changes
                    var newTransform = _player.GlobalTransform;
                    newTransform.Basis = new Basis(Vector3.Up, Mathf.Atan2(direction.X, direction.Z));
                    _player.GlobalTransform = newTransform;
                }
                
                // Move toward target if out of range
                if (AutoMoveToTarget && distance > AttackRange)
                {
                    Vector3 moveDir = (targetPos - playerPos).Normalized();
                    moveDir.Y = 0;
                    _player.Velocity = new Vector3(moveDir.X * MoveSpeed, _player.Velocity.Y, moveDir.Z * MoveSpeed);
                    _player.MoveAndSlide();
                }
                
 // Attack periodically when in range
 _attackTimer += delta;
 if (_attackTimer >= AttackInterval && distance <= AttackRange + 2.0f)
 {
 _attackTimer = 0.0;
 if (UseAbilities)
 {
 SendAbilityInput(_abilityIndex);
 _abilityIndex = (_abilityIndex + 1) % AbilityCount; // Cycle through abilities
 }
 else
 {
 SendAttackInput();
 }
 }
            }
            catch (Exception ex)
            {
                GD.PrintErr($"[DemoAutoCombat] Error in _Process: {ex.Message}");
            }
        }
        
        private EntityData FindNearestNPC()
        {
            Vector3 playerPos = _player?.GlobalPosition ?? Vector3.Zero;
            EntityData nearest = null;
            float nearestDist = float.MaxValue;
            
            GD.Print($"[DemoAutoCombat] Finding NPCs. Player pos={playerPos}, Entity count={GameState.Instance.Entities.Count}");
            
            foreach (var kvp in GameState.Instance.Entities)
            {
                uint entityId = kvp.Key;
                var entity = kvp.Value;
                
                // Skip local player and non-NPC entities (entityType 0=player, 3=NPC)
                if (entityId == GameState.Instance.LocalEntityId) continue;
                if (entity.Type != 3 && entity.Type != 0) continue; // Accept both NPC and player types
                
                float dist = playerPos.DistanceTo(entity.Position);
                GD.Print($"[DemoAutoCombat]  Entity {entityId} type={entity.Type} pos={entity.Position} dist={dist:F1}");
                
                if (dist < nearestDist && dist < 50.0f)
                {
                    nearestDist = dist;
                    nearest = entity;
                }
            }
            
            if (nearest != null)
            {
                GD.Print($"[DemoAutoCombat] Nearest target: {nearest.Id} at dist={nearestDist:F1}");
            }
            else
            {
                GD.Print("[DemoAutoCombat] No valid target found");
            }
            
            return nearest;
        }
        
 private void SendAttackInput()
 {
 // Inject attack input into the input queue
 // We do this by temporarily setting the attack flag in the next input packet
 // Since NetworkManager gathers input from Godot's Input system, we can use Input.ActionPress
 Input.ActionPress("attack");
 
 // Release after a short delay so it's a tap, not a hold
 var timer = new Timer();
 timer.WaitTime = 0.1f;
 timer.OneShot = true;
 timer.Timeout += () =>
 {
 Input.ActionRelease("attack");
 timer.QueueFree();
 };
 AddChild(timer);
 timer.Start();
 
 GD.Print($"[DemoAutoCombat] Attack sent toward entity {_lastTargetId}");
 }
 
 // [DEMO_AGENT] Send ability input based on ability slot index
 private void SendAbilityInput(int abilityIndex)
 {
 string actionName = abilityIndex switch
 {
 0 => "attack", // Melee attack
 1 => "ability_1", // Fireball
 2 => "ability_2", // Heal
 3 => "ability_3", // Power Strike
 _ => "attack"
 };
 
 // Press and immediately release the action
 Input.ActionPress(actionName);
 
 var timer = new Timer();
 timer.WaitTime = 0.1f;
 timer.OneShot = true;
 timer.Timeout += () =>
 {
 Input.ActionRelease(actionName);
 timer.QueueFree();
 };
 AddChild(timer);
 timer.Start();
 
 GD.Print($"[DemoAutoCombat] Ability {actionName} (slot {abilityIndex}) sent toward entity {_lastTargetId}");
 }
 }
}
