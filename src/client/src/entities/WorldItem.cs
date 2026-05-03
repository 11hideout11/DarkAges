using Godot;
using System;
using DarkAges.Networking;

namespace DarkAges.Client
{
    /// <summary>
    /// World item - represents dropped loot on the ground.
    /// Can be picked up by players.
    /// </summary>
    public partial class WorldItem : RigidBody3D
    {
        [Export] public uint ItemId { get; set; } = 0;
        [Export] public int Quantity { get; set; } = 1;
        [Export] public float DespawnTime { get; set; } = 60.0f;
        [Export] public float LootRadius { get; set; } = 2.0f;
        
        private bool _isGold = false;
        private bool _canLoot = true;
        private double _spawnTime;
        
        // Item data (would load from item database)
        private string _itemName = "Unknown Item";
        
        public override void _Ready()
        {
            _spawnTime = Time.GetTicksMsec() / 1000.0;
            
            // Setup collision
            CollisionLayer = 0;  // Don't collide with physics
            CollisionMask = 0;
            
            // Create visual
            CreateItemVisual();
            
            // Add to looters group
            AddToGroup("world_item");
            
            GD.Print($"[WorldItem] Spawned: {ItemId} x{Quantity}");
        }
        
        private void CreateItemVisual()
        {
            // Create mesh instance
            var mesh = new MeshInstance3D();
            mesh.Name = "ItemMesh";
            
            // Different visuals for different item types
            if (_isGold)
            {
                // Gold coin visual
                var sphere = new SphereMesh
                {
                    Radius = 0.15f,
                    Height = 0.3f
                };
                mesh.Mesh = sphere;
                
                var goldMat = new StandardMaterial3D
                {
                    AlbedoColor = new Color(1, 0.84f, 0),  // Gold
                    Metallic = 0.8f,
                    Roughness = 0.3f
                };
                mesh.MaterialOverride = goldMat;
            }
            else
            {
                // Item box visual
                var box = new BoxMesh
                {
                    Size = new Vector3(0.4f, 0.4f, 0.4f)
                };
                mesh.Mesh = box;
                
                var itemMat = new StandardMaterial3D
                {
                    AlbedoColor = GetItemColor(),
                    EmissionEnabled = true,
                    Emission = GetItemColor() * 0.3f
                };
                mesh.MaterialOverride = itemMat;
            }
            
            AddChild(mesh);
            
            // Floating animation
            var tween = CreateTween();
            var startPos = mesh.Position;
            
            while (true)
            {
                tween.TweenProperty(mesh, "position", startPos + new Vector3(0, 0.2f, 0), 1.0f);
                tween.TweenProperty(mesh, "position", startPos, 1.0f);
                // Loop would require different approach in GDScript
            }
        }
        
        private Color GetItemColor()
        {
            // Color based on item type (simplified)
            if (ItemId >= 100 && ItemId < 200)  // Consumable
                return new Color(0.8f, 0.2f, 0.2f);
            if (ItemId >= 200 && ItemId < 300)  // Material
                return new Color(0.5f, 0.5f, 0.5f);
            if (ItemId >= 300 && ItemId < 400)  // Equipment
                return new Color(0.6f, 0.6f, 0.8f);
            
            return new Color(0.9f, 0.9f, 0.7f);  // Default
        }
        
        public override void _Process(double delta)
        {
            // Check despawn timer
            double currentTime = Time.GetTicksMsec() / 1000.0;
            if (currentTime - _spawnTime > DespawnTime)
            {
                Despawn();
                return;
            }
            
            // Check for nearby players to auto-loot gold
            if (_isGold && _canLoot)
            {
                CheckPlayerProximity();
            }
        }
        
        private void CheckPlayerProximity()
        {
            // Get nearby players
            var area = GetTree().GetFirstNodeInGroup("local_player");
            if (area == null) return;
            
            var player = area as Node3D;
            if (player == null) return;
            
            var dist = GlobalPosition.DistanceTo(player.GlobalPosition);
            if (dist < LootRadius)
            {
                // Auto-loot gold
                AttemptPickup(player);
            }
        }
        
        private void _OnBodyEntered(Node body)
        {
            // Used when colliding with area
            if (body.Name == "Player" || body.IsInGroup("local_player"))
            {
                AttemptPickup(body);
            }
        }
        
        private void AttemptPickup(Node picker)
        {
            if (!_canLoot) return;
            
            _canLoot = false;
            
            // Request pickup from server
            if (NetworkManager.Instance != null)
            {
                var data = new byte[10];
                data[0] = 15;  // PACKET_LOOT_PICKUP
                BitConverter.GetBytes(ItemId).CopyTo(data, 1);
                BitConverter.GetBytes((uint)Quantity).CopyTo(data, 5);
                
                NetworkManager.Instance.CallDeferred("SendReliable", data);
            }
            
            // Visual feedback
            PlayPickupEffect();
            
            GD.Print($"[WorldItem] Looting: {ItemId} x{Quantity}");
        }
        
        private void PlayPickupEffect()
        {
            // Particle effect or animation on pickup
            var tween = CreateTween();
            tween.TweenProperty(this, "scale", Vector3.Zero, 0.2f);
            tween.TweenCallback(Callable.From(QueueFree));
        }
        
        private void Despawn()
        {
            // Fade out and remove
            var tween = CreateTween();
            tween.TweenProperty(this, "modulate:a", 0.0f, 0.5f);
            tween.TweenCallback(Callable.From(QueueFree));
        }
        
        /// <summary>
        /// Set item data from server
        /// </summary>
        public void Initialize(uint itemId, int quantity, bool isGold = false)
        {
            ItemId = itemId;
            Quantity = quantity;
            _isGold = isGold;
            
            // Get item name from database
            if (!isGold)
            {
                _itemName = GetItemName(itemId);
            }
            else
            {
                _itemName = $"{quantity} Gold";
            }
        }
        
        private string GetItemName(uint itemId)
        {
            // Would look up from items.json
            // Simplified for now
            if (itemId == 101) return "Health Potion";
            if (itemId == 201) return "Iron Ore";
            if (itemId == 301) return "Iron Sword";
            return $"Item {itemId}";
        }
        
        /// <summary>
        /// Get item name for display
        /// </summary>
        public string ItemName => _itemName;
    }
}