using Godot;
using System;
using System.Text;
using DarkAges.Networking;
using DarkAges.Client.UI;

namespace DarkAges.Client.UI
{
    /// <summary>
    /// Dialogue panel UI: displays NPC dialogue and response options.
    /// Shown when player interacts with an NPC; closed by selecting an option or pressing Escape.
    /// Uses UITheme for consistent styling.
    /// </summary>
    public partial class DialoguePanel : CanvasLayer
    {
        [Export] public int MaxOptions = 6;

        private Label _npcNameLabel;
        private RichTextLabel _dialogueText;
        private VBoxContainer _optionsContainer;

        private uint _currentDialogueId = 0;
        private uint _currentNpcId = 0;

        // Prefab for option buttons
        private PackedScene _optionButtonScene;

        public override void _Ready()
        {
            _npcNameLabel = GetNode<Label>("Panel/VBox/NPCName");
            _dialogueText = GetNode<RichTextLabel>("Panel/VBox/DialogueText");
            _optionsContainer = GetNode<VBoxContainer>("Panel/VBox/OptionsContainer");

            // Load or create option button template
            _optionButtonScene = GD.Load<PackedScene>("res://src/ui/OptionButton.tscn");
            if (_optionButtonScene == null)
            {
                // Create a simple Button as fallback
                GD.Print("[DialoguePanel] OptionButton.tscn not found, using Button fallback");
            }

            // Hide initially
            Visible = false;
        }

        /// <summary>
        /// Display dialogue from an NPC with selectable options.
        /// </summary>
        /// <param name="npcId">Entity ID of the speaking NPC</param>
        /// <param name="npcName">Display name of the NPC</param>
        /// <param name="dialogueText">The NPC's dialogue/greeting text</param>
        /// <param name="options">Array of player response options (may be empty for terminal dialogue)</param>
        /// <param name="dialogueId">Dialogue tree/root ID for session tracking</param>
        public void ShowDialogue(uint npcId, string npcName, string dialogueText, string[] options, uint dialogueId)
        {
            _currentNpcId = npcId;
            _currentDialogueId = dialogueId;

            // Set header
            _npcNameLabel.Text = npcName;

            // Set dialogue text (as plain text; RichTextLabel handles basic formatting)
            _dialogueText.Text = dialogueText;

            // Clear existing option buttons
            foreach (var child in _optionsContainer.GetChildren())
            {
                child.QueueFree();
            }

            // Create option buttons
            for (int i = 0; i < options.Length; i++)
            {
                Button optionBtn = null;

                if (_optionButtonScene != null)
                {
                    optionBtn = (Button)_optionButtonScene.Instantiate();
                }
                else
                {
                    // Fallback: create a standard Button node
                    optionBtn = new Button();
                }

                optionBtn.Text = options[i];
                optionBtn.SizeFlagsHorizontal = Control.SizeFlags.ExpandFill;

                // Capture option index for click handler
                int optionIndex = i;
                optionBtn.Pressed += () => OnOptionSelected(optionIndex);

                _optionsContainer.AddChild(optionBtn);
            }

            // Show panel
            Visible = true;

            GD.Print($"[DialoguePanel] Dialogue started with {npcName}, {options.Length} options");
        }

        /// <summary>
        /// Handle player selecting a dialogue option.
        /// Sends response to server and closes the panel.
        /// </summary>
        private void OnOptionSelected(int optionIndex)
        {
            GD.Print($"[DialoguePanel] Option {optionIndex} selected");

            // Send response to server via NetworkManager
            if (NetworkManager.Instance != null)
            {
                NetworkManager.Instance.SendDialogueResponse(_currentDialogueId, (byte)optionIndex);
            }

            // Hide panel until server sends next dialogue or closes conversation
            Hide();
        }

        public override void _Input(InputEvent @event)
        {
            // Escape key closes dialogue
            if (Visible && @event.IsActionPressed("ui_cancel"))
            {
                Hide();
                GetViewport().SetInputAsHandled();
            }
        }

        /// <summary>
        /// Hide the dialogue panel.
        /// </summary>
        public new void Hide()
        {
            Visible = false;
            _currentDialogueId = 0;
            _currentNpcId = 0;
        }
    }
}
