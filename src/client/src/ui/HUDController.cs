using Godot;
using System;
using DarkAges.Networking;
using DarkAges.Combat;
using DarkAges.Client.UI;

namespace DarkAges.Client.UI
{
    /// <summary>
    /// [CLIENT_AGENT] WP-7-4 Main HUD controller that coordinates all combat UI elements.
    /// Manages health bars, ability bar, target lock, combat text, death UI, and interaction prompt.
    /// Uses UITheme for consistent styling.
    /// </summary>
    public partial class HUDController : CanvasLayer
    {
        // UI References
        private HealthBarSystem _playerHealthBar;
        private HealthBarSystem _targetHealthBar;
        private AbilityBar _abilityBar;
        private TargetLockSystem _targetLockSystem;
        private CombatTextSystem _combatTextSystem;
        private DeathRespawnUI _deathRespawnUI;
        private ChatPanel _chatPanel;
        private QuestTracker _questTracker;
        private DialoguePanel _dialoguePanel;
        
        // Crosshair
        private TextureRect _crosshair;
        private TextureRect _hitMarker;
        
        // Party UI (simplified for now)
        private Panel _partyPanel;
        
        // Interaction Prompt
        private Label _interactionPrompt;
        // Replaced by per-NPC InteractionRange: private const float INTERACT_RANGE = 3.5f; // DEPRECATED // World units

        // Configuration
        [Export] public bool ShowDebugInfo = false;

        public override void _Ready()
        {
            GD.Print("[HUDController] WP-7-4 Combat UI initializing...");

            // Defer component initialization to avoid "busy setting up children" warnings.
            // This ensures all child scene instances (ChatPanel, QuestTracker, etc.)
            // have fully completed their own _Ready() before we query them.
            CallDeferred(MethodName.DeferredInitialize);

            // Connect to game state immediately (singletons are safe)
            GameState.Instance.ConnectionStateChanged += OnConnectionStateChanged;
            CombatEventSystem.Instance.DamageDealt += OnDamageDealt;

            // Initially hide until connected
            Visible = false;
        }

        public void DeferredInitialize()
        {
            InitializeComponents();
        }

        public override void _ExitTree()
        {
            if (GameState.Instance != null)
            {
                GameState.Instance.ConnectionStateChanged -= OnConnectionStateChanged;
            }
            if (CombatEventSystem.Instance != null)
            {
                CombatEventSystem.Instance.DamageDealt -= OnDamageDealt;
            }
        }
        
        private void InitializeComponents()
        {
            // Player Health (Top Left)
            _playerHealthBar = GetNode<HealthBarSystem>("SafeArea/MainLayout/TopBar/PlayerHealth");
            if (_playerHealthBar != null)
            {
                _playerHealthBar.SetPlayerName("Player");
            }
            
            // Target Health (Top Center)
            _targetHealthBar = GetNode<HealthBarSystem>("SafeArea/MainLayout/TopBar/TargetHealth");
            
            // Ability Bar (Bottom Center)
            _abilityBar = GetNode<AbilityBar>("SafeArea/MainLayout/BottomBar/AbilityBar");
            
            // Target Lock System
            _targetLockSystem = GetNode<TargetLockSystem>("TargetLockSystem");
            if (_targetLockSystem == null)
            {
                _targetLockSystem = new TargetLockSystem();
                AddChild(_targetLockSystem);
            }
            
            // Combat Text System
            _combatTextSystem = GetNode<CombatTextSystem>("CombatTextSystem");
            if (_combatTextSystem == null)
            {
                _combatTextSystem = new CombatTextSystem();
                AddChild(_combatTextSystem);
            }
            
            // Death/Respawn UI
            _deathRespawnUI = GetNode<DeathRespawnUI>("DeathRespawnUI");
            if (_deathRespawnUI == null)
            {
                _deathRespawnUI = new DeathRespawnUI();
                AddChild(_deathRespawnUI);
            }

            // Chat Panel
            _chatPanel = GetNode<ChatPanel>("ChatPanel");
            if (_chatPanel == null)
            {
                _chatPanel = new ChatPanel();
                AddChild(_chatPanel);
            }

            // Quest Tracker
            _questTracker = GetNode<QuestTracker>("QuestTracker");
            if (_questTracker == null)
            {
                _questTracker = new QuestTracker();
                AddChild(_questTracker);
            }


            // Dialogue Panel
            _dialoguePanel = GetNode<DialoguePanel>("DialoguePanel");
            if (_dialoguePanel == null)
            {
                var dialogueScene = GD.Load<PackedScene>("res://scenes/ui/DialoguePanel.tscn");
                if (dialogueScene != null)
                {
                    _dialoguePanel = (DialoguePanel)dialogueScene.Instantiate();
                    AddChild(_dialoguePanel);
                }
            }

            // Connect to dialogue events
            if (NetworkManager.Instance != null)
            {
                NetworkManager.Instance.DialogueStartReceived += OnDialogueStart;
            }

            // Interaction Prompt (Phase 2.5)
            _interactionPrompt = new Label();
            _interactionPrompt.Text = "Press E to interact";
            _interactionPrompt.HorizontalAlignment = HorizontalAlignment.Center;
            _interactionPrompt.VerticalAlignment = VerticalAlignment.Bottom;
            // Position near bottom center of screen, offset up by 80 pixels
            var vpSize = GetViewport().GetVisibleRect().Size;
            _interactionPrompt.Position = new Vector2(vpSize.X / 2 - 100, vpSize.Y - 80);
            _interactionPrompt.Visible = false;
            AddChild(_interactionPrompt);

            // Crosshair
            _crosshair = GetNode<TextureRect>("SafeArea/MainLayout/Center/Crosshair");
            
            // Hit marker (child of crosshair)
            _hitMarker = GetNodeOrNull<TextureRect>("SafeArea/MainLayout/Center/Crosshair/HitMarker");
            
            // Party panel
            _partyPanel = GetNodeOrNull<Panel>("SafeArea/MainLayout/TopBar/PartyPanel");
            if (_partyPanel != null)
            {
                _partyPanel.Visible = false;  // Hidden until party system implemented
            }
            
            GD.Print("[HUDController] All UI components initialized");
        }

        public override void _Process(double delta)
        {
            // Update target health bar based on locked target
            UpdateTargetHealthBar();
            
            // Show/hide hit marker
            UpdateHitMarker();

            // Toggle chat panel
            if (Input.IsActionJustPressed("ui_accept") && _chatPanel != null && !_chatPanel.Visible)
            {
                _chatPanel.ShowChat();
            }

            // Toggle quest tracker visibility
            if (Input.IsActionJustPressed("quest_toggle") && _questTracker != null)
            {
                _questTracker.Visible = !_questTracker.Visible;
            }

            // Update interaction prompt visibility (Phase 2.5)
            UpdateInteractionPrompt();
        }
        
        private void UpdateTargetHealthBar()
        {
            if (_targetHealthBar == null) return;
            
            if (_targetLockSystem.HasValidTarget())
            {
                uint targetId = _targetLockSystem.GetTargetId();
                var entity = GameState.Instance.GetEntity(targetId);
                
                if (entity != null)
                {
                    _targetHealthBar.Visible = true;
                    _targetHealthBar.SetPlayerName(entity.Name);
                    _targetHealthBar.SetServerHealth(entity.HealthPercent, 100);
                }
                else
                {
                    _targetHealthBar.Visible = false;
                }
            }
            else
            {
                _targetHealthBar.Visible = false;
            }
        }
        
        private void UpdateHitMarker()
        {
            if (_hitMarker == null) return;
            
            // Hide hit marker if not recently hit
            // The HitMarker component handles its own visibility timing
        }

        private void UpdateInteractionPrompt()
        {
            if (_interactionPrompt == null) return;

            var localId = GameState.Instance.LocalEntityId;
            if (localId == 0)
            {
                _interactionPrompt.Visible = false;
                return;
            }

            var playerEntity = GameState.Instance.GetEntity(localId);
            if (playerEntity == null)
            {
                _interactionPrompt.Visible = false;
                return;
            }

            Vector3 playerPos = playerEntity.Position;
            string? closestPrompt = null;
            float closestDist = float.MaxValue;

            foreach (var entity in GameState.Instance.Entities.Values)
            {
                // Interactable NPC: has DialogueTreeId set (replicated from server)
                if (entity.DialogueTreeId > 0)
                {
                    float interactRange = entity.InteractionRange > 0 ? entity.InteractionRange : 3.5f;
                    float dist = playerPos.DistanceTo(entity.Position);
                    if (dist <= interactRange && dist < closestDist)
                    {
                        closestDist = dist;
                        closestPrompt = entity.PromptText;
                    }
                }
            }

            _interactionPrompt.Visible = closestPrompt != null;
            if (closestPrompt != null)
            {
                _interactionPrompt.Text = closestPrompt;
            }
        }

        private void OnConnectionStateChanged(GameState.ConnectionState state)
        {
            Visible = (state == GameState.ConnectionState.Connected);
        }
        
        private void OnDamageDealt(uint targetId, int damage, bool isCritical, Vector3 position)
        {
            // Show hit marker on crosshair
            ShowHitMarker(isCritical);
        }

        private void OnDialogueStart(uint npcId, uint dialogueId, string npcName, string dialogueText, string[] options)
        {
            _dialoguePanel?.ShowDialogue(npcId, npcName, dialogueText, options, dialogueId);
        }

        
        private void ShowHitMarker(bool isCritical)
        {
            if (_hitMarker == null) return;
            
            _hitMarker.Visible = true;
            _hitMarker.Modulate = isCritical ? Colors.Yellow : Colors.White;
            
            // Scale animation
            var tween = CreateTween();
            tween.SetParallel();
            tween.TweenProperty(_hitMarker, "scale", new Vector2(1.5f, 1.5f), 0.1f);
            tween.TweenProperty(_hitMarker, "modulate:a", 0.0f, 0.3f).SetDelay(0.1f);
            tween.Chain().TweenCallback(Callable.From(() => 
            {
                _hitMarker.Visible = false;
                _hitMarker.Scale = Vector2.One;
                _hitMarker.Modulate = new Color(_hitMarker.Modulate.R, _hitMarker.Modulate.G, _hitMarker.Modulate.B, 1.0f);
            }));
        }
        
        /// <summary>
        /// Public API for external systems
        /// </summary>
        
        public void SetPlayerHealth(float current, float max)
        {
            _playerHealthBar?.SetServerHealth(current, max);
        }
        
        public void ShowDamageNumber(Vector3 position, int damage, bool isCritical = false)
        {
            _combatTextSystem?.ShowDamage(position, damage, isCritical);
        }
        
        public void ShowHealNumber(Vector3 position, int amount)
        {
            _combatTextSystem?.ShowHeal(position, amount);
        }
        
        public void StartAbilityCooldown(int slotIndex, float duration)
        {
            _abilityBar?.StartLocalCooldown(slotIndex, duration);
        }
        
        public bool IsAbilityReady(int slotIndex)
        {
            return _abilityBar?.IsAbilityReady(slotIndex) ?? false;
        }
        
        public void SetTarget(uint entityId)
        {
            // This would be called by target lock system
        }
        
        public void ClearTarget()
        {
            _targetLockSystem?.ClearTarget();
        }
        
        /// <summary>
        /// Check if all P0 requirements are met
        /// </summary>
        public bool AreRequirementsMet()
        {
            bool healthUpdateTimely = _playerHealthBar?.IsHealthUpdateTimely() ?? false;
            bool hasTargetSystem = _targetLockSystem != null;
            bool hasCombatText = _combatTextSystem != null;
            bool hasAbilityBar = _abilityBar != null;
            bool hasDeathUI = _deathRespawnUI != null;
            bool hasChatPanel = _chatPanel != null;
            
            return healthUpdateTimely && hasTargetSystem && hasCombatText && hasAbilityBar && hasDeathUI && hasChatPanel;
        }
    }
}
