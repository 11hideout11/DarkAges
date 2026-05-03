using Godot;
using System;
using DarkAges.Util;

namespace DarkAges.Camera
{
    /// <summary>
    /// [CLIENT_AGENT] Polished third-person camera controller.
    /// Features: smooth rotation, configurable deadzone (input threshold), collision avoidance via raycast push-in, height/rotation smoothing.
    /// Attached to the CameraRig node (parent of SpringArm3D).
    /// </summary>
    public partial class CameraController : Node3D
    {
        // --- Exported Parameters ---

        [Export] public float RotationSmoothing = 8.0f;          // Smoothing factor for rotation (higher = snappier)
        [Export] public float DistanceSmoothing = 5.0f;         // Smoothing factor for distance (SpringArm length)
        [Export] public float MinDistance = 2.0f;                // Minimum camera distance
        [Export] public float MaxDistance = 5.0f;                // Maximum camera distance (default 5m)
        [Export] public float InputSensitivity = 0.003f;         // Mouse sensitivity (degrees per pixel)
        [Export] public float InputDeadzone = 0.0f;              // Minimum mouse movement (pixels) to register; 0 = no deadzone
        [Export] public float CollisionMargin = 0.1f;            // Buffer distance from obstacle
        [Export] public float PitchClampUp = 45.0f;              // Max upward pitch (degrees)
        [Export] public float PitchClampDown = 60.0f;            // Max downward pitch (degrees)

        // --- State ---

        public float Yaw { get; private set; } = 0.0f;
        public float Pitch { get; private set; } = 0.0f;
        public float CurrentDistance { get; private set; } = 5.0f;

        private float _targetYaw = 0.0f;
        private float _targetPitch = 0.0f;
        private float _targetDistance = 5.0f;

        private SpringArm3D _springArm;
        private RayCast3D _raycast;

        public override void _Ready()
        {
            // Get SpringArm child
            _springArm = GetNode<SpringArm3D>("SpringArm3D");
            if (_springArm == null)
            {
                GD.PrintErr("[CameraController] SpringArm3D child not found!");
                return;
            }

            // Initialize distances
            _targetDistance = MaxDistance;
            CurrentDistance = _springArm.SpringLength;
            _currentDistanceSmooth = _springArm.SpringLength;

            // Initialize rotation from current CameraRig rotation
            var rot = Rotation;
            _targetYaw = rot.Y;
            _targetPitch = rot.X;
            Yaw = rot.Y;
            Pitch = rot.X;

            // Create RayCast3D for collision avoidance
            _raycast = new RayCast3D();
            _raycast.Name = "CameraCollisionRay";
            _raycast.TargetPosition = new Vector3(0, 0, -MaxDistance); // Cast along local -Z (camera forward direction)
            _raycast.CollisionMask = 1; // collide with world layer
            _raycast.Enabled = true;
            SafeAddChild(_raycast);

            // Ensure mouse capture (should already be set, but just in case)
            Input.MouseMode = Input.MouseModeEnum.Captured;
        }

        public override void _Input(InputEvent @event)
        {
            if (@event is InputEventMouseMotion mouseMotion)
            {
                Vector2 rel = mouseMotion.Relative;

                // Apply input deadzone: ignore small movements
                if (InputDeadzone > 0.0f && rel.Length() < InputDeadzone)
                    return;

                _targetYaw -= rel.X * InputSensitivity;
                _targetPitch -= rel.Y * InputSensitivity;

                // Clamp pitch to avoid flipping
                _targetPitch = Mathf.Clamp(_targetPitch, Mathf.DegToRad(-PitchClampDown), Mathf.DegToRad(PitchClampUp));
            }

            // Toggle mouse capture (shared with PredictedPlayer, but both will receive; safe)
            if (@event.IsActionPressed("ui_cancel"))
            {
                Input.MouseMode = Input.MouseMode == Input.MouseModeEnum.Captured
                    ? Input.MouseModeEnum.Visible
                    : Input.MouseModeEnum.Captured;
            }
        }

        private float _currentDistanceSmooth = 0.0f; // for smoothing

        public override void _PhysicsProcess(double delta)
        {
            float dt = (float)delta;

            // --- Rotation smoothing ---
            // Use LerpAngle for shortest path
            Yaw = Mathf.LerpAngle(Yaw, _targetYaw, RotationSmoothing * dt);
            Pitch = Mathf.LerpAngle(Pitch, _targetPitch, RotationSmoothing * dt);

            Rotation = new Vector3(Pitch, Yaw, 0.0f);

            // --- Collision-based distance adjustment ---
            UpdateDesiredDistance();

            // Smooth distance changes
            _currentDistanceSmooth = Mathf.Lerp(_currentDistanceSmooth, _targetDistance, DistanceSmoothing * dt);
            CurrentDistance = _currentDistanceSmooth;
            if (_springArm != null)
            {
                _springArm.SpringLength = CurrentDistance;
            }
        }

        private void UpdateDesiredDistance()
        {
            // Default to max distance
            float desired = MaxDistance;

            // Perform raycast from CameraRig along its local -Z axis (camera forward)
            // RayCast3D is oriented in local space; its TargetPosition is relative to node.
            // Ensure target position is set to current max distance (in case changed)
            _raycast.TargetPosition = new Vector3(0, 0, -MaxDistance);
            // Force update
            _raycast.ForceRaycastUpdate();

            if (_raycast.IsColliding())
            {
                // Get collision distance from CameraRig origin
                Vector3 collisionPoint = _raycast.GetCollisionPoint();
                float dist = GlobalPosition.DistanceTo(collisionPoint);
                // Subtract a small margin to keep camera slightly away from wall
                desired = Mathf.Max(MinDistance, dist - CollisionMargin);
            }
            else
            {
                desired = MaxDistance;
            }

            _targetDistance = desired;
        }
    }
}
