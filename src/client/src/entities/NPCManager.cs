using Godot;
using System;
using System.Collections.Generic;
using System.IO;
using System.Text.Json;
using DarkAges.Entities;
using DarkAges.Networking;

namespace DarkAges.Client.UI
{
    /// <summary>
    /// NPC Interaction System - handles NPC spawning and dialogue.
    /// </summary>
    public partial class NPCManager : Node
    {
        // Singleton
        private static NPCManager _instance;
        public static NPCManager Instance => _instance;
        
        // NPC data structures
        private class DialogueData
        {
            public string id = "";
            public uint npcId = 0;
            public string name = "";
            public string greeting = "";
            public Dictionary<string, DialogueNode> nodes = new();
            public List<DialogueOption> options = new();
        }
        
        private class DialogueNode
        {
            public string text = "";
            public string action = "";
            public string next = "";
        }
        
        private class DialogueOption
        {
            public string text = "";
            public string next = "";
            public string action = "";
            public string require = "";
        }
        
        private Dictionary<uint, DialogueData> _dialogues = new();
        private List<uint> _activeNPCs = new();
        
        // Interaction state
        private uint _interactionTarget = 0;
        private string _currentDialogueId = "";
        private bool _isInteracting = false;
        
        public override void _Ready()
        {
            _instance = this;
            LoadDialogues();
            GD.Print("[NPCManager] Initialized");
        }
        
        private void LoadDialogues()
        {
            var path = "res://data/dialogues.json";
            if (!File.Exists(path))
            {
                GD.Print($"[NPCManager] Dialogue file not found: {path}");
                return;
            }
            
            try
            {
                var json = File.ReadAllText(path);
                var data = JsonSerializer.Deserialize<DialoguesFile>(json);
                
                if (data?.dialogues != null)
                {
                    foreach (var dialogue in data.dialogues)
                    {
                        if (dialogue.npcId > 0)
                        {
                            _dialogues[dialogue.npcId] = dialogue;
                        }
                    }
                    GD.Print($"[NPCManager] Loaded {_dialogues.Count} dialogues");
                }
            }
            catch (Exception e)
            {
                GD.PrintErr($"[NPCManager] Failed to load dialogues: {e.Message}");
            }
        }
        
        private class DialoguesFile
        {
            public List<DialogueData> dialogues { get; set; }
        }
        
        public override void _Process(double delta)
        {
            CheckNPCProximity();
        }
        
        private void CheckNPCProximity()
        {
            if (_isInteracting) return;
            // Guard: LocalPlayer lookup not yet implemented
            // if (GameState.Instance?.LocalPlayer == null) return;
            _isInteracting = false;
            return;
            /* DISABLED: GameState.LocalPlayer not yet implemented
            var player = GameState.Instance.LocalPlayer;
            var playerPos = player.Position;
            
            // Find nearest NPC
            uint nearestNPC = 0;
            float nearestDist = 3.0f; // Interaction range
            
            foreach (var npcId in _activeNPCs)
            {
                // In real implementation, get NPC position from entity
                // For now, check against known NPC positions
            }
            
            if (nearestNPC != _interactionTarget)
            {
                _interactionTarget = nearestNPC;
                // Show interaction prompt
            }
            */
        }
        
        /// <summary>
        /// Start interaction with NPC
        /// </summary>
        public void InteractWithNPC(uint npcId)
        {
            if (!_dialogues.TryGetValue(npcId, out var dialogue))
            {
                GD.Print($"[NPCManager] No dialogue found for NPC {npcId}");
                return;
            }
            
            _currentDialogueId = dialogue.id;
            _isInteracting = true;
            
            // Open dialogue UI
            ShowDialogue(dialogue);
            
            GD.Print($"[NPCManager] Interacting with NPC {npcId}: {dialogue.name}");
        }
        
        private void ShowDialogue(DialogueData dialogue)
        {
            // Get dialogue panel and populate
            var ui = GetTree().GetFirstNodeInGroup("dialogue_panel");
            if (ui != null)
            {
                // Call methods to show dialogue
                // (Actual implementation depends on DialoguePanel.cs)
            }
        }
        
        /// <summary>
        /// Called when player presses interact key near NPC
        /// </summary>
        public void OnInteractPressed()
        {
            if (_interactionTarget > 0 && !_isInteracting)
            {
                InteractWithNPC(_interactionTarget);
            }
        }
        
        /// <summary>
        /// Advance dialogue
        /// </summary>
        public void SelectDialogueOption(int optionIndex)
        {
            if (!_dialogues.TryGetValue(_interactionTarget, out var dialogue)) return;
            if (optionIndex < 0 || optionIndex >= dialogue.options.Count) return;
            
            var option = dialogue.options[optionIndex];
            
            // Execute action if present
            if (!string.IsNullOrEmpty(option.action))
            {
                ExecuteDialogueAction(option.action);
            }
            
            // Check if this closes dialogue
            if (string.IsNullOrEmpty(option.next))
            {
                EndDialogue();
            }
            // Otherwise, advance to next node
        }
        
        private void ExecuteDialogueAction(string action)
        {
            switch (action)
            {
                case "open_shop":
                    GD.Print("[NPCManager] Opening shop...");
                    // Open shop UI
                    break;
                case "show_quest_1":
                    GD.Print("[NPCManager] Showing quest...");
                    // Show quest offering
                    break;
                case "accept_quest_1":
                    GD.Print("[NPCManager] Accepting quest...");
                    // Accept quest
                    break;
                case "show_tutorial":
                    GD.Print("[NPCManager] Showing tutorial...");
                    break;
            }
        }
        
        /// <summary>
        /// End current dialogue
        /// </summary>
        public void EndDialogue()
        {
            _isInteracting = false;
            _currentDialogueId = "";
            _interactionTarget = 0;
            
            GD.Print("[NPCManager] Dialogue ended");
        }
        
        /// <summary>
        /// Register NPC as active
        /// </summary>
        public void RegisterNPC(uint npcId, uint entityId)
        {
            if (!_activeNPCs.Contains(npcId))
            {
                _activeNPCs.Add(npcId);
            }
        }
        
        /// <summary>
        /// Check if currently in dialogue
        /// </summary>
        public bool IsInteracting => _isInteracting;
    }
}