using Godot;
using System;
using DarkAges.Networking;
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

        private ColorRect _background;
        private RichTextLabel _history;
        private LineEdit _input;

        public override void _Ready()
        {
            _background = GetNode<ColorRect>("Background");
            _history = GetNode<RichTextLabel>("History");
            _input = GetNode<LineEdit>("Input");

            // Apply theme background
            if (_background != null)
            {
                _background.Color = UITheme.PanelBackground;
            }

            // Apply theme to history
            if (_history != null)
            {
                var historyStyle = new StyleBoxFlat { BgColor = new Color(0, 0, 0, 0) };
                _history.AddThemeStyleboxOverride("normal", historyStyle);
            }

            // Apply theme to input
            if (_input != null)
            {
                var inputStyle = UITheme.CreatePanelStyle(cornerRadius: 4f, borderWidth: 1f);
                inputStyle.BgColor = new Color(0.1f, 0.1f, 0.15f, 0.9f);
                _input.AddThemeStyleboxOverride("normal", inputStyle);
                
                _input.Modulate = UITheme.TextPrimary;
            }

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
                GetViewport().SetInputAsHandled();
            }
        }

        private void OnChatMessageReceived(uint senderId, byte channel, string senderName, string message)
        {
            // Use theme channel colors
            Color col = UITheme.GetChannelColor(channel);

            // Format: [Sender]: Message
            string line = $"[color={col.ToHtml()}][{senderName}]: {message}[/color]\n";
            _history.AppendText(line);
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
