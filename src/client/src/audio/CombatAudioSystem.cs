using Godot;
using System;
using DarkAges.Networking;

namespace DarkAges.Audio
{
    /// <summary>
    /// [CLIENT_AGENT] Combat audio triggers - plays sounds on attack/hit events
    /// Connects to NetworkManager to receive combat events
    /// </summary>
    public partial class CombatAudioSystem : Node
    {
        public static CombatAudioSystem Instance { get; private set; }

        [Export] public bool EnableCombatAudio = true;
        [Export] public bool PlayHitSounds = true;
        [Export] public bool PlayAttackSounds = true;
        [Export] public bool Use3DSound = true;

        // Audio manager reference
        private AudioManager _audioManager;

        public override void _Ready()
        {
            Instance = this;
            
            // Find AudioManager
            _audioManager = GetTree().GetFirstNodeInGroup("AudioManager") as AudioManager;
            if (_audioManager == null)
            {
                _audioManager = AudioManager.Instance;
            }
            
            if (EnableCombatAudio)
            {
                ConnectToCombatEvents();
            }
            
            GD.Print("[CombatAudioSystem] Initialized");
        }

        private void ConnectToCombatEvents()
        {
            // Connect to NetworkManager for combat input
            if (NetworkManager.Instance != null)
            {
                NetworkManager.Instance.InputSent += OnInputSent;
            }
            
            // Connect to CombatEventSystem if available
            var combatEventSystem = GetTree().GetFirstNodeInGroup("CombatEventSystem");
            if (combatEventSystem != null)
            {
                // Combat events are handled through CombatEventSystem
            }
        }

        private void OnInputSent(bool attack, bool block)
        {
            if (!EnableCombatAudio) return;
            
            // Attack triggered - play swing sound
            if (attack && PlayAttackSounds)
            {
                PlayAttackSound();
            }
        }

        /// <summary>
        /// Play attack/swing sound
        /// </summary>
        public void PlayAttackSound()
        {
            if (_audioManager == null)
            {
                // Try to get instance
                _audioManager = AudioManager.Instance;
            }
            
            _audioManager?.PlayAttackSound();
        }

        /// <summary>
        /// Play hit/impact sound
        /// </summary>
        public void PlayHitSound()
        {
            if (_audioManager == null)
            {
                _audioManager = AudioManager.Instance;
            }
            
            _audioManager?.PlayHitSound();
        }

        /// <summary>
        /// Play block/deflect sound
        /// </summary>
        public void PlayBlockSound()
        {
            if (_audioManager == null)
            {
                _audioManager = AudioManager.Instance;
            }
            
            _audioManager?.PlayBlockSound();
        }

        /// <summary>
        /// Play death sound
        /// </summary>
        public void PlayDeathSound()
        {
            if (_audioManager == null)
            {
                _audioManager = AudioManager.Instance;
            }
            
            _audioManager?.PlayDeathSound();
        }

        /// <summary>
        /// Play 3D positional hit sound at world position
        /// </summary>
        public void PlayHitSound3D(Vector3 worldPosition)
        {
            if (_audioManager == null)
            {
                _audioManager = AudioManager.Instance;
            }
            
            // For 3D sounds, we'd need a reference node - using 2D for now
            _audioManager?.PlayHitSound();
        }

        /// <summary>
        /// Handle combat event from server (when damage is confirmed)
        /// </summary>
        public void OnCombatHit(int targetId, int damage, bool isFatal)
        {
            if (!EnableCombatAudio || !PlayHitSounds) return;
            
            if (isFatal)
            {
                PlayDeathSound();
            }
            else
            {
                PlayHitSound();
            }
        }

        /// <summary>
        /// Handle attack blocked event
        /// </summary>
        public void OnAttackBlocked()
        {
            if (!EnableCombatAudio) return;
            
            PlayBlockSound();
        }

        /// <summary>
        /// Enable/disable combat audio
        /// </summary>
        public void SetCombatAudioEnabled(bool enabled)
        {
            EnableCombatAudio = enabled;
        }

        public override void _ExitTree()
        {
            // Disconnect from NetworkManager
            if (NetworkManager.Instance != null)
            {
                NetworkManager.Instance.InputSent -= OnInputSent;
            }
            
            if (Instance == this)
            {
                Instance = null;
            }
        }
    }
}