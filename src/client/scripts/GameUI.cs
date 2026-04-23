using Godot;
using System;
using DarkAges.Networking;

namespace DarkAges
{
    /// <summary>
    /// [CLIENT_AGENT] UI controller for connection panel and debug info
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
    }
}
