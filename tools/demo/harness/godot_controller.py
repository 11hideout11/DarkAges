#!/usr/bin/env python3
"""Godot Controller for DarkAges Demo

Direct CLI-based Godot control. No MCP server required.
Provides: launch, stop, build verification, screenshot, GDScript execution.
"""

import os
import shutil
import subprocess
import sys
import tempfile
import time
from dataclasses import dataclass
from pathlib import Path
from typing import List, Optional

PROJECT_ROOT = Path("/root/projects/DarkAges")
GODOT_PATH = "/usr/local/bin/godot"
CLIENT_PATH = PROJECT_ROOT / "src/client"

RESET = "\033[0m"
BOLD = "\033[1m"
GREEN = "\033[92m"
YELLOW = "\033[93m"
RED = "\033[91m"
CYAN = "\033[96m"


@dataclass
class GodotResult:
    success: bool
    stdout: str = ""
    stderr: str = ""
    returncode: int = 0


class GodotController:
    def __init__(
        self,
        godot_path: str = GODOT_PATH,
        project_path: Path = CLIENT_PATH,
    ):
        self.godot_path = godot_path
        self.project_path = project_path
        self._proc: Optional[subprocess.Popen] = None

    def _run(self, args: List[str], timeout: int = 60, cwd: Optional[Path] = None) -> GodotResult:
        cmd = [self.godot_path] + args
        try:
            r = subprocess.run(
                cmd,
                cwd=str(cwd or self.project_path),
                capture_output=True,
                text=True,
                timeout=timeout,
            )
            return GodotResult(
                success=r.returncode == 0,
                stdout=r.stdout,
                stderr=r.stderr,
                returncode=r.returncode,
            )
        except subprocess.TimeoutExpired:
            return GodotResult(success=False, stderr="Timeout")
        except FileNotFoundError:
            return GodotResult(success=False, stderr=f"Godot not found: {self.godot_path}")

    def verify_installation(self) -> bool:
        r = self._run(["--version"], cwd=Path("/"))
        ok = r.success and "4.2" in (r.stdout + r.stderr)
        print(f"  Godot version: {(r.stdout + r.stderr).strip()}")
        return ok

    def import_project(self) -> bool:
        print(f"{CYAN}  Importing Godot project...{RESET}")
        r = self._run(["--headless", "--path", str(self.project_path), "--import"], timeout=120)
        if not r.success:
            print(f"{RED}  Import failed: {r.stderr[-500:]}{RESET}")
        else:
            print(f"{GREEN}  Import OK{RESET}")
        return r.success

    def build_solutions(self) -> bool:
        print(f"{CYAN}  Building C# solutions...{RESET}")
        r = self._run(
            ["--headless", "--path", str(self.project_path), "--build-solutions"],
            timeout=180,
        )
        if not r.success:
            print(f"{RED}  C# build failed: {r.stderr[-500:]}{RESET}")
        else:
            print(f"{GREEN}  C# build OK{RESET}")
        return r.success

    def launch_headless(self, server_host: str = "127.0.0.1", server_port: int = 7777) -> subprocess.Popen:
        """Launch Godot headless. Falls back to xvfb-run if dummy driver fails."""
        # Try xvfb-run first (virtual display) for true headless operation
        if shutil.which("xvfb-run"):
            cmd = [
                "xvfb-run", "-a", "--server-args=-screen 0 1280x720x24",
                self.godot_path,
                "--path", str(self.project_path),
                "--", "--server", server_host, "--port", str(server_port),
            ]
        else:
            cmd = [
                self.godot_path,
                "--headless", "--display-driver", "headless",
                "--path", str(self.project_path),
                "--", "--server", server_host, "--port", str(server_port),
            ]
        log_path = PROJECT_ROOT / "tools/demo/artifacts/logs/godot_headless.log"
        log_path.parent.mkdir(parents=True, exist_ok=True)
        log_file = open(log_path, "w")
        self._proc = subprocess.Popen(cmd, stdout=log_file, stderr=subprocess.STDOUT)
        print(f"{GREEN}  Godot headless started (PID {self._proc.pid}){RESET}")
        return self._proc

    def launch_headed(self, server_host: str = "127.0.0.1", server_port: int = 7777) -> subprocess.Popen:
        cmd = [
            self.godot_path,
            "--path", str(self.project_path),
            "--", "--server", server_host, "--port", str(server_port),
        ]
        log_path = PROJECT_ROOT / "tools/demo/artifacts/logs/godot_headed.log"
        log_path.parent.mkdir(parents=True, exist_ok=True)
        log_file = open(log_path, "w")
        self._proc = subprocess.Popen(cmd, stdout=log_file, stderr=subprocess.STDOUT)
        print(f"{GREEN}  Godot headed started (PID {self._proc.pid}){RESET}")
        return self._proc

    def stop(self, graceful: bool = True):
        if not self._proc:
            return
        print(f"  Stopping Godot (PID {self._proc.pid})...")
        if graceful:
            self._proc.terminate()
            try:
                self._proc.wait(timeout=5)
            except subprocess.TimeoutExpired:
                self._proc.kill()
                self._proc.wait(timeout=2)
        else:
            self._proc.kill()
        self._proc = None
        print(f"{GREEN}  Godot stopped{RESET}")

    def is_running(self) -> bool:
        return self._proc is not None and self._proc.poll() is None

    def execute_gdscript(self, code: str, timeout: int = 30) -> GodotResult:
        """Execute a GDScript snippet via --script and return output."""
        with tempfile.NamedTemporaryFile(mode="w", suffix=".gd", delete=False) as f:
            f.write(code)
            tmp_path = f.name
        try:
            r = self._run(
                ["--headless", "--path", str(self.project_path), "--script", tmp_path, "--quit"],
                timeout=timeout,
            )
        finally:
            os.unlink(tmp_path)
        return r

    def take_screenshot(self, output_path: Path) -> bool:
        """Capture screenshot using Godot GDScript. Works in headless mode.
        Falls back to OS tools if GDScript fails.
        """
        # Primary: Godot GDScript viewport capture
        gdscript = f'''
extends SceneTree
func _init():
    await create_timer(0.5).timeout
    var img = get_root().get_viewport().get_texture().get_image()
    var err = img.save_png("{output_path}")
    if err == OK:
        print("SCREENSHOT_OK")
    else:
        print("SCREENSHOT_FAIL: ", err)
    quit()
'''
        r = self.execute_gdscript(gdscript, timeout=15)
        if r.success and "SCREENSHOT_OK" in r.stdout:
            if output_path.exists():
                return True

        # Fallback: OS screenshot tools
        for tool in ["grim", "scrot", "import"]:
            if shutil.which(tool):
                if tool == "grim":
                    cmd = [tool, str(output_path)]
                elif tool == "scrot":
                    cmd = [tool, str(output_path)]
                else:
                    cmd = [tool, "-window", "root", str(output_path)]
                try:
                    subprocess.run(cmd, capture_output=True, timeout=10)
                    if output_path.exists():
                        return True
                except Exception:
                    pass
        print(f"{YELLOW}  Screenshot failed: no working capture method{RESET}")
        return False


def main():
    ctrl = GodotController()
    ok = ctrl.verify_installation()
    if not ok:
        sys.exit(1)
    ok = ctrl.import_project()
    if not ok:
        sys.exit(1)
    ok = ctrl.build_solutions()
    sys.exit(0 if ok else 1)


if __name__ == "__main__":
    main()
