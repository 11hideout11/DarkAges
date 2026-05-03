using Godot;
using System;
using DarkAges.Entities;

namespace DarkAges
{
    /// <summary>
    /// ProceduralCharacterVisual - Enhanced placeholder visual system
    /// Creates visually distinct procedural characters when real models aren't available
    /// 
    /// Usage:
    /// - Attach to player/remotedepot layer to enhance capsule placeholders
    /// - Set CharacterType to generate appropriate visual
    /// - Can upgrade to real model when available
    /// </summary>
    public partial class ProceduralCharacterVisual : Node3D
    {
        [Export]
        public CharacterModelLoader.CharacterType CharacterType = CharacterModelLoader.CharacterType.PlayerMale;

        [Export]
        public int EquipmentLevel = 0;

        [Export]
        public bool EnableColorVariation = true;

        private MeshInstance3D _visualMesh;
        private StandardMaterial3D _material;
        private Client.ModelManager _modelManager;

        public override void _Ready()
        {
            // Get or create ModelManager
            _modelManager = GetNodeOrNull<Client.ModelManager>("/root/ModelManager") as Client.ModelManager;
            if (_modelManager == null)
            {
                _modelManager = new Client.ModelManager();
                _modelManager.Name = "ModelManager";
                GetTree().CurrentScene.AddChild(_modelManager);
            }

            CreateProceduralVisual();
        }

        /// <summary>
        /// Create procedural visual based on character type
        /// </summary>
        private void CreateProceduralVisual()
        {
            // Determine variant based on equipment or random
            int variant = EnableColorVariation ? EquipmentLevel : 0;

            // Get ModelCategory from CharacterType
            var category = CharacterModelLoader.GetModelCategory(CharacterType);

            // Use ModelManager to create procedural character
            var meshInstance = _modelManager.CreateProceduralCharacter(category, variant);
            
            _visualMesh = meshInstance;
            AddChild(_visualMesh);

            // Match collision shape dimensions
            float radius = CharacterModelLoader.GetCollisionRadius(CharacterType);
            float height = CharacterModelLoader.GetVisualHeight(CharacterType);

            // Scale visual to match collision
            float visualScale = height / 1.8f; // Base height of player capsule
            _visualMesh.Scale = new Vector3(radius / 0.3f * visualScale, visualScale, radius / 0.3f * visualScale);

            GD.Print($"[ProceduralCharacterVisual] Created {CharacterType} (variant {variant}): H={height:F1}, R={radius:F1}");
        }

        /// <summary>
        /// Get current visual mesh for animation binding
        /// </summary>
        public MeshInstance3D GetVisualMesh() => _visualMesh;

        /// <summary>
        /// Set visual to match equipment level changes
        /// </summary>
        public void UpdateEquipmentLevel(int newLevel)
        {
            if (EquipmentLevel == newLevel)
                return;

            EquipmentLevel = newLevel;

            // Remove old mesh
            if (_visualMesh != null)
            {
                _visualMesh.QueueFree();
            }

            // Create new visual
            CreateProceduralVisual();
        }

        /// <summary>
        /// Get color for this character type
        /// </summary>
        public Color GetCharacterColor()
        {
            return _modelManager.GetCategoryColor(
                CharacterModelLoader.GetModelCategory(CharacterType));
        }

        /// <summary>
        /// Get variant color palette
        /// </summary>
        public Color[] GetColorPalette(int count = 4)
        {
            return _modelManager.GetVariantPalette(
                CharacterModelLoader.GetModelCategory(CharacterType), count);
        }
    }
}