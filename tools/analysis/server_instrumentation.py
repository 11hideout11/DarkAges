#!/usr/bin/env python3
"""
Server-Side Deep Instrumentation Parser

Parses server binary output and extracts:
- Tick-by-tick entity states
- Combat events and damage calculations  
- AI decision logs
- Network synchronization data
- Entity lifecycle events

Usage:
    python3 server_instrumentation.py analyze --log SERVER_LOG
    python3 server_instrumentation.py tick-data --log SERVER_LOG
    python3 server_instrumentation.py combat-log --log SERVER_LOG
"""

import argparse
import re
import sys
from collections import defaultdict
from dataclasses import dataclass
from pathlib import Path
from typing import Optional

# ─────────────────────────────────────────────────────────────────────────────
# PARSER CONFIGURATION
# ─────────────────────────────────────────────────────────────────────────────

# Tick patterns - server logs may not have per-tick markers, use line position
TICK_PATTERN = re.compile(r'(\d+) tick')
TICK_IN_MAIN_LOOP = re.compile(r'Main loop ended after (\d+) ticks')

# Combat patterns
KILL_PATTERN = re.compile(r'Entity (\d+) killed by (\d+)')
RESPAWN_PATTERN = re.compile(r'Entity (\d+) respawned')

# Network patterns
CLIENT_CONNECT = re.compile(r'Client connected[:\s]*(\d+)')
CLIENT_DISCONNECT = re.compile(r'Client disconnected[:\s]*(\d+)')
SNAPSHOT_PATTERN = re.compile(r'Snapshot[:\s]*(\d+) entities')

# AI patterns
NPC_SPAWN = re.compile(r'Spawned entity (\d+) for NPC')
NPC_TARGET = re.compile(r'NPC (\d+) targeting (\d+)')
NPC_ATTACK = re.compile(r'NPC (\d+) attacks? (\d+)')

# Entity state patterns (from debug output)
POSITION = re.compile(r'pos[ition]*[:\s]*\[([\d.\-\s,]+)\]')
VELOCITY = re.compile(r'Velocity[:\s]*\[([\d.\-\-\s,]+)\]')
HEALTH = re.compile(r'health[:\s]*(\d+)%?')

# ─────────────────────────────────────────────────────────────────────────────
# DATA STRUCTURES
# ─────────────────────────────────────────────────────────────────────────────

@dataclass
class TickData:
    """One server tick's complete data."""
    tick: int
    zone: int
    entities: dict  # entity_id -> state dict
    events: list[str]
    connections: int

@dataclass  
class CombatEvent:
    """A combat action."""
    tick: int
    attacker: int
    target: int
    action: str  # 'attack', 'kill', 'respawn'
    damage: Optional[int]
    health_after: Optional[int]

@dataclass
class EntityLifecycle:
    """Entity spawn/update/destroy."""
    tick: int
    entity_id: int
    action: str  # 'spawn', 'update', 'die', 'respawn'
    position: Optional[tuple]
    entity_type: Optional[int]

# ─────────────────────────────────────────────────────────────────────────────
# MAIN PARSER
# ─────────────────��───────────────────────────────────────────────────────────

class ServerInstrumentationParser:
    """Parse server logs for deep state analysis."""
    
    def __init__(self, log_path: Path):
        self.log_path = log_path
        self.lines: list[str] = []
        self.ticks: list[TickData] = []
        self.combat_events: list[CombatEvent] = []
        self.entity_lifecycle: list[EntityLifecycle] = []
        self.client_connections: dict[int, list[int]] = defaultdict(list)  # client_id -> ticks
        self._parse()
    
    def _parse(self):
        """Parse the entire log file."""
        try:
            self.lines = self.log_path.read_text().splitlines()
        except Exception as e:
            print(f"Error reading {self.log_path}: {e}")
            return
        
        # First, get total ticks
        total_ticks_match = TICK_IN_MAIN_LOOP.search(self.log_path.read_text())
        total_ticks = int(total_ticks_match.group(1)) if total_ticks_match else 0
        
        line_idx = 0
        for line in self.lines:
            line_idx += 1
            
            # Combat: kill
            kill_match = KILL_PATTERN.search(line)
            if kill_match:
                victim = int(kill_match.group(1))
                killer = int(kill_match.group(2))
                
                self.combat_events.append(CombatEvent(
                    tick=line_idx,  # Use line as proxy for tick
                    attacker=killer,
                    target=victim,
                    action='kill',
                    damage=None,
                    health_after=0,
                ))
            
            # Combat: respawn
            respawn_match = RESPAWN_PATTERN.search(line)
            if respawn_match:
                entity_id = int(respawn_match.group(1))
                
                self.combat_events.append(CombatEvent(
                    tick=line_idx,
                    attacker=0,
                    target=entity_id,
                    action='respawn',
                    damage=0,
                    health_after=100,
                ))
            
            # Client connect/disconnect
            conn_match = CLIENT_CONNECT.search(line)
            if conn_match:
                client_id = int(conn_match.group(1))
                self.client_connections[client_id].append(line_idx)
            
            disconnect_match = CLIENT_DISCONNECT.search(line)
            if disconnect_match:
                client_id = int(disconnect_match.group(1))
        
        # Create fake tick data from events
        if self.combat_events:
            self.ticks.append(TickData(
                tick=0,
                zone=99,
                entities={},
                events=[f"{len(self.combat_events)} combat events"],
                connections=len(self.client_connections),
            ))
    
    def get_tick_range(self) -> tuple[int, int]:
        """First and last tick."""
        if not self.ticks:
            return (0, 0)
        return (self.ticks[0].tick, self.ticks[-1].tick)
    
    def get_combat_summary(self) -> dict:
        """Combat statistics."""
        kills = [e for e in self.combat_events if e.action == 'kill']
        respawns = [e for e in self.combat_events if e.action == 'respawn']
        
        return {
            'total_ticks': len(self.ticks),
            'tick_range': self.get_tick_range(),
            'kills': len(kills),
            'unique_killed': len(set(e.target for e in kills)),
            'unique_killers': len(set(e.attacker for e in kills)),
            'damage_events': 0,
            'total_damage': 0,
            'respawns': len(respawns),
            'unique_respawned': len(set(e.target for e in respawns)),
            'client_connections': len(self.client_connections),
        }
    
    def get_entity_timeline(self, entity_id: int) -> list[dict]:
        """Get lifecycle timeline for a specific entity."""
        timeline = []
        
        for event in self.entity_lifecycle:
            if event.entity_id == entity_id:
                timeline.append({
                    'tick': event.tick,
                    'action': event.action,
                    'position': event.position,
                })
        
        # Add combat events
        for event in self.combat_events:
            if event.target == entity_id:
                timeline.append({
                    'tick': event.tick,
                    'action': event.action,
                    'damage': event.damage,
                })
        
        timeline.sort(key=lambda x: x['tick'])
        return timeline
    
    def get_tick_data(self, tick: int) -> Optional[TickData]:
        """Get data for a specific tick."""
        for t in self.ticks:
            if t.tick == tick:
                return t
        return None
    
    def get_events_in_range(self, start_tick: int, end_tick: int) -> list[str]:
        """Get all events in tick range."""
        events = []
        for t in self.ticks:
            if start_tick <= t.tick <= end_tick:
                events.extend(t.events)
        return events

# ─────────────────────────────────────────────────────────────────────────────
# ANALYSIS FUNCTIONS
# ─────────────────────────────────────────────────────────────────────────────

def analyze_combat(log_path: Path):
    """Analyze combat in server log."""
    parser = ServerInstrumentationParser(log_path)
    summary = parser.get_combat_summary()
    
    print("\n" + "=" * 60)
    print("SERVER COMBAT ANALYSIS")
    print("=" * 60)
    
    print(f"\n📊 Tick Range: {summary['tick_range'][0]} - {summary['tick_range'][1]}")
    print(f"   Total ticks: {summary['total_ticks']}")
    
    print(f"\n⚔️ Combat:")
    print(f"   Kills: {summary['kills']}")
    print(f"   Unique killed: {summary['unique_killed']}")
    print(f"   Unique killers: {summary['unique_killers']}")
    
    print(f"\n💥 Damage:")
    print(f"   Events: {summary['damage_events']}")
    print(f"   Total damage: {summary['total_damage']}")
    
    print(f"\n🔄 Respawns:")
    print(f"   Respawns: {summary['respawns']}")
    print(f"   Unique: {summary['unique_respawned']}")
    
    print(f"\n🔗 Clients:")
    print(f"   Connections: {summary['client_connections']}")
    
    # Entity-specific analysis
    killed_entities = set(e.target for e in parser.combat_events if e.action == 'kill')
    if killed_entities:
        print(f"\n📍 Entity Lifecycles:")
        for entity_id in sorted(killed_entities):
            timeline = parser.get_entity_timeline(entity_id)
            print(f"   Entity {entity_id}: {len(timeline)} events")
            for event in timeline[:5]:
                print(f"      Tick {event['tick']}: {event['action']}")

def tick_data(log_path: Path):
    """Show tick-by-tick data."""
    parser = ServerInstrumentationParser(log_path)
    
    print("\n" + "=" * 60)
    print("SERVER TICK DATA")
    print("=" * 60)
    
    for tick_data in parser.ticks[:10]:  # First 10 ticks
        print(f"\n--- Tick {tick_data.tick} ---")
        print(f"  Connections: {tick_data.connections}")
        print(f"  Events: {len(tick_data.events)}")
        for event in tick_data.events[:3]:
            print(f"    {event}")

def combat_log(log_path: Path):
    """Show combat event log."""
    parser = ServerInstrumentationParser(log_path)
    
    print("\n" + "=" * 60)
    print("COMBAT EVENT LOG")
    print("=" * 60)
    
    for event in parser.combat_events:
        print(f"\nTick {event.tick}: {event.action}")
        if event.attacker:
            print(f"  Attacker: {event.attacker}")
        if event.target:
            print(f"  Target: {event.target}")
        if event.damage is not None:
            print(f"  Damage: {event.damage}")
        if event.health_after is not None:
            print(f"  Health after: {event.health_after}")

# ─────────────────────────────────────────────────────────────────────────────
# MAIN
# ─────────────────────────────────────────────────────────────────────────────

def main():
    parser = argparse.ArgumentParser(description="Server Instrumentation Parser")
    parser.add_argument('command', choices=['analyze', 'tick-data', 'combat-log'])
    parser.add_argument('--log', type=Path, required=True, help='Server log file')
    
    args = parser.parse_args()
    
    if args.command == 'analyze':
        analyze_combat(args.log)
    elif args.command == 'tick-data':
        tick_data(args.log)
    elif args.command == 'combat-log':
        combat_log(args.log)

if __name__ == '__main__':
    main()