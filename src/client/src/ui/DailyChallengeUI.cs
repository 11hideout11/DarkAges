using Godot;
using System;

namespace DarkAges.Client.UI
{
    /// <summary>
    /// UI Panel for displaying daily challenges and tracking progress.
    /// Shows up to 3 daily challenges with progress bars, rewards, and claim buttons.
    /// </summary>
    public class DailyChallengeUI : Panel
    {
        // Challenge UI elements (up to 3)
        private Panel[] _challengePanels;
        private Label[] _challengeNames;
        private ProgressBar[] _progressBars;
        private Label[] _progressLabels;
        private Button[] _claimButtons;
        private Label[] _rewardLabels;

        // Configuration
        [Export] public int MaxChallenges = 3;
        [Export] public float ProgressUpdateSpeed = 0.1f; // Seconds to animate progress change

        // Current challenge data
        private DailyChallengeData[] _currentChallenges;

        public override void _Ready()
        {
            GD.Print("[DailyChallengeUI] Initializing daily challenge UI...");

            // Initialize arrays
            _challengePanels = new Panel[MaxChallenges];
            _challengeNames = new Label[MaxChallenges];
            _progressBars = new ProgressBar[MaxChallenges];
            _progressLabels = new Label[MaxChallenges];
            _claimButtons = new Button[MaxChallenges];
            _rewardLabels = new Label[MaxChallenges];

            // Find child nodes (assuming a specific layout)
            for (int i = 0; i < MaxChallenges; i++)
            {
                string baseName = $"Challenge{i + 1}";
                
                _challengePanels[i] = GetNode<Panel>($"{baseName}/Panel") ?? new Panel();
                _challengeNames[i] = GetNode<Label>($"{baseName}/Name") ?? new Label();
                _progressBars[i] = GetNode<ProgressBar>($"{baseName}/ProgressBar") ?? new ProgressBar();
                _progressLabels[i] = GetNode<Label>($"{baseName}/ProgressLabel") ?? new Label();
                _claimButtons[i] = GetNode<Button>($"{baseName}/ClaimButton") ?? new Button();
                _rewardLabels[i] = GetNode<Label>($"{baseName}/RewardLabel") ?? new Label();

                // Add to scene if they don't exist (for programmatic creation)
                if (_challengePanels[i].GetParent() == null)
                {
                    AddChild(_challengePanels[i]);
                    _challengePanels[i].AddChild(_challengeNames[i]);
                    _challengePanels[i].AddChild(_progressBars[i]);
                    _challengePanels[i].AddChild(_progressLabels[i]);
                    _challengePanels[i].AddChild(_claimButtons[i]);
                    _challengePanels[i].AddChild(_rewardLabels[i]);
                }

                // Connect claim button
                _claimButtons[i].Pressed += (s) => ClaimChallenge(i);
            }

            // Initially hide all panels
            for (int i = 0; i < MaxChallenges; i++)
            {
                _challengePanels[i].Visible = false;
            }

            GD.Print("[DailyChallengeUI] Daily challenge UI initialized");
        }

        /// <summary>
        /// Update the daily challenges display with new data from server.
        /// </summary>
        /// <param name="accountId">Player account ID</param>
        /// <param name="challengeId">Challenge ID</param>
        /// <param name="progress">Current progress</param>
        /// <param name="xpReward">XP reward amount</param>
        /// <param name="goldReward">Gold reward amount</param>
        /// <param name="itemReward">Item reward ID</param>
        public void UpdateChallenge(uint accountId, uint challengeId, uint progress, 
            uint xpReward, uint goldReward, uint itemReward)
        {
            GD.Print($"[DailyChallengeUI] Updating challenge {challengeId}: progress={progress}");

            // Find the panel index for this challenge (simple linear search)
            int index = FindChallengeIndex(challengeId);
            bool isNew = (index == -1);

            if (isNew)
            {
                // Find first empty slot
                index = Array.FindIndex(_currentChallenges, c => c == null);
                if (index == -1) return; // All slots full, can't add more

                // Initialize new challenge data
                _currentChallenges[index] = new DailyChallengeData
                {
                    ChallengeId = challengeId,
                    AccountId = accountId,
                    XpReward = xpReward,
                    GoldReward = goldReward,
                    ItemReward = itemReward,
                    Progress = progress,
                    Completed = (progress >= GetTargetForChallenge(challengeId))
                };

                // Show the panel
                _challengePanels[index].Visible = true;
            }
            else
            {
                // Update existing challenge
                var data = _currentChallenges[index];
                data.Progress = progress;
                data.Completed = (progress >= GetTargetForChallenge(challengeId));
                _currentChallenges[index] = data;
            }

            // Update UI
            UpdateChallengePanel(index);
        }

        /// <summary>
        /// Find the index of a challenge by ID.
        /// </summary>
        private int FindChallengeIndex(uint challengeId)
        {
            if (_currentChallenges == null) return -1;
            for (int i = 0; i < _currentChallenges.Length; i++)
            {
                if (_currentChallenges[i] != null && _currentChallenges[i].ChallengeId == challengeId)
                    return i;
            }
            return -1;
        }

        /// <summary>
        /// Get the target value for a challenge (would normally come from server data).
        /// This is a placeholder - in a real implementation, we'd have a challenge definition database.
        /// </summary>
        private uint GetTargetForChallenge(uint challengeId)
        {
            // Placeholder: all challenges have target 10 for now
            // In reality, this would look up from a challenge definition file or database
            return 10;
        }

        /// <summary>
        /// Update the visual appearance of a challenge panel.
        /// </summary>
        private void UpdateChallengePanel(int index)
        {
            if (index < 0 || index >= MaxChallenges || _currentChallenges[index] == null)
                return;

            var data = _currentChallenges[index];
            uint target = GetTargetForChallenge(data.ChallengeId);

            // Update name/description (placeholder)
            _challengeNames[index].Text = $"Challenge {data.ChallengeId + 1}: Defeat Enemies";
            _challengeNames[index].FontSize = 16;

            // Update progress bar
            float progressRatio = Math.Min((float)data.Progress / target, 1.0f);
            _progressBars[index].Value = progressRatio;
            _progressBars[index].Visible = true;

            // Update progress label
            _progressLabels[index].Text = $"{data.Progress} / {target}";
            _progressLabels[index].FontSize = 14;

            // Update reward label
            _rewardLabels[index].Text = $"Rewards: {data.XpReward} XP, {data.GoldReward} Gold";
            _rewardLabels[index].FontSize = 14;

            // Update claim button
            bool canClaim = data.Completed && !data.Claimed;
            _claimButtons[index].Visible = canClaim;
            _claimButtons[index].Text = canClaim ? "Claim Rewards" : "";
            _claimButtons[index].Disabled = !canClaim;

            // Style based on completion
            if (data.Completed)
            {
                _challengePanels[index].Modulate = new Color(1.0f, 0.9f, 0.7f); // Gold tint for completed
            }
            else
            {
                _challengePanels[index].Modulate = Colors.White;
            }
        }

        /// <summary>
        /// Handle claim button press for a challenge.
        /// </summary>
        private void ClaimChallenge(int index)
        {
            if (index < 0 || index >= MaxChallenges || _currentChallenges[index] == null)
                return;

            var data = _currentChallenges[index];

            GD.Print($"[DailyChallengeUI] Claiming challenge {data.ChallengeId}");

            // In a real implementation, this would send a packet to the server to claim rewards
            // For now, we'll just mark it as claimed and update UI
            data.Claimed = true;
            _currentChallenges[index] = data;

            // Update UI to show claimed state
            _claimButtons[index].Visible = false;
            _rewardLabels[index].Text += " (Claimed!)";

            // Show a notification or toast
            ShowClaimNotification(data.XpReward, data.GoldReward, data.ItemReward);
        }

        /// <summary>
        /// Show a notification when rewards are claimed.
        /// </summary>
        private void ShowClaimNotification(uint xp, uint gold, uint item)
        {
            // Simple notification using a Label (would be better as a toast system)
            var notification = new Label();
            notification.Text = $"Claimed {xp} XP, {gold} Gold!";
            notification.Theme = GD.Load<Theme>("res://ui/themes/DefaultTheme.tres");
            notification.RectMinSize = new Vector2(200, 50);
            notification.Modulate = new Color(1.0f, 1.0f, 1.0f);
            
            // Position at top center
            var vpSize = GetViewport().GetVisibleRect().Size;
            notification.Position = new Vector2(vpSize.X / 2 - 100, 20);
            
            AddChild(notification);

            // Animate it fading out
            var tween = new Tween();
            AddChild(tween);
            tween.InterpolateProperty(notification, "modulate:a", 1.0f, 0.0f, 2.0f);
            tween.InterpolateCallback(notification, 2000, () => notification.QueueFree());
            tween.Start();
        }

        /// <summary>
        /// Structure to hold daily challenge data.
        /// </summary>
        private struct DailyChallengeData
        {
            public uint ChallengeId;
            public uint AccountId;
            public uint Progress;
            public uint XpReward;
            public uint GoldReward;
            public uint ItemReward;
            public bool Completed;
            public bool Claimed;
        }
    }
}