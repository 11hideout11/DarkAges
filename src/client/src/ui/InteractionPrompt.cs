using Godot;
using System;

namespace DarkAges.Client.UI
{
    /// <summary>
    /// Interaction prompt: floating UI that appears when near an interactable entity.
    /// Shows "Press E to interact" or context-specific prompt.
    /// Position tracks the world position of the target entity.
    /// </summary>
    public partial class InteractionPrompt : Control
    {
        private Label _promptLabel;
        private ProgressBar _distanceBar;
        
        private uint _targetEntityId = 0;
        private string _promptText = "Press E to interact";
        private float _interactionRange = 3.0f;
        private bool _isVisible = false;
        
        public override void _Ready()
        {
            // Create UI elements with theme styling
            _promptLabel = new Label();
            _promptLabel.HorizontalAlignment = HorizontalAlignment.Center;
            _promptLabel.AddThemeFontSizeOverride("font_size", UITheme.FontSizeBody);
            _promptLabel.Modulate = UITheme.TextPrimary;
            // Add outline effect via shadow
            _promptLabel.Set("outline_size", 2);
            AddChild(_promptLabel);
            
            _distanceBar = new ProgressBar();
            _distanceBar.MinValue = 0;
            _distanceBar.MaxValue = 100;
            _distanceBar.Value = 100;
            _distanceBar.CustomMinimumSize = new Vector2(100, 8);
            _distanceBar.Position = new Vector2(0, 24);
            
            // Apply themed progress bar styling
            var bgStyle = UITheme.CreateProgressBackgroundStyle(cornerRadius: 4f);
            _distanceBar.AddThemeStyleboxOverride("background", bgStyle);
            var fillStyle = UITheme.CreateProgressFillStyle(UITheme.AccentPrimary, cornerRadius: 4f);
            _distanceBar.AddThemeStyleboxOverride("fill", fillStyle);
            
            AddChild(_distanceBar);
            
            // Initially hidden
            Visible = false;
        }
        
        /// <summary>
        /// Show interaction prompt for a target entity.
        /// </summary>
        public void ShowPrompt(uint entityId, string prompt, float maxRange)
        {
            _targetEntityId = entityId;
            _promptText = prompt;
            _interactionRange = maxRange;
            _promptLabel.Text = prompt;
            Visible = true;
            _isVisible = true;
        }
        
        /// <summary>
        /// Update distance to target - updates progress bar.
        /// </summary>
        public void UpdateDistance(float currentDistance)
        {
            if (!_isVisible) return;
            
            // Update progress bar (closer = more filled)
            float percent = Math.Clamp(1.0f - (currentDistance / _interactionRange), 0, 1) * 100;
            _distanceBar.Value = percent;
            
            // Hide if too far
            if (currentDistance > _interactionRange)
            {
                HidePrompt();
            }
        }
        
        /// <summary>
        /// Hide the interaction prompt.
        /// </summary>
        public void HidePrompt()
        {
            Visible = false;
            _isVisible = false;
            _targetEntityId = 0;
        }
        
        /// <summary>
        /// Check if a prompt is currently displayed.
        /// </summary>
        public bool IsShowingPrompt => _isVisible;
        
        /// <summary>
        /// Get the current target entity ID.
        /// </summary>
        public uint TargetEntityId => _targetEntityId;
        
        public override void _Process(double delta)
        {
            base._Process(delta);
            
            // In a full implementation, would track target entity world position
            // and update prompt position each frame
        }
    }
}