using Godot;
using System;
using System.Collections.Generic;
using System.IO;
using System.Text.Json;
using DarkAges.Networking;

namespace DarkAges.Client.Utils
{
    /// <summary>
    /// [INSTRUMENTATION] Deep client state logging for analysis
    /// Exports structured state data for correlation with server events
    /// Writes to /tmp/darkages_client_state.json every tick
    /// </summary>
    public partial class ClientDeepInstrumentation : Node
    {
        [Export] public bool Enabled = true;
        [Export] public float ExportIntervalSec = 1.0f;  // Every second
        [Export] public string OutputPath = "/tmp/darkages_client_state.json";
        
        private float _timer = 0f;
        private int _tickCount = 0;
        
        // Cached references
        private PredictedPlayer? _player;
        private Camera3D? _camera;
        private RemotePlayerManager? _remoteManager;
        private NetworkManager? _network;
        
        // State history (last 100 ticks)
        private List<ClientStateSnapshot> _history = new();
        private const int MaxHistory = 100;
        
        public override void _Ready()
        {
            if (!Enabled) return;
            
            // Find key nodes
            _player = GetTree().CurrentScene?.GetNodeOrNull<PredictedPlayer>("Main/Players/Player");
            _camera = _player?.GetNodeOrNull<Camera3D>("CameraRig/SpringArm3D/Camera3D");
            _remoteManager = GetTree().CurrentScene?.GetNodeOrNull<RemotePlayerManager>("Main/RemotePlayerManager");
            _network = GetTree().CurrentScene?.GetNodeOrNull<NetworkManager>("Main/NetworkManager");
            
            GD.Print($"[ClientDeepInstrumentation] Enabled, output: {OutputPath}");
        }
        
        public override void _Process(double delta)
        {
            if (!Enabled) return;
            
            float dt = (float)delta;
            _timer += dt;
            _tickCount++;
            
            if (_timer >= ExportIntervalSec)
            {
                _timer = 0f;
                ExportState();
            }
        }
        
        private void ExportState()
        {
            var snapshot = new ClientStateSnapshot
            {
                Timestamp = Time.GetTicksMsec() / 1000.0,
                Tick = _tickCount,
                ConnectionState = GameState.Instance.CurrentConnectionState.ToString(),
                LocalEntityId = GameState.Instance.LocalEntityId,
                ServerTick = GameState.Instance.ServerTick,
                EntityCount = GameState.Instance.Entities.Count,
            };
            
            // Network state
            if (_network != null)
            {
                snapshot.Network = new NetworkState
                {
                    Connected = _network.IsConnected,
                    LatencyMs = _network.GetLatencyMs(),
                    SnapshotsReceived = _network.GetSnapshotsReceived(),
                    LastServerTick = _network.GetLastServerTick(),
                };
            }
            
            // Player state
            if (_player != null && IsInstanceValid(_player))
            {
                snapshot.Player = new PlayerState
                {
                    Position = ToFloatArray(_player.GlobalPosition),
                    Velocity = ToFloatArray(_player.Velocity),
                    IsOnFloor = _player.IsOnFloor(),
                    IsAttacking = _player.Get("IsAttacking").AsBool(),
                    IsBlocking = _player.Get("IsBlocking").AsBool(),
                    IsDodging = _player.Get("IsDodging").AsBool(),
                    IsDead = _player.Get("_isDead").AsBool(),
                    Health = _player.Get("Health").AsInt32(),
                    PredictionError = _player.Get("_predictionError").AsSingle(),
                };
            }
            
            // Camera state
            if (_camera != null && IsInstanceValid(_camera))
            {
                Vector3 camPos = _camera.GlobalPosition;
                Vector3 lookAt = _player?.GlobalPosition ?? Vector3.Zero;
                
                snapshot.Camera = new CameraState
                {
                    Position = ToFloatArray(camPos),
                    LookAt = ToFloatArray(lookAt),
                    Distance = camPos.DistanceTo(lookAt),
                };
            }
            
            // Remote entities
            if (_remoteManager != null)
            {
                snapshot.RemoteEntities = new List<RemoteEntityState>();
                
                foreach (var kvp in GameState.Instance.Entities)
                {
                    uint entityId = kvp.Key;
                    var entity = kvp.Value;
                    
                    if (entityId != GameState.Instance.LocalEntityId)
                    {
                        snapshot.RemoteEntities.Add(new RemoteEntityState
                        {
                            EntityId = entityId,
                            Type = entity.Type,
                            Position = ToFloatArray(entity.Position),
                            Rotation = ToFloatArray(entity.Rotation),
                            Health = entity.Health,
                            State = entity.State,
                        });
                    }
                }
            }
            
            // Add to history
            _history.Add(snapshot);
            if (_history.Count > MaxHistory)
            {
                _history.RemoveAt(0);
            }
            
            // Write to disk
            WriteStateFile(snapshot);
        }
        
        private void WriteStateFile(ClientStateSnapshot snapshot)
        {
            try
            {
                var report = new ClientStateReport
                {
                    Current = snapshot,
                    History = _history.ToArray(),
                    Summary = new ClientStateSummary
                    {
                        TotalTicks = _tickCount,
                        UniqueEntities = new HashSet<int>(snapshot.RemoteEntities.ConvertAll(e => (int)e.EntityId)).Count,
                    }
                };
                
                string json = JsonSerializer.Serialize(report, new JsonSerializerOptions { WriteIndented = false });
                File.WriteAllText(OutputPath, json);
            }
            catch (Exception ex)
            {
                GD.PrintErr($"[ClientDeepInstrumentation] Write error: {ex.Message}");
            }
        }
        
        private static float[] ToFloatArray(Vector3 v) => new float[] { v.X, v.Y, v.Z };
        
        // ── Data structures ──────────────────────────────────────────────
        
        public class ClientStateSnapshot
        {
            public double Timestamp { get; set; }
            public int Tick { get; set; }
            public string ConnectionState { get; set; } = "";
            public uint LocalEntityId { get; set; }
            public uint ServerTick { get; set; }
            public int EntityCount { get; set; }
            public NetworkState? Network { get; set; }
            public PlayerState? Player { get; set; }
            public CameraState? Camera { get; set; }
            public List<RemoteEntityState> RemoteEntities { get; set; } = new();
        }
        
        public class NetworkState
        {
            public bool Connected { get; set; }
            public float LatencyMs { get; set; }
            public int SnapshotsReceived { get; set; }
            public uint LastServerTick { get; set; }
        }
        
        public class PlayerState
        {
            public float[] Position { get; set; } = new float[3];
            public float[] Velocity { get; set; } = new float[3];
            public bool IsOnFloor { get; set; }
            public bool IsAttacking { get; set; }
            public bool IsBlocking { get; set; }
            public bool IsDodging { get; set; }
            public bool IsDead { get; set; }
            public int Health { get; set; }
            public float PredictionError { get; set; }
        }
        
        public class CameraState
        {
            public float[] Position { get; set; } = new float[3];
            public float[] LookAt { get; set; } = new float[3];
            public float Distance { get; set; }
        }
        
        public class RemoteEntityState
        {
            public uint EntityId { get; set; }
            public int Type { get; set; }
            public float[] Position { get; set; } = new float[3];
            public float[] Rotation { get; set; } = new float[3];
            public int Health { get; set; }
            public int State { get; set; }
        }
        
        public class ClientStateReport
        {
            public ClientStateSnapshot Current { get; set; } = new();
            public ClientStateSnapshot[] History { get; set; } = Array.Empty<ClientStateSnapshot>();
            public ClientStateSummary Summary { get; set; } = new();
        }
        
        public class ClientStateSummary
        {
            public int TotalTicks { get; set; }
            public int UniqueEntities { get; set; }
        }
    }
}