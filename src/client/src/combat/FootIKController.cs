using Godot;
using System;
using DarkAges.Combat.FSM;

namespace DarkAges.Combat
{
    /// <summary>
    /// [CLIENT_AGENT] Foot IK Controller for terrain alignment.
    /// Manages SkeletonIK3D nodes to align player feet with uneven ground.
    /// Features:
    /// - Raycast-based terrain detection
    /// - Smooth foot alignment with interpolation
    /// - State-aware (disabled during jump/dodge)
    /// - Angle limiting for natural foot placement
    /// </summary>
    [GlobalClass]
    public partial class FootIKController : Node
    {
        [Export] public bool EnableFootIK = true;
        [Export] public float RaycastDistance = 1.5f;
        [Export] public float InterpolationSpeed = 10.0f;
        [Export] public float MaxFootAngle = 45.0f; // Max angle feet can rotate
        [Export] public float FootOffset = 0.05f; // Slight offset above ground
        [Export] public uint IKUpdateInterval = 2; // Update every N frames
        
        private SkeletonIK3D _leftFootIK;
        private SkeletonIK3D _rightFootIK;
        private AnimationStateMachine _animStateMachine;
        private CharacterBody3D _player;
        
        // Raycast states
        private bool _leftFootGrounded = false;
        private bool _rightFootGrounded = false;
        private Vector3 _leftFootTargetPos;
        private Vector3 _rightFootTargetPos;
        private Quaternion _leftFootTargetRot;
        private Quaternion _rightFootTargetRot;
        
        // Current interpolated values
        private Vector3 _leftFootCurrentPos;
        private Vector3 _rightFootCurrentPos;
        private Quaternion _leftFootCurrentRot = Quaternion.Identity;
        private Quaternion _rightFootCurrentRot = Quaternion.Identity;
        
        private uint _frameCounter = 0;
        
        public override void _Ready()
        {
            _player = GetParent<CharacterBody3D>();
            if (_player == null)
            {
                GD.PrintErr("[FootIKController] Parent must be CharacterBody3D");
                return;
            }
            
            _leftFootIK = GetParent().GetNode<SkeletonIK3D>("LeftFootIK");
            _rightFootIK = GetParent().GetNode<SkeletonIK3D>("RightFootIK");
            _animStateMachine = GetParent().GetNodeOrNull<AnimationStateMachine>("AnimationStateMachine");
            
            if (_leftFootIK == null || _rightFootIK == null)
            {
                GD.PrintErr("[FootIKController] SkeletonIK3D nodes not found");
                return;
            }
            
            // Start IK disabled by default
            _leftFootIK.Start(false);
            _rightFootIK.Start(false);
            
            GD.Print("[FootIKController] Initialized");
        }
        
        public override void _PhysicsProcess(double delta)
        {
            _frameCounter++;
            
            // Throttle updates
            if (_frameCounter % IKUpdateInterval != 0)
                return;
                
            if (!EnableFootIK || _player == null)
            {
                DisableIK();
                return;
            }
            
            // Disable IK during states where feet shouldn't adapt
            if (ShouldDisableIK())
            {
                DisableIK();
                return;
            }
            
            // Perform raycasts and update IK targets
            UpdateFootPosition(Foot.Left, delta);
            UpdateFootPosition(Foot.Right, delta);
            
            // Apply interpolated targets
            ApplyIKTargets();
        }
        
        private bool ShouldDisableIK()
        {
            if (_animStateMachine == null)
                return false;
                
            var state = _animStateMachine.CurrentState;
            return state == AnimationStateMachine.StateType.Dodging ||
                   state == AnimationStateMachine.StateType.Hit ||
                   state == AnimationStateMachine.StateType.Dead ||
                   _player is CharacterBody3D cb && !cb.IsOnFloor();
        }
        
        private void DisableIK()
        {
            _leftFootIK?.Start(false);
            _rightFootIK?.Start(false);
        }
        
        private void EnableIK()
        {
            _leftFootIK?.Start(true);
            _rightFootIK?.Start(true);
        }
        
        private void UpdateFootPosition(Foot foot, double delta)
        {
            var footIK = foot == Foot.Left ? _leftFootIK : _rightFootIK;
            if (footIK == null) return;
            
            // Get foot bone world position
            Vector3 footWorldPos = GetFootWorldPosition(footIK);
            
            // Raycast down from foot height
            var spaceState = _player.GetWorld3D().DirectSpaceState;
            var from = footWorldPos + Vector3.Up * 0.5f;
            var to = footWorldPos - Vector3.Up * RaycastDistance;
            
            var query = PhysicsRayQueryParameters3D.Create(from, to);
            query.CollisionMask = 1; // Terrain layer
            query.Exclude = new Godot.Collections.Array<Rid> { _player.GetRid() };
            
            var result = spaceState.IntersectRay(query);
            
            bool grounded = result.Count > 0;
            
            if (grounded)
            {
                Vector3 hitPos = (Vector3)result["position"];
                Vector3 hitNormal = (Vector3)result["normal"];
                
                // Calculate target position with offset
                Vector3 targetPos = hitPos + Vector3.Up * FootOffset;
                
                // Calculate target rotation based on terrain normal
                Quaternion targetRot = CalculateFootRotation(hitNormal);
                
                if (foot == Foot.Left)
                {
                    _leftFootGrounded = true;
                    _leftFootTargetPos = targetPos;
                    _leftFootTargetRot = targetRot;
                }
                else
                {
                    _rightFootGrounded = true;
                    _rightFootTargetPos = targetPos;
                    _rightFootTargetRot = targetRot;
                }
            }
            else
            {
                if (foot == Foot.Left)
                    _leftFootGrounded = false;
                else
                    _rightFootGrounded = false;
            }
        }
        
        private void ApplyIKTargets()
        {
            float dt = (float)GetProcessDeltaTime() * IKUpdateInterval;
            float lerpFactor = Mathf.Clamp(InterpolationSpeed * dt, 0.0f, 1.0f);
            
            // Left foot
            if (_leftFootGrounded && _leftFootIK != null)
            {
                _leftFootCurrentPos = _leftFootCurrentPos.Lerp(_leftFootTargetPos, lerpFactor);
                _leftFootCurrentRot = _leftFootCurrentRot.Slerp(_leftFootTargetRot, lerpFactor);
                
                Transform3D targetTransform = new Transform3D(
                    new Basis(_leftFootCurrentRot),
                    _leftFootCurrentPos
                );
                _leftFootIK.Target = targetTransform;
                _leftFootIK.Start(true);
            }
            else if (_leftFootIK != null)
            {
                _leftFootIK.Start(false);
            }
            
            // Right foot
            if (_rightFootGrounded && _rightFootIK != null)
            {
                _rightFootCurrentPos = _rightFootCurrentPos.Lerp(_rightFootTargetPos, lerpFactor);
                _rightFootCurrentRot = _rightFootCurrentRot.Slerp(_rightFootTargetRot, lerpFactor);
                
                Transform3D targetTransform = new Transform3D(
                    new Basis(_rightFootCurrentRot),
                    _rightFootCurrentPos
                );
                _rightFootIK.Target = targetTransform;
                _rightFootIK.Start(true);
            }
            else if (_rightFootIK != null)
            {
                _rightFootIK.Start(false);
            }
        }
        
        private Vector3 GetFootWorldPosition(SkeletonIK3D footIK)
        {
            // Get the skeleton from the IK node
            var skeleton = footIK.GetParentOrNull<Skeleton3D>();
            if (skeleton == null)
            {
                // Fallback to player position + offset
                return _player.GlobalPosition + new Vector3(
                    footIK.Name.ToString().Contains("Left") ? -0.2f : 0.2f,
                    0.1f,
                    0.1f
                );
            }
            
            int boneIdx = skeleton.FindBone(footIK.TipBone);
            if (boneIdx == -1)
                return _player.GlobalPosition;
                
            return skeleton.ToGlobal(skeleton.GetBoneGlobalPose(boneIdx).Origin);
        }
        
        private Quaternion CalculateFootRotation(Vector3 surfaceNormal)
        {
            // Calculate rotation to align foot with surface
            Vector3 up = Vector3.Up;
            Vector3 axis = up.Cross(surfaceNormal).Normalized();
            float angle = Mathf.Acos(up.Dot(surfaceNormal));
            
            // Clamp angle
            angle = Mathf.Clamp(angle, 0, Mathf.DegToRad(MaxFootAngle));
            
            if (axis.LengthSquared() < 0.001f)
                return Quaternion.Identity;
                
            return new Quaternion(axis, angle);
        }
        
        public void SetEnabled(bool enabled)
        {
            EnableFootIK = enabled;
        }
        
        private enum Foot
        {
            Left,
            Right
        }
    }
}
