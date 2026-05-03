using Godot;
using System;
using DarkAges.Networking;
using System.Collections.Generic;

namespace DarkAges.Client.UI
{
    /// <summary>
    /// Quest tracker panel: displays active quests and objective progress.
    /// Toggles with the J key (or 'quest_toggle' action).
    /// </summary>
    public partial class QuestTracker : CanvasLayer
    {
        [Export] public int MaxDisplayedQuests = 10;
        [Export] public int MaxDisplayedZoneObjectives = 5;

        private RichTextLabel _questList;
        private RichTextLabel _zoneObjectiveList;
        private Dictionary<uint, Dictionary<uint, (uint current, uint required, byte status)>> _quests;
        private Dictionary<string, (ushort current, ushort required, byte type, byte wave)> _zoneObjectives;

        public override void _Ready()
        {
            _questList = GetNode<RichTextLabel>("QuestList");
            _zoneObjectiveList = GetNode<RichTextLabel>("ZoneObjectiveList");
            _quests = new Dictionary<uint, Dictionary<uint, (uint, uint, byte)>>();
            _zoneObjectives = new Dictionary<string, (ushort, ushort, byte, byte)>();

            // Subscribe to network quest updates
            if (NetworkManager.Instance != null)
            {
                NetworkManager.Instance.QuestUpdateReceived += OnQuestUpdateReceived;
                NetworkManager.Instance.ZoneObjectiveUpdateReceived += OnZoneObjectiveUpdateReceived;
                GD.Print("[QuestTracker] Subscribed to quest and zone objective updates");
            }
        }

        public override void _ExitTree()
        {
            if (NetworkManager.Instance != null)
            {
                NetworkManager.Instance.QuestUpdateReceived -= OnQuestUpdateReceived;
                NetworkManager.Instance.ZoneObjectiveUpdateReceived -= OnZoneObjectiveUpdateReceived;
            }
        }

        public override void _Process(double delta)
        {
            // Toggle visibility with 'quest_toggle' input action (default: J key)
            if (Input.IsActionJustPressed("quest_toggle"))
            {
                Visible = !Visible;
            }
        }

        private void OnQuestUpdateReceived(uint questId, uint objectiveIndex, uint current, uint required, byte status)
        {
            GD.Print($"[QuestTracker] Update: quest={questId} obj={objectiveIndex} cur={current}/{required} status={status}");

            if (!_quests.ContainsKey(questId))
            {
                _quests[questId] = new Dictionary<uint, (uint, uint, byte)>();
            }
            var objDict = _quests[questId];
            objDict[objectiveIndex] = (current, required, status);

            RefreshDisplay();
        }

        private void OnZoneObjectiveUpdateReceived(byte eventType, string objectiveId, ushort currentProgress, ushort requiredProgress, byte waveNumber, string message)
        {
            // Store zone objective progress
            _zoneObjectives[objectiveId] = (currentProgress, requiredProgress, eventType, waveNumber);
            
            GD.Print($"[QuestTracker] Zone objective: id={objectiveId} progress={currentProgress}/{requiredProgress} wave={waveNumber}");
            
            RefreshDisplay();
        }

        private void RefreshDisplay()
        {
            if (_questList == null) return;

            _questList.Clear();
            foreach (var kvp in _quests)
            {
                uint questId = kvp.Key;
                var objectives = kvp.Value;

                string questTitle = questId == 99 ? "Kill Rats" : $"Quest {questId}";
                _questList.AppendText($"[color=Yellow]{questTitle}[/color]\n");

                foreach (var objKvp in objectives)
                {
                    uint idx = objKvp.Key;
                    var (cur, req, status) = objKvp.Value;
                    string statusText = status == 1 ? "[color=Green]✓ COMPLETE[/color]" : $"[color=White]{cur}/{req}[/color]";
                    _questList.AppendText($"  Obj {idx}: {statusText}\n");
                }
            }
            
            // Also display zone objectives if any
            if (_zoneObjectiveList != null && _zoneObjectives.Count > 0)
            {
                _zoneObjectiveList.Clear();
                _zoneObjectiveList.AppendText("[color=Cyan]Zone Objectives:[/color]\n");
                foreach (var kvp in _zoneObjectives)
                {
                    string objId = kvp.Key;
                    var (cur, req, type, wave) = kvp.Value;
                    string statusText = cur >= req ? "[color=Green]✓ COMPLETE[/color]" : $"[color=White]{cur}/{req}[/color]";
                    string waveText = wave > 0 ? $" (Wave {wave})" : "";
                    _zoneObjectiveList.AppendText($"  {objId}: {statusText}{waveText}\n");
                }
            }
        }

        /// <summary>
        /// Show the quest tracker and grab focus (optional, used by HUD keybind)
        /// </summary>
        public void ShowTracker()
        {
            Visible = true;
        }
    }
}
