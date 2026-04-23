using Godot;
using System;

namespace DarkAges.Utils
{
    /// <summary>
    /// [CLIENT_AGENT] Visual dust particles when player moves
    /// Creates subtle ground dust effect at player's feet during movement
    /// </summary>
    public partial class MovementTrailSystem : Node3D
    {
        [Export] public bool Enabled { get; set; } = true;
        [Export] public float TrailLifetime { get; set; } = 0.8f;
        [Export] public float DustSize { get; set; } = 0.15f;
        
        private GpuParticles3D _dustEmitter;
        private CharacterBody3D _playerCharacter;
        private Vector3 _lastPosition;
        private bool _wasMoving = false;
        
        public override void _Ready()
        {
            if (!Enabled) return;
            
            SetupDustEmitter();
            
            // Get parent character
            _playerCharacter = GetParent() as CharacterBody3D;
            if (_playerCharacter != null)
            {
                _lastPosition = _playerCharacter.GlobalPosition;
            }
            
            GD.Print("[MovementTrailSystem] Initialized");
        }
        
        private void SetupDustEmitter()
        {
            _dustEmitter = new GpuParticles3D();
            _dustEmitter.Name = "DustTrailEmitter";
            
            // Position at player's feet
            _dustEmitter.Position = new Vector3(0, 0.05f, 0);
            
            // Basic particle settings
            _dustEmitter.Emitting = false; // Start off
            _dustEmitter.Amount = 8;
            _dustEmitter.Lifetime = TrailLifetime;
            _dustEmitter.OneShot = false;
            _dustEmitter.Explosiveness = 0.0f;
            _dustEmitter.Randomness = 0.3f;
            
            // Create particle material - all physics properties go here
            var particleMaterial = new ParticleProcessMaterial();
            
            // Set up emission
            particleMaterial.EmissionShape = ParticleProcessMaterial.EmissionShapeEnum.Sphere;
            particleMaterial.EmissionSphereRadius = 0.3f;
            
            // Direction and spread
            particleMaterial.Direction = new Vector3(0, 1, 0);
            particleMaterial.Spread = 90.0f;
            
            // Velocity
            particleMaterial.Flatness = 0.5f;
            
            // Initial velocity (upward drift)
            particleMaterial.InitialVelocityMin = 0.1f;
            particleMaterial.InitialVelocityMax = 0.4f;
            
            // Gravity (light gravity to make dust fall slowly)
            particleMaterial.Gravity = new Vector3(0, -0.5f, 0);
            
            // Scale
            particleMaterial.ScaleMin = DustSize * 0.7f;
            particleMaterial.ScaleMax = DustSize * 1.3f;
            
            // Color over lifetime (dust: brownish gray, fades out)
            var gradient = new Gradient();
            gradient.AddPoint(0.0f, new Color(0.5f, 0.4f, 0.3f, 0.6f));
            gradient.AddPoint(0.7f, new Color(0.5f, 0.4f, 0.3f, 0.3f));
            gradient.AddPoint(1.0f, new Color(0.5f, 0.4f, 0.3f, 0.0f));
            
            // Convert gradient to texture for color ramp
            var gradientTexture = new GradientTexture1D();
            gradientTexture.Gradient = gradient;
            
            particleMaterial.Color = new Color(1.0f, 1.0f, 1.0f, 1.0f);
            particleMaterial.ColorRamp = gradientTexture;
            
            // Turbulence
            particleMaterial.TurbulenceEnabled = true;
            particleMaterial.TurbulenceNoiseStrength = 0.15f;
            particleMaterial.TurbulenceNoiseScale = 3.0f;
            
            _dustEmitter.ProcessMaterial = particleMaterial;
            
            // Draw pass - simple quad
            var quadMesh = new QuadMesh();
            quadMesh.Size = new Vector2(0.3f, 0.3f);
            
            _dustEmitter.DrawPass1 = quadMesh;
            
            // Visibility bounds
            _dustEmitter.VisibilityAabb = new Aabb(new Vector3(-5, -5, -5), new Vector3(10, 10, 10));
            
            AddChild(_dustEmitter);
        }
        
        public override void _Process(double delta)
        {
            if (!Enabled || _playerCharacter == null || _dustEmitter == null) return;
            
            // Check if moving
            Vector3 currentPos = _playerCharacter.GlobalPosition;
            float moveDelta = (currentPos - _lastPosition).Length();
            bool isMoving = moveDelta > 0.01f && _playerCharacter.IsOnFloor();
            
            // Emit dust when moving on ground
            if (isMoving && _playerCharacter.IsOnFloor())
            {
                if (!_dustEmitter.Emitting) 
                {
                    _dustEmitter.Emitting = true;
                    _dustEmitter.Restart();
                }
                
                // Adjust emission rate based on speed
                float speed = (float)(moveDelta / delta);
                _dustEmitter.Amount = Mathf.Min(16, Mathf.Max(4, Mathf.RoundToInt(speed * 0.5f)));
            }
            else
            {
                _dustEmitter.Emitting = false;
            }
            
            _lastPosition = currentPos;
            _wasMoving = isMoving;
        }
        
        public void SetEnabled(bool enabled)
        {
            Enabled = enabled;
            if (!enabled && _dustEmitter != null)
            {
                _dustEmitter.Emitting = false;
            }
        }
    }
}
