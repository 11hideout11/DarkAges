# Godot Networking - Quick Reference (4.2)

**NOTE**: DarkAges uses custom UDP + Protobuf. Do NOT use Godot's high-level networking for game logic.

Godot's networking is designed for peer-to-peer or client-hosted games.

- **ENetMultiplayerPeer**: P2P via ENet protocol (not authoritative)
- **MultiplayerAPI**: SceneMultiplayer (RPC on nodes), MultiplayerSynchronizer (4.2 new?)
- These are unsuitable for MMO authoritative server architecture.

For DarkAges client:
- Use `UdpClient` (C# System.Net.Sockets) for custom UDP communication
- Or wrap Godot's `PacketPeerUDP` but that's lower-level

See: https://docs.godotengine.org/en/4.2/tutorials/networking/
