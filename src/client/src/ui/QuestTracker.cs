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

        private RichTextLabel _questList;
        private Dictionary<uint, Dictionary<uint, (uint current, uint required, byte status)>> _quests;

        public override void _Ready()
        {
            _questList = GetNode<RichTextLabel>("QuestList");
            _quests = new Dictionary<uint, Dictionary<uint, (uint, uint, byte)>>();

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
            // For now, log zone objective events.
            // TODO: display in dedicated zone objective UI panel
            GD.Print($"[QuestTracker] Zone objective event: type={eventType} id={objectiveId} progress={currentProgress}/{requiredProgress} wave={waveNumber} msg=\"{message}\"");
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
