using Godot;
using System;
using DarkAges.Networking;
using DarkAges.Entities;
using DarkAges.Client.Utils;

namespace DarkAges
{
	/// <summary>
	/// [CLIENT_AGENT] Main game scene controller
	/// Manages world loading, player spawning, and entity interpolation
	/// </summary>
	public partial class Main : Node3D
	{
		[Export] public PackedScene? PlayerScene;
		[Export] public PackedScene? RemotePlayerScene;
		
		private Node3D? _playersContainer;
		private PredictedPlayer? _localPlayer;
		private RemotePlayerManager? _remotePlayerManager;
		private int _snapshotCount = 0;
		
		// Demo auto-combat state
		private double _autoAttackTimer = 0.0;
		private const double AUTO_ATTACK_INTERVAL = 0.8;
		private bool _autoAttackEnabled = false;
		
		// Demo auto-movement state
		private bool _autoMoveEnabled = false;
		private double _movePhaseTimer = 0.0;
		private int _movePhase = 0;
		private const double MOVE_PHASE_DURATION = 3.0;
		private double _lookTimer = 0.0;
		private float _lookYaw = 0.0f;

		public override void _Ready()
		{
			GD.Print("[Main] Game starting...");
			
			// Parse CLI args for bot mode
			var args = OS.GetCmdlineUserArgs();
			// DEBUG: print all args
			GD.Print($"[Main] Cmdline args count={args.Length}");
			foreach (var a in args) GD.Print($"[Main] Arg: {a}");
			foreach (var arg in args)
			{
				if (arg == "--bot-mode")
				{
					_autoAttackEnabled = true;
					_autoMoveEnabled = true;
					GD.Print("[Main] Bot mode enabled (--bot-mode)");
					break;
				}
				// Auto-enable bot mode for demo runs
				if (arg == "--demo-duration")
				{
					_autoAttackEnabled = true;
					_autoMoveEnabled = true;
					GD.Print("[Main] Demo mode detected — auto-combat enabled");
					// no break: continue in case also --bot-mode explicitly
				}
			}
			
			_playersContainer = GetNode<Node3D>("Players");
			
			// Setup RemotePlayerManager for handling remote players
			_remotePlayerManager = new RemotePlayerManager();
			_remotePlayerManager.RemotePlayerScene = RemotePlayerScene;
			_playersContainer.AddChild(_remotePlayerManager);
			
			// Connect to network events
			if (NetworkManager.Instance != null)
			{
				NetworkManager.Instance.SnapshotReceived += OnSnapshotReceived;
				NetworkManager.Instance.ConnectionResult += OnConnectionResult;
			}
			
			// Connect to game state events
			GameState.Instance.EntitySpawned += OnEntitySpawned;
			GameState.Instance.EntityDespawned += OnEntityDespawned;
			
			// Find local player
			_localPlayer = GetNode<PredictedPlayer>("Players/Player");
		}

		public override void _ExitTree()
		{
			// Release any held inputs on exit
			ReleaseAllMovementInputs();
			
			if (NetworkManager.Instance != null)
			{
				NetworkManager.Instance.SnapshotReceived -= OnSnapshotReceived;
				NetworkManager.Instance.ConnectionResult -= OnConnectionResult;
			}
			
			GameState.Instance.EntitySpawned -= OnEntitySpawned;
			GameState.Instance.EntityDespawned -= OnEntityDespawned;
			
			_remotePlayerManager?.ClearAll();
		}

		public override void _Process(double delta)
		{
			if (GameState.Instance.CurrentConnectionState != GameState.ConnectionState.Connected)
				return;
			
			// Demo auto-combat: periodically send attack inputs
			if (_autoAttackEnabled)
			{
				_autoAttackTimer += delta;
				if (_autoAttackTimer >= AUTO_ATTACK_INTERVAL)
				{
					_autoAttackTimer = 0.0;
					SendAutoAttack();
				}
			}
			
			// Demo auto-movement: walk in a pattern
			if (_autoMoveEnabled)
			{
				UpdateAutoMovement(delta);
			}
		}
		
		private void UpdateAutoMovement(double delta)
		{
			_movePhaseTimer += delta;
			_lookTimer += delta;
			
			// Cycle through movement phases every MOVE_PHASE_DURATION seconds
			if (_movePhaseTimer >= MOVE_PHASE_DURATION)
			{
				_movePhaseTimer = 0.0;
				_movePhase = (_movePhase + 1) % 8;
				GD.Print($"[Main] Auto-move phase {_movePhase}");
			}
			
			// Release all inputs first, then press the current phase's inputs
			ReleaseAllMovementInputs();
			
			switch (_movePhase)
			{
				case 0: // Forward
					Input.ActionPress("move_forward");
					break;
				case 1: // Forward + Right (curve)
					Input.ActionPress("move_forward");
					Input.ActionPress("move_right");
					break;
				case 2: // Right
					Input.ActionPress("move_right");
					break;
				case 3: // Backward + Right
					Input.ActionPress("move_backward");
					Input.ActionPress("move_right");
					break;
				case 4: // Backward
					Input.ActionPress("move_backward");
					break;
				case 5: // Backward + Left
					Input.ActionPress("move_backward");
					Input.ActionPress("move_left");
					break;
				case 6: // Left
					Input.ActionPress("move_left");
					break;
				case 7: // Forward + Left
					Input.ActionPress("move_forward");
					Input.ActionPress("move_left");
					break;
			}
			
			// Slowly rotate camera to look around
			if (_lookTimer >= 0.05)
			{
				_lookTimer = 0.0;
				_lookYaw += 0.02f;
				// Inject mouse motion for camera rotation
				var mouseMotion = new InputEventMouseMotion();
				mouseMotion.Relative = new Vector2(Mathf.Sin(_lookYaw) * 2.0f, 0);
				Input.ParseInputEvent(mouseMotion);
			}
		}
		
		private void ReleaseAllMovementInputs()
		{
			Input.ActionRelease("move_forward");
			Input.ActionRelease("move_backward");
			Input.ActionRelease("move_left");
			Input.ActionRelease("move_right");
		}
		
		private void SendAutoAttack()
		{
			// Inject attack input via Godot's input system
			Input.ActionPress("attack");
			
			// Release after a short delay
			var timer = new Godot.Timer();
			timer.WaitTime = 0.1;
			timer.OneShot = true;
			timer.Timeout += () =>
			{
				Input.ActionRelease("attack");
				timer.QueueFree();
			};
			CallDeferred("add_child", timer);
			
			GD.Print("[Main] Auto-attack sent");
		}

		private void OnConnectionResult(bool success, string error)
		{
			if (success)
			{
				GD.Print("[Main] Connected to server!");
				
				if (!_autoAttackEnabled && !_autoMoveEnabled)
				{
					GD.Print("[Main] Human control mode — WASD to move, Mouse to look, Space to jump, Left Click to attack, Shift to sprint, Q to dodge, E to lock-on");
				}

				// [DEMO] Auto-exit after duration if --demo-duration is specified
				var args = OS.GetCmdlineUserArgs();
				for (int i = 0; i < args.Length; i++)
				{
					if (args[i] == "--demo-duration" && i + 1 < args.Length)
					{
						if (float.TryParse(args[i + 1], out float duration))
						{
							GD.Print($"[Main] Auto-exit scheduled {duration}s after connection");
							var timer = new Godot.Timer();
							timer.WaitTime = duration;
							timer.OneShot = true;
							timer.Timeout += () =>
							{
								GD.Print($"[Main] Demo duration reached. Exiting. Entities seen: {GameState.Instance.Entities.Count}, Snapshots: {_snapshotCount}");
								GetTree().Quit();
							};
							AddChild(timer);
							timer.Start();
						}
					}
				}
			}
			else
			{
				GD.PrintErr($"[Main] Connection failed: {error}");
			}
		}

		private void OnSnapshotReceived(uint serverTick, byte[] data)
		{
			_snapshotCount++;
			// Enhanced snapshot processing is now in NetworkManager
			// RemotePlayerManager receives the signal and distributes to entities
			// This handler can be used for additional game logic if needed
		}

		private void OnEntitySpawned(uint entityId, Vector3 position)
		{
			// Local player is handled separately
			if (entityId == GameState.Instance.LocalEntityId)
			{
				GD.Print($"[Main] Local player entity confirmed: {entityId}");
				return;
			}
			
			GD.Print($"[Main] Entity spawned event: {entityId} at {position}");
			// Remote player instantiation is handled by RemotePlayerManager
		}

		private void OnEntityDespawned(uint entityId)
		{
			GD.Print($"[Main] Entity despawned event: {entityId}");
			// Remote player cleanup is handled by RemotePlayerManager
		}

		public void ConnectToServer(string address, int port)
		{
			NetworkManager.Instance?.Connect(address, port);
		}
	}
}
