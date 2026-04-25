#!/usr/bin/env python3
"""
Unified Analysis - Combines client and server analysis into coherent model.

Usage:
    python3 unified_analysis.py --log-dir /path/to/artifacts/logs
    python3 unified_analysis.py --watch            # Watch for new logs
    python3 unified_analysis.py --validate     # Run validation tests
"""

import argparse
import json
import sys
import time
from dataclasses import asdict
from datetime import datetime
from pathlib import Path
from typing import Optional

# Add parent to path for imports
sys.path.insert(0, str(Path(__file__).parent))

from realtime_analyzer import (
    ClientLogParser, ServerLogParser, 
    CorrelationEngine, generate_analysis_report
)
from server_instrumentation import ServerInstrumentationParser
from tick_correlator import TickCorrelator

# ─────────────────────────────────────────────────────────────────────────────
# UNIFIED ANALYSIS
# ─────────────────────────────────────────────────────────────────────────────

class UnifiedAnalysis:
    """Combines all data sources into coherent model."""
    
    def __init__(self, client_log: Path, server_log: Path, instrument_dir: Optional[Path] = None):
        self.client_log = client_log
        self.server_log = server_log
        self.instrument_dir = instrument_dir

        self.client = ClientLogParser(client_log)
        self.server = ServerLogParser(server_log)
        self.server_deep = ServerInstrumentationParser(server_log)
        self.correlation = CorrelationEngine(self.client, self.server)

        # Load tick snapshots if provided
        self.tick_correlator = None
        self.unified_tick_model = {}
        self.divergence = {}
        if instrument_dir:
            client_snap = instrument_dir / "client"
            server_snap = instrument_dir / "server"
            if client_snap.exists() and server_snap.exists():
                self.tick_correlator = TickCorrelator(client_snap, server_snap)
                self.tick_correlator.load_snapshots()
                self.unified_tick_model = self.tick_correlator.to_unified_model()
                self.divergence = self.tick_correlator.compute_divergence_metrics()
    
    def generate_model(self) -> dict:
        """Generate unified game state model."""
        # Start with log-based model
        client_combat = self.client.get_combat_events()
        server_combat = self.server_deep.get_combat_summary()

        model = {
            'generated_at': datetime.now().isoformat(),
            'sources': {
                'client_log': str(self.client_log.name),
                'server_log': str(self.server_log.name),
            },
            'timeline': {
                'server_ticks': server_combat.get('tick_range', (0, 0)),
                'server_total_ticks': server_combat.get('total_ticks', 0),
            },
            'entities': {
                'connected_clients': server_combat.get('client_connections', 0),
                'unique_combatants': len(set(
                    e.get('attacker') for e in client_combat if e.get('attacker')
                ).union(
                    set(e.get('target') for e in client_combat if e.get('target'))
                )),
            },
            'combat': {
                'client': {
                    'attacks_sent': len([e for e in self.client.events if e.event_type == 'auto_combat']),
                    'hits_received': len(client_combat),
                    'total_damage': sum(e.get('damage', 0) for e in client_combat),
                },
                'server': {
                    'kills': server_combat.get('kills', 0),
                    'respawns': server_combat.get('respawns', 0),
                },
            },
        }

        # EXTENSION: If tick snapshots are available, augment with precise world model
        if self.tick_correlator:
            model['tick_model'] = self.unified_tick_model
            model['divergence'] = self.divergence

        return model
    
    def _build_entity_timeline(self) -> list[dict]:
        """Build entity lifecycle timeline."""
        timeline = []
        
        # From server: spawn/respawn/kill
        for event in self.server_deep.combat_events:
            timeline.append({
                'tick': event.tick,
                'source': 'server',
                'action': event.action,
                'attacker': event.attacker,
                'target': event.target,
            })
        
        # From client: combat events
        for e in self.client.get_combat_events():
            timeline.append({
                'tick': e.get('timestamp', 0),
                'source': 'client',
                'action': 'attack',
                'attacker': e.get('attacker'),
                'target': e.get('target'),
                'damage': e.get('damage'),
            })
        
        # Sort by tick
        timeline.sort(key=lambda x: x['tick'])
        return timeline
    
    def validate_hypothesis(self, hypothesis: str) -> dict:
        """Test a hypothesis about the game state."""
        
        results = {
            'hypothesis': hypothesis,
            'passed': False,
            'evidence': [],
            'recommendations': [],
        }
        
        if hypothesis == 'combat_works':
            # Hypothesis: Combat end-to-end works
            hits = len(self.client.get_combat_events())
            kills = self.server_deep.get_combat_summary().get('kills', 0)
            
            if hits > 0 and kills > 0:
                results['passed'] = True
                results['evidence'] = [
                    f"Client recorded {hits} combat hits",
                    f"Server recorded {kills} kills",
                ]
            else:
                results['evidence'] = [
                    f"Client hits: {hits}",
                    f"Server kills: {kills}",
                ]
                results['recommendations'] = [
                    "Check if auto-combat is enabled",
                    "Verify NPC entities exist",
                ]
        
        elif hypothesis == 'network_sync':
            # Hypothesis: Client and server are synchronized
            prediction = self.client.get_prediction_stats()
            
            if prediction['max_error'] < 2.0:
                results['passed'] = True
                results['evidence'] = [
                    f"Max prediction error: {prediction['max_error']:.2f}m",
                    "Network synchronization working",
                ]
            else:
                results['evidence'] = [
                    f"Max error: {prediction['max_error']:.2f}m (> 2m threshold)",
                ]
                results['recommendations'] = [
                    "Review client prediction parameters",
                    "Check server tick rate",
                ]
        
        elif hypothesis == 'multiplayer':
            # Hypothesis: Multiple clients connected
            connections = self.server_deep.get_combat_summary().get('client_connections', 0)
            
            if connections >= 2:
                results['passed'] = True
                results['evidence'] = [
                    f"{connections} clients connected",
                ]
            else:
                results['evidence'] = [
                    f"Only {connections} client(s)",
                ]
                results['recommendations'] = [
                    "Ensure multiple clients connect in demo",
                ]
        
        return results

# ──────────────────────���──────────────────────────────────────────────────────
# VALIDATION SUITE
# ─────────────────────────────────────────────────────────────────────────────

class ValidationSuite:
    """Run automated validation tests."""
    
    def __init__(self, log_dir: Path, instrument_dir: Optional[Path] = None):
        self.log_dir = log_dir
        self.instrument_dir = instrument_dir
    
    def find_latest_logs(self) -> tuple[Path, Path]:
        """Find most recent client and server logs."""
        client_logs = sorted(self.log_dir.glob("godot_*.log"), 
                         key=lambda p: p.stat().st_mtime)
        server_logs = sorted(self.log_dir.glob("server_*.log"), 
                          key=lambda p: p.stat().st_mtime)
        
        if not client_logs or not server_logs:
            raise FileNotFoundError(f"No logs found in {self.log_dir}")
        
        return client_logs[-1], server_logs[-1]
    
    def run_all_validations(self) -> dict:
        """Run all validation hypotheses."""

        client_log, server_log = self.find_latest_logs()
        analysis = UnifiedAnalysis(
            client_log=client_log,
            server_log=server_log,
            instrument_dir=self.instrument_dir
        )
        
        hypotheses = [
            'combat_works',
            'network_sync', 
            'multiplayer',
        ]
        
        results = {
            'timestamp': datetime.now().isoformat(),
            'logs': {
                'client': str(client_log.name),
                'server': str(server_log.name),
            },
            'tests': [],
            'summary': {
                'total': len(hypotheses),
                'passed': 0,
                'failed': 0,
            }
        }
        
        for hypothesis in hypotheses:
            result = analysis.validate_hypothesis(hypothesis)
            results['tests'].append(result)
            
            if result['passed']:
                results['summary']['passed'] += 1
            else:
                results['summary']['failed'] += 1
        
        # Generate full model
        results['model'] = analysis.generate_model()
        
        return results

# ─────────────────────────────────────────────────────────────────────────────
# CLI
# ─────────────────────────────────────────────────────────────────────────────

def main():
    parser = argparse.ArgumentParser(description="Unified Game Analysis")
    parser.add_argument('--log-dir', type=Path,
                       default=Path("/root/projects/DarkAges/tools/demo/artifacts/logs"))
    parser.add_argument('--instrument-dir', type=Path, default=Path('/tmp/darkages_snapshots'),
                       help='Base directory containing client/ and server/ tick snapshots')
    parser.add_argument('--validate', action='store_true')
    parser.add_argument('--model', action='store_true')
    parser.add_argument('--output', type=Path)
    parser.add_argument('--quiet', action='store_true')
    
    args = parser.parse_args()
    
    if args.validate:
        # Run validation suite
        suite = ValidationSuite(args.log_dir, args.instrument_dir)
        
        try:
            results = suite.run_all_validations()
        except FileNotFoundError as e:
            print(f"Error: {e}")
            print("Run demo first: python3 tools/demo/full_demo.py --quick")
            sys.exit(1)
        
        # Print summary
        if not args.quiet:
            print("\n" + "=" * 60)
            print("VALIDATION RESULTS")
            print("=" * 60)
            print(f"\nLogs: {results['logs']['client']}")
            print(f"      {results['logs']['server']}")
            print(f"\n{results['summary']['passed']}/{results['summary']['total']} tests passed")
            
            for test in results['tests']:
                status = "✅ PASS" if test['passed'] else "❌ FAIL"
                print(f"\n{status} {test['hypothesis']}")
                for evidence in test['evidence']:
                    print(f"   • {evidence}")
                for rec in test['recommendations']:
                    print(f"   → {rec}")
            
            model = results.get('model', {})
            print(f"\n📊 Model Summary:")
            print(f"   Combat hits: {model.get('combat', {}).get('client', {}).get('hits_received', 0)}")
            print(f"   Server kills: {model.get('combat', {}).get('server', {}).get('kills', 0)}")
            print(f"   Respawns: {model.get('combat', {}).get('server', {}).get('respawns', 0)}")
        
        if args.output:
            args.output.write_text(json.dumps(results, indent=2))
            if not args.quiet:
                print(f"\nResults written to {args.output}")
        
        sys.exit(0 if results['summary']['failed'] == 0 else 1)
    
    elif args.model:
        # Generate model
        suite = ValidationSuite(args.log_dir, args.instrument_dir)
        client_log, server_log = suite.find_latest_logs()
        analysis = UnifiedAnalysis(
            client_log=client_log,
            server_log=server_log,
            instrument_dir=args.instrument_dir if args.instrument_dir else None
        )

        model = analysis.generate_model()

        if args.output:
            args.output.write_text(json.dumps(model, indent=2))

        print(json.dumps(model, indent=2))

        sys.exit(0)
    
    else:
        # Default: interactive analysis
        suite = ValidationSuite(args.log_dir, args.instrument_dir)
        client_log, server_log = suite.find_latest_logs()
        analysis = UnifiedAnalysis(
            client_log=client_log,
            server_log=server_log,
            instrument_dir=args.instrument_dir if args.instrument_dir else None
        )

        print(f"\nAnalyzing:")
        print(f"  Client: {client_log.name}")
        print(f"  Server: {server_log.name}")

        model = analysis.generate_model()
        print(f"\nCombat:")
        print(f"  Client hits: {model['combat']['client']['hits_received']}")
        print(f"  Total damage: {model['combat']['client']['total_damage']}")
        print(f"  Server kills: {model['combat']['server']['kills']}")
        print(f"  Server respawns: {model['combat']['server']['respawns']}")

        # Print divergence if available
        if analysis.divergence:
            print(f"\nDivergence:")
            print(f"  Avg position delta: {analysis.divergence.get('avg_position_delta_m', 0):.3f} m")
            print(f"  Max delta: {analysis.divergence.get('max_delta_m', 0):.3f} m")
            print(f"  Synchronized ticks: {analysis.divergence.get('tick_count', 0)}")

        if args.output:
            args.output.write_text(json.dumps(model, indent=2))

if __name__ == '__main__':
    main()