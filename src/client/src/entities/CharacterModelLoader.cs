using Godot;
using System;
using System.Collections.Generic;
using System.Threading.Tasks;

namespace DarkAges.Entities
{
    /// <summary>
    /// Manages loading and swapping 3D character/monster models at runtime.
    /// Falls back to capsule placeholder if model assets are not available.
    /// </summary>
    public partial class CharacterModelLoader : Node
    {
        // Model cache to avoid reloading
        private Dictionary<string, PackedScene> _modelCache = new Dictionary<string, PackedScene>();
        
        // Model path configuration
        private const string ModelBasePath = "res://src/client/assets/characters/";
        
        // Character type identifiers
        public enum CharacterType
        {
            None = 0,
            PlayerMale = 1,
            PlayerFemale = 2,
            Goblin = 10,
            Skeleton = 11,
            Orc = 12,
            Troll = 13,
            NPC = 20,
            Boss = 30
        }
        
        // Character type to model path mapping
        private static readonly Dictionary<CharacterType, string> ModelPaths = new Dictionary<CharacterType, string>
        {
            { CharacterType.PlayerMale, "player_male/player_male.tscn" },
            { CharacterType.PlayerFemale, "player_female/player_female.tscn" },
            { CharacterType.Goblin, "monsters/goblin/goblin.tscn" },
            { CharacterType.Skeleton, "monsters/skeleton/skeleton.tscn" },
            { CharacterType.Orc, "monsters/orc/orc.tscn" },
            { CharacterType.Troll, "monsters/troll/troll.tscn" },
            { CharacterType.NPC, "npcs/village_guide/village_guide.tscn" },
            { CharacterType.Boss, "monsters/boss/boss.tscn" }
        };
        
        // Material colors for placeholders (when no model available)
        private static readonly Dictionary<CharacterType, Color> PlaceholderColors = new Dictionary<CharacterType, Color>
        {
            { CharacterType.PlayerMale, new Color(0.2, 0.6, 1.0) },      // Blue
            { CharacterType.PlayerFemale, new Color(1.0, 0.6, 0.8) },    // Pink
            { CharacterType.Goblin, new Color(0.3, 0.6, 0.3) },       // Green
            { CharacterType.Skeleton, new Color(0.9, 0.9, 0.9) },     // White/Gray
            { CharacterType.Orc, new Color(0.4, 0.5, 0.3) },        // Dark Green
            { CharacterType.Troll, new Color(0.5, 0.3, 0.2) },          // Brown
            { CharacterType.NPC, new Color(0.8, 0.7, 0.5) },           // Tan
            { CharacterType.Boss, new Color(0.8, 0.2, 0.2) }           // Red
        };
        
        public override void _Ready()
        {
            GD.Print("[CharacterModelLoader] Initialized");
        }
        
        /// <summary>
        /// Attempt to load a model scene for the given character type.
        /// Returns null if model doesn't exist, falls back to placeholder.
        /// </summary>
        public PackedScene LoadModel(CharacterType type)
        {
            string path = ModelPaths.GetValueOrDefault(type, string.Empty);
            if (string.IsNullOrEmpty(path))
            {
                return null;
            }
            
            string fullPath = ModelBasePath + path;
            
            // Check cache first
            if (_modelCache.TryGetValue(fullPath, out var cached))
            {
                return cached;
            }
            
            // Try to load from disk
            try
            {
                var scene = GD.Load<PackedScene>(fullPath);
                if (scene != null)
                {
                    _modelCache[fullPath] = scene;
                    GD.Print($"[CharacterModelLoader] Loaded model: {fullPath}");
                    return scene;
                }
            }
            catch (Exception ex)
            {
                GD.PrintErr($"[CharacterModelLoader] Failed to load {fullPath}: {ex.Message}");
            }
            
            return null;
        }
        
        /// <summary>
        /// Get placeholder color for a character type (used when no model is loaded).
        /// </summary>
        public Color GetPlaceholderColor(CharacterType type)
        {
            return PlaceholderColors.GetValueOrDefault(type, new Color(0.5, 0.5, 0.5));
        }

        /// <summary>
        /// Convert CharacterType to ModelManager category for procedural generation.
        /// </summary>
        public static Client.ModelManager.ModelCategory GetModelCategory(CharacterType type)
        {
            return type switch
            {
                CharacterType.PlayerMale => Client.ModelManager.ModelCategory.PlayerMale,
                CharacterType.PlayerFemale => Client.ModelManager.ModelCategory.PlayerFemale,
                CharacterType.Goblin => Client.ModelManager.ModelCategory.Goblin,
                CharacterType.Skeleton => Client.ModelManager.ModelCategory.Skeleton,
                CharacterType.Orc => Client.ModelManager.ModelCategory.Orc,
                CharacterType.Troll => Client.ModelManager.ModelCategory.Troll,
                _ => Client.ModelManager.ModelCategory.PlayerMale
            };
        }

        /// <summary>
        /// Get visual radius for collision detection based on character type.
        /// Different monster types have different collision sizes.
        /// </summary>
        public static float GetCollisionRadius(CharacterType type)
        {
            return type switch
            {
                CharacterType.PlayerMale => 0.3f,
                CharacterType.PlayerFemale => 0.25f,
                CharacterType.Goblin => 0.35f,
                CharacterType.Skeleton => 0.2f,
                CharacterType.Orc => 0.45f,
                CharacterType.Troll => 0.55f,
                CharacterType.NPC => 0.3f,
                CharacterType.Boss => 0.5f,
                _ => 0.3f
            };
        }

        /// <summary>
        /// Get visual height for the character type.
        /// </summary>
        public static float GetVisualHeight(CharacterType type)
        {
            return type switch
            {
                CharacterType.PlayerMale => 1.8f,
                CharacterType.PlayerFemale => 1.6f,
                CharacterType.Goblin => 1.2f,
                CharacterType.Skeleton => 1.85f,
                CharacterType.Orc => 2.0f,
                CharacterType.Troll => 2.4f,
                CharacterType.NPC => 1.7f,
                CharacterType.Boss => 2.5f,
                _ => 1.8f
            };
        }
        
        /// <summary>
        /// Check if a model exists for the given type.
        /// </summary>
        public bool HasModel(CharacterType type)
        {
            return LoadModel(type) != null;
        }
        
        /// <summary>
        /// Clear model cache (useful for debugging or reloading).
        /// </summary>
        public void ClearCache()
        {
            _modelCache.Clear();
            GD.Print("[CharacterModelLoader] Cache cleared");
        }
        
        /// <summary>
        /// Parse character type from string (e.g., "goblin", "skeleton", "orc").
        /// </summary>
        public static CharacterType ParseCharacterType(string typeName)
        {
            if (string.IsNullOrEmpty(typeName))
                return CharacterType.None;
            
            string lower = typeName.ToLowerInvariant().Trim();
            
            return lower switch
            {
                "player_male" or "player" or "male" => CharacterType.PlayerMale,
                "player_female" or "female" => CharacterType.PlayerFemale,
                "goblin" => CharacterType.Goblin,
                "skeleton" => CharacterType.Skeleton,
                "orc" => CharacterType.Orc,
                "troll" => CharacterType.Troll,
                "npc" or "guide" or "merchant" or "elder" => CharacterType.NPC,
                "boss" => CharacterType.Boss,
                _ => CharacterType.None
            };
        }
    }
}