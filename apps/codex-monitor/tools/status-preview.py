#!/usr/bin/env python3
"""Render the formal target UI in LIVE, WAIT, and STALE states."""
import os
import shutil
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
OUT_DIR = ROOT / "build" / "status-preview"
HOST_EXE = OUT_DIR / "status-preview-renderer.exe"

def main():
    cc = os.environ.get("JZ2440_HOST_CC") or shutil.which("gcc")
    if not cc:
        raise SystemExit("host gcc not found; set JZ2440_HOST_CC")
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    command = [cc, "-std=c99", "-O2", "-ffunction-sections", "-fdata-sections",
               "-I", str(ROOT / "tools" / "preview_host_include"),
               "-I", str(ROOT / "apps" / "codex-monitor" / "target" / "include"),
               str(ROOT / "apps" / "codex-monitor" / "tools" / "preview_main.c"), "-Wl,--gc-sections",
               "-o", str(HOST_EXE)]
    subprocess.run(command, check=True)
    try:
        from PIL import Image
    except ImportError:
        Image = None
    for mode in ("live", "wait", "stale"):
        ppm = OUT_DIR / (mode + ".ppm")
        with ppm.open("wb") as output:
            subprocess.run([str(HOST_EXE), mode], check=True, stdout=output)
        if Image is not None:
            Image.open(ppm).save(OUT_DIR / (mode + ".png"), optimize=False)
    print(OUT_DIR)

if __name__ == "__main__":
    main()
