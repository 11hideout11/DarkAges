using Godot;
using System;
using System.IO;
using System.Text.Json;
using System.Collections.Generic;
using DarkAges.Networking;
using DarkAges.Entities;
using DarkAges;

namespace DarkAges.Utils
{
    /// <summary>
    /// [INSTRUMENTATION] Comprehensive client state recorder.
    /// Captures entity states, local player data, and network info at 10 Hz.
    /// Writes per-tick JSON snapshots when DARKAGES_INSTRUMENT=1.
    /// </summary>
    public partial class DemoInstrumentation : Node
    {
        [Export] public bool Enabled = false;
        [Export] public float ExportIntervalSec = 0.1f;
        [Export] public string OutputDir = "/tmp/darkages_snapshots/client";

        private float _timer = 0f;
        private int _tickCount = 0;
        private bool _initialized = false;

        private PredictedPlayer? _player;
        private RemotePlayerManager? _remoteManager;
        private NetworkManager? _network;

        private static readonly JsonSerializerOptions JsonOpts = new JsonSerializerOptions
        {
            WriteIndented = false,
            DefaultIgnoreCondition = System.Text.Json.Serialization.JsonIgnoreCondition.WhenWritingNull
        };

        public override void _Ready()
        {
            if (!Enabled)
            {
                var env = System.Environment.GetEnvironmentVariable("DARKAGES_INSTRUMENT");
                if (env == "1" || env == "true") Enabled = true;
            }

            if (!Enabled) return;

            Directory.CreateDirectory(OutputDir);

            var scene = GetTree().CurrentScene;
            if (scene == null) { GD.PrintErr("[Instrument] No current scene"); return; }

            _player = scene.GetNodeOrNull<PredictedPlayer>("Players/Player");
            _remoteManager = scene.GetNodeOrNull<RemotePlayerManager>("RemotePlayerManager");
            _network = scene.GetNodeOrNull<NetworkManager>("NetworkManager");

            GD.Print($"[ClientInstrument] Enabled → {OutputDir} @ {(1.0/ExportIntervalSec):F1}Hz");
            _initialized = true;
        }

        public override void _Process(double delta)
        {
            if (!Enabled || !_initialized) return;

            _timer += (float)delta;
            if (_timer >= ExportIntervalSec)
            {
                _timer = 0f;
                CaptureAndExport();
            }
        }

        private void CaptureAndExport()
        {
            try
            {
                var snapshot = new Dictionary<string, object>
                {
                    ["tick"] = _tickCount++,
                    ["timestamp_ms"] = (long)(Time.GetTicksMsec() * 10000),
                    ["server_tick"] = GameState.Instance.ServerTick,
                    ["entities"] = new List<object>(),
                    ["player_count"] = 0,
                    ["npc_count"] = 0
                };

                // Iterate over GameState.Entities
                foreach (var kvp in GameState.Instance.Entities)
                {
                    uint eid = kvp.Key;
                    DarkAges.EntityData entity = kvp.Value;
                    var entDict = new Dictionary<string, object>
                    {
                        ["entity_id"] = (int)eid,
                        ["position"] = new float[] { entity.Position.X, entity.Position.Y, entity.Position.Z },
                        ["velocity"] = new float[] { entity.Velocity.X, entity.Velocity.Y, entity.Velocity.Z },
                        ["health"] = (int)(entity.HealthPercent * 100),
                        ["state"] = (int)entity.AnimState
                    };
                    ((List<object>)snapshot["entities"]).Add(entDict);

                    if (entity.Type == 1) snapshot["player_count"] = (int)snapshot["player_count"] + 1;
                    else snapshot["npc_count"] = (int)snapshot["npc_count"] + 1;
                }

                // Local player details
                if (_player != null && IsInstanceValid(_player))
                {
                    var localEntity = GameState.Instance.GetEntity(GameState.Instance.LocalEntityId);
                    float health = localEntity?.HealthPercent ?? 0;
                    var local = new Dictionary<string, object>
                    {
                        ["entity_id"] = (int)GameState.Instance.LocalEntityId,
                        ["position"] = new float[] { _player.GlobalPosition.X, _player.GlobalPosition.Y, _player.GlobalPosition.Z },
                        ["velocity"] = new float[] { _player.Velocity.X, _player.Velocity.Y, _player.Velocity.Z },
                        ["health"] = (int)(health * 100),
                        ["state"] = 0
                    };
                    ((List<object>)snapshot["entities"]).Add(local);
                }

                string json = JsonSerializer.Serialize(snapshot, JsonOpts);
                string filename = Path.Combine(OutputDir, $"client_{_tickCount:012d}.json");
                File.WriteAllText(filename, json);
            }
            catch (Exception ex)
            {
                GD.PrintErr($"[Instrument] Capture error: {ex.Message}");
            }
        }

        public override void _ExitTree()
        {
            GD.Print($"[ClientInstrument] Session complete. {_tickCount} ticks → {OutputDir}");
        }
    }
}
