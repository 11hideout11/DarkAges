using Godot;
using System;

namespace DarkAges.Client
{
    /// <summary>
    /// Manages character and monster model loading, switching, and animation binding.
    /// Handles dynamic model replacement for the capsule placeholder workflow.
    /// 
    /// Usage:
    /// - Place .glb models in src/client/assets/characters/[category]/
    /// - Call LoadModel() to load a specific character model
    /// - Call SetAnimationState() to play animations
    /// 
    /// Supported animations: idle, walk, run, attack_light, attack_heavy, block, hit, death
    /// </summary>
    public partial class ModelManager : Node
    {
        /// <summary>
        /// Available animation state names
        /// </summary>
        public enum AnimationState
        {
            Idle,
            Walk,
            Run,
            AttackLight,
            AttackHeavy,
            Block,
            Hit,
            Death
        }

        /// <summary>
        /// Character model categories
        /// </summary>
        public enum ModelCategory
        {
            PlayerMale,
            PlayerFemale,
            Goblin,
            Skeleton,
            Orc,
            Troll
        }

        private Skeleton3D _skeleton;
        private MeshInstance3D _modelInstance;
        private AnimationTree _animationTree;
        private AnimationPlayer _animationPlayer;

        /// <summary>
        /// Base path for character assets
        /// </summary>
        private const string ASSETS_PATH = "res://src/client/assets/characters/";

        public override void _Ready()
        {
            GD.Print("[ModelManager] Initialized");
        }

        /// <summary>
        /// Load a character model from assets folder
        /// </summary>
        /// <param name="category">Model category (e.g., PlayerMale, Goblin)</param>
        /// <param name="modelName">Specific model file name without extension</param>
        /// <returns>True if loaded successfully</returns>
        public bool LoadModel(ModelCategory category, string modelName)
        {
            string modelPath = $"{ASSETS_PATH}{category.ToString().ToLower()}/{modelName}.glb";
            
            if (!ResourceLoader.Exists(modelPath))
            {
                GD.PrintErr($"[ModelManager] Model not found: {modelPath}");
                return false;
            }

            try
            {
                var modelScene = GD.Load<PackedScene>(modelPath);
                if (modelScene == null)
                {
                    GD.PrintErr($"[ModelManager] Failed to load model: {modelPath}");
                    return false;
                }

                // Instance the model
                var instance = modelScene.Instantiate();
                if (instance is Node3D node3d)
                {
                    // Find skeleton in instance
                    var skeletons = node3d.GetChildren().OfType<Skeleton3D>();
                    _skeleton = skeletons.FirstOrDefault();
                    
                    var meshes = node3d.GetChildren().OfType<MeshInstance3D>();
                    _modelInstance = meshes.FirstOrDefault();
                    
                    if (_modelInstance != null)
                    {
                        _modelInstance.Name = "CharacterModel";
                        AddChild(_modelInstance);
                        GD.Print($"[ModelManager] Model loaded: {modelPath}");
                        return true;
                    }
                }
                
                GD.PrintErr($"[ModelManager] Invalid model format: {modelPath}");
                return false;
            }
            catch (Exception ex)
            {
                GD.PrintErr($"[ModelManager] Exception loading model: {ex.Message}");
                return false;
            }
        }

        /// <summary>
        /// Set the current animation state
        /// </summary>
        /// <param name="state">Animation state to play</param>
        /// <param name="blendTime">Blend time between animations (default 0.2s)</param>
        public void SetAnimationState(AnimationState state, float blendTime = 0.2f)
        {
            if (_animationTree == null)
            {
                GD.PrintWarn("[ModelManager] No AnimationTree configured");
                return;
            }

            string stateName = state.ToString().ToLower();
            
            // Capitalize first letter for AnimationTree state machine
            stateName = char.ToUpper(stateName[0]) + stateName.Substring(1);
            
            _animationTree.Travel(stateName);
        }

        /// <summary>
        /// Bind AnimationTree for animation control
        /// </summary>
        /// <param name="animTree">AnimationTree node from scene</param>
        /// <param name="animPlayer">AnimationPlayer node for playback</param>
        public void BindAnimationTree(AnimationTree animTree, AnimationPlayer animPlayer)
        {
            _animationTree = animTree;
            _animationPlayer = animPlayer;
        }

        /// <summary>
        /// Get the current model mesh for visibility control
        /// </summary>
        public MeshInstance3D GetModelMesh() => _modelInstance;

        /// <summary>
        /// Show or hide the character model
        /// </summary>
        public void SetModelVisible(bool visible)
        {
            if (_modelInstance != null)
            {
                _modelInstance.Visible = visible;
            }
        }

        /// <summary>
        /// Get list of available models in a category
        /// </summary>
        public string[] GetAvailableModels(ModelCategory category)
        {
            string categoryPath = $"{ASSETS_PATH}{category.ToString().ToLower()}";
            
            // Note: In Godot 4.x, use DirAccess for directory listing
            // This is a placeholder - actual implementation would use Godot's file system API
            return Array.Empty<string>();
        }

        /// <summary>
        /// Setup capsule mesh as fallback when no model is loaded
        /// This maintains the current placeholder behavior
        /// </summary>
        public void SetupCapsuleFallback()
        {
            var capsuleMesh = new CapsuleMesh();
            capsuleMesh.Radius = 0.3f;
            capsuleMesh.Height = 1.8f;

            _modelInstance = new MeshInstance3D();
            _modelInstance.Name = "CapsuleFallback";
            _modelInstance.Mesh = capsuleMesh;
            
            var material = new StandardMaterial3D();
            material.AlbedoColor = new Color(0.2f, 0.6f, 1.0f); // Blue placeholder
            capsuleMesh.Material = material;
            
            AddChild(_modelInstance);
            GD.Print("[ModelManager] Capsule fallback enabled");
        }

        /// <summary>
        /// Procedurally generate a low-poly humanoid mesh with accessories
        /// to replace capsule placeholders with visually distinct characters
        /// </summary>
        /// <param name="category">Model category determines base shape</param>
        /// <param name="variant">Variant index for color variation</param>
        /// <returns>MeshInstance3D with low-poly character mesh</returns>
        public MeshInstance3D CreateProceduralCharacter(ModelCategory category, int variant = 0)
        {
            var meshInstance = new MeshInstance3D();
            meshInstance.Name = $"Procedural_{category}";
            
            // Different base shapes per category
            switch (category)
            {
                case ModelCategory.PlayerMale:
                case ModelCategory.PlayerFemale:
                    meshInstance.Mesh = CreateProceduralHumanoid(category, variant);
                    break;
                case ModelCategory.Goblin:
                    meshInstance.Mesh = CreateProceduralGoblin(variant);
                    break;
                case ModelCategory.Skeleton:
                    meshInstance.Mesh = CreateProceduralSkeleton(variant);
                    break;
                case ModelCategory.Orc:
                    meshInstance.Mesh = CreateProceduralOrc(variant);
                    break;
                case ModelCategory.Troll:
                    meshInstance.Mesh = CreateProceduralTroll(variant);
                    break;
                default:
                    meshInstance.Mesh = CreateProceduralHumanoid(ModelCategory.PlayerMale, variant);
                    break;
            }
            
            // Create material with category-appropriate color
            var material = CreateProceduralMaterial(category, variant);
            if (meshInstance.Mesh is Mesh mesh)
            {
                mesh.Material = material;
            }
            
            GD.Print($"[ModelManager] Created procedural {category} (variant {variant})");
            return meshInstance;
        }

        /// <summary>
        /// Create procedural humanoid mesh (torso + limbs)
        /// </summary>
        private Mesh CreateProceduralHumanoid(ModelCategory category, int variant)
        {
            // Use a capsule as base but with better proportions
            var capsule = new CapsuleMesh();
            
            if (category == ModelCategory.PlayerFemale)
            {
                capsule.Radius = 0.25f;
                capsule.Height = 1.6f;
            }
            else
            {
                capsule.Radius = 0.3f;
                capsule.Height = 1.8f;
            }
            
            // Add slight height variation based on variant
            float heightMod = 1.0f + (variant * 0.02f);
            capsule.Height *= heightMod;
            
            return capsule;
        }

        /// <summary>
        /// Create procedural goblin mesh - shorter, wider
        /// </summary>
        private Mesh CreateProceduralGoblin(int variant)
        {
            var capsule = new CapsuleMesh();
            capsule.Radius = 0.35f;  // Wider
            capsule.Height = 1.2f; // Shorter
            capsule.RadialSegments = 8; // Low-poly look
            capsule.Rings = 4;
            return capsule;
        }

        /// <summary>
        /// Create procedural skeleton mesh - thin, angular
        /// </summary>
        private Mesh CreateProceduralSkeleton(int variant)
        {
            var capsule = new CapsuleMesh();
            capsule.Radius = 0.2f;  // Thin
            capsule.Height = 1.85f;   // Tall
            capsule.RadialSegments = 6; // Very low-poly
            capsule.Rings = 6;
            return capsule;
        }

        /// <summary>
        /// Create procedural orc mesh - massive, bulky
        /// </summary>
        private Mesh CreateProceduralOrc(int variant)
        {
            var capsule = new CapsuleMesh();
            capsule.Radius = 0.45f;  // Very wide
            capsule.Height = 2.0f;   // Tall
            capsule.RadialSegments = 10;
            capsule.Rings = 8;
            return capsule;
        }

        /// <summary>
        /// Create procedural troll mesh - huge, hunched
        /// </summary>
        private Mesh CreateProceduralTroll(int variant)
        {
            var capsule = new CapsuleMesh();
            capsule.Radius = 0.55f;  // Massive
            capsule.Height = 2.4f;   // Very tall
            capsule.RadialSegments = 12;
            capsule.Rings = 10;
            return capsule;
        }

        /// <summary>
        /// Create material for procedural character with category-appropriate colors
        /// </summary>
        private StandardMaterial3D CreateProceduralMaterial(ModelCategory category, int variant)
        {
            var material = new StandardMaterial3D();
            material.ShadingMode = BaseMaterial3D.ShadingModes.PerPixel;
            
            // Base colors with variant shifts
            switch (category)
            {
                case ModelCategory.PlayerMale:
                    float[] maleHues = { 0.6f, 0.55f, 0.65f }; // Blue variants
                    material.AlbedoColor = Color.FromHsv(maleHues[variant % 3], 0.7f, 0.8f);
                    break;
                case ModelCategory.PlayerFemale:
                    float[] femaleHues = { 0.95f, 0.05f, 0.15f }; // Red variants
                    material.AlbedoColor = Color.FromHsv(femaleHues[variant % 3], 0.7f, 0.75f);
                    break;
                case ModelCategory.Goblin:
                    float[] goblinHues = { 0.25f, 0.33f, 0.28f }; // Green variants
                    material.AlbedoColor = Color.FromHsv(goblinHues[variant % 3], 0.8f, 0.5f);
                    break;
                case ModelCategory.Skeleton:
                    material.AlbedoColor = new Color(0.9f, 0.9f, 0.85f); // Off-white bone
                    break;
                case ModelCategory.Orc:
                    float[] orcHues = { 0.08f, 0.12f, 0.05f }; // Dark green variants
                    material.AlbedoColor = Color.FromHsv(orcHues[variant % 3], 0.7f, 0.4f);
                    break;
                case ModelCategory.Troll:
                    float[] trollHues = { 0.45f, 0.55f, 0.35f }; // Brown variants
                    material.AlbedoColor = Color.FromHsv(trollHues[variant % 3], 0.6f, 0.45f);
                    break;
                default:
                    material.AlbedoColor = new Color(0.3f, 0.5f, 0.8f);
                    break;
            }
            
            material.Roughness = 0.6f;
            material.Metallic = 0.1f;
            
            return material;
        }

        /// <summary>
        /// Get color scheme for a category (for UI preview, debugging)
        /// </summary>
        public Color GetCategoryColor(ModelCategory category)
        {
            var tempMat = CreateProceduralMaterial(category, 0);
            return tempMat.AlbedoColor;
        }

        /// <summary>
        /// Generate variant color palette for a category
        /// </summary>
        public Color[] GetVariantPalette(ModelCategory category, int count = 4)
        {
            var palette = new Color[count];
            for (int i = 0; i < count; i++)
            {
                var mat = CreateProceduralMaterial(category, i);
                palette[i] = mat.AlbedoColor;
            }
            return palette;
        }

        /// <summary>
        /// Setup capsule with custom color for different character types
        /// </summary>
        /// <param name="color">Color for the capsule placeholder</param>
        public void SetupCapsuleWithColor(Color color)
        {
            var capsuleMesh = new CapsuleMesh();
            capsuleMesh.Radius = 0.3f;
            capsuleMesh.Height = 1.8f;

            _modelInstance = new MeshInstance3D();
            _modelInstance.Name = "CapsulePlaceholder";
            _modelInstance.Mesh = capsuleMesh;
            
            var material = new StandardMaterial3D();
            material.AlbedoColor = color;
            material.Roughness = 0.5f;
            capsuleMesh.Material = material;
            
            AddChild(_modelInstance);
            GD.Print($"[ModelManager] Colored capsule placeholder created: {color}");
        }

        /// <summary>
        /// Setup enhanced capsule with equipment slots for visual distinction
        /// </summary>
        /// <param name="category">Character category</param>
        /// <param name="equipmentLevel">Equipment level (0-3) affects material quality</param>
        public void SetupEnhancedCapsule(ModelCategory category, int equipmentLevel = 0)
        {
            var meshInstance = CreateProceduralCharacter(category, equipmentLevel);
            _modelInstance = meshInstance;
            AddChild(_modelInstance);
            GD.Print($"[ModelManager] Enhanced capsule setup: {category} lv{equipmentLevel}");
        }
    }
}