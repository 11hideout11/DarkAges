using Godot;
using System;
using DarkAges.Networking;
using DarkAges.Combat;

namespace DarkAges.Particles
{
    /// <summary>
    /// [CLIENT_AGENT] Combat particle system - manages hit effect spawning
    /// </summary>
    public partial class CombatParticleSystem : Node
    {
        public static CombatParticleSystem Instance { get; private set; }

        [Export] public bool EnableParticles = true;
        [Export] public PackedScene HitEffectScene;
        
        // Effect spawn position offset
        [Export] public Vector3 EffectOffset = Vector3.Zero;

        // Object pool for hit effects
        private const int POOL_SIZE = 16;
        private Node3D[] _effectPool;
        private int _currentIndex = 0;

        // Parent node for effects
        private Node3D _effectParent;

        public override void _Ready()
        {
            Instance = this;
            
            // Create effect parent
            _effectParent = new Node3D { Name = "HitEffects" };
            GetTree().CurrentScene?.AddChild(_effectParent);
            
            // Initialize pool
            InitializePool();
            
            // Connect to combat events
            ConnectToCombatEvents();
            
            GD.Print("[CombatParticleSystem] Initialized");
        }

        private void InitializePool()
        {
            _effectPool = new Node3D[POOL_SIZE];
            
            // Try to load hit effect scene
            if (HitEffectScene == null)
            {
                var path = "res://scenes/HitEffect.tscn";
                if (ResourceLoader.Exists(path))
                {
                    HitEffectScene = GD.Load<PackedScene>(path);
                }
            }
            
            // Pre-instantiate pool
            if (HitEffectScene != null)
            {
                for (int i = 0; i < POOL_SIZE; i++)
                {
                    var effect = HitEffectScene.Instantiate<Node3D>();
                    effect.Name = $"HitEffect_{i}";
                    effect.Visible = false;
                    _effectParent?.AddChild(effect);
                    _effectPool[i] = effect;
                }
            }
        }

        private void ConnectToCombatEvents()
        {
            // Connect to attack input
            if (NetworkManager.Instance != null)
            {
                NetworkManager.Instance.InputSent += OnInputSent;
            }
        }

        private void OnInputSent(bool attack, bool block)
        {
            if (!EnableParticles || !attack) return;
            
            // Will spawn dust effect at attack swing position
            SpawnDustEffect();
        }

        /// <summary>
        /// Spawn spark effect for hit
        /// </summary>
        public void SpawnSparkEffect(Vector3 position, Vector3 normal)
        {
            if (!EnableParticles) return;
            
            var effect = GetPooledEffect();
            if (effect != null)
            {
                effect.GlobalPosition = position + EffectOffset;
                effect.Rotation = new Vector3(
                    Mathf.Atan2(normal.x, normal.z),
                    0,
                    Mathf.Atan2(-normal.y, Mathf.Sqrt(normal.x * normal.x + normal.z * normal.z))
                );
                
                var hitEffect = effect as HitEffect;
                if (hitEffect != null)
                {
                    hitEffect.PlaySparks();
                }
                
                effect.Visible = true;
                
                // Auto-hide after effect lifetime
                var timer = GetTree().CreateTimer(0.5f);
                timer.Timeout += () =>
                {
                    effect.Visible = false;
                };
            }
        }

        /// <summary>
        /// Spawn blood effect for flesh hit
        /// </summary>
        public void SpawnBloodEffect(Vector3 position, Vector3 direction)
        {
            if (!EnableParticles) return;
            
            var effect = GetPooledEffect();
            if (effect != null)
            {
                effect.GlobalPosition = position + EffectOffset;
                
                var hitEffect = effect as HitEffect;
                if (hitEffect != null)
                {
                    hitEffect.PlayBlood();
                }
                
                effect.Visible = true;
                
                var timer = GetTree().CreateTimer(0.6f);
                timer.Timeout += () =>
                {
                    effect.Visible = false;
                };
            }
        }

        /// <summary>
        /// Spawn dust effect on swing/miss
        /// </summary>
        public void SpawnDustEffect()
        {
            // Spawn at player position with slight offset
            var player = GetTree().GetFirstNodeInGroup("Player") as Node3D;
            if (player == null) return;
            
            // Calculate spawn position in front of player
            var forward = -player.GlobalTransform.Basis.Z;
            forward.y = 0;
            forward = forward.Normalized();
            
            var spawnPos = player.GlobalPosition + forward * 1.5f + EffectOffset;
            spawnPos.y = 0.5f;
            
            var effect = GetPooledEffect();
            if (effect != null)
            {
                effect.GlobalPosition = spawnPos;
                
                var hitEffect = effect as HitEffect;
                if (hitEffect != null)
                {
                    hitEffect.PlayDust();
                }
                
                effect.Visible = true;
                
                var timer = GetTree().CreateTimer(0.5f);
                timer.Timeout += () =>
                {
                    effect.Visible = false;
                };
            }
        }

        private Node3D GetPooledEffect()
        {
            _currentIndex = (_currentIndex + 1) % POOL_SIZE;
            return _effectPool[_currentIndex];
        }

        /// <summary>
        /// Enable/disable particles
        /// </summary>
        public void SetParticlesEnabled(bool enabled)
        {
            EnableParticles = enabled;
            
            if (!enabled && _effectPool != null)
            {
                foreach (var effect in _effectPool)
                {
                    effect.Visible = false;
                }
            }
        }

        public override void _ExitTree()
        {
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