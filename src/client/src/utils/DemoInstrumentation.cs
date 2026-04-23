using Godot;
using System;
using System.IO;
using System.Text.Json;
using System.Collections.Generic;
using DarkAges.Networking;

namespace DarkAges.Utils
{
    /// <summary>
    /// [DEMO_INSTRUMENTATION] Real-time metrics collection for automated testing
    /// Writes structured metrics to /tmp/darkages_instrumentation.json every 0.5s
    /// </summary>
    public partial class DemoInstrumentation : Node
    {
        [Export] public float SampleIntervalSec = 0.5f;
        [Export] public string OutputPath = "/tmp/darkages_instrumentation.json";
        
        private float _timer = 0f;
        private int _frameCount = 0;
        private float _fpsAccumulator = 0f;
        private List<MetricsSample> _samples = new();
        private PredictedPlayer? _player;
        private Camera3D? _camera;
        
        public override void _Ready()
        {
            GD.Print("[DemoInstrumentation] Initialized — writing to " + OutputPath);
        }
        
        public override void _Process(double delta)
        {
            float dt = (float)delta;
            _timer += dt;
            _frameCount++;
            _fpsAccumulator += 1.0f / dt;
            
            // Find player and camera if not already cached
            if (_player == null || !IsInstanceValid(_player))
            {
                _player = GetTree().CurrentScene?.GetNode<PredictedPlayer>("Players/Player");
            }
            if (_camera == null || !IsInstanceValid(_camera))
            {
                _camera = _player?.GetNode<Camera3D>("CameraRig/SpringArm3D/Camera3D");
            }
            
            if (_timer >= SampleIntervalSec)
            {
                RecordSample();
                _timer = 0f;
            }
        }
        
        private void RecordSample()
        {
            var sample = new MetricsSample
            {
                TimestampSec = Time.GetTicksMsec() / 1000.0,
                ConnectionState = GameState.Instance.CurrentConnectionState.ToString(),
                LocalEntityId = GameState.Instance.LocalEntityId,
                ServerTick = GameState.Instance.ServerTick,
                EntityCount = GameState.Instance.Entities.Count,
                Fps = _frameCount > 0 ? _fpsAccumulator / _frameCount : 0f,
                SnapshotsReceived = GameState.Instance.ServerTick,
            };
            
            if (_player != null && IsInstanceValid(_player))
            {
                sample.PlayerPosition = new float[] { _player.GlobalPosition.X, _player.GlobalPosition.Y, _player.GlobalPosition.Z };
                sample.PlayerVelocity = new float[] { _player.Velocity.X, _player.Velocity.Y, _player.Velocity.Z };
                sample.IsOnFloor = _player.IsOnFloor();
                sample.PredictionError = _player.Get("_predictionError").AsSingle();
                sample.LastProcessedServerInput = _player.Get("_lastProcessedServerInput").AsUInt32();
            }
            
            if (_camera != null && IsInstanceValid(_camera))
            {
                sample.CameraPosition = new float[] { _camera.GlobalPosition.X, _camera.GlobalPosition.Y, _camera.GlobalPosition.Z };
                sample.CameraLookAt = new float[] { _player?.GlobalPosition.X ?? 0, _player?.GlobalPosition.Y ?? 0, _player?.GlobalPosition.Z ?? 0 };
            }
            
            _samples.Add(sample);
            _frameCount = 0;
            _fpsAccumulator = 0f;
            
            // Flush to disk every sample for real-time monitoring
            FlushToDisk();
        }
        
        private void FlushToDisk()
        {
            try
            {
                var report = new InstrumentationReport
                {
                    SessionStart = _samples.Count > 0 ? _samples[0].TimestampSec : 0,
                    LastUpdate = Time.GetTicksMsec() / 1000.0,
                    SampleCount = _samples.Count,
                    Samples = _samples.ToArray()
                };
                
                string json = JsonSerializer.Serialize(report, new JsonSerializerOptions { WriteIndented = true });
                File.WriteAllText(OutputPath, json);
            }
            catch (Exception ex)
            {
                GD.PrintErr($"[DemoInstrumentation] Failed to write metrics: {ex.Message}");
            }
        }
        
        public override void _ExitTree()
        {
            FlushToDisk();
            GD.Print($"[DemoInstrumentation] Session complete. {_samples.Count} samples written to {OutputPath}");
        }
        
        // ── Data structures ───────────────────────────────────────────────
        public struct MetricsSample
        {
            public double TimestampSec { get; set; }
            public string ConnectionState { get; set; }
            public uint LocalEntityId { get; set; }
            public uint ServerTick { get; set; }
            public int EntityCount { get; set; }
            public float Fps { get; set; }
            public uint SnapshotsReceived { get; set; }
            public float[] PlayerPosition { get; set; }
            public float[] PlayerVelocity { get; set; }
            public bool IsOnFloor { get; set; }
            public float PredictionError { get; set; }
            public uint LastProcessedServerInput { get; set; }
            public float[] CameraPosition { get; set; }
            public float[] CameraLookAt { get; set; }
        }
        
        public struct InstrumentationReport
        {
            public double SessionStart { get; set; }
            public double LastUpdate { get; set; }
            public int SampleCount { get; set; }
            public MetricsSample[] Samples { get; set; }
        }
    }
}
