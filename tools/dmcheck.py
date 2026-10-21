#!/usr/bin/env python3
"""Real-engine deathmatch map gate (wraps tests/dm/dmcheck.cpp).

    python3 tools/dmcheck.py data/dmhangar hangar.pd1 [--sealed]
    python3 tools/dmcheck.py --jumpprobe          # measure the jump limit

Compiles the checker with g++ (cached under build-native/dmcheck), runs it
against assets/pack and prints its METRIC/FAIL lines. Exit code 0 = pass.
See docs/DM-MAP-GUIDE.md for what each gate means.
"""
import argparse
import hashlib
import json
import pathlib
import subprocess
import sys
import tempfile

ROOT = pathlib.Path(__file__).resolve().parents[1]
ENGINE = ["ai", "collision", "config", "d3dgraphics-debugfont", "datafile", "debug", "event",
          "gamemain", "gui-object", "object", "objectmanager", "parameter", "resource", "scene",
          "soundmanager", "statemachine"]


def sources():
    src = [ROOT / "tests/dm/dmcheck.cpp"]
    src += [ROOT / "src" / f"{n}.cpp" for n in ENGINE]
    src += sorted((ROOT / "src/net").glob("*.cpp"))
    src += [ROOT / "src/platform/common.cpp", ROOT / "src/platform/input-common.cpp", ROOT / "src/platform/vfs.cpp"]
    src += [ROOT / "src/platform/stub" / f"{n}-stub.cpp" for n in ("window", "input", "sound", "d3dgraphics")]
    return src


def build():
    h = hashlib.sha256()
    for p in sources() + sorted((ROOT / "src").glob("*.h")) + sorted((ROOT / "src/net").glob("*.h")):
        h.update(p.read_bytes())
    out = ROOT / "build-native" / "dmcheck" / ("dmcheck-" + h.hexdigest()[:12])
    if not out.exists():
        out.parent.mkdir(parents=True, exist_ok=True)
        cmd = ["g++", "-std=c++17", "-O2", "-DOPENXOPS_PLATFORM_STUB", "-I", str(ROOT / "src"),
               *map(str, sources()), "-lz", "-o", str(out)]
        r = subprocess.run(cmd, capture_output=True, text=True)
        if r.returncode:
            sys.stderr.write(r.stdout + r.stderr)
            raise SystemExit("dmcheck: build failed")
    return out


def jump_fixture(dest: pathlib.Path):
    """Flat floor with steps 4..16 units high every 30 units along x."""
    blocks = [{"comment": "floor", "min": [-40.0, -2.0, -60.0], "max": [780.0, 0.0, 60.0], "texture": "floor.bmp"}]
    for k in range(25):
        h = 4.0 + k
        blocks.append({"comment": f"step {h:.0f}", "min": [k * 30.0 - 8, 0.0, -5.0], "max": [k * 30.0 + 8, h, 15.0],
                       "texture": "floor.bmp"})
    doc = {"schema": 1, "textures": ["floor.bmp"], "blocks": blocks,
           "points": [{"type": "player_spawn", "pos": [0.0, 1.0, -30.0], "rotation": 0.0, "human_type": 43}]}
    j = dest / "jump.json"
    j.write_text(json.dumps(doc))
    out = dest / "data" / "jumptest"
    out.mkdir(parents=True)
    subprocess.run([sys.executable, "-m", "tools.mapgen", "build", str(j), "-o", str(out), "--pd1-name", "jump.pd1"],
                   cwd=ROOT, check=True, capture_output=True)
    (out / "floor.bmp").write_bytes((ROOT / "assets/pack/data/ufoescape/floor.bmp").read_bytes())
    return dest


def main(argv=None):
    ap = argparse.ArgumentParser()
    ap.add_argument("mapdir", nargs="?")
    ap.add_argument("pd1", nargs="?")
    ap.add_argument("--sealed", action="store_true")
    ap.add_argument("--jumpprobe", action="store_true")
    ap.add_argument("--pack", default=str(ROOT / "assets/pack"))
    a = ap.parse_args(argv)
    exe = build()
    if a.jumpprobe:
        with tempfile.TemporaryDirectory() as t:
            fx = jump_fixture(pathlib.Path(t))
            r = subprocess.run([str(exe), a.pack, "data/jumptest", "jump.pd1", "--jumpprobe", "--overlay", str(fx)],
                               capture_output=True, text=True)
    else:
        args = [str(exe), a.pack, a.mapdir, a.pd1] + (["--sealed"] if a.sealed else [])
        r = subprocess.run(args, capture_output=True, text=True)
    sys.stdout.write(r.stdout)
    sys.stderr.write(r.stderr[-2000:])
    return r.returncode


if __name__ == "__main__":
    raise SystemExit(main())
