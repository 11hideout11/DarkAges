using Godot;
using System;
using DarkAges.Networking;
using DarkAges.Combat;

namespace DarkAges.Particles
{
    /// <summary>
    /// [CLIENT_AGENT] Combat particle system - manages all combat particle spawning
    /// Supports: Hit effects (sparks, blood, dust), Spell effects (fire, ice, lightning, heal), Death effects (explosion, smoke)
    /// </summary>
    public partial class CombatParticleSystem : Node
    {
        public static CombatParticleSystem Instance { get; private set; }

        [Export] public bool EnableParticles = true;
        
        // Effect scenes
        [Export] public PackedScene HitEffectScene;
        [Export] public PackedScene SpellEffectScene;
        [Export] public PackedScene DeathEffectScene;
        
        // Effect spawn position offset
        [Export] public Vector3 EffectOffset = Vector3.Zero;

        // Object pool sizes
        private const int HIT_POOL_SIZE = 16;
        private const int SPELL_POOL_SIZE = 8;
        private const int DEATH_POOL_SIZE = 4;
        
        // Object pools
        private Node3D[] _hitEffectPool;
        private Node3D[] _spellEffectPool;
        private Node3D[] _deathEffectPool;
        
        private int _currentHitIndex = 0;
        private int _currentSpellIndex = 0;
        private int _currentDeathIndex = 0;

        // Parent node for effects
        private Node3D _effectParent;

        public override void _Ready()
        {
            Instance = this;
            
            // Create effect parent
            _effectParent = new Node3D { Name = "CombatEffects" };
            GetTree().CurrentScene?.AddChild(_effectParent);
            
            // Initialize pools
            InitializePools();
            
            // Connect to combat events
            ConnectToCombatEvents();
            
            GD.Print("[CombatParticleSystem] Initialized with hit/spell/death support");
        }

        private void InitializePools()
        {
            // Initialize hit effect pool
            _hitEffectPool = new Node3D[HIT_POOL_SIZE];
            if (HitEffectScene == null)
            {
                var hitPath = "res://scenes/HitEffect.tscn";
                if (ResourceLoader.Exists(hitPath))
                {
                    HitEffectScene = GD.Load<PackedScene>(hitPath);
                }
            }
            
            if (HitEffectScene != null)
            {
                for (int i = 0; i < HIT_POOL_SIZE; i++)
                {
                    var effect = HitEffectScene.Instantiate<Node3D>();
                    effect.Name = $"HitEffect_{i}";
                    effect.Visible = false;
                    _effectParent?.AddChild(effect);
                    _hitEffectPool[i] = effect;
                }
            }
            
            // Initialize spell effect pool
            _spellEffectPool = new Node3D[SPELL_POOL_SIZE];
            if (SpellEffectScene == null)
            {
                var spellPath = "res://scenes/SpellEffect.tscn";
                if (ResourceLoader.Exists(spellPath))
                {
                    SpellEffectScene = GD.Load<PackedScene>(spellPath);
                }
            }
            
            if (SpellEffectScene != null)
            {
                for (int i = 0; i < SPELL_POOL_SIZE; i++)
                {
                    var effect = SpellEffectScene.Instantiate<Node3D>();
                    effect.Name = $"SpellEffect_{i}";
                    effect.Visible = false;
                    _effectParent?.AddChild(effect);
                    _spellEffectPool[i] = effect;
                }
            }
            
            // Initialize death effect pool
            _deathEffectPool = new Node3D[DEATH_POOL_SIZE];
            if (DeathEffectScene == null)
            {
                var deathPath = "res://scenes/DeathEffect.tscn";
                if (ResourceLoader.Exists(deathPath))
                {
                    DeathEffectScene = GD.Load<PackedScene>(deathPath);
                }
            }
            
            if (DeathEffectScene != null)
            {
                for (int i = 0; i < DEATH_POOL_SIZE; i++)
                {
                    var effect = DeathEffectScene.Instantiate<Node3D>();
                    effect.Name = $"DeathEffect_{i}";
                    effect.Visible = false;
                    _effectParent?.AddChild(effect);
                    _deathEffectPool[i] = effect;
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
            SpawnDustEffect();
        }

        #region Hit Effects

        /// <summary>
        /// Spawn spark effect for metal hit
        /// </summary>
        public void SpawnSparkEffect(Vector3 position, Vector3 normal)
        {
            if (!EnableParticles) return;
            
            var effect = GetPooledHitEffect();
            if (effect != null)
            {
                effect.GlobalPosition = position + EffectOffset;
                effect.Rotation = new Vector3(
                    Mathf.Atan2(normal.X, normal.Z),
                    0,
                    Mathf.Atan2(-normal.Y, Mathf.Sqrt(normal.X * normal.X + normal.Z * normal.Z))
                );
                
                var hitEffect = effect as DarkAges.Combat.HitEffect;
                if (hitEffect != null)
                {
                    hitEffect.PlaySparks();
                }
                
                effect.Visible = true;
                ScheduleHide(effect, 0.5f);
            }
        }

        /// <summary>
        /// Spawn blood effect for flesh hit
        /// </summary>
        public void SpawnBloodEffect(Vector3 position, Vector3 direction)
        {
            if (!EnableParticles) return;
            
            var effect = GetPooledHitEffect();
            if (effect != null)
            {
                effect.GlobalPosition = position + EffectOffset;
                
                var hitEffect = effect as DarkAges.Combat.HitEffect;
                if (hitEffect != null)
                {
                    hitEffect.PlayBlood();
                }
                
                effect.Visible = true;
                ScheduleHide(effect, 0.6f);
            }
        }

        /// <summary>
        /// Spawn dust effect on swing/miss
        /// </summary>
        public void SpawnDustEffect()
        {
            var player = GetTree().GetFirstNodeInGroup("Player") as Node3D;
            if (player == null) return;
            
            var forward = -player.GlobalTransform.Basis.Z;
            forward.Y = 0;
            forward = forward.Normalized();
            
            var spawnPos = player.GlobalPosition + forward * 1.5f + EffectOffset;
            spawnPos.Y = 0.5f;
            
            var effect = GetPooledHitEffect();
            if (effect != null)
            {
                effect.GlobalPosition = spawnPos;
                
                var hitEffect = effect as DarkAges.Combat.HitEffect;
                if (hitEffect != null)
                {
                    hitEffect.PlayDust();
                }
                
                effect.Visible = true;
                ScheduleHide(effect, 0.5f);
            }
        }

        #endregion

        #region Spell Effects

        /// <summary>
        /// Spawn fire spell effect
        /// </summary>
        public void SpawnFireSpellEffect(Vector3 position)
        {
            if (!EnableParticles) return;
            
            SpawnSpellEffect(SpellEffect.SpellType.Fire, position);
        }

        /// <summary>
        /// Spawn ice spell effect
        /// </summary>
        public void SpawnIceSpellEffect(Vector3 position)
        {
            if (!EnableParticles) return;
            
            SpawnSpellEffect(SpellEffect.SpellType.Ice, position);
        }

        /// <summary>
        /// Spawn lightning spell effect
        /// </summary>
        public void SpawnLightningSpellEffect(Vector3 position)
        {
            if (!EnableParticles) return;
            
            SpawnSpellEffect(SpellEffect.SpellType.Lightning, position);
        }

        /// <summary>
        /// Spawn heal spell effect
        /// </summary>
        public void SpawnHealSpellEffect(Vector3 position)
        {
            if (!EnableParticles) return;
            
            SpawnSpellEffect(SpellEffect.SpellType.Heal, position);
        }

        private void SpawnSpellEffect(SpellEffect.SpellType spellType, Vector3 position)
        {
            var effect = GetPooledSpellEffect();
            if (effect != null)
            {
                effect.GlobalPosition = position + EffectOffset;
                
                var spellEffect = effect as SpellEffect;
                if (spellEffect != null)
                {
                    spellEffect.PlayEffect(spellType);
                }
                
                effect.Visible = true;
                ScheduleHide(effect, 1.0f);
            }
        }

        #endregion

        #region Death Effects

        /// <summary>
        /// Spawn explosion death effect (violent death)
        /// </summary>
        public void SpawnExplosionDeathEffect(Vector3 position)
        {
            if (!EnableParticles) return;
            
            SpawnDeathEffect(DeathEffect.DeathType.Explosion, position);
        }

        /// <summary>
        /// Spawn smoke death effect (fade out death)
        /// </summary>
        public void SpawnSmokeDeathEffect(Vector3 position)
        {
            if (!EnableParticles) return;
            
            SpawnDeathEffect(DeathEffect.DeathType.Smoke, position);
        }

        private void SpawnDeathEffect(DeathEffect.DeathType deathType, Vector3 position)
        {
            var effect = GetPooledDeathEffect();
            if (effect != null)
            {
                effect.GlobalPosition = position + EffectOffset;
                
                var deathEffect = effect as DeathEffect;
                if (deathEffect != null)
                {
                    deathEffect.PlayEffect(deathType);
                }
                
                effect.Visible = true;
                ScheduleHide(effect, 1.5f);
            }
        }

        #endregion

        #region Pool Management

        private Node3D GetPooledHitEffect()
        {
            _currentHitIndex = (_currentHitIndex + 1) % HIT_POOL_SIZE;
            return _hitEffectPool[_currentHitIndex];
        }

        private Node3D GetPooledSpellEffect()
        {
            _currentSpellIndex = (_currentSpellIndex + 1) % SPELL_POOL_SIZE;
            return _spellEffectPool[_currentSpellIndex];
        }

        private Node3D GetPooledDeathEffect()
        {
            _currentDeathIndex = (_currentDeathIndex + 1) % DEATH_POOL_SIZE;
            return _deathEffectPool[_currentDeathIndex];
        }

        private void ScheduleHide(Node3D effect, float delay)
        {
            var timer = GetTree().CreateTimer(delay);
            timer.Timeout += () =>
            {
                if (effect != null && IsInstanceValid(effect))
                {
                    effect.Visible = false;
                }
            };
        }

        private bool IsInstanceValid(Node node)
        {
            return node != null && node.GetParent() != null;
        }

        #endregion

        /// <summary>
        /// Enable/disable all particles
        /// </summary>
        public void SetParticlesEnabled(bool enabled)
        {
            EnableParticles = enabled;
            
            if (!enabled)
            {
                HideAllPools();
            }
        }

        private void HideAllPools()
        {
            if (_hitEffectPool != null)
            {
                foreach (var effect in _hitEffectPool)
                {
                    effect.Visible = false;
                }
            }
            if (_spellEffectPool != null)
            {
                foreach (var effect in _spellEffectPool)
                {
                    effect.Visible = false;
                }
            }
            if (_deathEffectPool != null)
            {
                foreach (var effect in _deathEffectPool)
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