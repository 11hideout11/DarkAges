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
    }
}