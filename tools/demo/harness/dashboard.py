#!/usr/bin/env python3
"""DarkAges Demo Dashboard

Live terminal UI using the `rich` library.
Usage: python3 run_demo.py --full --dashboard
"""

import time
from pathlib import Path
from typing import Optional

from rich.console import Console
from rich.live import Live
from rich.panel import Panel
from rich.layout import Layout
from rich.text import Text
from rich.spinner import Spinner


class DemoDashboard:
    def __init__(self, supervisor):
        self.supervisor = supervisor
        self.console = Console()
        self.live: Optional[Live] = None
        self._running = False

    def _make_layout(self) -> Layout:
        layout = Layout()
        layout.split_column(
            Layout(name="header", size=3),
            Layout(name="main", ratio=1),
            Layout(name="footer", size=3),
        )
        layout["main"].split_row(
            Layout(name="left", ratio=1),
            Layout(name="right", ratio=1),
        )
        return layout

    def _render_header(self) -> Panel:
        status = self.supervisor.status()
        mode = "full" if self.supervisor.clients else "server-only"
        text = Text()
        text.append("DarkAges Demo", style="bold cyan")
        text.append(f"  Mode: {mode}  Port: {status.get('server_port', '?')}", style="white")
        if status.get("circuit_tripped"):
            text.append("  CIRCUIT TRIPPED", style="bold red")
        return Panel(text, style="cyan")

    def _render_server_panel(self) -> Panel:
        s = self.supervisor.server
        text = Text()
        if s.proc and s.proc.poll() is None:
            text.append("● RUNNING\n", style="bold green")
            text.append(f"PID: {s.proc.pid}\n", style="white")
        else:
            text.append("● STOPPED\n", style="bold red")
        text.append(f"Healthy: {s.healthy}\n", style="yellow" if not s.healthy else "green")
        text.append(f"Restarts: {s.restart_count}\n", style="white")
        text.append(f"Port: {self.supervisor.server_port}\n", style="white")
        return Panel(text, title="[b]Server[/b]", border_style="green" if s.healthy else "red")

    def _render_client_panel(self) -> Panel:
        if not self.supervisor.clients:
            return Panel("No clients", title="[b]Client[/b]")
        text = Text()
        for cid, state in self.supervisor.clients.items():
            if state.proc and state.proc.poll() is None:
                text.append(f"{cid}: ● RUNNING (PID {state.proc.pid})\n", style="green")
            else:
                text.append(f"{cid}: ● STOPPED\n", style="red")
            text.append(f"  Healthy: {state.healthy}  Restarts: {state.restart_count}\n", style="white")
        return Panel(text, title="[b]Client[/b]")

    def _render_metrics_panel(self) -> Panel:
        text = Text()
        try:
            import psutil
            mem = psutil.virtual_memory()
            cpu = psutil.cpu_percent(interval=0.1)
            text.append(f"CPU: {cpu:.1f}%\n", style="red" if cpu > 80 else "white")
            text.append(f"RAM: {mem.percent:.1f}% ({mem.used//1024//1024}MB / {mem.total//1024//1024}MB)\n",
                       style="red" if mem.percent > 85 else "white")
        except Exception:
            text.append("psutil not available\n", style="yellow")
        return Panel(text, title="[b]System[/b]")

    def _render_footer(self) -> Panel:
        return Panel("Press Ctrl+C to stop demo", style="dim")

    def _update(self):
        layout = self._make_layout()
        layout["header"].update(self._render_header())
        layout["left"].update(self._render_server_panel())
        layout["right"].update(self._render_client_panel())
        layout["footer"].update(self._render_footer())
        return layout

    def run(self, duration_sec: Optional[int] = None):
        self._running = True
        with Live(self._update(), refresh_per_second=2, screen=True) as live:
            start = time.time()
            try:
                while self._running:
                    if duration_sec and time.time() - start >= duration_sec:
                        break
                    live.update(self._update())
                    time.sleep(0.5)
            except KeyboardInterrupt:
                pass
            finally:
                self._running = False

    def stop(self):
        self._running = False
