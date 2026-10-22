#!/usr/bin/env python3
"""Rebuild every free-pack map from its authored source, in one place.

    python3 tools/rebuild_maps.py            # regenerate + rebuild + rehash
    python3 tools/rebuild_maps.py --check    # exit 1 if any binary is stale

Generators (tools/mapgen/examples/*.py) write their JSON first; each JSON is
then compiled with tools.mapgen (which also resolves coplanar z-fighting).
Changed pack files get their sha256 refreshed in assets/provenance.jsonl.
"""
import argparse
import hashlib
import json
import pathlib
import subprocess
import sys
import tempfile

ROOT = pathlib.Path(__file__).resolve().parents[1]
EX = ROOT / "tools/mapgen/examples"
PACK = ROOT / "assets/pack"

# (generator or None, json, pack dir, pd1 name)
MAPS = [
    (None, "map10_demo.json", "data/map10", "op.pd1"),
    (None, "map10_demo.json", "data/menu", None),          # menu shares map10's blocks
    (None, "starter_compound.json", "data/starter", "start.pd1"),
    (None, "ufo_field.json", "data/ufofield", "ufo.pd1"),
    ("ufo_escape.py", "ufo_escape.json", "data/ufoescape", "escape.pd1"),
    ("dm_hangar.py", "dm_hangar.json", "data/dmhangar", "hangar.pd1"),
    ("dm_field.py", "dm_field.json", "data/dmfield", "field.pd1"),
    ("dm_shipdeck.py", "dm_shipdeck.json", "data/dmship", "ship.pd1"),
    ("dm_spire.py", "dm_spire.json", "data/dmspire", "spire.pd1"),
]


def build(json_name, pd1_name, out):
    cmd = [sys.executable, "-m", "tools.mapgen", "build", str(EX / json_name), "-o", str(out),
           "--pd1-name", pd1_name or "unused.pd1"]
    subprocess.run(cmd, cwd=ROOT, check=True, capture_output=True)


def main(argv=None):
    ap = argparse.ArgumentParser()
    ap.add_argument("--check", action="store_true")
    a = ap.parse_args(argv)
    stale, changed = [], []
    for gen, js, d, pd in MAPS:
        if gen and not a.check:
            subprocess.run([sys.executable, str(EX / gen)], cwd=ROOT, check=True, capture_output=True)
        with tempfile.TemporaryDirectory() as t:
            build(js, pd, pathlib.Path(t))
            for name in ["temp.bd1"] + ([pd] if pd else []):
                new = (pathlib.Path(t) / name).read_bytes()
                dest = PACK / d / name
                if dest.read_bytes() != new:
                    stale.append(f"{d}/{name}")
                    if not a.check:
                        dest.write_bytes(new)
                        changed.append(f"{d}/{name}")
    if a.check:
        for s in stale:
            print("STALE", s)
        return 1 if stale else 0
    if changed:
        prov = ROOT / "assets/provenance.jsonl"
        rows = [json.loads(l) for l in prov.read_text().splitlines()]
        for r in rows:
            if r["path"] in changed:
                r["sha256"] = hashlib.sha256((PACK / r["path"]).read_bytes()).hexdigest()
        prov.write_text("".join(json.dumps(r, ensure_ascii=False) + "\n" for r in rows))
    for c in changed:
        print("rebuilt", c)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
