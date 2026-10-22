#!/usr/bin/env python3
"""Programmatic authoring for DM FIELD: a deathmatch variant of UFO FIELD.

Reuses the UFO FIELD geometry (continuous farm ground, tall boundary stores,
gated pasture, through barn, loft, hay bales, UFO and cows) verbatim from tools/mapgen/examples/ufo_field.json, but strips all
hostile humans / staged waves / AI waypoints / the scripted event chain and
adds 10 DM spawns plus weapon pickups scattered around the hay bales, cows
and the UFO. Cows (small_object, addon/cow.txt) are kept: they do not
depend on the mission .mif/event chain, only on the addon file which this
map's own .mif also references.

Run: python3 tools/mapgen/examples/dm_field.py
     (writes tools/mapgen/examples/dm_field.json next to this file)
"""
from __future__ import annotations

import json
import math
from pathlib import Path
from typing import Dict, List

HERE = Path(__file__).resolve().parent
SRC = HERE / "ufo_field.json"


def point(kind, pos, rotation=0.0, **kw):
    entry = {"type": kind, "pos": list(pos), "rotation": rotation}
    entry.update(kw)
    return entry


def face(dx, dz):
    return math.atan2(dx, dz)


def build_points() -> List[Dict]:
    src = json.loads(SRC.read_text())
    # Keep only the non-hostile content: cows (small_object). Drop
    # staged_human (waves), waypoint (AI paths), raw (event chain links)
    # and the original rancher player_spawn -- DM wiring replaces it.
    points: List[Dict] = [p for p in src["points"] if p["type"] == "small_object"]

    # 10 DM spawns around the field: behind hay bales, near cows, and
    # near the UFO ramp, spread so no two are close together.
    dm_positions = [
        (-100.0, 2.0, 120.0), (100.0, 2.0, 120.0),
        (-150.0, 2.0, 70.0), (150.0, 2.0, 70.0),
        (-100.0, 2.0, -20.0), (100.0, 2.0, -20.0),
        (0.0, 2.0, 160.0), (0.0, 2.0, 60.0),
        (-60.0, 2.0, -70.0), (60.0, 2.0, -70.0),
    ]
    for idx, (sx, sy, sz) in enumerate(dm_positions):
        rot = face(-sx, -(sz + 60.0))  # roughly face the UFO / field centre
        points.append(point("raw", (sx, sy, sz), rotation=rot, p1=60, p2=0, p3=0, p4=idx))

    # One ordinary player_spawn (human_type 43, same as UFO FIELD's
    # rancher) so the map still loads as a regular mission.
    points.append(point("player_spawn", (0.0, 2.0, 200.0), rotation=math.pi, human_type=43))

    # Weapon pickups: classic ids (src/parameter.cpp Weapon[]) plus alien
    # weapons 24-26 from UFO FIELD's own drop table.
    weapon_spots = [
        ((-100.0, 2.0, 60.0), 23, 40),   # LEVER 30-30 near cow 0 (field's own rifle)
        ((100.0, 2.0, 60.0), 1, 90),     # MP5 near cow 1
        ((-56.0, 2.0, -98.0), 24, 50),   # ZAP PISTOL by the UFO ramp
        ((56.0, 2.0, -98.0), 26, 50),    # PLASMA RIFLE by the UFO ramp
        ((0.0, 2.0, 160.0), 9, 90),      # M4 near the barn-side hay row
        ((-150.0, 2.0, 70.0), 10, 90),   # AK47 far hay bales
        ((150.0, 2.0, 70.0), 13, 4),     # GRENADE far hay bales
    ]
    for (wx, wy, wz), wid, ammo in weapon_spots:
        points.append(point("weapon_pickup", (wx, wy, wz), weapon_id=wid, ammo=ammo))

    return points


def build_doc() -> Dict:
    src = json.loads(SRC.read_text())
    return {"schema": 1, "textures": src["textures"], "blocks": src["blocks"],
            "points": build_points()}


def main() -> int:
    doc = build_doc()
    out_path = HERE / "dm_field.json"
    out_path.write_text(json.dumps(doc, indent=2) + "\n", encoding="utf-8")
    print(f"wrote {out_path}: {len(doc['blocks'])} blocks, {len(doc['points'])} points")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
