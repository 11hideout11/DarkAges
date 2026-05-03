using Godot;
using System;

namespace DarkAges.Combat
{
    /// <summary>
    /// [CLIENT_AGENT] Death effect controller - spawns particle bursts on death
    /// </summary>
    public partial class DeathEffect : Node3D
    {
        public enum DeathType
        {
            Explosion, // Violent death (bosses, explosions)
            Smoke,     // Fade out death (standard NPCs)
        }

        [Export] public float EffectLifetime = 1.5f;
        [Export] public bool AutoDelete = true;

        // Particle systems
        private GPUParticles3D _explosion;
        private GPUParticles3D _smoke;

        // State
        private DeathType _currentDeath = DeathType.Explosion;
        private bool _isPlaying = false;

        public override void _Ready()
        {
            // Get particle references
            _explosion = GetNodeOrNull<GPUParticles3D>("Explosion");
            _smoke = GetNodeOrNull<GPUParticles3D>("Smoke");
        }

        /// <summary>
        /// Play explosion effect (violent death)
        /// </summary>
        public void PlayExplosion()
        {
            PlayEffect(DeathType.Explosion);
        }

        /// <summary>
        /// Play smoke effect (fade death)
        /// </summary>
        public void PlaySmoke()
        {
            PlayEffect(DeathType.Smoke);
        }

        /// <summary>
        /// Play the specified death type
        /// </summary>
        public void PlayEffect(DeathType deathType)
        {
            StopAll();
            
            _currentDeath = deathType;
            
            switch (deathType)
            {
                case DeathType.Explosion:
                    if (_explosion != null)
                    {
                        _explosion.Emitting = true;
                    }
                    break;
                    
                case DeathType.Smoke:
                    if (_smoke != null)
                    {
                        _smoke.Emitting = true;
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
        public static void SpawnAt(DeathType deathType, Vector3 worldPosition)
        {
            var scene = GD.Load<PackedScene>("res://scenes/DeathEffect.tscn");
            if (scene == null)
            {
                GD.Warning("[DeathEffect] Failed to load DeathEffect.tscn");
                return;
            }
            
            var instance = scene.Instantiate<DeathEffect>();
            if (instance == null)
            {
                GD.Warning("[DeathEffect] Failed to instantiate");
                return;
            }
            
            instance.GlobalPosition = worldPosition;
            
            var main = instance.GetTree()?.CurrentScene;
            if (main != null)
            {
                main.AddChild(instance);
                
                // Play the effect
                switch (deathType)
                {
                    case DeathType.Explosion:
                        instance.PlayExplosion();
                        break;
                    case DeathType.Smoke:
                        instance.PlaySmoke();
                        break;
                }
            }
            else
            {
                instance.QueueFree();
            }
        }

        /// <summary>
        /// Spawn explosion effect at position
        /// </summary>
        public static void SpawnExplosion(Vector3 position)
        {
            SpawnAt(DeathType.Explosion, position);
        }

        /// <summary>
        /// Spawn smoke effect at position
        /// </summary>
        public static void SpawnSmoke(Vector3 position)
        {
            SpawnAt(DeathType.Smoke, position);
        }

        private void StopAll()
        {
            if (_explosion != null) _explosion.Emitting = false;
            if (_smoke != null) _smoke.Emitting = false;
            _isPlaying = false;
        }
    }
}