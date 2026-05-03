using Godot;
using System;
using DarkAges.Entities;

namespace DarkAges.Audio
{
    /// <summary>
    /// [CLIENT_AGENT] Footstep audio system - plays footstep sounds based on player movement
    /// </summary>
    public partial class FootstepAudioSystem : Node
    {
        public static FootstepAudioSystem Instance { get; private set; }

        [Export] public bool EnableFootsteps = true;
        [Export] public float WalkStepInterval = 0.5f;
        [Export] public float RunStepInterval = 0.3f;
        [Export] public float StopThreshold = 0.1f;

        private AudioManager _audioManager;
        private float _stepTimer = 0f;
        private bool _wasMoving = false;
        private PlayerEntity _player;

        public override void _Ready()
        {
            Instance = this;
            
            _audioManager = AudioManager.Instance;
            
            GD.Print("[FootstepAudioSystem] Initialized");
        }

        public override void _Process(double delta)
        {
            if (!EnableFootsteps) return;
            
            if (_player == null)
            {
                FindPlayer();
                return;
            }
            
            float velocity = _player.Velocity.Length();
            bool isMoving = velocity > StopThreshold;
            
            if (isMoving)
            {
                float stepInterval = _player.IsRunning ? RunStepInterval : WalkStepInterval;
                _stepTimer += (float)delta;
                
                if (_stepTimer >= stepInterval)
                {
                    PlayFootstep();
                    _stepTimer = 0f;
                }
                _wasMoving = true;
            }
            else
            {
                _stepTimer = 0f;
                _wasMoving = false;
            }
        }

        private void FindPlayer()
        {
            var players = GetTree().GetNodesInGroup("Player");
            if (players.Count > 0)
            {
                _player = players[0] as PlayerEntity;
            }
        }

        private void PlayFootstep()
        {
            if (_audioManager == null)
            {
                _audioManager = AudioManager.Instance;
            }
            
            if (_audioManager == null) return;
            
            // Play footstep sound
            _audioManager.PlaySfx("footstep", 0.6f);
        }

        /// <summary>
        /// Enable/disable footstep audio
        /// </summary>
        public void SetFootstepsEnabled(bool enabled)
        {
            EnableFootsteps = enabled;
            if (!enabled)
            {
                _stepTimer = 0f;
            }
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