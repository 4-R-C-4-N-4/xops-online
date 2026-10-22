#!/usr/bin/env python3
"""Find z-fighting in generated maps: visible block faces that share a plane.

    python3 tools/geomcheck.py assets/pack/data/dmhangar/temp.bd1 [...]
    python3 tools/geomcheck.py --all          # every free-pack map

See tools/mapgen/zfight.py for the rule. tools.mapgen build resolves fights
automatically; this checker audits the committed binaries. Exit 1 if any
visible coplanar overlap is found.
"""
import argparse
import pathlib
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))
from tools.mapgen import bd1  # noqa: E402
from tools.mapgen.zfight import FACE_NAMES, fights  # noqa: E402


def check(path, verbose=True):
    data = bd1.loads(pathlib.Path(path).read_bytes())
    found = fights(data.blocks)
    if verbose:
        for a, b, ar in found:
            print(f"ZFIGHT {path}: block {a['block']} {FACE_NAMES[a['face']]} vs block {b['block']} "
                  f"{FACE_NAMES[b['face']]}, plane n=({a['n'][0]:.2f},{a['n'][1]:.2f},{a['n'][2]:.2f}) "
                  f"d={a['d']:.2f}, overlap {ar:.1f} sq units")
        print(f"{path}: {len(data.blocks)} blocks, {len(found)} visible coplanar overlaps")
    return found


def all_maps():
    return sorted((ROOT / "assets/pack/data").glob("*/temp.bd1"))


def main(argv=None):
    ap = argparse.ArgumentParser()
    ap.add_argument("maps", nargs="*")
    ap.add_argument("--all", action="store_true")
    a = ap.parse_args(argv)
    paths = all_maps() if a.all or not a.maps else a.maps
    total = sum(len(check(p)) for p in paths)
    return 1 if total else 0


if __name__ == "__main__":
    raise SystemExit(main())
