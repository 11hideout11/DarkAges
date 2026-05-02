using Godot;
using System;
using DarkAges.Networking;

namespace DarkAges
{
    /// <summary>
    /// [CLIENT_AGENT] UI controller for connection panel and debug info
    /// Extended with NPC interaction, dialogue, quest tracking, and inventory
    /// </summary>
    public partial class GameUI : CanvasLayer
    {
        private Panel? _connectionPanel;
        private Panel? _debugPanel;
        private LineEdit? _addressInput;
        private LineEdit? _portInput;
        private Button? _connectButton;
        private Label? _statusLabel;
        private Label? _pingLabel;
        private Label? _fpsLabel;
        private Label? _entitiesLabel;
        private Label? _positionLabel;
        private Label? _predictionLabel;
        
        // Interaction and Dialogue UI
        private Panel? _interactionPanel;
        private Label? _interactionPrompt;
        private Panel? _dialoguePanel;
        private Label? _dialogueNpcName;
        private Label? _dialogueText;
        private VBoxContainer? _dialogueOptions;
        private Panel? _questPanel;
        private Label? _questTitle;
        private Label? _questProgress;
        private Panel? _inventoryPanel;
        private Label? _goldLabel;
        
        private Main? _main;

        public override void _Ready()
        {
            // Get UI references
            _connectionPanel = GetNode<Panel>("ConnectionPanel");
            _debugPanel = GetNode<Panel>("DebugPanel");
            _addressInput = GetNode<LineEdit>("ConnectionPanel/VBoxContainer/ServerAddress");
            _portInput = GetNode<LineEdit>("ConnectionPanel/VBoxContainer/ServerPort");
            _connectButton = GetNode<Button>("ConnectionPanel/VBoxContainer/ConnectButton");
            _statusLabel = GetNode<Label>("ConnectionPanel/VBoxContainer/StatusLabel");
            
            _pingLabel = GetNode<Label>("DebugPanel/VBoxContainer/PingLabel");
            _fpsLabel = GetNode<Label>("DebugPanel/VBoxContainer/FpsLabel");
            _entitiesLabel = GetNode<Label>("DebugPanel/VBoxContainer/EntitiesLabel");
            _positionLabel = GetNode<Label>("DebugPanel/VBoxContainer/PositionLabel");
            _predictionLabel = GetNode<Label>("DebugPanel/VBoxContainer/PredictionLabel");
            
            // Connect signals
            _connectButton.Pressed += OnConnectPressed;
            GameState.Instance.ConnectionStateChanged += OnConnectionStateChanged;
            NetworkManager.Instance.ConnectionResult += OnConnectionResult;
            
            // Find main scene
            _main = GetTree().CurrentScene as Main;
            
            // Interaction/dialogue/quest/inventory UI is owned by HUDController in Main.tscn.
            // Do not initialize or subscribe here as well, otherwise both UI layers will
            // respond to the same gameplay/network signals and show duplicate UI.
            
            // Show connection panel initially
            UpdateUIState(GameState.ConnectionState.Disconnected);

            // [DEMO] Auto-connect if --auto-connect flag is present in command-line args
            var args = OS.GetCmdlineUserArgs();
            bool autoConnect = false;
            string autoAddress = "127.0.0.1";
            int autoPort = 7777;
            for (int i = 0; i < args.Length; i++)
            {
                if (args[i] == "--auto-connect")
                    autoConnect = true;
                else if (args[i] == "--server" && i + 1 < args.Length)
                    autoAddress = args[i + 1];
                else if (args[i] == "--port" && i + 1 < args.Length)
                    int.TryParse(args[i + 1], out autoPort);
            }
            if (autoConnect)
            {
                GD.Print($"[GameUI] Auto-connecting to {autoAddress}:{autoPort}...");
                if (_addressInput != null) _addressInput.Text = autoAddress;
                if (_portInput != null) _portInput.Text = autoPort.ToString();
                // Defer connect to next frame so UI is fully initialized
                CallDeferred(nameof(OnConnectPressed));
            }
        }

        public override void _ExitTree()
        {
            GameState.Instance.ConnectionStateChanged -= OnConnectionStateChanged;
            NetworkManager.Instance.ConnectionResult -= OnConnectionResult;
            // Clean up new event handlers
            if (NetworkManager.Instance != null)
            {
                NetworkManager.Instance.DialogueStartReceived -= OnDialogueStart;
                NetworkManager.Instance.QuestUpdateReceived -= OnQuestUpdateReceived;
            }
            if (GameState.Instance != null)
            {
                GameState.Instance.EntitySpawned -= OnEntitySpawned;
            }
        }

        public override void _Process(double delta)
        {
            // Update debug info
            if (GameState.Instance.CurrentConnectionState == GameState.ConnectionState.Connected)
            {
                UpdateDebugInfo();
            }
        }

        private void OnConnectPressed()
        {
            if (_addressInput == null || _portInput == null) return;
            
            string address = _addressInput.Text;
            if (!int.TryParse(_portInput.Text, out int port))
            {
                port = 7777;
            }
            
            UpdateStatus("Connecting...");
            _connectButton.Disabled = true;
            
            _main?.ConnectToServer(address, port);
        }

        private void OnConnectionStateChanged(GameState.ConnectionState state)
        {
            UpdateUIState(state);
        }

        private void OnConnectionResult(bool success, string error)
        {
            _connectButton.Disabled = false;
            
            if (!success)
            {
                UpdateStatus($"Failed: {error}");
            }
        }

        private void UpdateUIState(GameState.ConnectionState state)
        {
            switch (state)
            {
                case GameState.ConnectionState.Disconnected:
                    _connectionPanel.Visible = true;
                    _debugPanel.Visible = false;
                    UpdateStatus("Disconnected");
                    break;
                    
                case GameState.ConnectionState.Connecting:
                    _connectionPanel.Visible = true;
                    _debugPanel.Visible = false;
                    UpdateStatus("Connecting...");
                    break;
                    
                case GameState.ConnectionState.Connected:
                    _connectionPanel.Visible = false;
                    _debugPanel.Visible = true;
                    break;
                    
                case GameState.ConnectionState.Error:
                    _connectionPanel.Visible = true;
                    _debugPanel.Visible = false;
                    _connectButton.Disabled = false;
                    break;
            }
        }

        private void UpdateStatus(string message)
        {
            if (_statusLabel != null)
            {
                _statusLabel.Text = message;
            }
        }

        private void UpdateDebugInfo()
        {
            // Ping
            if (_pingLabel != null)
            {
                _pingLabel.Text = $"Ping: {GameState.Instance.LastRttMs} ms";
            }
            
            // FPS
            if (_fpsLabel != null)
            {
                _fpsLabel.Text = $"FPS: {Engine.GetFramesPerSecond()}";
            }
            
            // Entities
            if (_entitiesLabel != null)
            {
                _entitiesLabel.Text = $"Entities: {GameState.Instance.Entities.Count}";
            }
            
            // Position
            if (_positionLabel != null)
            {
                var player = GetTree().CurrentScene?.GetNode<CharacterBody3D>("Players/Player");
                if (player != null)
                {
                    _positionLabel.Text = $"Pos: {player.Position.X:F1}, {player.Position.Y:F1}, {player.Position.Z:F1}";
                }
            }
            
            // Prediction error
            if (_predictionLabel != null)
            {
                var predictedPlayer = GetTree().CurrentScene?.GetNode<PredictedPlayer>("Players/Player");
                if (predictedPlayer != null)
                {
                    float error = predictedPlayer.GetPredictionError();
                    _predictionLabel.Text = $"Prediction Error: {error:F3}m";
                }
            }
        }
        
        // ============================================================================
        // NPC Interaction UI
        // ============================================================================
        
        private void SetupInteractionUI()
        {
            // Interaction prompt - shows when near interactive NPC
            _interactionPanel = new Panel();
            _interactionPanel.Name = "InteractionPanel";
            _interactionPanel.SetAnchorsPreset(Control.LayoutPreset.BottomWide);
            _interactionPanel.OffsetTop = -120;
            _interactionPanel.OffsetLeft = 10;
            _interactionPanel.OffsetRight = -10;
            _interactionPanel.OffsetBottom = -60;
            _interactionPanel.Visible = false;
            AddChild(_interactionPanel);
            
            var container = new HBoxContainer();
            container.SetAnchorsPreset(Control.LayoutPreset.FullRect);
            container.SizeFlagsVertical = Control.SizeFlags.ShrinkCenter;
            _interactionPanel.AddChild(container);
            
            _interactionPrompt = new Label();
            _interactionPrompt.Text = "[E] Interact";
            _interactionPrompt.HorizontalAlignment = HorizontalAlignment.Center;
            container.AddChild(_interactionPrompt);
        }
        
        private void OnEntityInteractionRange(EntityData data)
        {
            // Show interaction prompt when in range of interactive NPC
            if (data.InteractionRange > 0 && data.DialogueTreeId > 0)
            {
                ShowInteractionPrompt($"[E] Talk to {data.Name}");
            }
            else
            {
                HideInteractionPrompt();
            }
        }
        
        private void ShowInteractionPrompt(string message)
        {
            if (_interactionPanel != null && _interactionPrompt != null)
            {
                _interactionPrompt.Text = message;
                _interactionPanel.Visible = true;
            }
        }
        
        private void HideInteractionPrompt()
        {
            if (_interactionPanel != null)
            {
                _interactionPanel.Visible = false;
            }
        }
        
        // Check for nearby interactive NPCs
        private void CheckInteractionProximity()
        {
            var player = GetTree().CurrentScene?.GetNode<CharacterBody3D>("Players/Player");
            if (player == null) return;
            
            Vector3 playerPos = player.Position;
            bool foundInteraction = false;
            
            foreach (var kvp in GameState.Instance.Entities)
            {
                var entity = kvp.Value;
                if (entity.NpcId > 0 && entity.InteractionRange > 0)
                {
                    float dist = playerPos.DistanceTo(entity.Position);
                    if (dist <= entity.InteractionRange)
                    {
                        ShowInteractionPrompt($"[E] Talk to {entity.Name}");
                        foundInteraction = true;
                        break;
                    }
                }
            }
            
            if (!foundInteraction)
            {
                HideInteractionPrompt();
            }
        }
        
        private void OnEntitySpawned(uint entityId, Vector3 position)
        {
            var entity = GameState.Instance.GetEntity(entityId);
            if (entity != null && entity.NpcId > 0 && entity.InteractionRange > 0)
            {
                // Will be checked in next _Process
            }
        }
        
        // ============================================================================
        // Dialogue UI
        // ============================================================================
        
        private void SetupDialogueUI()
        {
            _dialoguePanel = new Panel();
            _dialoguePanel.Name = "DialoguePanel";
            _dialoguePanel.SetAnchorsPreset(Control.LayoutPreset.Center);
            _dialoguePanel.SizeFlagsHorizontal = Control.SizeFlags.ShrinkCenter;
            _dialoguePanel.SizeFlagsVertical = Control.SizeFlags.ShrinkCenter;
            _dialoguePanel.SetSize(new Vector2(400, 300));
            _dialoguePanel.Visible = false;
            AddChild(_dialoguePanel);
            
            var vbox = new VBoxContainer();
            vbox.SetAnchorsPreset(Control.LayoutPreset.FullRect);
            vbox.AddThemeConstantOverride("separation", 10);
            _dialoguePanel.AddChild(vbox);
            
            _dialogueNpcName = new Label();
            _dialogueNpcName.Text = "NPC Name";
            _dialogueNpcName.AddThemeFontSizeOverride("font_size", 18);
            vbox.AddChild(_dialogueNpcName);
            
            _dialogueText = new Label();
            _dialogueText.Text = "Dialogue text...";
            _dialogueText.AutowrapMode = TextServer.AutowrapMode.Word;
            vbox.AddChild(_dialogueText);
            
            _dialogueOptions = new VBoxContainer();
            _dialogueOptions.AddThemeConstantOverride("separation", 5);
            vbox.AddChild(_dialogueOptions);
        }
        
        private void ShowDialogue(string npcName, string text, string[] options)
        {
            if (_dialogueNpcName != null) _dialogueNpcName.Text = npcName;
            if (_dialogueText != null) _dialogueText.Text = text;
            
            // Clear and add option buttons
            if (_dialogueOptions != null)
            {
                foreach (var child in _dialogueOptions.GetChildren())
                {
                    child.QueueFree();
                }
                
                for (int i = 0; i < options.Length; i++)
                {
                    var button = new Button();
                    button.Text = options[i];
                    int optionIndex = i;
                    button.Pressed += () => OnDialogueOptionSelected(optionIndex);
                    _dialogueOptions.AddChild(button);
                }
            }
            
            _dialoguePanel.Visible = true;
            GameState.Instance.SetDialogueActive(true, npcId, dialogueId);
        }
        
        private void OnDialogueOptionSelected(int optionIndex)
        {
            _dialoguePanel.Visible = false;
            GameState.Instance.SetDialogueActive(false);
            NetworkManager.Instance.SendDialogueResponse(optionIndex);
        }
        
        private void OnDialogueStart(uint npcId, uint dialogueId, string npcName, string dialogueText, string[] options)
        {
            // Handle legacy event signature
            ShowDialogue(npcName, dialogueText, options);
        }
        
        // ============================================================================
        // Quest Tracking UI
        // ============================================================================
        
        private void SetupQuestUI()
        {
            _questPanel = new Panel();
            _questPanel.Name = "QuestPanel";
            _questPanel.SetAnchorsPreset(Control.LayoutPreset.TopRight);
            _questPanel.OffsetLeft = -210;
            _questPanel.OffsetTop = 10;
            _questPanel.OffsetRight = -10;
            _questPanel.OffsetBottom = 130;
            _questPanel.Visible = false;
            AddChild(_questPanel);
            
            var vbox = new VBoxContainer();
            vbox.SetAnchorsPreset(Control.LayoutPreset.FullRect);
            vbox.AddThemeConstantOverride("separation", 5);
            _questPanel.AddChild(vbox);
            
            var title = new Label();
            title.Text = "Quests";
            title.AddThemeFontSizeOverride("font_size", 14);
            title.HorizontalAlignment = HorizontalAlignment.Center;
            vbox.AddChild(title);
            
            _questTitle = new Label();
            _questTitle.Text = "No active quests";
            _questTitle.AddThemeFontSizeOverride("font_size", 12);
            vbox.AddChild(_questTitle);
            
            _questProgress = new Label();
            _questProgress.Text = "";
            _questProgress.AddThemeFontSizeOverride("font_size", 11);
            vbox.AddChild(_questProgress);
        }
        
        private void OnQuestUpdateReceived(uint questId, uint objectiveIndex, uint current, uint required, byte status)
        {
            bool completed = (status == 2); // QuestStatus::Completed
            ShowQuest($"Quest {questId}", current, required, completed);
        }
        
        private void ShowQuest(string questName, uint current, uint required, bool completed)
        {
            if (_questPanel != null) _questPanel.Visible = true;
            if (_questTitle != null) _questTitle.Text = questName;
            if (_questProgress != null)
            {
                if (completed)
                {
                    _questProgress.Text = "COMPLETED!";
                }
                else
                {
                    _questProgress.Text = $"{current} / {required}";
                }
            }
        }
        
        // ============================================================================
        // Inventory UI  
        // ============================================================================
        
        private void SetupInventoryUI()
        {
            // Inventory UI disabled until a real inventory/gold update event is
            // available and this panel can be wired to it.
        }
    }
}
