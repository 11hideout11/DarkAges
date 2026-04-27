using Godot;
using System;
using System.Text;

namespace DarkAges.Client.UI
{
    /// <summary>
    /// Chat panel UI: displays chat history and input.
    /// Toggles with Enter key; Escape hides.
    /// </summary>
    public partial class ChatPanel : CanvasLayer
    {
        [Export] public int MaxHistoryLines = 100;

        private RichTextLabel _history;
        private LineEdit _input;
        private Color _colorGlobal = Colors.White;
        private Color _colorLocal = Colors.Yellow;
        private Color _colorWhisper = Colors.Cyan;
        private Color _colorParty = Colors.LightGreen;
        private Color _colorGuild = Colors.LightPink;
        private Color _colorSystem = Colors.OrangeRed;

        public override void _Ready()
        {
            _history = GetNode<RichTextLabel>("History");
            _input = GetNode<LineEdit>("Input");

            // Connect NetworkManager chat signal
            NetworkManager.Instance.ChatMessageReceived += OnChatMessageReceived;

            _input.TextSubmitted += OnTextSubmitted;
            _input.Clear();
        }

        public override void _ExitTree()
        {
            if (NetworkManager.Instance != null)
            {
                NetworkManager.Instance.ChatMessageReceived -= OnChatMessageReceived;
            }
        }

        public override void _Process(double delta)
        {
            // Toggle chat with Enter (when not typing) or Escape to hide
            if (Input.IsActionJustPressed("ui_cancel") && Visible)
            {
                Visible = false;
                GetTree().SetInputAsHandled();
            }
        }

        private void OnChatMessageReceived(uint senderId, byte channel, string senderName, string message)
        {
            // Determine channel color
            Color col = _colorGlobal;
            switch (channel)
            {
                case 0: col = _colorSystem; break;   // System
                case 1: col = _colorLocal; break;    // Local
                case 2: col = _colorGlobal; break;   // Global
                case 3: col = _colorWhisper; break;  // Whisper
                case 4: col = _colorParty; break;    // Party
                case 5: col = _colorGuild; break;    // Guild
            }

            string prefix = $"[{senderName}]: ";
            string line = $"{prefix}{message}";
            _history.AppendText($"[color={col.ToHtml()}]{line}[/color]\n");
        }

        private void OnTextSubmitted(string text)
        {
            if (string.IsNullOrWhiteSpace(text)) return;

            // Default channel: Global (2). Target=0 for global.
            NetworkManager.Instance.SendChatMessage(2, 0, text.Trim());
            _input.Clear();

            // Optionally hide after sending? Keep open for rapid replies.
        }

        /// <summary>
        /// Focus input and show panel. Call from HUD or keybinding.
        /// </summary>
        public void ShowChat()
        {
            Visible = true;
            _input.GrabFocus();
        }
    }
}
