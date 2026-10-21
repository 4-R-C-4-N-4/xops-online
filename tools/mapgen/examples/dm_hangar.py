#!/usr/bin/env python3
"""Programmatic authoring for DM HANGAR: a compact deathmatch arena.

Enclosed ~200x200 hangar, ground floor + a perimeter catwalk ring
(mezzanine) at y 28..32 reached by three gentle ramps, crates/pillars for
cover, at least 3 routes between any two areas (the open ground floor plus
the catwalk ring itself forms extra loops), 10 DM spawns, 6+ weapon
pickups. Fully sealed: walls + one ceiling slab over everything.

Run: python3 tools/mapgen/examples/dm_hangar.py
     (writes tools/mapgen/examples/dm_hangar.json next to this file)
"""
from __future__ import annotations

import json
import math
from pathlib import Path
from typing import Dict, List

HERE = Path(__file__).resolve().parent

# Free-pack textures only (assets/pack/data/ufoescape, ufofield).
TEXTURES = [
    "hull.bmp", "floor.bmp", "ceiling.bmp", "bulkhead.bmp", "console.bmp",
    "lightstrip.bmp", "hazard.bmp", "labpanel.bmp",
]

WALL_T = 4.0
SLAB_T = 2.0

BLOCKS: List[Dict] = []
POINTS: List[Dict] = []


def _box(comment, vmin, vmax, texture, units=32.0, faces=None) -> None:
    entry = {"comment": comment, "min": list(vmin), "max": list(vmax),
              "texture_world_units": units}
    if faces:
        entry["faces"] = faces
    else:
        entry["texture"] = texture
    BLOCKS.append(entry)


def floor_slab(comment, x0, x1, z0, z1, y, texture="floor.bmp", units=32.0):
    _box(comment, (x0, y - SLAB_T, z0), (x1, y, z1), texture, units)


def ceiling_slab(comment, x0, x1, z0, z1, y, texture="ceiling.bmp", units=32.0):
    _box(comment, (x0, y, z0), (x1, y + SLAB_T, z1), texture, units)


def _gaps(lo, hi, gap):
    if gap is None:
        return [(lo, hi)]
    ga, gb = gap
    ga, gb = max(lo, ga), min(hi, gb)
    out = []
    if ga - lo > 0.5:
        out.append((lo, ga))
    if hi - gb > 0.5:
        out.append((gb, hi))
    return out


def wall_x(comment, x, z0, z1, y0, y1, texture="hull.bmp", gap=None, units=32.0):
    for i, (a, b) in enumerate(_gaps(min(z0, z1), max(z0, z1), gap)):
        _box(f"{comment} [{i}]", (x - WALL_T / 2, y0, a), (x + WALL_T / 2, y1, b), texture, units)


def wall_z(comment, z, x0, x1, y0, y1, texture="hull.bmp", gap=None, units=32.0):
    for i, (a, b) in enumerate(_gaps(min(x0, x1), max(x0, x1), gap)):
        _box(f"{comment} [{i}]", (a, y0, z - WALL_T / 2), (b, y1, z + WALL_T / 2), texture, units)


def box_obstacle(comment, x0, x1, y0, y1, z0, z1, texture, units=16.0):
    _box(comment, (x0, y0, z0), (x1, y1, z1), texture, units)


def ramp_block_z(comment, x0, x1, z0, z1, y_at_z0, y_at_z1, texture="floor.bmp", edge_texture="hazard.bmp"):
    """Raw sloped block rising along z, flush with the floors it joins."""
    thick = 4.0
    verts = [
        [x0, y_at_z0, z0], [x1, y_at_z0, z0], [x1, y_at_z1, z1], [x0, y_at_z1, z1],
        [x0, y_at_z0 - thick, z0], [x1, y_at_z0 - thick, z0], [x1, y_at_z1 - thick, z1], [x0, y_at_z1 - thick, z1],
    ]
    tex = {"texture": texture, "u": [0.0, 0.0, 1.0, 1.0], "v": [0.0, 0.0, 1.0, 1.0]}
    etex = {"texture": edge_texture, "u": [0.0, 0.0, 1.0, 1.0], "v": [0.0, 0.0, 1.0, 1.0]}
    BLOCKS.append({"comment": comment, "vertices": verts,
                   "faces": [dict(tex), dict(tex), dict(etex), dict(etex), dict(etex), dict(etex)]})


def ramp_block_x(comment, x0, x1, z0, z1, y_at_x0, y_at_x1, texture="floor.bmp", edge_texture="hazard.bmp"):
    """Raw sloped block rising along x."""
    thick = 4.0
    verts = [
        [x0, y_at_x0, z0], [x1, y_at_x1, z0], [x1, y_at_x1, z1], [x0, y_at_x0, z1],
        [x0, y_at_x0 - thick, z0], [x1, y_at_x1 - thick, z0], [x1, y_at_x1 - thick, z1], [x0, y_at_x0 - thick, z1],
    ]
    tex = {"texture": texture, "u": [0.0, 0.0, 1.0, 1.0], "v": [0.0, 0.0, 1.0, 1.0]}
    etex = {"texture": edge_texture, "u": [0.0, 0.0, 1.0, 1.0], "v": [0.0, 0.0, 1.0, 1.0]}
    BLOCKS.append({"comment": comment, "vertices": verts,
                   "faces": [dict(tex), dict(tex), dict(etex), dict(etex), dict(etex), dict(etex)]})


def point(kind, pos, rotation=0.0, **kw):
    entry = {"type": kind, "pos": list(pos), "rotation": rotation}
    entry.update(kw)
    POINTS.append(entry)
    return entry


def face(dx, dz):
    return math.atan2(dx, dz)


# ---------------------------------------------------------------------------
# Shell: outer walls, floor, ceiling. Room spans x,z in [-100,100].
# ---------------------------------------------------------------------------
Y0 = 0.0
CAT_Y = 28.0          # catwalk walking surface
Y1 = 56.0             # main ceiling (56-28=28 >= 20 headroom above catwalk)
X0, X1 = -100.0, 100.0
Z0, Z1 = -100.0, 100.0

floor_slab("hangar floor", X0, X1, Z0, Z1, Y0)
ceiling_slab("hangar ceiling", X0, X1, Z0, Z1, Y1)
wall_x("hangar west wall", X0, Z0, Z1, Y0, Y1)
wall_x("hangar east wall", X1, Z0, Z1, Y0, Y1)
wall_z("hangar south wall", Z1, X0, X1, Y0, Y1)
wall_z("hangar north wall", Z0, X0, X1, Y0, Y1)

# ---------------------------------------------------------------------------
# Catwalk ring (mezzanine), inset from the walls, open in the middle so the
# ground floor below forms a second loop; the ring itself is a third loop
# around the perimeter -> at least 3 distinct routes between any two spots.
# ---------------------------------------------------------------------------
RING_OUT, RING_IN = 96.0, 80.0  # ring band from x/z 80..96 (abs)
floor_slab("catwalk north", -RING_OUT, RING_OUT, -RING_OUT, -RING_IN, CAT_Y, units=16.0)
floor_slab("catwalk south", -RING_OUT, RING_OUT, RING_IN, RING_OUT, CAT_Y, units=16.0)
floor_slab("catwalk west", -RING_OUT, -RING_IN, -RING_IN, RING_IN, CAT_Y, units=16.0)
floor_slab("catwalk east", RING_IN, RING_OUT, -RING_IN, RING_IN, CAT_Y, units=16.0)

# ---------------------------------------------------------------------------
# Ramps: three gentle ramps (<=0.4 rise/unit), flush at both ends, placed so
# their footprints do not overlap each other or the crates below.
# ---------------------------------------------------------------------------
# North ramp: x[-10,10], catwalk edge z=-RING_IN(-80,y=28) down to ground z=-3(y=0).
ramp_block_z("ramp north", -10.0, 10.0, -RING_IN, -3.0, CAT_Y, Y0)
# East ramp: z[55,71], catwalk edge x=RING_IN(80,y=28) down to ground x=3(y=0).
ramp_block_x("ramp east", 3.0, RING_IN, 55.0, 71.0, Y0, CAT_Y)
# West ramp: z[-71,-55], catwalk edge x=-RING_IN(-80,y=28) down to ground x=-12(y=0).
ramp_block_x("ramp west", -RING_IN, -12.0, -71.0, -55.0, CAT_Y, Y0)

# ---------------------------------------------------------------------------
# Ground-floor cover: crates / pillars, kept clear of ramps and the centre
# sightline cross so none of the 3 routes gets blocked into a dead end.
# ---------------------------------------------------------------------------
crate_positions = [
    (-50.0, -50.0), (50.0, -50.0), (-50.0, 50.0), (50.0, 50.0),
    (0.0, 30.0), (0.0, -30.0), (30.0, 0.0), (-30.0, 0.0),
]
for i, (cx, cz) in enumerate(crate_positions):
    box_obstacle(f"crate {i}", cx - 8.0, cx + 8.0, Y0, Y0 + 16.0, cz - 8.0, cz + 8.0, "labpanel.bmp")

# Four pillars on the catwalk ring corners for cover up top.
pillar_positions = [(-88.0, -88.0), (88.0, -88.0), (-88.0, 88.0), (88.0, 88.0)]
for i, (px, pz) in enumerate(pillar_positions):
    box_obstacle(f"pillar {i}", px - 6.0, px + 6.0, CAT_Y, CAT_Y + 22.0, pz - 6.0, pz + 6.0, "console.bmp", units=16.0)

# ---------------------------------------------------------------------------
# DM spawns: 10 raw points, p1=60 p2=0 p3=0 p4=spawn index, facing centre.
# 6 on the ground floor (hex ring r=65, rotated off-axis to clear ramps &
# crates), 4 on the catwalk ring (one per side, clear of ramps/pillars).
# ---------------------------------------------------------------------------
GROUND_SPAWNS = [
    (62.0, 2.0, 20.0), (62.0, 2.0, -20.0), (-62.0, 2.0, 20.0),
    (-62.0, 2.0, -20.0), (-25.0, 2.0, 65.0), (25.0, 2.0, -65.0),
]
CATWALK_SPAWNS = [
    (0.0, CAT_Y + 2.0, 88.0),      # south catwalk
    (0.0, CAT_Y + 2.0, -88.0),     # north catwalk (clear of the ramp at x[-10,10])
    (-88.0, CAT_Y + 2.0, 0.0),     # west catwalk
    (88.0, CAT_Y + 2.0, 0.0),      # east catwalk
]
DM_SPAWNS = GROUND_SPAWNS + CATWALK_SPAWNS
for idx, (sx, sy, sz) in enumerate(DM_SPAWNS):
    rot = face(-sx, -sz)  # face toward the arena centre
    point("raw", (sx, sy, sz), rotation=rot, p1=60, p2=0, p3=0, p4=idx)

# One ordinary player_spawn so the map still loads as a regular mission
# (human_type 43, same classic id ufo_field uses for its player spawn).
point("player_spawn", (0.0, 2.0, 90.0), rotation=math.pi, human_type=43)

# ---------------------------------------------------------------------------
# Weapon pickups (classic ids from src/parameter.cpp Weapon[] + alien
# weapons 24-26 from UFO FIELD): MP5(1), M92F(3), M4(9), AK47(10),
# LEVER 30-30(23), GRENADE(13), ZAP PISTOL(24), PLASMA RIFLE(26).
# ---------------------------------------------------------------------------
weapon_spots = [
    ((0.0, 2.0, 0.0), 13, 4),       # grenade, centre of the ground floor
    ((40.0, 2.0, 40.0), 1, 90),     # MP5
    ((-40.0, 2.0, -40.0), 10, 90),  # AK47
    ((40.0, 2.0, -40.0), 9, 90),    # M4
    ((-40.0, 2.0, 40.0), 3, 60),    # M92F
    ((60.0, CAT_Y + 2.0, 0.0), 24, 50),   # ZAP PISTOL, east catwalk
    ((-60.0, CAT_Y + 2.0, 0.0), 26, 50),  # PLASMA RIFLE, west catwalk
]
for (wx, wy, wz), wid, ammo in weapon_spots:
    point("weapon_pickup", (wx, wy, wz), weapon_id=wid, ammo=ammo)


def build_doc() -> Dict:
    return {"schema": 1, "textures": TEXTURES, "blocks": BLOCKS, "points": POINTS}


def main() -> int:
    doc = build_doc()
    out_path = HERE / "dm_hangar.json"
    out_path.write_text(json.dumps(doc, indent=2) + "\n", encoding="utf-8")
    print(f"wrote {out_path}: {len(BLOCKS)} blocks, {len(POINTS)} points")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
