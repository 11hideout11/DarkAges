#!/usr/bin/env python3
"""Correlates client and server tick snapshots into a unified world model.

Reads JSON tick files from <base_dir>/client/ and <base_dir>/server/
and produces:
  - get_entity_timeline(entity_id) -> list of per-tick states
  - compute_divergence_metrics() -> dict
  - to_unified_model() -> dict (queryable JSON)
"""

import json
from pathlib import Path
from typing import Dict, List, Tuple, Optional
from dataclasses import dataclass, asdict
from datetime import datetime

@dataclass
class EntityState:
    entity_id: int
    position: Tuple[float, float, float]
    velocity: Tuple[float, float, float]
    yaw: float
    pitch: float
    health: int   # 0-100
    state: int    # placeholder
    source: str   # "server" or "client"
    tick: int
    timestamp_ms: int

class TickCorrelator:
    def __init__(self, client_dir: Path, server_dir: Path):
        self.client_dir = client_dir
        self.server_dir = server_dir
        self.client_ticks: Dict[int, dict] = {}
        self.server_ticks: Dict[int, dict] = {}

    def load_snapshots(self):
        """Load all client and server JSON tick snapshots."""
        for f in sorted(self.client_dir.glob("*.json")):
            with open(f) as fp:
                data = json.load(fp)
            tick = data.get("tick", 0)
            self.client_ticks[tick] = data

        for f in sorted(self.server_dir.glob("*.json")):
            with open(f) as fp:
                data = json.load(fp)
            tick = data.get("tick", 0)
            self.server_ticks[tick] = data

    def get_entity_timeline(self, entity_id: int) -> List[EntityState]:
        """Build continuous timeline for a given entity across ticks."""
        timeline = []
        all_ticks = sorted(set(self.server_ticks.keys()) | set(self.client_ticks.keys()))
        for tick in all_ticks:
            # Prefer server as authoritative
            if tick in self.server_ticks:
                for ent in self.server_ticks[tick].get("entities", []):
                    if ent["entity_id"] == entity_id:
                        pos = ent["position"]
                        vel = ent.get("velocity", [0.0, 0.0, 0.0])
                        timeline.append(EntityState(
                            entity_id=entity_id,
                            position=tuple(pos),
                            velocity=tuple(vel),
                            yaw=ent.get("yaw", 0.0),
                            pitch=ent.get("pitch", 0.0),
                            health=ent.get("health", 0),
                            state=ent.get("state", 0),
                            source="server",
                            tick=tick,
                            timestamp_ms=self.server_ticks[tick].get("timestamp_ms", 0)
                        ))
                        break
            elif tick in self.client_ticks:
                for ent in self.client_ticks[tick].get("entities", []):
                    if ent["entity_id"] == entity_id:
                        pos = ent.get("position", [0,0,0])
                        vel = ent.get("velocity", [0,0,0])
                        timeline.append(EntityState(
                            entity_id=entity_id,
                            position=tuple(pos),
                            velocity=tuple(vel),
                            yaw=ent.get("yaw", 0.0),
                            pitch=ent.get("pitch", 0.0),
                            health=ent.get("health", 0),
                            state=ent.get("state", 0),
                            source="client",
                            tick=tick,
                            timestamp_ms=self.client_ticks[tick].get("timestamp_ms", 0)
                        ))
                        break
        return timeline

    def compute_divergence_metrics(self) -> Dict:
        """Compute average client-server positional divergence per tick."""
        divergences = []
        matched_ticks = 0
        for tick in self.server_ticks:
            if tick not in self.client_ticks:
                continue
            server_snap = self.server_ticks[tick]
            client_snap = self.client_ticks[tick]
            server_entities = {e["entity_id"]: e for e in server_snap.get("entities", [])}
            client_entities = {e["entity_id"]: e for e in client_snap.get("entities", [])}
            per_tick_deltas = []
            for eid, s_ent in server_entities.items():
                if eid in client_entities:
                    c_ent = client_entities[eid]
                    s_pos = s_ent["position"]
                    c_pos = c_ent.get("position", [0,0,0])
                    dx = s_pos[0] - c_pos[0]
                    dy = s_pos[1] - c_pos[1]
                    dz = s_pos[2] - c_pos[2]
                    dist = (dx*dx + dy*dy + dz*dz) ** 0.5
                    per_tick_deltas.append(dist)
            if per_tick_deltas:
                divergences.append(sum(per_tick_deltas) / len(per_tick_deltas))
                matched_ticks += 1

        return {
            "tick_count": matched_ticks,
            "avg_position_delta_m": sum(divergences) / len(divergences) if divergences else 0.0,
            "max_delta_m": max(divergences) if divergences else 0.0,
        }

    def to_unified_model(self) -> Dict:
        """Export full unified timeline as a queryable model."""
        all_entities = set()
        for snap in self.server_ticks.values():
            for e in snap.get("entities", []):
                all_entities.add(e["entity_id"])
        for snap in self.client_ticks.values():
            for e in snap.get("entities", []):
                all_entities.add(e["entity_id"])

        model = {
            "generated_at": datetime.now().isoformat(),
            "total_ticks": len(set(self.server_ticks) | set(self.client_ticks)),
            "total_entities": len(all_entities),
            "entities": {},
        }

        for eid in all_entities:
            timeline = self.get_entity_timeline(eid)
            model["entities"][str(eid)] = [asdict(s) for s in timeline]

        return model
