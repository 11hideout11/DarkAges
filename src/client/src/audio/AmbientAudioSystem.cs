using Godot;
using System;

namespace DarkAges.Audio
{
    /// <summary>
    /// [CLIENT_AGENT] Ambient audio system - plays environmental/ambient sounds
    /// </summary>
    public partial class AmbientAudioSystem : Node
    {
        public static AmbientAudioSystem Instance { get; private set; }

        [Export] public bool EnableAmbient = true;
        [Export] public float AmbientVolume = 0.4f;
        [Export] public bool AutoStartOnReady = true;

        // Ambient audio player
        private AudioStreamPlayer _ambientPlayer;
        
        // Current zone ambient track
        private string _currentAmbientTrack = "ambient_forest";

        public override void _Ready()
        {
            Instance = this;
            
            SetupAmbientPlayer();
            
            if (AutoStartOnReady && EnableAmbient)
            {
                PlayAmbient();
            }
            
            GD.Print("[AmbientAudioSystem] Initialized");
        }

        private void SetupAmbientPlayer()
        {
            _ambientPlayer = new AudioStreamPlayer
            {
                Name = "AmbientPlayer",
                VolumeDb = 0f,
                Bus = "Master"
            };
            AddChild(_ambientPlayer);
        }

        /// <summary>
        /// Play ambient sound
        /// </summary>
        public void PlayAmbient()
        {
            if (!EnableAmbient) return;
            
            // Try to load ambient track
            string path = $"res://assets/audio/{_currentAmbientTrack}.wav";
            if (ResourceLoader.Exists(path))
            {
                var stream = ResourceLoader.Load<AudioStream>(path);
                if (stream != null)
                {
                    _ambientPlayer.Stream = stream;
                    _ambientPlayer.VolumeDb = LinearToDb(AmbientVolume);
                    _ambientPlayer.Play();
                    GD.Print($"[AmbientAudioSystem] Playing: {_currentAmbientTrack}");
                }
            }
        }

        /// <summary>
        /// Stop ambient sound
        /// </summary>
        public void StopAmbient()
        {
            _ambientPlayer?.Stop();
        }

        /// <summary>
        /// Set ambient track by zone type
        /// </summary>
        public void SetZoneAmbient(string ambientTrack)
        {
            bool wasPlaying = _ambientPlayer?.IsPlaying() ?? false;
            
            _currentAmbientTrack = ambientTrack;
            
            if (wasPlaying && EnableAmbient)
            {
                // Restart with new track
                StopAmbient();
                PlayAmbient();
            }
        }

        /// <summary>
        /// Set ambient volume
        /// </summary>
        public void SetAmbientVolume(float volume)
        {
            AmbientVolume = Mathf.Clamp(volume, 0f, 1f);
            if (_ambientPlayer != null)
            {
                _ambientPlayer.VolumeDb = LinearToDb(AmbientVolume);
            }
        }

        /// <summary>
        /// Enable/disable ambient audio
        /// </summary>
        public void SetAmbientEnabled(bool enabled)
        {
            EnableAmbient = enabled;
            
            if (enabled)
            {
                PlayAmbient();
            }
            else
            {
                StopAmbient();
            }
        }

        private float LinearToDb(float linear)
        {
            if (linear <= 0f) return -80f;
            return Mathf.LinearToDb(linear);
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