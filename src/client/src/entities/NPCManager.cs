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
            
            // Get local player - use PredictedPlayer if available
            var player = GetTree.CurrentScene?.GetNodeOrNull<PredictedPlayer>("../Player");
            if (player == null)
                player = GetTree().Root.GetNodeOrNull<PredictedPlayer>("Main/Players/Player");
            if (player == null)
                player = GetTree().Root.GetNodeOrNull<PredictedPlayer>("Players/Player");
            if (player == null)
                return;
            
            var playerPos = player.GlobalPosition;
            uint localEntityId = GameState.Instance.LocalEntityId;

            // Find nearest NPC within interaction range
            uint nearestNPC = 0;
            float nearestDist = float.MaxValue;
            string nearestName = "";

            foreach (var kvp in GameState.Instance.Entities)
            {
                uint entityId = kvp.Key;
                EntityData entity = kvp.Value;

                // Skip local player
                if (entityId == localEntityId)
                    continue;

                // Only process NPC-type entities (EntityType::NPC = 3)
                if (entity.Type != 3)
                    continue;

                // Skip entities with no interaction range (non-interactable)
                if (entity.InteractionRange <= 0.0f)
                    continue;

                // Calculate distance from player to NPC
                float dist = playerPos.DistanceTo(entity.Position);

                if (dist < entity.InteractionRange && dist < nearestDist)
                {
                    nearestNPC = entityId;
                    nearestDist = dist;
                    nearestName = entity.PromptText;
                }
            }

            if (nearestNPC != _interactionTarget)
            {
                _interactionTarget = nearestNPC;
                
                // Show/hide interaction prompt
                if (nearestNPC != 0)
                {
                    // Try to find dialogue data for this NPC
                    string promptText = nearestName;
                    if (_dialogues.TryGetValue(nearestNPC, out var dialogue))
                    {
                        promptText = $"Press E to talk to {dialogue.name}";
                    }
                    else
                    {
                        // Fall back to entity's prompt text if no dialogue registered
                        if (string.IsNullOrEmpty(promptText) || promptText == "Press E to interact")
                            promptText = $"Press E to interact";
                    }
                    
                    var prompt = GetTree().GetFirstNodeInGroup("interaction_prompt");
                    if (prompt is InteractionPrompt ip)
                    {
                        ip.ShowPrompt(nearestNPC, promptText, nearestDist);
                    }
                }
                else
                {
                    var prompt = GetTree().GetFirstNodeInGroup("interaction_prompt");
                    if (prompt is InteractionPrompt ip)
                    {
                        ip.HidePrompt();
                    }
                }
            }
            
            // Check for interaction key press
            if (Input.IsActionJustPressed("interact") && _interactionTarget != 0)
            {
                InteractWithNPC(_interactionTarget);
            }
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