#!/usr/bin/env python3
"""Programmatic authoring for DM HANGAR v2: a larger multi-level hangar
complex (docs/DM-MAP-GUIDE.md).

Central hangar bay (260x260, ceiling 64) with a catwalk/mezzanine ring at
y=30 (width 24) reached by three ramps and two gravity lifts, railings and
hanging bulkhead cover on the ring, pillars + crates for ground cover, and
four side rooms (control/armory/cargo/maintenance) around the perimeter,
two of which (cargo, maintenance) also carry an upper loft opening
straight onto the catwalk ring, giving two connected levels.

Run: python3 tools/mapgen/examples/dm_hangar.py
     (writes tools/mapgen/examples/dm_hangar.json next to this file)
"""
from __future__ import annotations

import json
import math
from pathlib import Path
from typing import Dict, List, Optional, Tuple

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


def _gaps(lo, hi, gap: Optional[Tuple[float, float]]):
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
# Central hangar bay shell. x,z in [-130,130], floor y0, ceiling y 64.
# ---------------------------------------------------------------------------
Y0 = 0.0
CAT_Y = 30.0           # catwalk/mezzanine walking surface
BAY_Y1 = 64.0           # main ceiling (34 clear above catwalk)
X0, X1 = -130.0, 130.0
Z0, Z1 = -130.0, 130.0

floor_slab("hangar floor", X0, X1, Z0, Z1, Y0)
ceiling_slab("hangar ceiling", X0, X1, Z0, Z1, BAY_Y1)

# North / south walls: single ground-level door (full height to the room's
# own 44-high ceiling), lintel above that back up to the main ceiling.
wall_z("hangar north wall lower", Z0, X0, X1, Y0, 44.0, gap=(-38.0, 38.0))
_box("hangar north lintel", (X0, 44.0, Z0 - WALL_T / 2), (X1, BAY_Y1, Z0 + WALL_T / 2), "hull.bmp")
wall_z("hangar south wall lower", Z1, X0, X1, Y0, 44.0, gap=(-38.0, 38.0))
_box("hangar south lintel", (X0, 44.0, Z1 - WALL_T / 2), (X1, BAY_Y1, Z1 + WALL_T / 2), "hull.bmp")

# Paired doors around a central divider give each north/south room
# separate entry and exit lanes instead of a single dead-end doorway.
box_obstacle("north door divider", -8, 8, Y0, 44, -132, -128, "hull.bmp")
box_obstacle("south door divider", -8, 8, Y0, 44, 128, 132, "hull.bmp")

# East / west walls: one tall opening (y0..54) serves both the ground-floor
# room (cargo/maintenance) and the upper loft that sits above it, so the
# two levels of that side room are both reachable through the same bay
# wall; a lintel caps the last 10 units up to the main ceiling.
wall_x("hangar east wall lower", 130, Z0, -38, Y0, 54)
wall_x("hangar east wall lower", 130, 38, Z1, Y0, 54)
# Upper lintels cap the two ground flank doors; centre remains the loft mouth.
wall_x("east flank lintel north", 130, -38, -20, 28, 54)
wall_x("east flank lintel south", 130, 20, 38, 28, 54)
_box("hangar east lintel", (X1 - WALL_T / 2, 54.0, Z0), (X1 + WALL_T / 2, BAY_Y1, Z1), "hull.bmp")
wall_x("hangar west wall lower", -130, Z0, -38, Y0, 54)
wall_x("hangar west wall lower", -130, 38, Z1, Y0, 54)
# Upper lintels cap the two ground flank doors; centre remains the loft mouth.
wall_x("west flank lintel north", -130, -38, -20, 28, 54)
wall_x("west flank lintel south", -130, 20, 38, 28, 54)
_box("hangar west lintel", (X0 - WALL_T / 2, 54.0, Z0), (X0 + WALL_T / 2, BAY_Y1, Z1), "hull.bmp")

# ---------------------------------------------------------------------------
# Catwalk / mezzanine ring: inset 6 from the walls, 24 wide, open in the
# middle so the ground floor forms a second loop and the ring itself a
# third -- plus 3 ramps and 2 lifts giving 5 distinct vertical links.
# ---------------------------------------------------------------------------
RING_OUT, RING_IN = 124.0, 100.0
floor_slab("catwalk north", -RING_OUT, RING_OUT, -RING_OUT, -RING_IN, CAT_Y, units=16.0)
floor_slab("catwalk south", -RING_OUT, RING_OUT, RING_IN, RING_OUT, CAT_Y, units=16.0)
floor_slab("catwalk west", -RING_OUT, -RING_IN, -RING_IN, RING_IN, CAT_Y, units=16.0)
floor_slab("catwalk east", RING_IN, RING_OUT, -RING_IN, RING_IN, CAT_Y, units=16.0)

# Bridges across the 6-unit gap between the ring's outer edge and the bay
# wall, so the east/west loft rooms are flush with the ring (same y).
floor_slab("east loft bridge", RING_OUT, X1, -20.0, 20.0, CAT_Y, units=16.0)
floor_slab("west loft bridge", X0, -RING_OUT, -20.0, 20.0, CAT_Y, units=16.0)

# Railings (8..12 high) along the ring's inner lip, with gaps left open at
# every ramp/lift landing so nobody gets walled off from a vertical route.
RAIL_H0, RAIL_H1 = CAT_Y, CAT_Y + 10.0
wall_z("rail north", -RING_IN, -RING_IN, RING_IN, RAIL_H0, RAIL_H1, texture="lightstrip.bmp",
       gap=(30.0, 75.0), units=16.0)
wall_z("rail south", RING_IN, -RING_IN, RING_IN, RAIL_H0, RAIL_H1, texture="lightstrip.bmp",
       gap=(-10.0, 10.0), units=16.0)
wall_x("rail east", RING_IN, -RING_IN, RING_IN, RAIL_H0, RAIL_H1, texture="lightstrip.bmp",
       gap=(-25.0, 25.0), units=16.0)
wall_x("rail west", -RING_IN, -RING_IN, RING_IN, RAIL_H0, RAIL_H1, texture="lightstrip.bmp",
       gap=(-25.0, 25.0), units=16.0)

# Hanging cover: bulkheads/pipes suspended from the ceiling to 26 above the
# catwalk (y=56), clear of the 20-unit player above a y30 walking surface.
HANG_Y0, HANG_Y1 = CAT_Y + 26.0, BAY_Y1 - 2.0
hang_positions = [(-60.0, -112.0), (60.0, -112.0), (-60.0, 112.0), (60.0, 112.0),
                   (-112.0, -60.0), (-112.0, 60.0), (112.0, -60.0), (112.0, 60.0)]
for i, (hx, hz) in enumerate(hang_positions):
    box_obstacle(f"hanging bulkhead {i}", hx - 9.0, hx + 9.0, HANG_Y0, HANG_Y1, hz - 9.0, hz + 9.0,
                 "bulkhead.bmp")

# Ramps (<=0.4 rise per unit): east, west (both to/from the floor's open
# middle) and a north auxiliary ramp offset from the lift column. Guard
# rails along both lateral edges of every ramp stop its sheer sides from
# reading as an ambiguous (14.5..17.5) ledge against the open floor.
ramp_block_x("ramp east", 20.0, 100.0, -20.0, 20.0, Y0, CAT_Y)
box_obstacle("ramp east guard north", 20.0, 100.0, Y0, BAY_Y1, 20.0, 22.0, "bulkhead.bmp")
box_obstacle("ramp east guard south", 20.0, 100.0, Y0, BAY_Y1, -22.0, -20.0, "bulkhead.bmp")
ramp_block_x("ramp west", -100.0, -20.0, -20.0, 20.0, CAT_Y, Y0)
box_obstacle("ramp west guard north", -100.0, -20.0, Y0, BAY_Y1, 20.0, 22.0, "bulkhead.bmp")
box_obstacle("ramp west guard south", -100.0, -20.0, Y0, BAY_Y1, -22.0, -20.0, "bulkhead.bmp")
ramp_block_z("ramp north-aux", 40.0, 64.0, -100.0, -20.0, CAT_Y, Y0)
box_obstacle("ramp aux guard east", 64.0, 66.0, Y0, BAY_Y1, -100.0, -20.0, "bulkhead.bmp")
box_obstacle("ramp aux guard west", 38.0, 40.0, Y0, BAY_Y1, -100.0, -20.0, "bulkhead.bmp")

# Gravity lifts (p1=61): shaft rises through the open middle of the bay
# (clear of any floor/ceiling overhead) and is pushed toward the ring once
# near the top; landing directly under a solid slab gets the shoulder-
# height collision ray stuck on its underside well short of the target.
LIFT_R = 7
point("raw", (0.0, Y0, -93.0), rotation=math.pi, p1=61, p2=LIFT_R, p3=int(CAT_Y))
point("raw", (0.0, Y0, 93.0), rotation=0.0, p1=61, p2=LIFT_R, p3=int(CAT_Y))

# ---------------------------------------------------------------------------
# Ground floor cover: pillars (>=18, full cover) and crates (8..14, low
# cover) spread so every piece is flankable, clear of ramps/lift columns.
# ---------------------------------------------------------------------------
pillar_positions = [(-55.0, -55.0), (15.0, -55.0), (-55.0, 55.0), (55.0, 55.0)]
for i, (px, pz) in enumerate(pillar_positions):
    box_obstacle(f"pillar {i}", px - 7.0, px + 7.0, Y0, 40.0, pz - 7.0, pz + 7.0, "console.bmp", units=16.0)

crate_positions = [
    (-30.0, 35.0), (30.0, -35.0), (0.0, -40.0), (0.0, 40.0),
    (-70.0, 20.0), (70.0, -20.0), (-20.0, -70.0), (20.0, 70.0),
    (-85.0, -45.0), (85.0, 45.0), (-45.0, 85.0), (45.0, -85.0),
]
for i, (cx, cz) in enumerate(crate_positions):
    box_obstacle(f"crate {i}", cx - 8.0, cx + 8.0, Y0, Y0 + 12.0, cz - 8.0, cz + 8.0, "labpanel.bmp")

# Central core: a tall blocking structure right in the middle of the bay,
# clear of both lift shafts and all three ramps, that kills the long
# corner-to-corner sightlines a 260x260 open floor would otherwise have.
box_obstacle("central core", -18.0, 18.0, Y0, 26.0, -18.0, 18.0, "bulkhead.bmp", units=16.0)

# A couple of low steps (<=3) near the bay centre for minor texture variety
# and to show the clear height-band policy in practice.
box_obstacle("step 0", -30.0, -20.0, Y0, Y0 + 2.5, -30.0, -20.0, "hazard.bmp", units=16.0)
box_obstacle("step 1", 20.0, 30.0, Y0, Y0 + 2.5, 20.0, 30.0, "hazard.bmp", units=16.0)

# ---------------------------------------------------------------------------
# Side rooms, ground level: control (north), armory (south), cargo (east),
# maintenance (west). Cargo/maintenance also have an upper loft opening
# straight onto the catwalk ring through the same bay-wall gap.
# ---------------------------------------------------------------------------
ROOM_Y1 = 44.0

# North: CONTROL ROOM.
NR_X0, NR_X1, NR_Z0, NR_Z1 = -45.0, 45.0, -200.0, -130.0
floor_slab("control room floor", NR_X0, NR_X1, NR_Z0, NR_Z1, Y0)
ceiling_slab("control room ceiling", NR_X0, NR_X1, NR_Z0, NR_Z1, ROOM_Y1)
wall_z("control room north wall", NR_Z0, NR_X0, NR_X1, Y0, ROOM_Y1)
wall_x("control room west wall", NR_X0, NR_Z0, NR_Z1, Y0, ROOM_Y1)
wall_x("control room east wall", NR_X1, NR_Z0, NR_Z1, Y0, ROOM_Y1)
box_obstacle("control room console", -10.0, 10.0, Y0, Y0 + 10.0, -195.0, -185.0, "console.bmp", units=16.0)
# Entry baffle: a full-height pillar just inside the doorway, off the
# spawn's exact centreline, blocks the long straight shot from the spawn
# out through the door into the open bay (cuts spawn exposure).
box_obstacle("control room baffle", -6.0, 6.0, Y0, ROOM_Y1, -168.0, -158.0, "bulkhead.bmp")

# South: ARMORY.
SR_X0, SR_X1, SR_Z0, SR_Z1 = -45.0, 45.0, 130.0, 200.0
floor_slab("armory floor", SR_X0, SR_X1, SR_Z0, SR_Z1, Y0)
ceiling_slab("armory ceiling", SR_X0, SR_X1, SR_Z0, SR_Z1, ROOM_Y1)
wall_z("armory south wall", SR_Z1, SR_X0, SR_X1, Y0, ROOM_Y1)
wall_x("armory west wall", SR_X0, SR_Z0, SR_Z1, Y0, ROOM_Y1)
wall_x("armory east wall", SR_X1, SR_Z0, SR_Z1, Y0, ROOM_Y1)
box_obstacle("armory rack", -10.0, 10.0, Y0, Y0 + 12.0, 185.0, 195.0, "bulkhead.bmp", units=16.0)
box_obstacle("armory baffle", -6.0, 6.0, Y0, ROOM_Y1, 158.0, 168.0, "bulkhead.bmp")

# East: CARGO STORE (ground) + loft (upper, opens straight onto the ring).
ER_X0, ER_X1, ER_Z0, ER_Z1 = 130.0, 200.0, -45.0, 45.0
floor_slab("cargo floor", ER_X0, ER_X1, ER_Z0, ER_Z1, Y0)
ceiling_slab("cargo ceiling", ER_X0, ER_X1, ER_Z0, ER_Z1, BAY_Y1)
wall_x("cargo east wall", ER_X1, ER_Z0, ER_Z1, Y0, BAY_Y1)
wall_z("cargo north wall", ER_Z0, ER_X0, ER_X1, Y0, BAY_Y1)
wall_z("cargo south wall", ER_Z1, ER_X0, ER_X1, Y0, BAY_Y1)
box_obstacle("cargo crate stack", 150.0, 170.0, Y0, Y0 + 12.0, -10.0, 10.0, "labpanel.bmp", units=16.0)

ELX0, ELX1, ELZ0, ELZ1 = RING_OUT, 164.0, -20.0, 20.0
floor_slab("east loft floor", ELX0, ELX1, ELZ0, ELZ1, CAT_Y, units=16.0)
ceiling_slab("east loft ceiling", ELX0, ELX1, ELZ0, ELZ1, BAY_Y1 - 2.0)
wall_x("east loft east wall", ELX1, ELZ0, ELZ1, CAT_Y, BAY_Y1 - 2.0)
wall_z("east loft north wall", ELZ0, ELX0, ELX1, CAT_Y, BAY_Y1 - 2.0)
wall_z("east loft south wall", ELZ1, ELX0, ELX1, CAT_Y, BAY_Y1 - 2.0)

# West: MAINTENANCE BAY (ground) + loft (upper).
WR_X0, WR_X1, WR_Z0, WR_Z1 = -200.0, -130.0, -45.0, 45.0
floor_slab("maintenance floor", WR_X0, WR_X1, WR_Z0, WR_Z1, Y0)
ceiling_slab("maintenance ceiling", WR_X0, WR_X1, WR_Z0, WR_Z1, BAY_Y1)
wall_x("maintenance west wall", WR_X0, WR_Z0, WR_Z1, Y0, BAY_Y1)
wall_z("maintenance north wall", WR_Z0, WR_X0, WR_X1, Y0, BAY_Y1)
wall_z("maintenance south wall", WR_Z1, WR_X0, WR_X1, Y0, BAY_Y1)
box_obstacle("maintenance bench", -170.0, -150.0, Y0, Y0 + 10.0, -10.0, 10.0, "console.bmp", units=16.0)

WLX0, WLX1, WLZ0, WLZ1 = -164.0, -RING_OUT, -20.0, 20.0
floor_slab("west loft floor", WLX0, WLX1, WLZ0, WLZ1, CAT_Y, units=16.0)
ceiling_slab("west loft ceiling", WLX0, WLX1, WLZ0, WLZ1, BAY_Y1 - 2.0)
wall_x("west loft west wall", WLX0, WLZ0, WLZ1, CAT_Y, BAY_Y1 - 2.0)
wall_z("west loft north wall", WLZ0, WLX0, WLX1, CAT_Y, BAY_Y1 - 2.0)
wall_z("west loft south wall", WLZ1, WLX0, WLX1, CAT_Y, BAY_Y1 - 2.0)

# ---------------------------------------------------------------------------
# DM spawns (10): eight in side rooms and two on opposite ring corners,
# facing toward the bay. All pairs remain at least 40 units apart.
# ---------------------------------------------------------------------------
DM_SPAWNS = [
    (-35.0, 2.0, -190.0), (-112.0, CAT_Y + 2.0, -112.0), (35.0, 2.0, -190.0),   # control room
    (-35.0, 2.0, 190.0), (112.0, CAT_Y + 2.0, 112.0), (35.0, 2.0, 190.0),      # armory
    (150.0, 2.0, -30.0), (150.0, 2.0, 30.0),                          # cargo
    (-150.0, 2.0, -30.0), (-150.0, 2.0, 30.0),                        # maintenance
]
for idx, (sx, sy, sz) in enumerate(DM_SPAWNS):
    rot = face(-sx, -sz)  # face back toward the bay / doorway
    point("raw", (sx, sy, sz), rotation=rot, p1=60, p2=0, p3=0, p4=idx)

# One ordinary player_spawn so the map still loads as a regular mission.
point("player_spawn", (0.0, 2.0, 160.0), rotation=math.pi, human_type=43)

# ---------------------------------------------------------------------------
# Weapon pickups: basics near spawns, power weapons in contested spots
# (lift landings, bay centre).
# ---------------------------------------------------------------------------
weapon_spots = [
    ((0.0, 2.0, -175.0), 1, 90),       # MP5, control room
    ((0.0, 2.0, 175.0), 3, 60),        # M92F, armory
    ((165.0, 2.0, 30.0), 9, 90),       # M4, cargo (clear of the crate stack)
    ((-165.0, 2.0, 30.0), 10, 90),     # AK47, maintenance (clear of the bench)
    ((40.0, 2.0, 40.0), 13, 4),        # grenade, bay centre (contested, clear of the core)
    ((0.0, CAT_Y + 2.0, -112.0), 26, 50),  # PLASMA RIFLE, top of the north lift
    ((0.0, CAT_Y + 2.0, 112.0), 23, 20),   # LEVER 30-30, top of the south lift
    ((-112.0, CAT_Y + 2.0, 80.0), 24, 50),  # ZAP PISTOL, west ring
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
