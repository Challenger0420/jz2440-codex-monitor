#!/usr/bin/env python3
"""Build the host preview from the target renderer, then emit PPM and PNG."""
import os
import shutil
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
OUT = ROOT / "build" / "preview.ppm"
HOST_EXE = ROOT / "build" / "preview-renderer.exe"

def main():
    cc = os.environ.get("JZ2440_HOST_CC") or shutil.which("gcc")
    if not cc:
        raise SystemExit("host gcc not found; set JZ2440_HOST_CC")
    HOST_EXE.parent.mkdir(parents=True, exist_ok=True)
    command = [cc, "-std=c99", "-O2", "-ffunction-sections", "-fdata-sections",
               "-I", str(ROOT / "tools" / "preview_host_include"),
               "-I", str(ROOT / "apps" / "codex-monitor" / "target" / "include"),
               str(ROOT / "apps" / "codex-monitor" / "tools" / "preview_main.c"), "-Wl,--gc-sections",
               "-o", str(HOST_EXE)]
    subprocess.run(command, check=True)
    with OUT.open("wb") as output:
        subprocess.run([str(HOST_EXE)], check=True, stdout=output)
    try:
        from PIL import Image
    except ImportError:
        print(str(OUT))
        print("PNG skipped: Pillow is not installed")
        return
    Image.open(OUT).save(ROOT / "build" / "preview.png", optimize=False)
    print(str(OUT))
    print(str(ROOT / "build" / "preview.png"))

if __name__ == "__main__":
    main()
