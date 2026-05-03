using Godot;
using System;

namespace DarkAges.Combat
{
    /// <summary>
    /// [CLIENT_AGENT] Hit effect controller - spawns particle bursts on combat hits
    /// </summary>
    public partial class HitEffect : Node3D
    {
        public enum EffectType
        {
            Sparks,   // Metal on metal
            Blood,    // Flesh hit
            Dust      // Miss/block impact
        }

        [Export] public float EffectLifetime = 0.5f;
        [Export] public bool AutoDelete = true;

        // Particle systems
        private GpuParticles3D _sparks;
        private GpuParticles3D _blood;
        private GpuParticles3D _dust;

        // State
        private EffectType _currentEffect = EffectType.Sparks;
        private bool _isPlaying = false;

        public override void _Ready()
        {
            // Get particle references
            _sparks = GetNodeOrNull<GpuParticles3D>("Sparks");
            _blood = GetNodeOrNull<GpuParticles3D>("Blood");
            _dust = GetNodeOrNull<GpuParticles3D>("Dust");
            
            // Pre-warm particles (emit once)
            if (_sparks != null) _sparks.Emitting = false;
            if (_blood != null) _blood.Emitting = false;
            if (_dust != null) _dust.Emitting = false;
        }

        /// <summary>
        /// Play spark effect (default - metal hit)
        /// </summary>
        public void PlaySparks()
        {
            PlayEffect(EffectType.Sparks);
        }

        /// <summary>
        /// Play blood effect (flesh hit)
        /// </summary>
        public void PlayBlood()
        {
            PlayEffect(EffectType.Blood);
        }

        /// <summary>
        /// Play dust effect (miss/block)
        /// </summary>
        public void PlayDust()
        {
            PlayEffect(EffectType.Dust);
        }

        /// <summary>
        /// Play the specified effect type
        /// </summary>
        public void PlayEffect(EffectType effectType)
        {
            StopAll();
            
            _currentEffect = effectType;
            
            switch (effectType)
            {
                case EffectType.Sparks:
                    if (_sparks != null)
                    {
                        _sparks.Emitting = true;
                    }
                    break;
                    
                case EffectType.Blood:
                    if (_blood != null)
                    {
                        _blood.Emitting = true;
                    }
                    break;
                    
                case EffectType.Dust:
                    if (_dust != null)
                    {
                        _dust.Emitting = true;
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
        public static void SpawnAt(EffectType effectType, Vector3 worldPosition)
        {
            var scene = GD.Load<PackedScene>("res://scenes/HitEffect.tscn");
            if (scene == null)
            {
                GD.PushWarning("[HitEffect] Failed to load HitEffect.tscn");
                return;
            }
            
            var instance = scene.Instantiate<HitEffect>();
            if (instance == null)
            {
                GD.PushWarning("[HitEffect] Failed to instantiate");
                return;
            }
            
            instance.GlobalPosition = worldPosition;
            
            var main = instance.GetTree()?.CurrentScene;
            if (main != null)
            {
                main.AddChild(instance);
                
                // Play the effect
                switch (effectType)
                {
                    case EffectType.Sparks:
                        instance.PlaySparks();
                        break;
                    case EffectType.Blood:
                        instance.PlayBlood();
                        break;
                    case EffectType.Dust:
                        instance.PlayDust();
                        break;
                }
            }
            else
            {
                instance.QueueFree();
            }
        }

        /// <summary>
        /// Spawn spark effect at position
        /// </summary>
        public static void SpawnSparks(Vector3 position)
        {
            SpawnAt(EffectType.Sparks, position);
        }

        /// <summary>
        /// Spawn blood effect at position
        /// </summary>
        public static void SpawnBlood(Vector3 position)
        {
            SpawnAt(EffectType.Blood, position);
        }

        /// <summary>
        /// Spawn dust effect at position
        /// </summary>
        public static void SpawnDust(Vector3 position)
        {
            SpawnAt(EffectType.Dust, position);
        }

        private void StopAll()
        {
            if (_sparks != null) _sparks.Emitting = false;
            if (_blood != null) _blood.Emitting = false;
            if (_dust != null) _dust.Emitting = false;
            _isPlaying = false;
        }
    }
}