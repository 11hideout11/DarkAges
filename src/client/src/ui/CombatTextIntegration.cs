using Godot;
using System;

namespace DarkAges.Client.UI
{
    /// <summary>
    /// Integration script to connect CombatTextSystem to combat flow.
    /// Attach to Player or place in scene to enable damage/heal numbers.
    /// </summary>
    [GlobalClass]
    public partial class CombatTextIntegration : Node
    {
        [Export]
        public CombatTextSystem CombatTextSystem
        {
            get => _combatTextSystem;
            set => _combatTextSystem = value;
        }
        
        private CombatTextSystem _combatTextSystem;
        
        public override void _Ready()
        {
            // Auto-find CombatTextSystem if not assigned
            if (_combatTextSystem == null)
            {
                _combatTextSystem = GetNodeOrNull<CombatTextSystem>("/root/CombatTextSystem");
                if (_combatTextSystem == null)
                {
                    // Create if doesn't exist
                    _combatTextSystem = new CombatTextSystem();
                    _combatTextSystem.Name = "CombatTextSystem";
                    GetTree().Root.AddChild(_combatTextSystem);
                }
            }
            
            // Connect to combat events
            ConnectToCombat();
            
            GD.Print("[CombatTextIntegration] Ready");
        }
        
        private void ConnectToCombat()
        {
            // Find AttackFeedbackSystem
            var attackFeedback = GetNodeOrNull<Node>("/root/Player/AttackFeedback");
            if (attackFeedback != null)
            {
                // Would connect to signals if defined
                // attackFeedback.Connect("HitRegistered", this, nameof(OnHitRegistered));
            }
        }
        
        /// <summary>
        /// Show damage number at world position
        /// </summary>
        public void ShowDamage(float amount, Vector3 worldPosition, bool isCritical = false)
        {
            if (_combatTextSystem == null) return;
            
            _combatTextSystem.ShowDamage(amount, worldPosition, isCritical);
        }
        
        /// <summary>
        /// Show healing number at world position
        /// </summary>
        public void ShowHeal(float amount, Vector3 worldPosition)
        {
            if (_combatTextSystem == null) return;
            
            _combatTextSystem.ShowHeal(amount, worldPosition);
        }
        
        /// <summary>
        /// Show miss (no damage)
        /// </summary>
        public void ShowMiss(Vector3 worldPosition)
        {
            if (_combatTextSystem == null) return;
            
            _combatTextSystem.ShowMiss(worldPosition);
        }
        
        /// <summary>
        /// Show blocked damage
        /// </summary>
        public void ShowBlocked(Vector3 worldPosition)
        {
            if (_combatTextSystem == null) return;
            
            _combatTextSystem.ShowBlocked(worldPosition);
        }
    }
}