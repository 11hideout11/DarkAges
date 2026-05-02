using Godot;
using System;
using System.Collections.Generic;

namespace DarkAges.Audio
{
    /// <summary>
    /// [CLIENT_AGENT] Audio resource manager - singleton for loading and playing combat sounds
    /// </summary>
    public partial class AudioManager : Node
    {
        public static AudioManager Instance { get; private set; }

        [Export] public float MasterVolume = 1.0f;
        [Export] public float SfxVolume = 0.8f;
        [Export] public float MusicVolume = 0.5f;

        // Loaded audio streams
        private Dictionary<string, AudioStreamWav> _sfxLibrary = new();
        
        // Audio players for simultaneous sounds
        private AudioStreamPlayer _sfxPlayer;
        private AudioStreamPlayer3D _sfxPlayer3D;
        
        // Pool for combat sounds
        private AudioStreamPlayer[] _combatSoundPool;
        private int _currentPoolIndex = 0;
        private const int POOL_SIZE = 8;

        public override void _Ready()
        {
            Instance = this;
            
            SetupAudioPlayers();
            LoadSfxLibrary();
            
            GD.Print("[AudioManager] Initialized");
        }

        private void SetupAudioPlayers()
        {
            // 2D SFX player
            _sfxPlayer = new AudioStreamPlayer
            {
                Name = "SfxPlayer",
                VolumeDb = 0f
            };
            AddChild(_sfxPlayer);
            
            // 3D positional player
            _sfxPlayer3D = new AudioStreamPlayer3D
            {
                Name = "SfxPlayer3D",
                VolumeDb = 0f,
                UnitSize = 1.0f,
                MaxDistance = 20f
            };
            AddChild(_sfxPlayer3D);
            
            // Create sound pool for rapid-fire combat sounds
            _combatSoundPool = new AudioStreamPlayer[POOL_SIZE];
            for (int i = 0; i < POOL_SIZE; i++)
            {
                _combatSoundPool[i] = new AudioStreamPlayer
                {
                    Name = $"CombatSfx_{i}"
                };
                AddChild(_combatSoundPool[i]);
            }
        }

        private void LoadSfxLibrary()
        {
            // Load combat SFX from audio folder
            string[] sfxFiles = new string[]
            {
                "sword_swing",
                "sword_hit",
                "sword_block",
                "footstep",
                "death",
                "jump",
                "heal",
                "coin_collect"
            };
            
            foreach (string sfx in sfxFiles)
            {
                try
                {
                    string path = $"res://assets/audio/{sfx}.wav";
                    if (ResourceLoader.Exists(path))
                    {
                        var stream = ResourceLoader.Load<AudioStreamWav>(path);
                        if (stream != null)
                        {
                            _sfxLibrary[sfx] = stream;
                            GD.Print($"[AudioManager] Loaded: {sfx}");
                        }
                    }
                    else
                    {
                        // Create placeholder (silent) for missing SFX
                        _sfxLibrary[sfx] = CreateSilentStream();
                    }
                }
                catch (Exception ex)
                {
                    GD.Warning($"[AudioManager] Failed to load {sfx}: {ex.Message}");
                    _sfxLibrary[sfx] = CreateSilentStream();
                }
            }
        }

        private AudioStreamWav CreateSilentStream()
        {
            // Create a silent placeholder (0.1s empty)
            var stream = new AudioStreamWav();
            // Default constructor creates silent data
            return stream;
        }

        /// <summary>
        /// Play a 2D sound effect
        /// </summary>
        public void PlaySfx(string sfxName, float volumeScale = 1.0f)
        {
            if (!_sfxLibrary.TryGetValue(sfxName, out var stream))
            {
                GD.Warning($"[AudioManager] SFX not found: {sfxName}");
                return;
            }
            
            // Use sound pool for rapid-fire sounds
            var player = _combatSoundPool[_currentPoolIndex];
            _currentPoolIndex = (_currentPoolIndex + 1) % POOL_SIZE;
            
            player.Stream = stream;
            player.VolumeDb = LinearToDb(SfxVolume * volumeScale * MasterVolume);
            player.Play();
        }

        /// <summary>
        /// Play a 3D positional sound effect
        /// </summary>
        public void PlaySfx3D(string sfxName, Node3D source, float volumeScale = 1.0f)
        {
            if (!_sfxLibrary.TryGetValue(sfxName, out var stream))
            {
                GD.Warning($"[AudioManager] SFX not found: {sfxName}");
                return;
            }
            
            _sfxPlayer3D.Stream = stream;
            _sfxPlayer3D.GlobalPosition = source.GlobalPosition;
            _sfxPlayer3D.VolumeDb = LinearToDb(SfxVolume * volumeScale * MasterVolume);
            _sfxPlayer3D.Play();
        }

        /// <summary>
        /// Play attack sound
        /// </summary>
        public void PlayAttackSound()
        {
            PlaySfx("sword_swing");
        }

        /// <summary>
        /// Play hit sound
        /// </summary>
        public void PlayHitSound()
        {
            PlaySfx("sword_hit", 0.9f);
        }

        /// <summary>
        /// Play block/deflect sound
        /// </summary>
        public void PlayBlockSound()
        {
            PlaySfx("sword_block", 0.7f);
        }

        /// <summary>
        /// Play death sound
        /// </summary>
        public void PlayDeathSound()
        {
            PlaySfx("death", 1.0f);
        }

        /// <summary>
        /// Set master volume (0.0 to 1.0)
        /// </summary>
        public void SetMasterVolume(float volume)
        {
            MasterVolume = Mathf.Clamp(volume, 0f, 1f);
        }

        /// <summary>
        /// Set SFX volume (0.0 to 1.0)
        /// </summary>
        public void SetSfxVolume(float volume)
        {
            SfxVolume = Mathf.Clamp(volume, 0f, 1f);
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