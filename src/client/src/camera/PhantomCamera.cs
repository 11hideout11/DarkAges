using Godot;
using System;
using System.Linq;

namespace DarkAges.Camera
{
    /// <summary>
    /// [CLIENT_AGENT] Phantom Camera - Sophisticated third-person camera with lock-on targeting.
    /// 
    /// Features:
    /// - Smooth spring-based third-person follow
    /// - Lock-on targeting with automatic focus
    /// - Camera collision avoidance via raycast
    /// - Configurable deadzone and sensitivity
    /// - Smooth transitions between free-look and lock-on modes
    /// 
    /// Usage:
    /// 1. Attach to CameraRig node in Player.tscn
    /// 2. Configure export properties for feel
    /// 3. Call LockOn(targetEntity) / ReleaseLockOn() from input
    /// </summary>
    [GlobalClass]
    public partial class PhantomCamera : Node3D
    {
        // ============================================
        // EXPORT PROPERTIES
        // ============================================
        
        [Export]
        public float RotationSmoothing
        {
            get => _rotationSmoothing;
            set => _rotationSmoothing = value;
        }
        
        [Export]
        public float DistanceSmoothing
        {
            get => _distanceSmoothing;
            set => _distanceSmoothing = value;
        }
        
        [Export]
        public float MinDistance
        {
            get => _minDistance;
            set => _minDistance = value;
        }
        
        [Export]
        public float MaxDistance
        {
            get => _maxDistance;
            set => _maxDistance = value;
        }
        
        [Export]
        public float InputSensitivity
        {
            get => _inputSensitivity;
            set => _inputSensitivity = value;
        }
        
        [Export]
        public float InputDeadzone
        {
            get => _inputDeadzone;
            set => _inputDeadzone = value;
        }
        
        [Export]
        public float CollisionMargin
        {
            get => _collisionMargin;
            set => _collisionMargin = value;
        }
        
        [Export]
        public float PitchClampUp
        {
            get => _pitchClampUp;
            set => _pitchClampUp = value;
        }
        
        [Export]
        public float PitchClampDown
        {
            get => _pitchClampDown;
            set => _pitchClampDown = value;
        }
        
        [Export]
        public float LockOnDistance
        {
            get => _lockOnDistance;
            set => _lockOnDistance = value;
        }
        
        [Export]
        public float LockOnTransitionSpeed
        {
            get => _lockOnTransitionSpeed;
            set => _lockOnTransitionSpeed = value;
        }
        
        [Export]
        public float LockOnAngleThreshold
        {
            get => _lockOnAngleThreshold;
            set => _lockOnAngleThreshold = value;
        }
        
        // ============================================
        // STATE
        // ============================================
        
        public float Yaw { get; private set; }
        public float Pitch { get; private set; }
        public float CurrentDistance { get; private set; }
        
        public bool IsLockedOn => _lockedTarget != null;
        
        public Node3D LockedTarget => _lockedTarget;
        
        // ============================================
        // PRIVATE FIELDS
        // ============================================
        
        // Default values
        private float _rotationSmoothing = 8.0f;
        private float _distanceSmoothing = 5.0f;
        private float _minDistance = 1.5f;
        private float _maxDistance = 5.0f;
        private float _inputSensitivity = 0.003f;
        private float _inputDeadzone = 0.0f;
        private float _collisionMargin = 0.1f;
        private float _pitchClampUp = 45.0f;
        private float _pitchClampDown = 60.0f;
        private float _lockOnDistance = 3.0f;
        private float _lockOnTransitionSpeed = 5.0f;
        private float _lockOnAngleThreshold = 15.0f; // degrees
        
        // State
        private float _targetYaw = 0.0f;
        private float _targetPitch = 0.0f;
        private float _targetDistance = 5.0f;
        private float _currentDistanceSmooth = 5.0f;
        
        // Lock-on
        private Node3D _lockedTarget = null;
        private Vector3 _lockOnOffset = new Vector3(0, 1.5f, 0); // Target above head
        private float _lockOnTransition = 0.0f; // 0 = free, 1 = locked
        
        // References
        private SpringArm3D _springArm;
        private RayCast3D _raycast;
        private Node3D _cameraRig;
        
        // ============================================
        // LIFECYCLE
        // ============================================
        
        public override void _Ready()
        {
            // Get SpringArm child
            _springArm = GetNode<SpringArm3D>("SpringArm3D");
            if (_springArm == null)
            {
                GD.PrintErr("[PhantomCamera] SpringArm3D child not found!");
                return;
            }
            
            // Get CameraRig parent
            _cameraRig = GetParent<Node3D>();
            
            // Initialize distances
            _targetDistance = _maxDistance;
            CurrentDistance = _springArm.SpringLength;
            _currentDistanceSmooth = _springArm.SpringLength;
            
            // Get initial rotation
            var rot = Rotation;
            _targetYaw = rot.Y;
            _targetPitch = rot.X;
            Yaw = rot.Y;
            Pitch = rot.X;
            
            // Create collision raycast
            _raycast = new RayCast3D();
            _raycast.Name = "CameraCollisionRay";
            _raycast.TargetPosition = new Vector3(0, 0, -_maxDistance);
            _raycast.CollisionMask = 1;
            _raycast.Enabled = true;
            AddChild(_raycast);
            
            GD.Print("[PhantomCamera] Ready");
        }
        
        public override void _Input(InputEvent @event)
        {
            if (@event is InputEventMouseMotion mouseMotion)
            {
                Vector2 rel = mouseMotion.Relative;
                
                // Skip rotation input when locked on (mouse look disabled during lock-on)
                if (IsLockedOn) return;
                
                // Apply deadzone
                if (_inputDeadzone > 0.0f && rel.Length() < _inputDeadzone)
                    return;
                
                _targetYaw -= rel.X * _inputSensitivity;
                _targetPitch -= rel.Y * _inputSensitivity;
                
                _targetPitch = Mathf.Clamp(_targetPitch, Mathf.DegToRad(-_pitchClampDown), Mathf.DegToRad(_pitchClampUp));
            }
            
            // Toggle mouse capture
            if (@event.IsActionPressed("ui_cancel"))
            {
                Input.MouseMode = Input.MouseMode == Input.MouseModeEnum.Captured
                    ? Input.MouseModeEnum.Visible
                    : Input.MouseModeEnum.Captured;
            }
            
            // Lock-on toggle (custom action - should be configured in input map)
            if (@event.IsActionPressed("lock_on"))
            {
                if (IsLockedOn)
                    ReleaseLockOn();
                else
                    AttemptLockOn();
            }
            
            // Switch lock-on target
            if (@event.IsActionPressed("lock_on_next") && IsLockedOn)
            {
                SwitchLockOnTarget(1);
            }
            
            if (@event.IsActionPressed("lock_on_prev") && IsLockedOn)
            {
                SwitchLockOnTarget(-1);
            }
        }
        
        public override void _PhysicsProcess(double delta)
        {
            float dt = (float)delta;
            
            // Update lock-on transition
            UpdateLockOnTransition(dt);
            
            // Update rotation target based on mode
            if (IsLockedOn)
            {
                UpdateLockOnTarget(dt);
            }
            
            // Smooth rotation
            Yaw = Mathf.LerpAngle(Yaw, _targetYaw, _rotationSmoothing * dt);
            Pitch = Mathf.LerpAngle(Pitch, _targetPitch, _rotationSmoothing * dt);
            
            Rotation = new Vector3(Pitch, Yaw, 0.0f);
            
            // Update distance with collision
            UpdateDesiredDistance();
            
            // Smooth distance
            _currentDistanceSmooth = Mathf.Lerp(_currentDistanceSmooth, _targetDistance, _distanceSmoothing * dt);
            CurrentDistance = _currentDistanceSmooth;
            
            if (_springArm != null)
            {
                _springArm.SpringLength = CurrentDistance;
            }
        }
        
        // ============================================
        // LOCK-ON API
        // ============================================
        
        /// <summary>
        /// Attempt to lock onto nearest valid target in range
        /// </summary>
        public void AttemptLockOn()
        {
            var player = GetParent().GetParent() as Node3D; // CameraRig -> Player
            if (player == null) return;
            
            // Find nearest enemy in range
            Node3D bestTarget = null;
            float bestAngle = float.MaxValue;
            
            // Simple implementation: check nearby CharacterBody3D nodes
            // In full implementation, would use spatial query or enemy manager
            var nearby = GetTree().GetNodesInGroup("enemies");
            
            foreach (var node in nearby)
            {
                if (node is not Node3D enemy) continue;
                
                float dist = player.GlobalPosition.DistanceTo(enemy.GlobalPosition);
                if (dist > 20.0f) continue; // Max lock-on range
                
                // Check angle from camera forward
                Vector3 toEnemy = (enemy.GlobalPosition - player.GlobalPosition).Normalized();
                Vector3 camForward = (-GlobalTransform.Basis.Z).Normalized();
                
                float angle = toEnemy.AngleTo(camForward);
                float angleDeg = Mathf.RadToDeg(angle);
                
                if (angleDeg < _lockOnAngleThreshold && angleDeg < bestAngle)
                {
                    bestAngle = angleDeg;
                    bestTarget = enemy as Node3D;
                }
            }
            
            if (bestTarget != null)
            {
                SetLockedTarget(bestTarget);
            }
        }
        
        /// <summary>
        /// Set lock-on target directly
        /// </summary>
        public void SetLockedTarget(Node3D target)
        {
            _lockedTarget = target;
            _lockOnTransition = 0.0f;
            
            GD.Print($"[PhantomCamera] Locked onto: {target?.Name}");
        }
        
        /// <summary>
        /// Release lock-on
        /// </summary>
        public void ReleaseLockOn()
        {
            _lockedTarget = null;
            _lockOnTransition = 0.0f;
            
            GD.Print("[PhantomCamera] Released lock-on");
        }
        
        /// <summary>
        /// Switch to next/previous lock-on target
        /// </summary>
        public void SwitchLockOnTarget(int direction)
        {
            if (_lockedTarget == null) return;
            
            // Get all enemies (including the current one) and sort by distance
            var player = GetParent().GetParent() as Node3D;
            var nearby = GetTree().GetNodesInGroup("enemies");
            
            var targets = new System.Collections.Generic.List<Node3D>();
            foreach (var node in nearby)
            {
                if (node is Node3D target)
                {
                    float dist = player.GlobalPosition.DistanceTo(target.GlobalPosition);
                    if (dist < 20.0f)
                    {
                        targets.Add(target);
                    }
                }
            }
            
            if (targets.Count == 0) return;
            
            // Sort by distance
            targets = targets.OrderBy(t => player.GlobalPosition.DistanceTo(t.GlobalPosition)).ToList();
            
            // Find current target's index, then advance by direction
            int currentIdx = -1;
            for (int i = 0; i < targets.Count; i++)
            {
                if (targets[i] == _lockedTarget)
                {
                    currentIdx = i;
                    break;
                }
            }
            
            // If current target not found, start from 0; otherwise step by direction
            int newIdx = currentIdx == -1
                ? 0
                : (currentIdx + direction + targets.Count) % targets.Count;
            
            SetLockedTarget(targets[newIdx]);
        }
        
        // ============================================
        // PRIVATE HELPERS
        // ============================================
        
        private void UpdateLockOnTransition(float dt)
        {
            float target = IsLockedOn ? 1.0f : 0.0f;
            
            if (Mathf.Abs(_lockOnTransition - target) > 0.01f)
            {
                _lockOnTransition = Mathf.MoveToward(_lockOnTransition, target, _lockOnTransitionSpeed * dt);
            }
        }
        
        private void UpdateLockOnTarget(float dt)
        {
            if (_lockedTarget == null) return;
            
            var player = GetParent().GetParent() as Node3D;
            if (player == null) return;
            
            // Calculate desired camera position behind target
            Vector3 targetPos = _lockedTarget.GlobalPosition + _lockOnOffset;
            Vector3 toTarget = targetPos - player.GlobalPosition;
            toTarget = toTarget.Normalized();
            
            // Set desired yaw to face target
            _targetYaw = Mathf.Atan2(toTarget.X, toTarget.Z);
            
            // Set desired pitch to look slightly up at target
            float horizontalDist = new Vector2(toTarget.X, toTarget.Z).Length();
            _targetPitch = Mathf.Atan2(toTarget.Y, horizontalDist) * 0.5f; // Look at upper body
            
            // Reduce distance when locked on
            _targetDistance = _lockOnDistance;
        }
        
        private void UpdateDesiredDistance()
        {
            float desired = IsLockedOn ? _lockOnDistance : _maxDistance;
            
            // Check collision
            _raycast.TargetPosition = new Vector3(0, 0, -_maxDistance);
            _raycast.ForceRaycastUpdate();
            
            if (_raycast.IsColliding())
            {
                Vector3 collisionPoint = _raycast.GetCollisionPoint();
                float dist = GlobalPosition.DistanceTo(collisionPoint);
                // Only push camera *in*, never extend beyond the mode's desired distance
                float collisionAdjusted = Mathf.Max(_minDistance, dist - _collisionMargin);
                desired = Mathf.Min(desired, collisionAdjusted);
            }
            
            _targetDistance = desired;
        }
    }
}