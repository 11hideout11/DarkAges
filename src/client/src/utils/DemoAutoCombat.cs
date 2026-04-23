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
        
        private PredictedPlayer _player;
        private double _attackTimer = 0.0;
        private uint _lastTargetId = 0;
        
        public override void _Ready()
        {
            _player = GetNodeOrNull<PredictedPlayer>("../PredictedPlayer");
            if (_player == null)
            {
                _player = GetTree().Root.GetNodeOrNull<PredictedPlayer>("Main/PredictedPlayer");
            }
            
            GD.Print($"[DemoAutoCombat] Initialized, Enabled={Enabled}");
        }
        
        public override void _PhysicsProcess(double delta)
        {
            if (!Enabled || _player == null) return;
            if (GameState.Instance.CurrentConnectionState != GameState.ConnectionState.Connected) return;
            
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
            
            // Face target
            Vector3 direction = (targetPos - playerPos).Normalized();
            direction.Y = 0;
            if (direction.LengthSquared() > 0.01f)
            {
                _player.LookAt(targetPos, Vector3.Up);
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
                SendAttackInput();
            }
        }
        
        private EntityData FindNearestNPC()
        {
            Vector3 playerPos = _player?.GlobalPosition ?? Vector3.Zero;
            EntityData nearest = null;
            float nearestDist = float.MaxValue;
            
            foreach (var kvp in GameState.Instance.Entities)
            {
                uint entityId = kvp.Key;
                var entity = kvp.Value;
                
                // Skip local player and non-NPC entities (entityType 0=player, 3=NPC)
                if (entityId == GameState.Instance.LocalEntityId) continue;
                if (entity.Type != 3 && entity.Type != 0) continue; // Accept both NPC and player types
                
                float dist = playerPos.DistanceTo(entity.Position);
                if (dist < nearestDist && dist < 50.0f) // Only consider entities within 50 units
                {
                    nearestDist = dist;
                    nearest = entity;
                }
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
    }
}
