using Godot;
using System;

namespace DarkAges.Audio
{
    /// <summary>
    /// [CLIENT_AGENT] UI audio system - plays sounds for UI interactions
    /// </summary>
    public partial class UIAudioSystem : Node
    {
        public static UIAudioSystem Instance { get; private set; }

        [Export] public bool EnableUIAudio = true;
        [Export] public bool PlayHoverSounds = false;
        [Export] public bool PlayClickSounds = true;
        [Export] public bool PlayOpenCloseSounds = true;

        private AudioManager _audioManager;

        public override void _Ready()
        {
            Instance = this;
            
            _audioManager = AudioManager.Instance;
            
            // Connect to UI signals if UI exists
            var gameUI = GetTree().GetFirstNodeInGroup("GameUI");
            if (gameUI != null)
            {
                // Will integrate with UI signals later
            }
            
            GD.Print("[UIAudioSystem] Initialized");
        }

        /// <summary>
        /// Play button click sound
        /// </summary>
        public void PlayClickSound()
        {
            if (!EnableUIAudio || !PlayClickSounds) return;
            
            if (_audioManager == null)
            {
                _audioManager = AudioManager.Instance;
            }
            
            _audioManager?.PlaySfx("ui_click", 0.7f);
        }

        /// <summary>
        /// Play hover sound
        /// </summary>
        public void PlayHoverSound()
        {
            if (!EnableUIAudio || !PlayHoverSounds) return;
            
            if (_audioManager == null)
            {
                _audioManager = AudioManager.Instance;
            }
            
            _audioManager?.PlaySfx("ui_hover", 0.4f);
        }

        /// <summary>
        /// Play menu open sound
        /// </summary>
        public void PlayMenuOpenSound()
        {
            if (!EnableUIAudio || !PlayOpenCloseSounds) return;
            
            if (_audioManager == null)
            {
                _audioManager = AudioManager.Instance;
            }
            
            _audioManager?.PlaySfx("menu_open", 0.6f);
        }

        /// <summary>
        /// Play menu close sound
        /// </summary>
        public void PlayMenuCloseSound()
        {
            if (!EnableUIAudio || !PlayOpenCloseSounds) return;
            
            if (_audioManager == null)
            {
                _audioManager = AudioManager.Instance;
            }
            
            _audioManager?.PlaySfx("menu_close", 0.6f);
        }

        /// <summary>
        /// Play error sound
        /// </summary>
        public void PlayErrorSound()
        {
            if (!EnableUIAudio) return;
            
            if (_audioManager == null)
            {
                _audioManager = AudioManager.Instance;
            }
            
            _audioManager?.PlaySfx("ui_error", 0.5f);
        }

        /// <summary>
        /// Play success sound
        /// </summary>
        public void PlaySuccessSound()
        {
            if (!EnableUIAudio) return;
            
            if (_audioManager == null)
            {
                _audioManager = AudioManager.Instance;
            }
            
            _audioManager?.PlaySfx("ui_success", 0.6f);
        }

        /// <summary>
        /// Enable/disable UI audio
        /// </summary>
        public void SetUIAudioEnabled(bool enabled)
        {
            EnableUIAudio = enabled;
        }

        public override void _ExitTree()
        {
            if (Instance == this)
            {
                Instance = null;
            }
        }
    }
}