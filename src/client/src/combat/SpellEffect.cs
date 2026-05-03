using Godot;
using System;

namespace DarkAges.Combat
{
    /// <summary>
    /// [CLIENT_AGENT] Spell effect controller - spawns particle bursts for spellcasting
    /// </summary>
    public partial class SpellEffect : Node3D
    {
        public enum SpellType
        {
            Fire,      // Fire damage spells
            Ice,       // Ice/frost spells
            Lightning, // Lightning/electric spells
            Heal       // Healing spells
        }

        [Export] public float EffectLifetime = 1.0f;
        [Export] public bool AutoDelete = true;

        // Particle systems
        private GpuParticles3D _fire;
        private GpuParticles3D _ice;
        private GpuParticles3D _lightning;
        private GpuParticles3D _heal;

        // State
        private SpellType _currentSpell = SpellType.Fire;
        private bool _isPlaying = false;

        public override void _Ready()
        {
            // Get particle references
            _fire = GetNodeOrNull<GpuParticles3D>("Fire");
            _ice = GetNodeOrNull<GpuParticles3D>("Ice");
            _lightning = GetNodeOrNull<GpuParticles3D>("Lightning");
            _heal = GetNodeOrNull<GpuParticles3D>("Heal");
        }

        /// <summary>
        /// Play fire effect
        /// </summary>
        public void PlayFire()
        {
            PlayEffect(SpellType.Fire);
        }

        /// <summary>
        /// Play ice effect
        /// </summary>
        public void PlayIce()
        {
            PlayEffect(SpellType.Ice);
        }

        /// <summary>
        /// Play lightning effect
        /// </summary>
        public void PlayLightning()
        {
            PlayEffect(SpellType.Lightning);
        }

        /// <summary>
        /// Play heal effect
        /// </summary>
        public void PlayHeal()
        {
            PlayEffect(SpellType.Heal);
        }

        /// <summary>
        /// Play the specified spell type
        /// </summary>
        public void PlayEffect(SpellType spellType)
        {
            StopAll();
            
            _currentSpell = spellType;
            
            switch (spellType)
            {
                case SpellType.Fire:
                    if (_fire != null)
                    {
                        _fire.Emitting = true;
                    }
                    break;
                    
                case SpellType.Ice:
                    if (_ice != null)
                    {
                        _ice.Emitting = true;
                    }
                    break;
                    
                case SpellType.Lightning:
                    if (_lightning != null)
                    {
                        _lightning.Emitting = true;
                    }
                    break;
                    
                case SpellType.Heal:
                    if (_heal != null)
                    {
                        _heal.Emitting = true;
                    }
                    break;
            }
            
            _isPlaying = true;
            
            // Auto-delete after lifetime
            if (AutoDelete)
            {
                GetTree().CreateTimer(EffectLifetime).Timeout += () =>
                {
                    QueueFree();
                };
            }
        }

        /// <summary>
        /// Play effect at world position
        /// </summary>
        public static void SpawnAt(SpellType spellType, Vector3 worldPosition)
        {
            var scene = GD.Load<PackedScene>("res://scenes/SpellEffect.tscn");
            if (scene == null)
            {
                GD.PushWarning("[SpellEffect] Failed to load SpellEffect.tscn");
                return;
            }
            
            var instance = scene.Instantiate<SpellEffect>();
            if (instance == null)
            {
                GD.PushWarning("[SpellEffect] Failed to instantiate");
                return;
            }
            
            instance.GlobalPosition = worldPosition;
            
            var main = instance.GetTree()?.CurrentScene;
            if (main != null)
            {
                main.AddChild(instance);
                
                // Play the effect
                switch (spellType)
                {
                    case SpellType.Fire:
                        instance.PlayFire();
                        break;
                    case SpellType.Ice:
                        instance.PlayIce();
                        break;
                    case SpellType.Lightning:
                        instance.PlayLightning();
                        break;
                    case SpellType.Heal:
                        instance.PlayHeal();
                        break;
                }
            }
            else
            {
                instance.QueueFree();
            }
        }

        /// <summary>
        /// Spawn fire effect at position
        /// </summary>
        public static void SpawnFire(Vector3 position)
        {
            SpawnAt(SpellType.Fire, position);
        }

        /// <summary>
        /// Spawn ice effect at position
        /// </summary>
        public static void SpawnIce(Vector3 position)
        {
            SpawnAt(SpellType.Ice, position);
        }

        /// <summary>
        /// Spawn lightning effect at position
        /// </summary>
        public static void SpawnLightning(Vector3 position)
        {
            SpawnAt(SpellType.Lightning, position);
        }

        /// <summary>
        /// Spawn heal effect at position
        /// </summary>
        public static void SpawnHeal(Vector3 position)
        {
            SpawnAt(SpellType.Heal, position);
        }

        private void StopAll()
        {
            if (_fire != null) _fire.Emitting = false;
            if (_ice != null) _ice.Emitting = false;
            if (_lightning != null) _lightning.Emitting = false;
            if (_heal != null) _heal.Emitting = false;
            _isPlaying = false;
        }
    }
}