#!/usr/bin/env python3
"""Build the formal renderer's four-font-role inspection image."""
import os
import shutil
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
OUT = ROOT / "build" / "font-refinement" / "font-role-debug.ppm"
EXE = ROOT / "build" / "font-refinement" / "font-role-debug-renderer.exe"

def main():
    cc = os.environ.get("JZ2440_HOST_CC") or shutil.which("gcc")
    if not cc:
        raise SystemExit("host gcc not found; set JZ2440_HOST_CC")
    EXE.parent.mkdir(parents=True, exist_ok=True)
    command = [cc, "-std=c99", "-O2", "-I", str(ROOT / "tools" / "preview_host_include"),
               "-I", str(ROOT / "apps" / "codex-monitor" / "target" / "include"),
               str(ROOT / "apps" / "codex-monitor" / "tools" / "font_role_debug_main.c"), "-o", str(EXE)]
    subprocess.run(command, check=True)
    with OUT.open("wb") as output:
        subprocess.run([str(EXE)], check=True, stdout=output)
    try:
        from PIL import Image
    except ImportError:
        print(OUT)
        return
    png = OUT.with_suffix(".png")
    Image.open(OUT).save(png, optimize=False)
    print(OUT)
    print(png)

if __name__ == "__main__":
    main()
