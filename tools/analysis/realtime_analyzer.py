#!/usr/bin/env python3
"""
Real-Time Game State Analyzer

Comprehensive instrumentation and analysis pipeline for DarkAges that collects,
correlates, and analyzes data from:

1. GODOT CLIENT (source-level + render):
   - Node hierarchy and signals
   - Physics state (velocity, position, collision)
   - Animation parameters and state machine transitions
   - Script execution logs
   - Network messages sent/received
   - Render frame metadata

2. SERVER (world-state):
   - Entity positions, velocities, states
   - Combat events
   - AI decision logs
   - Network synchronization
   - Tick-by-tick world snapshots

3. CORRELATION:
   - Unified timestamp matching across client/server
   - Event causality chains
   - State reconciliation analysis

Usage:
    python3 realtime_analyzer.py --help
    python3 realtime_analyzer.py --watch          # Real-time monitoring
    python3 realtime_analyzer.py --analyze   # Post-run analysis
    python3 realtime_analyzer.py --validate # Hypothesis testing
"""

import argparse
import asyncio
import json
import os
import re
import sys
import time
from collections import defaultdict
from dataclasses import dataclass, field
from datetime import datetime
from pathlib import Path
from typing import Any, Optional

# Configuration
PROJECT_ROOT = Path("/root/projects/DarkAges")
ARTIFACTS = PROJECT_ROOT / "tools/demo/artifacts"
ANALYSIS_DIR = PROJECT_ROOT / "tools/analysis"
CLIENT_LOGS = ARTIFACTS / "logs"
SERVER_LOGS = ARTIFACTS / "logs"

# ─────────────────────────────────────────────────────────────────────────────
# DATA STRUCTURES
# ─────────────────────────────────────────────────────────────────────────────

@dataclass
class ClientEvent:
    """A single client-side event with metadata."""
    timestamp: float  # Unix timestamp
    source: str      # Component name (e.g., 'NetworkManager', 'PredictedPlayer')
    event_type: str  # Event category
    data: dict      # Raw event data
    frame: int      # Game frame number
    tick: int       # Server tick (if applicable)

@dataclass
class ServerEvent:
    """A single server-side event."""
    timestamp: float  # Unix timestamp  
    tick: int        # Server tick
    source: str      # System (e.g., 'CombatSystem', 'NPCAISystem')
    event_type: str  # Event category
    data: dict      # Raw event data

@dataclass  
class CorrelatedEvent:
    """An event with both client and server context."""
    timestamp: float
    client_events: list[ClientEvent]
    server_events: list[ServerEvent]
    causality_chain: list[str]  # Ordered explanation

@dataclass
class EntityState:
    """Snapshot of a single entity's state at a point in time."""
    entity_id: int
    position: tuple[float, float, float]
    velocity: tuple[float, float, float]
    rotation: tuple[float, float, float]  # Euler angles
    state: str                      # FSM state name
    health: int
    timestamp: float
    source: str                     # 'client' or 'server'

# ─────────────────────────────────────────────────────────────────────────────
# CLIENT LOG PARSING
# ────────────────────���────────────────────────────────────────────────────────

class ClientLogParser:
    """Parse Godot client logs into structured events."""
    
    # Event patterns for different client systems
    PATTERNS = {
        'network': [
            (r'\[NetworkManager\] (\w+): (.+)', 'network'),
            (r'\[UDP\] (.+?): (.+)', 'udp'),
        ],
        'combat': [
            (r'\[CombatEvent\] (.+)', 'combat_event'),
            (r'\[NetworkManager\] CombatEvent (.+)', 'combat_event'),
            (r'\[DemoAutoCombat\] (.+)', 'auto_combat'),
            (r'\[AttackFeedback\] (.+)', 'attack_feedback'),
        ],
        'player': [
            (r'\[PredictedPlayer\] (.+)', 'player'),
            (r'\[HUDController\] (.+)', 'hud'),
        ],
        'state': [
            (r'State: (\w+)', 'fsm_state'),
            (r'Animation: (\w+)', 'animation'),
        ],
        'physics': [
            (r'Position: \[(.+)\]', 'position'),
            (r'Velocity: \[(.+)\]', 'velocity'),
            (r'Collision: (\w+)', 'collision'),
        ],
        'prediction': [
            (r'Correction: (.+)', 'correction'),
            (r'ServerGhost: \[(.+)\]', 'server_ghost'),
            (r'Prediction Error: (.+)m', 'prediction_error'),
        ],
    }
    
    def __init__(self, log_path: Path):
        self.log_path = log_path
        self.events: list[ClientEvent] = []
        self.entity_states: dict[int, list[EntityState]] = defaultdict(list)
        self._parse()
    
    def _parse_line(self, line: str, frame: int) -> Optional[ClientEvent]:
        """Parse a single log line into a ClientEvent if possible."""
        timestamp = self._extract_timestamp(line)
        
        for category, patterns in self.PATTERNS.items():
            for pattern, event_type in patterns:
                match = re.search(pattern, line)
                if match:
                    data = {'raw': line}
                    # Extract named groups if present
                    if match.groups():
                        if event_type == 'network':
                            data['action'] = match.group(1)
                            data['detail'] = match.group(2) if match.lastindex >= 2 else ''
                        elif event_type == 'combat_event':
                            data = self._parse_combat_event(match.group(1))
                        elif event_type == 'position':
                            data['coords'] = [float(x) for x in match.group(1).split(',')]
                        elif event_type == 'velocity':
                            data['coords'] = [float(x) for x in match.group(1).split(',')]
                        elif event_type == 'prediction_error':
                            data['error'] = float(match.group(1))
                        else:
                            data['detail'] = match.group(1)
                    
                    return ClientEvent(
                        timestamp=timestamp,
                        source=category,
                        event_type=event_type,
                        data=data,
                        frame=frame,
                        tick=0  # Will be correlated later
                    )
        return None
    
    def _parse_combat_event(self, detail: str) -> dict:
        """Parse combat event detail into structured data."""
        data = {'detail': detail}
        
        # Pattern: attacker=10 target=11 dmg=20 hp=99%
        if 'attacker=' in detail:
            for part in detail.split():
                if '=' in part:
                    key, val = part.split('=')
                    # Remove trailing % from hp
                    if val.endswith('%'):
                        val = val[:-1]
                    try:
                        data[key] = int(val)
                    except:
                        data[key] = val
        return data
    
    def _extract_timestamp(self, line: str) -> float:
        """Extract timestamp from log line or return current time."""
        # Godot logs often don't have timestamps, use file position as proxy
        return time.time()
    
    def _parse(self):
        """Parse entire log file."""
        frame = 0
        try:
            content = self.log_path.read_text()
            for line in content.splitlines():
                event = self._parse_line(line, frame)
                if event:
                    self.events.append(event)
                    self._extract_entity_states(event)
                frame += 1
        except Exception as e:
            print(f"Error parsing {self.log_path}: {e}")
    
    def _extract_entity_states(self, event: ClientEvent):
        """Extract entity state updates from events."""
        # This would need more sophisticated parsing
        pass
    
    def get_events_by_type(self, event_type: str) -> list[ClientEvent]:
        """Get all events of a specific type."""
        return [e for e in self.events if e.event_type == event_type]
    
    def get_combat_events(self) -> list[dict]:
        """Get all combat events as structured data."""
        events = []
        for e in self.events:
            if e.event_type == 'combat_event':
                events.append({
                    'timestamp': e.timestamp,
                    'attacker': e.data.get('attacker'),
                    'target': e.data.get('target'),
                    'damage': e.data.get('dmg'),
                    'health_pct': e.data.get('hp'),
                    'raw': e.data.get('detail', ''),
                })
        return events
    
    def get_network_traffic(self) -> dict:
        """Get network message counts by type."""
        traffic = defaultdict(int)
        for e in self.events:
            if e.source == 'network':
                traffic[e.data.get('action', 'unknown')] += 1
        return dict(traffic)
    
    def get_prediction_stats(self) -> dict:
        """Get client-side prediction statistics."""
        errors = []
        for e in self.events:
            if e.event_type == 'prediction_error':
                errors.append(e.data.get('error', 0))
        
        return {
            'max_error': max(errors) if errors else 0,
            'avg_error': sum(errors)/len(errors) if errors else 0,
            'samples': len(errors),
        }

# ─────────────────────────────────────────────────────────────────────────────
# SERVER LOG PARSING  
# ─────────────────────────────────────────────────────────────────────────────

class ServerLogParser:
    """Parse server logs into structured events."""
    
    # Key server events
    SERVER_PATTERNS = {
        'combat': [
            (r'Entity (\d+) (.+?) by (\d+)', 'kill'),
            (r'combat (.+)', 'combat'),
            (r'hit (.+)', 'hit'),
            (r'damage (.+)', 'damage'),
        ],
        'network': [
            (r'Client connected: (\d+)', 'client_connect'),
            (r'Snapshot: (.+)', 'snapshot'),
            (r'Sent hit confirmation: (\d+)', 'hit_confirm'),
        ],
        'ai': [
            (r'NPC (\d+) (.+)', 'npc_action'),
        ],
        'entity': [
            (r'Spawned entity (\d+)', 'spawn'),
            (r'Entity (\d+) (.+)', 'entity_update'),
        ],
    }
    
    def __init__(self, log_path: Path):
        self.log_path = log_path
        self.events: list[ServerEvent] = []
        self.entity_states: dict[int, list[EntityState]] = defaultdict(list)
        self._parse()
    
    def _parse_line(self, line: str, tick: int) -> Optional[ServerEvent]:
        """Parse a server log line."""
        timestamp = time.time()  # Server logs lack timestamps
        
        for category, patterns in self.SERVER_PATTERNS.items():
            for pattern, event_type in patterns:
                match = re.search(pattern, line)
                if match:
                    data = {'raw': line}
                    
                    if event_type == 'kill':
                        data['victim'] = int(match.group(1))
                        data['killer'] = int(match.group(3))
                        data['action'] = match.group(2)
                    elif event_type == 'client_connect':
                        data['client_id'] = int(match.group(1))
                    elif event_type == 'spawn':
                        data['entity_id'] = int(match.group(1))
                    elif event_type == 'hit_confirm':
                        data['damage'] = int(match.group(1))
                    elif event_type == 'entity_update':
                        data['entity_id'] = int(match.group(1))
                        data['action'] = match.group(2)
                    
                    return ServerEvent(
                        timestamp=timestamp,
                        tick=tick,
                        source=category,
                        event_type=event_type,
                        data=data,
                    )
        return None
    
    def _parse(self):
        """Parse server log file."""
        tick = 0
        tick_pattern = re.compile(r'\[ZONE \d+\] .* tick (\d+)')
        
        try:
            content = self.log_path.read_text()
            for line in content.splitlines():
                # Track server tick
                tick_match = tick_pattern.search(line)
                if tick_match:
                    tick = int(tick_match.group(1))
                
                event = self._parse_line(line, tick)
                if event:
                    self.events.append(event)
        except Exception as e:
            print(f"Error parsing server log: {e}")
    
    def get_kill_events(self) -> list[dict]:
        """Get all kill events."""
        kills = []
        for e in self.events:
            if e.event_type == 'kill':
                kills.append({
                    'tick': e.tick,
                    'victim': e.data['victim'],
                    'killer': e.data['killer'],
                    'action': e.data['action'],
                    'raw': e.data['raw'],
                })
        return kills
    
    def get_combat_summary(self) -> dict:
        """Get combat statistics."""
        kills = self.get_kill_events()
        respawns = len([e for e in self.events if 'respawn' in e.data.get('raw', '')])
        
        return {
            'total_kills': len(kills),
            'total_respawns': respawns,
            'unique_victims': len(set(k['victim'] for k in kills)),
            'unique_killers': len(set(k['killer'] for k in kills)),
        }
    
    def get_tick_range(self) -> tuple[int, int]:
        """Get first and last tick."""
        ticks = [e.tick for e in self.events if e.tick > 0]
        return (min(ticks), max(ticks)) if ticks else (0, 0)

# ─────────────────────────────────────────────────────────────────────────────
# CORRELATION ENGINE
# ─────────────────────────────────────────────────────────────────────────────

class CorrelationEngine:
    """Correlate client and server events for coherent state analysis."""
    
    def __init__(self, client_parser: ClientLogParser, server_parser: ServerLogParser):
        self.client = client_parser
        self.server = server_parser
    
    def correlate_combat(self) -> list[CorrelatedEvent]:
        """Match combat events between client and server."""
        # For now, correlate by finding matching timestamps (file position proxy)
        correlations = []
        
        client_combat = self.client.get_combat_events()
        server_kills = self.server.get_kill_events()
        
        # Simple correlation: events that occurred
        for i, ck in enumerate(client_combat):
            for j, sk in enumerate(server_kills):
                # If attacker matches target, correlate
                if ck.get('target') == sk['victim']:
                    correlations.append(CorrelatedEvent(
                        timestamp=ck['timestamp'],
                        client_events=[],  # Would populate
                        server_events=[],
                        causality_chain=[
                            f"Client: Entity {ck.get('attacker')} attacks {ck.get('target')} for {ck.get('damage')} damage",
                            f"Server: Entity {sk['victim']} {sk['action']} by {sk['killer']}",
                            f"Result: {ck.get('health_pct')}% HP remaining",
                        ]
                    ))
        
        return correlations
    
    def get_state_timeline(self) -> list[dict]:
        """Build a unified state timeline."""
        timeline = []
        
        # Add server events
        for e in self.server.events:
            timeline.append({
                'tick': e.tick,
                'source': 'server',
                'type': e.event_type,
                'data': e.data,
            })
        
        # Add client events
        for e in self.client.events:
            timeline.append({
                'frame': e.frame,
                'source': 'client',
                'type': e.event_type,
                'data': e.data,
            })
        
        # Sort by tick/frame
        timeline.sort(key=lambda x: (x.get('tick', 0), x.get('frame', 0)))
        
        return timeline

# ─────────────────────────────────────────────────────────────────────────────
# ANALYSIS REPORTS
# ─────────────────────────────────────────────────────────────────────────────

def generate_analysis_report(client_log: Path, server_log: Path) -> dict:
    """Generate a comprehensive analysis report."""
    
    client = ClientLogParser(client_log)
    server = ServerLogParser(server_log)
    correlation = CorrelationEngine(client, server)
    
    # Get statistics
    client_combat = client.get_combat_events()
    server_combat = server.get_combat_summary()
    server_kills = server.get_kill_events()
    correlation_events = correlation.correlate_combat()
    
    # Network analysis
    network = client.get_network_traffic()
    prediction = client.get_prediction_stats()
    
    report = {
        'generated': datetime.now().isoformat(),
        'logs': {
            'client': str(client_log.name),
            'server': str(server_log.name),
        },
        'summary': {
            'duration_ticks': server.get_tick_range(),
            'total_client_events': len(client.events),
            'total_server_events': len(server.events),
        },
        'combat': {
            'client': {
                'attacks_sent': len([e for e in client.events if e.event_type == 'auto_combat' and 'sent' in e.data.get('detail', '')]),
                'hits_received': len(client_combat),
                'total_damage': sum(e.get('damage', 0) for e in client_combat),
                'unique_targets': len(set(e.get('target') for e in client_combat if e.get('target'))),
                'unique_attackers': len(set(e.get('attacker') for e in client_combat if e.get('attacker'))),
            },
            'server': server_combat,
            'correlations': len(correlation_events),
        },
        'network': {
            'client_messages': network,
            'prediction': prediction,
        },
        'timeline': correlation.get_state_timeline()[:100],  # First 100 events
    }
    
    return report

def print_report(report: dict):
    """Print analysis report in human-readable format."""
    
    print("\n" + "=" * 70)
    print("DARKAGES GAME STATE ANALYSIS REPORT")
    print("=" * 70)
    
    print(f"\n📁 Files:")
    print(f"   Client: {report['logs']['client']}")
    print(f"   Server: {report['logs']['server']}")
    
    print(f"\n📊 Summary:")
    s = report['summary']
    print(f"   Duration: {s['duration_ticks'][0]} - {s['duration_ticks'][1]} ticks")
    print(f"   Total events: {s['total_client_events']} client + {s['total_server_events']} server")
    
    print(f"\n⚔️ Combat Analysis:")
    c = report['combat']['client']
    print(f"   Client hits: {c['hits_received']}")
    print(f"   Total damage: {c['total_damage']}")
    print(f"   Unique attackers: {c['unique_attackers']}")
    print(f"   Unique targets: {c['unique_targets']}")
    
    s = report['combat']['server']
    print(f"   Server kills: {s['total_kills']}")
    print(f"   Server respawns: {s['total_respawns']}")
    
    print(f"\n🌐 Network:")
    n = report['network']
    for msg_type, count in n['client_messages'].items():
        print(f"   {msg_type}: {count}")
    
    p = n['prediction']
    print(f"   Prediction error: max={p['max_error']:.2f}m, avg={p['avg_error']:.2f}m")
    
    print(f"\n🔗 Correlations:")
    print(f"   Combat event matches: {report['combat']['correlations']}")
    
    print("\n" + "=" * 70)

# ─────────────────────────────────────────────────────────────────────────────
# COMMAND LINE INTERFACE
# ─────────────────────────────────────────────────────────────────────────────

def find_latest_logs() -> tuple[Path, Path]:
    """Find the most recent client and server logs."""
    client_logs = sorted(CLIENT_LOGS.glob("godot_*.log"), key=lambda p: p.stat().st_mtime)
    server_logs = sorted(SERVER_LOGS.glob("server_*.log"), key=lambda p: p.stat().st_mtime)
    
    if not client_logs or not server_logs:
        raise FileNotFoundError("No logs found in artifacts/logs/")
    
    return client_logs[-1], server_logs[-1]

def main():
    parser = argparse.ArgumentParser(description="DarkAges Real-Time Game State Analyzer")
    parser.add_argument('--watch', action='store_true', help='Real-time log watching')
    parser.add_argument('--analyze', action='store_true', help='Analyze latest logs')
    parser.add_argument('--validate', action='store_true', help='Validate hypothesis')
    parser.add_argument('--client', type=Path, help='Specific client log')
    parser.add_argument('--server', type=Path, help='Specific server log')
    parser.add_argument('--output', type=Path, help='Output JSON report path')
    parser.add_argument('--quiet', action='store_true', help='Suppress printed output')
    
    args = parser.parse_args()
    
    # Find logs
    client_log = args.client if args.client else find_latest_logs()[0]
    server_log = args.server if args.server else find_latest_logs()[1]
    
    if args.watch:
        print("Real-time watching not yet implemented")
        return
    
    if args.analyze or args.validate or (not args.watch and not args.validate):
        # Run analysis
        report = generate_analysis_report(client_log, server_log)
        
        # Output
        if args.output:
            args.output.write_text(json.dumps(report, indent=2))
            print(f"Report written to {args.output}")
        
        if not args.quiet:
            print_report(report)

if __name__ == '__main__':
    main()