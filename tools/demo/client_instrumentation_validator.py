#!/usr/bin/env python3
"""DarkAges MMO — Client Instrumentation Validator

Reads /tmp/darkages_instrumentation.json (written by DemoInstrumentation.cs)
and validates physical, visual, and network correctness criteria.

Usage:
    python3 client_instrumentation_validator.py [--json PATH] [--screenshots DIR]

Exit codes:
    0 = All criteria passed
    1 = One or more criteria failed
"""

import argparse
import json
import sys
from pathlib import Path
from dataclasses import dataclass
from typing import List, Dict, Any

DEFAULT_JSON = "/tmp/darkages_instrumentation.json"

# ── Criteria thresholds ──────────────────────────────────────────────────
CRITERIA = {
    "min_connected_samples": 5,
    "max_player_y_drift": -0.5,      # Player Y should never go below this
    "min_fps_average": 5.0,
    "min_entity_count": 3,
    "max_prediction_error": 2.0,
    "min_snapshots_received": 20,
    "max_rtt_ms": 500,
}

@dataclass
class ValidationResult:
    name: str
    passed: bool
    value: Any
    threshold: Any
    detail: str

class InstrumentationValidator:
    def __init__(self, json_path: str):
        self.json_path = Path(json_path)
        self.samples: List[Dict] = []
        self.results: List[ValidationResult] = []
        self.connected_samples: List[Dict] = []
        
    def load(self) -> bool:
        if not self.json_path.exists():
            print(f"[FAIL] Instrumentation file not found: {self.json_path}")
            return False
        try:
            with open(self.json_path) as f:
                data = json.load(f)
            self.samples = data.get("Samples", [])
            self.connected_samples = [
                s for s in self.samples
                if s.get("ConnectionState") == "Connected"
            ]
            print(f"[INFO] Loaded {len(self.samples)} samples ({len(self.connected_samples)} connected)")
            return True
        except Exception as e:
            print(f"[FAIL] Failed to parse instrumentation: {e}")
            return False
    
    def check(self, name: str, passed: bool, value, threshold, detail: str):
        self.results.append(ValidationResult(name, passed, value, threshold, detail))
        status = "PASS" if passed else "FAIL"
        print(f"[{status}] {name}: {detail}")
    
    def validate(self):
        if not self.samples:
            self.check("has_samples", False, 0, ">0", "No samples recorded")
            return
        
        # 1. Connection state
        self.check(
            "connection_established",
            len(self.connected_samples) >= CRITERIA["min_connected_samples"],
            len(self.connected_samples),
            f">={CRITERIA['min_connected_samples']}",
            f"{len(self.connected_samples)} connected samples"
        )
        
        if not self.connected_samples:
            print("[WARN] No connected samples — skipping physics/network checks")
            return
        
        # 2. Ground collision / no falling
        ys = [s["PlayerPosition"][1] for s in self.connected_samples if s.get("PlayerPosition")]
        min_y = min(ys) if ys else 0
        max_y = max(ys) if ys else 0
        avg_y = sum(ys) / len(ys) if ys else 0
        
        self.check(
            "ground_collision",
            min_y >= CRITERIA["max_player_y_drift"],
            f"min={min_y:.2f} avg={avg_y:.2f} max={max_y:.2f}",
            f"Y >= {CRITERIA['max_player_y_drift']}",
            "Player Y never fell below threshold" if min_y >= CRITERIA["max_player_y_drift"] else f"Player fell to Y={min_y:.2f}"
        )
        
        # 3. IsOnFloor consistency
        floor_samples = [s for s in self.connected_samples if s.get("IsOnFloor") is not None]
        if floor_samples:
            floor_ratio = sum(1 for s in floor_samples if s["IsOnFloor"]) / len(floor_samples)
            self.check(
                "is_on_floor",
                floor_ratio >= 0.8,
                f"{floor_ratio*100:.0f}%",
                ">=80%",
                f"IsOnFloor true for {floor_ratio*100:.0f}% of samples"
            )
        
        # 4. FPS performance
        fps_values = [s["Fps"] for s in self.connected_samples if s.get("Fps", 0) > 0]
        if fps_values:
            avg_fps = sum(fps_values) / len(fps_values)
            min_fps = min(fps_values)
            self.check(
                "fps_average",
                avg_fps >= CRITERIA["min_fps_average"],
                f"avg={avg_fps:.1f} min={min_fps:.1f}",
                f">={CRITERIA['min_fps_average']}",
                f"Average FPS: {avg_fps:.1f}"
            )
        
        # 5. Entity visibility (NPCs)
        entity_counts = [s["EntityCount"] for s in self.connected_samples]
        max_entities = max(entity_counts) if entity_counts else 0
        avg_entities = sum(entity_counts) / len(entity_counts) if entity_counts else 0
        
        self.check(
            "entity_visibility",
            max_entities >= CRITERIA["min_entity_count"],
            f"max={max_entities} avg={avg_entities:.1f}",
            f">={CRITERIA['min_entity_count']}",
            f"Peak entities visible: {max_entities}"
        )
        
        # 6. Prediction error
        errors = [s["PredictionError"] for s in self.connected_samples if s.get("PredictionError") is not None]
        if errors:
            avg_err = sum(errors) / len(errors)
            max_err = max(errors)
            self.check(
                "prediction_error",
                max_err <= CRITERIA["max_prediction_error"],
                f"avg={avg_err:.3f}m max={max_err:.3f}m",
                f"<={CRITERIA['max_prediction_error']}m",
                f"Max prediction error: {max_err:.3f}m"
            )
        
        # 7. Snapshot reception
        ticks = [s["ServerTick"] for s in self.connected_samples if s.get("ServerTick")]
        if ticks:
            snapshot_delta = max(ticks) - min(ticks)
            self.check(
                "snapshot_reception",
                snapshot_delta >= CRITERIA["min_snapshots_received"],
                snapshot_delta,
                f">={CRITERIA['min_snapshots_received']}",
                f"Server tick delta: {snapshot_delta}"
            )
        
        # 8. Camera tracking (camera should be near player)
        cam_samples = [s for s in self.connected_samples if s.get("CameraPosition") and s.get("PlayerPosition")]
        if cam_samples:
            distances = []
            for s in cam_samples:
                cx, cy, cz = s["CameraPosition"]
                px, py, pz = s["PlayerPosition"]
                dist = ((cx-px)**2 + (cy-py)**2 + (cz-pz)**2) ** 0.5
                distances.append(dist)
            avg_dist = sum(distances) / len(distances)
            self.check(
                "camera_tracking",
                1.0 <= avg_dist <= 10.0,
                f"avg_dist={avg_dist:.2f}m",
                "1-10m",
                f"Camera avg distance from player: {avg_dist:.2f}m"
            )
    
    def summary(self) -> bool:
        passed = sum(1 for r in self.results if r.passed)
        total = len(self.results)
        print(f"\n{'='*60}")
        print(f"VALIDATION SUMMARY: {passed}/{total} checks passed")
        print(f"{'='*60}")
        for r in self.results:
            icon = "✅" if r.passed else "❌"
            print(f"{icon} {r.name}: {r.detail}")
        return passed == total
    
    def write_report(self, path: str):
        report = {
            "timestamp": str(Path(path).stat().st_mtime) if Path(path).exists() else None,
            "samples_total": len(self.samples),
            "samples_connected": len(self.connected_samples),
            "criteria": CRITERIA,
            "results": [
                {
                    "name": r.name,
                    "passed": r.passed,
                    "value": str(r.value),
                    "threshold": str(r.threshold),
                    "detail": r.detail
                }
                for r in self.results
            ],
            "overall_pass": all(r.passed for r in self.results)
        }
        Path(path).write_text(json.dumps(report, indent=2))
        print(f"[INFO] Report written to {path}")

def main():
    parser = argparse.ArgumentParser(description="DarkAges Client Instrumentation Validator")
    parser.add_argument("--json", default=DEFAULT_JSON, help="Path to instrumentation JSON")
    parser.add_argument("--report", default="/tmp/darkages_instrumentation_report.json", help="Output report path")
    args = parser.parse_args()
    
    validator = InstrumentationValidator(args.json)
    if not validator.load():
        sys.exit(1)
    
    validator.validate()
    ok = validator.summary()
    validator.write_report(args.report)
    sys.exit(0 if ok else 1)

if __name__ == "__main__":
    main()
