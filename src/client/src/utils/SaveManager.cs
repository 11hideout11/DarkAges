using Godot;
using System;
using System.Collections.Generic;
using System.IO;
using System.Text.Json;

namespace DarkAges.Client
{
    /// <summary>
    /// Save/Load system for persisting player progress locally.
    /// Supports multiple save slots and settings persistence.
    /// </summary>
    public partial class SaveManager : Node
    {
        public const int MAX_SAVE_SLOTS = 3;
        private const string SAVE_FOLDER = "saves/";
        private const string SETTINGS_FILE = "user://settings.json";
        
        // Save data structure
        private class SaveData
        {
            public int saveVersion = 1;
            public PlayerSaveData player = new();
            public InventorySaveData inventory = new();
            public EquipmentSaveData equipment = new();
            public List<int> abilities = new();
            public List<QuestSaveData> quests = new();
            public string savedAt = "";
        }
        
        private class PlayerSaveData
        {
            public string playerId = "";
            public string name = "";
            public int level = 1;
            public int experience = 0;
            public int gold = 0;
            public float posX = 0, posY = 0, posZ = 0;
            public uint zoneId = 1;
        }
        
        private class InventorySaveData
        {
            public List<InventorySlotSave> slots = new();
        }
        
        private class InventorySlotSave
        {
            public int slot = 0;
            public int itemId = 0;
            public int quantity = 0;
        }
        
        private class EquipmentSaveData
        {
            public int mainHand = 0;
            public int offHand = 0;
            public int head = 0;
            public int chest = 0;
            public int legs = 0;
            public int feet = 0;
            public int ring = 0;
            public int amulet = 0;
        }
        
        private class QuestSaveData
        {
            public int id = 0;
            public string status = "";
            public Dictionary<string, int> progress = new();
        }
        
        // Settings structure
        private class GameSettings
        {
            public GraphicsSettings graphics = new();
            public AudioSettings audio = new();
            public ControlsSettings controls = new();
        }
        
        private class GraphicsSettings
        {
            public string quality = "medium";
            public int width = 1920;
            public int height = 1080;
            public bool fullscreen = false;
            public bool vsync = true;
        }
        
        private class AudioSettings
        {
            public float masterVolume = 0.8f;
            public float sfxVolume = 0.7f;
            public float musicVolume = 0.5f;
            public bool mute = false;
        }
        
        private class ControlsSettings
        {
            public float mouseSensitivity = 1.0f;
            public bool invertY = false;
        }
        
        // Singleton
        private static SaveManager _instance;
        public static SaveManager Instance => _instance;
        
        public override void _Ready()
        {
            _instance = this;
            EnsureSaveDirectory();
            GD.Print("[SaveManager] Initialized");
        }
        
        private void EnsureSaveDirectory()
        {
            var saveDir = GetSaveDirectory();
            if (!DirExists(saveDir))
            {
                MakeDirectory(saveDir);
            }
        }
        
        private string GetSaveDirectory()
        {
            return Path.Combine(OS.GetUserDataDir(), SAVE_FOLDER);
        }
        
        private string GetSavePath(int slot)
        {
            return Path.Combine(GetSaveDirectory(), $"save_{slot}.json");
        }
        
        /// <summary>
        /// Save current game state to slot
        /// </summary>
        public bool SaveGame(int slotIndex = 0)
        {
            if (slotIndex < 0 || slotIndex >= MAX_SAVE_SLOTS)
            {
                GD.PrintErr($"[SaveManager] Invalid slot: {slotIndex}");
                return false;
            }
            
            var saveData = new SaveData
            {
                saveVersion = 1,
                savedAt = DateTime.UtcNow.ToString("o")
            };
            
            // Capture player state from GameState
            if (GameState.Instance != null && GameState.Instance.LocalEntityId > 0)
            {
                // Try to get entity data
                if (GameState.Instance.Entities.TryGetValue(GameState.Instance.LocalEntityId, out var entity))
                {
                    saveData.player = new PlayerSaveData
                    {
                        name = entity.Name,
                        posX = entity.Position.X,
                        posY = entity.Position.Y,
                        posZ = entity.Position.Z,
                        zoneId = 1
                    };
                }
                else
                {
                    // Store basic position if no entity data
                    saveData.player = new PlayerSaveData
                    {
                        name = "Player",
                        posX = 0,
                        posY = 0,
                        posZ = 0,
                        zoneId = 1
                    };
                }
            }
            
            // Serialize to JSON
            try
            {
                var json = JsonSerializer.Serialize(saveData);
                var path = GetSavePath(slotIndex);
                
                // Write file
                using var file = File.OpenWrite(path);
                using var writer = new StreamWriter(file);
                writer.Write(json);
                
                GD.Print($"[SaveManager] Saved to slot {slotIndex}");
                return true;
            }
            catch (Exception e)
            {
                GD.PrintErr($"[SaveManager] Save failed: {e.Message}");
                return false;
            }
        }
        
        /// <summary>
        /// Load game state from slot
        /// </summary>
        public bool LoadGame(int slotIndex = 0)
        {
            if (slotIndex < 0 || slotIndex >= MAX_SAVE_SLOTS)
            {
                GD.PrintErr($"[SaveManager] Invalid slot: {slotIndex}");
                return false;
            }
            
            var path = GetSavePath(slotIndex);
            if (!File.Exists(path))
            {
                GD.Print($"[SaveManager] No save data in slot {slotIndex}");
                return false;
            }
            
            try
            {
                var json = File.ReadAllText(path);
                var saveData = JsonSerializer.Deserialize<SaveData>(json);
                
                if (saveData == null)
                {
                    GD.PrintErr($"[SaveManager] Failed to parse save data");
                    return false;
                }
                
                // Restore player state to GameState
                if (GameState.Instance?.LocalPlayer != null)
                {
                    var player = GameState.Instance.LocalPlayer;
                    player.Name = saveData.player.name;
                    player.Level = saveData.player.level;
                    player.Experience = saveData.player.experience;
                    player.Gold = saveData.player.gold;
                    
                    // Teleport to saved position
                    var position = new Vector3(
                        saveData.player.posX,
                        saveData.player.posY,
                        saveData.player.posZ
                    );
                    player.Teleport(position, saveData.player.zoneId);
                }
                
                GD.Print($"[SaveManager] Loaded from slot {slotIndex}");
                return true;
            }
            catch (Exception e)
            {
                GD.PrintErr($"[SaveManager] Load failed: {e.Message}");
                return false;
            }
        }
        
        /// <summary>
        /// Delete save in slot
        /// </summary>
        public bool DeleteSave(int slotIndex = 0)
        {
            if (slotIndex < 0 || slotIndex >= MAX_SAVE_SLOTS)
                return false;
            
            var path = GetSavePath(slotIndex);
            if (File.Exists(path))
            {
                try
                {
                    File.Delete(path);
                    GD.Print($"[SaveManager] Deleted slot {slotIndex}");
                    return true;
                }
                catch (Exception e)
                {
                    GD.PrintErr($"[SaveManager] Delete failed: {e.Message}");
                }
            }
            return false;
        }
        
        /// <summary>
        /// Check if save exists in slot
        /// </summary>
        public bool HasSaveData(int slotIndex = 0)
        {
            if (slotIndex < 0 || slotIndex >= MAX_SAVE_SLOTS)
                return false;
            
            return File.Exists(GetSavePath(slotIndex));
        }
        
        /// <summary>
        /// Get info about save slots
        /// </summary>
        public List<SaveSlotInfo> GetSaveSlots()
        {
            var slots = new List<SaveSlotInfo>();
            
            for (int i = 0; i < MAX_SAVE_SLOTS; i++)
            {
                var info = new SaveSlotInfo
                {
                    slotIndex = i,
                    exists = HasSaveData(i)
                };
                
                if (info.exists)
                {
                    try
                    {
                        var json = File.ReadAllText(GetSavePath(i));
                        var data = JsonSerializer.Deserialize<SaveData>(json);
                        if (data != null)
                        {
                            info.playerName = data.player.name;
                            info.level = data.player.level;
                            info.zoneId = data.player.zoneId;
                            if (DateTime.TryParse(data.savedAt, out var time))
                            {
                                info.savedAt = time;
                            }
                        }
                    }
                    catch { }
                }
                
                slots.Add(info);
            }
            
            return slots;
        }
        
        /// <summary>
        /// Save game settings
        /// </summary>
        public void SaveSettings(GameSettings settings)
        {
            try
            {
                var json = JsonSerializer.Serialize(settings);
                var path = Path.Combine(OS.GetUserDataDir(), SETTINGS_FILE);
                
                using var file = File.OpenWrite(path);
                using var writer = new StreamWriter(file);
                writer.Write(json);
                
                GD.Print("[SaveManager] Settings saved");
            }
            catch (Exception e)
            {
                GD.PrintErr($"[SaveManager] Settings save failed: {e.Message}");
            }
        }
        
        /// <summary>
        /// Load game settings
        /// </summary>
        public GameSettings LoadSettings()
        {
            var path = Path.Combine(OS.GetUserDataDir(), SETTINGS_FILE);
            if (!File.Exists(path))
            {
                return new GameSettings();
            }
            
            try
            {
                var json = File.ReadAllText(path);
                return JsonSerializer.Deserialize<GameSettings>(json) ?? new GameSettings();
            }
            catch
            {
                return new GameSettings();
            }
        }
        
        /// <summary>
        /// Auto-save trigger (call periodically)
        /// </summary>
        public void AutoSave()
        {
            // Auto-save to slot 0
            SaveGame(0);
        }
    }
    
    /// <summary>
    /// Save slot information
    /// </summary>
    public class SaveSlotInfo
    {
        public int slotIndex;
        public bool exists;
        public string playerName = "";
        public int level = 1;
        public uint zoneId = 1;
        public DateTime savedAt;
    }
}