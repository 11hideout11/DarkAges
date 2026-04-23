using Godot;
using System;
using DarkAges.Networking;

namespace DarkAges.Combat
{
    /// <summary>
    /// [CLIENT_AGENT] Visual feedback for player attacks
    /// Shows weapon swing arc and hit effects when attacking
    /// </summary>
    public partial class AttackFeedbackSystem : Node3D
    {
        [Export] public bool ShowWeapon = true;
        [Export] public float SwingDuration = 0.3f;
        [Export] public float SwingArc = Mathf.Pi / 2; // 90 degree swing
        
        // Weapon visual
        private MeshInstance3D _weaponMesh;
        private Node3D _weaponPivot;
        
        // Animation state
        private bool _isSwinging = false;
        private float _swingTime = 0f;
        private float _swingDirection = 1f; // Alternates left/right
        
        // Cached player reference
        private PredictedPlayer _player;

        public override void _Ready()
        {
            SetupWeapon();
            
            // Connect to input
            if (NetworkManager.Instance != null)
            {
                NetworkManager.Instance.InputSent += OnInputSent;
            }
            
            _player = GetParent<PredictedPlayer>();
        }
        
        public override void _ExitTree()
        {
            if (NetworkManager.Instance != null)
            {
                NetworkManager.Instance.InputSent -= OnInputSent;
            }
        }
        
        private void SetupWeapon()
        {
            // Create pivot point at player side
            _weaponPivot = new Node3D
            {
                Name = "WeaponPivot",
                Position = new Vector3(0.5f, 1.0f, 0f)
            };
            AddChild(_weaponPivot);
            
            // Create simple sword mesh
            _weaponMesh = new MeshInstance3D
            {
                Name = "WeaponMesh",
                Visible = ShowWeapon
            };
            
            // Build a simple sword from primitives
            var swordMesh = new BoxMesh
            {
                Size = new Vector3(0.1f, 1.2f, 0.05f)
            };
            _weaponMesh.Mesh = swordMesh;
            
            // Position sword in hand
            _weaponMesh.Position = new Vector3(0, 0.6f, 0.3f);
            _weaponMesh.Rotation = new Vector3(Mathf.Pi / 4, 0, 0);
            
            // Material
            var swordMat = new StandardMaterial3D
            {
                AlbedoColor = new Color(0.8f, 0.7f, 0.5f),
                Metallic = 0.8f,
                Roughness = 0.2f
            };
            _weaponMesh.MaterialOverride = swordMat;
            
            _weaponPivot.AddChild(_weaponMesh);
            
            // Add swing trail particle effect
            CreateSwingTrail();
        }
        
        private void CreateSwingTrail()
        {
            var trail = new MeshInstance3D
            {
                Name = "SwingTrail",
                Mesh = new QuadMesh { Size = new Vector2(0.3f, 2.4f) },
                Visible = false
            };
            
            var trailMat = new StandardMaterial3D
            {
                AlbedoColor = new Color(1, 0.8f, 0.3f, 0.5f),
                Transparency = BaseMaterial3D.TransparencyEnum.Alpha,
                Emission = new Color(0.8f, 0.6f, 0.1f),
                EmissionEnergyMultiplier = 2.0f,
                NoDepthTest = true
            };
            trail.MaterialOverride = trailMat;
            
            _weaponPivot.AddChild(trail);
        }
        
        private void OnInputSent(InputState input)
        {
            if (input.Attack && !_isSwinging)
            {
                TriggerSwing();
            }
        }
        
        private void TriggerSwing()
        {
            _isSwinging = true;
            _swingTime = 0f;
            
            // Alternate swing direction
            _weaponPivot.Rotation = new Vector3(0f, _swingDirection * -SwingArc / 2f, 0f);
            
            // Show swing trail
            var trail = _weaponPivot.GetNodeOrNull<MeshInstance3D>("SwingTrail");
            if (trail != null)
            {
                trail.Visible = true;
            }
            
            _swingDirection *= -1; // Alternate for next swing
        }
        
        public override void _Process(double delta)
        {
            if (!_isSwinging) return;
            
            _swingTime += (float)delta;
            float progress = _swingTime / SwingDuration;
            
            if (progress >= 1f)
            {
                // Swing complete
                _isSwinging = false;
                _weaponPivot.Rotation = new Vector3(0f, 0f, 0f);
                
                // Hide trail
                var trail = _weaponPivot.GetNodeOrNull<MeshInstance3D>("SwingTrail");
                if (trail != null)
                {
                    trail.Visible = false;
                }
                return;
            }
            
            // Smooth swing arc
            float angle = Mathf.Sin(progress * Mathf.Pi) * SwingArc * _swingDirection;
            _weaponPivot.Rotation = new Vector3(0f, angle + _swingDirection * SwingArc / 2f, 0f);
            
            // Fade trail
            var trailNode = _weaponPivot.GetNodeOrNull<MeshInstance3D>("SwingTrail");
            if (trailNode != null && trailNode.MaterialOverride is StandardMaterial3D mat)
            {
                float alpha = 1f - progress * progress;
                mat = (StandardMaterial3D)mat.Duplicate();
                mat.AlbedoColor = new Color(1, 0.8f, 0.3f, alpha * 0.5f);
                trailNode.MaterialOverride = mat;
            }
        }
    }
}
