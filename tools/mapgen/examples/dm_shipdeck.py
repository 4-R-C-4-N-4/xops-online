#!/usr/bin/env python3
"""Programmatic authoring for DM SHIPDECK: a three-level alien ship deck.

Layout (docs/DM-MAP-GUIDE.md): sealed interior, 360x300 footprint, a tall
central goo-reactor shaft running the full height of the ship crossed by
catwalks near the top, three walking levels (engineering deck at y=0, mid
deck of corridors/labs at y=32, upper catwalk ring at y=64) linked by three
ramps and three gravity lifts, cover everywhere (pillars, crates, goo
troughs, specimen tubes, hanging bulkheads over the catwalk), 12 DM spawns
and 9 weapon pickups (one of each docs-mandated id).

Run: python3 tools/mapgen/examples/dm_shipdeck.py
     (writes tools/mapgen/examples/dm_shipdeck.json next to this file)
"""
from __future__ import annotations

import json
import math
from pathlib import Path
from typing import Dict, List, Tuple

HERE = Path(__file__).resolve().parent

# Free-pack textures only (assets/pack/data/ufoescape).
TEXTURES = [
    "hull.bmp", "floor.bmp", "ceiling.bmp", "bulkhead.bmp", "console.bmp",
    "lightstrip.bmp", "hazard.bmp", "labpanel.bmp", "goo.bmp", "vat.bmp",
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
    thick = 4.0
    verts = [
        [x0, y_at_x0, z0], [x1, y_at_x1, z0], [x1, y_at_x1, z1], [x0, y_at_x0, z1],
        [x0, y_at_x0 - thick, z0], [x1, y_at_x1 - thick, z0], [x1, y_at_x1 - thick, z1], [x0, y_at_x0 - thick, z1],
    ]
    tex = {"texture": texture, "u": [0.0, 0.0, 1.0, 1.0], "v": [0.0, 0.0, 1.0, 1.0]}
    etex = {"texture": edge_texture, "u": [0.0, 0.0, 1.0, 1.0], "v": [0.0, 0.0, 1.0, 1.0]}
    BLOCKS.append({"comment": comment, "vertices": verts,
                   "faces": [dict(tex), dict(tex), dict(etex), dict(etex), dict(etex), dict(etex)]})


def octagonal_vat(comment, cx, cz, half, y0, y1, side_tex="vat.bmp", top_tex="goo.bmp"):
    chamfer = half * 0.45
    tex_top = {"texture": top_tex, "u": [0.0, 0.0, 1.0, 1.0], "v": [0.0, 0.0, 1.0, 1.0]}
    tex_side = {"texture": side_tex, "u": [0.0, 0.0, 1.0, 1.0], "v": [0.0, 0.0, 1.0, 1.0]}

    def vat_box(tag, x0, x1, z0, z1):
        verts = [
            [x0, y1, z0], [x1, y1, z0], [x1, y1, z1], [x0, y1, z1],
            [x0, y0, z0], [x1, y0, z0], [x1, y0, z1], [x0, y0, z1],
        ]
        faces = [dict(tex_top), dict(tex_side), dict(tex_side), dict(tex_side),
                 dict(tex_side), dict(tex_side)]
        BLOCKS.append({"comment": f"{comment} {tag}", "vertices": verts, "faces": faces})

    vat_box("centre", cx - half, cx + half, cz - half + chamfer, cz + half - chamfer)
    vat_box("north cap", cx - half + chamfer, cx + half - chamfer, cz - half, cz - half + chamfer)
    vat_box("south cap", cx - half + chamfer, cx + half - chamfer, cz + half - chamfer, cz + half)


def point(kind, pos, rotation=0.0, **kw):
    entry = {"type": kind, "pos": list(pos), "rotation": rotation}
    entry.update(kw)
    POINTS.append(entry)
    return entry


def face(dx, dz):
    return math.atan2(dx, dz)


def rect_minus_holes(comment_prefix, x0, x1, z0, z1, y, holes: List[Tuple[float, float, float, float]],
                      texture="floor.bmp", units=32.0, kind="floor"):
    """Fill rectangle [x0,x1]x[z0,z1] at height y with floor/ceiling slabs,
    leaving the given axis-aligned hole rectangles open. Grid-slices on the
    union of all hole boundaries so every resulting piece is either fully
    inside a hole (skipped) or fully outside every hole (kept)."""
    xs = sorted({x0, x1, *[h[0] for h in holes], *[h[1] for h in holes]})
    xs = [v for v in xs if x0 - 1e-6 <= v <= x1 + 1e-6]
    zs = sorted({z0, z1, *[h[2] for h in holes], *[h[3] for h in holes]})
    zs = [v for v in zs if z0 - 1e-6 <= v <= z1 + 1e-6]
    n = 0
    for i in range(len(xs) - 1):
        ax, bx = xs[i], xs[i + 1]
        if bx - ax < 0.5:
            continue
        cx = (ax + bx) / 2
        for j in range(len(zs) - 1):
            az, bz = zs[j], zs[j + 1]
            if bz - az < 0.5:
                continue
            cz = (az + bz) / 2
            if any(h[0] < cx < h[1] and h[2] < cz < h[3] for h in holes):
                continue
            n += 1
            if kind == "floor":
                floor_slab(f"{comment_prefix} [{n}]", ax, bx, az, bz, y, texture, units)
            else:
                ceiling_slab(f"{comment_prefix} [{n}]", ax, bx, az, bz, y, texture, units)


# ---------------------------------------------------------------------------
# Shell: outer walls run the full height (sealed hull), floor at the bottom,
# ceiling at the top. Footprint 360x300 = 108000 sq units of play area.
# ---------------------------------------------------------------------------
Y_ENG = 0.0     # engineering deck (lower): goo vats, troughs, pipes
Y_MID = 32.0    # mid deck: corridors + labs with specimen tubes
Y_CAT = 64.0    # upper catwalks crossing the reactor shaft
Y_TOP = 96.0    # sealed roof

X0, X1 = -180.0, 180.0
Z0, Z1 = -150.0, 150.0
SX0, SX1 = -34.0, 34.0   # central reactor shaft (open full height)
SZ0, SZ1 = -34.0, 34.0

floor_slab("engineering deck floor", X0, X1, Z0, Z1, Y_ENG)
ceiling_slab("ship roof", X0, X1, Z0, Z1, Y_TOP)
wall_x("hull west wall", X0, Z0, Z1, Y_ENG, Y_TOP)
wall_x("hull east wall", X1, Z0, Z1, Y_ENG, Y_TOP)
wall_z("hull south wall", Z1, X0, X1, Y_ENG, Y_TOP)
wall_z("hull north wall", Z0, X0, X1, Y_ENG, Y_TOP)

# ---------------------------------------------------------------------------
# Ramps linking the three decks. Each ramp is flanked by full-height walls
# (ufo_escape "ramp shaft" pattern) so there is never a floor at mismatched
# height directly beside a sloped surface (the cause of ambiguous ledges).
# R1: engineering(y0) -> mid(y32), south side, walled shaft x[-22,22].
# R2east/R2west: mid(y32) -> catwalk(y64), flush onto the shaft's crossing
# catwalk bridge at x=+-4 (their flanks are open shaft air, not floor, so
# no wall is needed there).
# ---------------------------------------------------------------------------
ramp_block_z("ramp R1 eng->mid (south)", -20.0, 20.0, 34.0, 120.0, Y_ENG, Y_MID)
wall_x("ramp R1 shaft west wall", -22.0, 34.0, 120.0, Y_ENG, Y_CAT)
wall_x("ramp R1 shaft east wall", 22.0, 34.0, 120.0, Y_ENG, Y_CAT)

# R2/R3: mid->cat ramps live in the east/west ring (shaft is too narrow for
# a 32-unit rise at <=0.4 slope), each walled off from the flanking ring
# floor so no ambiguous ledge forms beside the slope.
ramp_block_x("ramp R2 mid->cat (east)", 50.0, 140.0, -12.0, 12.0, Y_MID, Y_CAT)
wall_z("ramp R2 shaft south wall", -14.0, 50.0, 140.0, Y_MID, Y_TOP)
wall_z("ramp R2 shaft north wall", 14.0, 50.0, 140.0, Y_MID, Y_TOP)

ramp_block_x("ramp R3 mid->cat (west)", -140.0, -50.0, -12.0, 12.0, Y_CAT, Y_MID)
wall_z("ramp R3 shaft south wall", -14.0, -140.0, -50.0, Y_MID, Y_TOP)
wall_z("ramp R3 shaft north wall", 14.0, -140.0, -50.0, Y_MID, Y_TOP)

# Mid deck floor (also serves as the engineering ceiling over the ring):
# holes = reactor shaft + the R1 ramp's notch (so the ramp can rise through).
MID_HOLES = [
    (SX0, SX1, SZ0, SZ1),
    (-22.0, 22.0, 34.0, 120.0),
    (50.0, 140.0, -12.0, 12.0),
    (-140.0, -50.0, -12.0, 12.0),
]
rect_minus_holes("mid deck floor", X0, X1, Z0, Z1, Y_MID, MID_HOLES, "floor.bmp", 32.0, "floor")

# Catwalk deck floor (perimeter rooms); holes for the shaft and the two
# walled mid->cat ramp notches.
CAT_HOLES = [
    (SX0, SX1, SZ0, SZ1),
    (50.0, 140.0, -12.0, 12.0),
    (-140.0, -50.0, -12.0, 12.0),
]
rect_minus_holes("catwalk deck floor", X0, X1, Z0, Z1, Y_CAT, CAT_HOLES, "floor.bmp", 32.0, "floor")

# Catwalk cross-bridges over the reactor shaft (the "+" crossing) -- the
# ramps above flush onto these at x=+-4.
floor_slab("catwalk bridge N-S", -4.0, 4.0, SZ0, SZ1, Y_CAT, "floor.bmp", units=16.0)
floor_slab("catwalk bridge E-W", SX0, SX1, -4.0, 4.0, Y_CAT, "floor.bmp", units=16.0)

# ---------------------------------------------------------------------------
# Central goo reactor: a tall octagonal vat rising from the engineering
# floor almost to the catwalk crossing (8 units clearance under the bridge).
# ---------------------------------------------------------------------------
octagonal_vat("reactor core", 0.0, 0.0, 20.0, Y_ENG, 54.0, "vat.bmp", "goo.bmp")

# ---------------------------------------------------------------------------
# Gravity lifts (p1=61): foot just inside the open shaft, well clear of the
# crossing bridges and the ramp footprints, exit toward the nearest ring
# edge so the player lands on real floor near the column.
# L1: engineering -> mid, north edge.  L2: engineering -> catwalk, SE
# corner (straight shot, skips the mid floor since the shaft stays open).
# L3: engineering -> catwalk, NW corner, feeding the plasma rifle.
# ---------------------------------------------------------------------------
point("raw", (0.0, Y_ENG, -32.0), rotation=face(0.0, -1.0), p1=61, p2=6, p3=32)
point("raw", (30.0, Y_ENG, 20.0), rotation=face(1.0, 0.0), p1=61, p2=6, p3=64)
point("raw", (-30.0, Y_ENG, -20.0), rotation=face(-1.0, 0.0), p1=61, p2=6, p3=64)

# ---------------------------------------------------------------------------
# Engineering deck cover: goo troughs (low, <=12), pipe pillars (>=18),
# clustered around the ring, clear of the ramp and reactor base.
# ---------------------------------------------------------------------------
trough_positions = [
    (-120.0, -100.0), (120.0, -100.0), (-120.0, 100.0), (120.0, 100.0),
    (-150.0, 0.0), (150.0, 0.0),
]
for i, (tx, tz) in enumerate(trough_positions):
    box_obstacle(f"goo trough {i}", tx - 14.0, tx + 14.0, Y_ENG, Y_ENG + 11.0, tz - 8.0, tz + 8.0, "goo.bmp")

pipe_positions = [
    (-70.0, -70.0), (70.0, -70.0), (-70.0, 70.0), (70.0, 70.0),
    (-150.0, -90.0), (150.0, -90.0), (-150.0, 90.0), (150.0, 90.0),
]
for i, (px, pz) in enumerate(pipe_positions):
    box_obstacle(f"eng pipe pillar {i}", px - 6.0, px + 6.0, Y_ENG, Y_ENG + 24.0, pz - 6.0, pz + 6.0, "bulkhead.bmp")

# ---------------------------------------------------------------------------
# Mid deck cover: specimen tubes (small_object) as waist-high flankable
# cover plus labpanel crates, kept clear of the ramps/holes.
# ---------------------------------------------------------------------------
crate_positions = [
    (-130.0, -60.0), (130.0, -60.0), (-130.0, 60.0), (130.0, 60.0),
    (0.0, -140.0), (0.0, 140.0),
]
for i, (cx, cz) in enumerate(crate_positions):
    box_obstacle(f"mid crate {i}", cx - 9.0, cx + 9.0, Y_MID, Y_MID + 12.0, cz - 9.0, cz + 9.0, "labpanel.bmp")

tube_positions = [
    (-100.0, -20.0), (100.0, -20.0), (-100.0, 20.0), (100.0, 20.0),
    (-60.0, -130.0), (60.0, -130.0),
]
for i, (tx, tz) in enumerate(tube_positions):
    point("small_object", (tx, Y_MID + 2.0, tz), rotation=0.0, object_type=12)

# Two parallel lab corridors per deck, open at both ends and through a
# central cross passage. Full-height partitions break the long open lanes.
for deck, roof in ((Y_ENG, Y_MID - SLAB_T), (Y_MID, Y_CAT - SLAB_T), (Y_CAT, Y_TOP)):
    for z in (-92.0, 92.0):
        wall_z(f"lab corridor bulkhead y{deck} z{z}", z, -110.0, 110.0,
               deck, roof, "labpanel.bmp", gap=(-26.0, 26.0))

# ---------------------------------------------------------------------------
# Catwalk deck cover: console pillars (>=18) plus hanging bulkheads from the
# roof (bottom at Y_CAT+26, the docs-allowed minimum) breaking the long
# diagonal sightlines across the reactor shaft.
# ---------------------------------------------------------------------------
cat_pillars = [
    (-150.0, -110.0), (150.0, -110.0), (-150.0, 110.0), (150.0, 110.0),
    (-60.0, -20.0), (60.0, 20.0),
]
for i, (px, pz) in enumerate(cat_pillars):
    box_obstacle(f"catwalk pillar {i}", px - 6.0, px + 6.0, Y_CAT, Y_CAT + 24.0, pz - 6.0, pz + 6.0, "console.bmp", units=16.0)

hanging_positions = [(-18.0, -70.0), (18.0, 70.0), (-70.0, 18.0), (70.0, -18.0)]
for i, (hx, hz) in enumerate(hanging_positions):
    box_obstacle(f"catwalk hanging bulkhead {i}", hx - 10.0, hx + 10.0, Y_CAT + 26.0, Y_TOP, hz - 5.0, hz + 5.0, "bulkhead.bmp", units=16.0)

# ---------------------------------------------------------------------------
# DM spawns: 12 raw points, p1=60 p4=index, tucked in alcove-like corners
# facing the open lanes, spread across all three decks.
# ---------------------------------------------------------------------------
ENG_SPAWNS = [
    (-160.0, 2.0, -130.0), (160.0, 2.0, -130.0), (-160.0, 2.0, 130.0), (160.0, 2.0, 130.0),
    (-160.0, 2.0, 0.0), (160.0, 2.0, 0.0),
]
MID_SPAWNS = [
    (-160.0, Y_MID + 2.0, -60.0), (160.0, Y_MID + 2.0, -60.0),
    (-160.0, Y_MID + 2.0, 60.0), (160.0, Y_MID + 2.0, 60.0),
]
CAT_SPAWNS = [
    (-160.0, Y_CAT + 2.0, -120.0), (160.0, Y_CAT + 2.0, 120.0),
]
DM_SPAWNS = ENG_SPAWNS + MID_SPAWNS + CAT_SPAWNS
for idx, (sx, sy, sz) in enumerate(DM_SPAWNS):
    rot = face(-sx, -sz)  # face toward the arena centre / open lane
    point("raw", (sx, sy, sz), rotation=rot, p1=60, p2=0, p3=0, p4=idx)

# One ordinary player_spawn so the map also loads as a regular mission.
point("player_spawn", (0.0, Y_ENG + 2.0, -140.0), rotation=math.pi, human_type=43)

# ---------------------------------------------------------------------------
# Weapon pickups: one of each docs-mandated id. Basics near spawns,
# power weapons/grenades in contested spots (lift landings, shaft crossing).
# ---------------------------------------------------------------------------
weapon_spots = [
    ((-150.0, 2.0, -110.0), 1, 90),                       # MP5, near eng spawn
    ((150.0, Y_MID + 2.0, -40.0), 3, 60),                  # M92F, mid deck
    ((-150.0, Y_MID + 2.0, 40.0), 9, 90),                  # M4, mid deck
    ((150.0, 2.0, 110.0), 10, 90),                         # AK47, eng spawn
    ((-150.0, 2.0, 40.0), 24, 50),                         # alien zap, eng
    ((110.0, Y_CAT + 2.0, -130.0), 25, 100),               # photon SMG, catwalk
    ((-45.0, Y_CAT + 2.0, -20.0), 26, 80),                 # plasma rifle, atop lift L3
    ((0.0, Y_ENG + 2.0, 26.0), 23, 20),                    # lever-action, shaft entrance
    ((-100.0, Y_MID + 2.0, 100.0), 13, 4),                 # grenade, mid deck corner
]
for (wx, wy, wz), wid, ammo in weapon_spots:
    point("weapon_pickup", (wx, wy, wz), weapon_id=wid, ammo=ammo)


def build_doc() -> Dict:
    return {"schema": 1, "textures": TEXTURES, "blocks": BLOCKS, "points": POINTS}


def main() -> int:
    doc = build_doc()
    out_path = HERE / "dm_shipdeck.json"
    out_path.write_text(json.dumps(doc, indent=2) + "\n", encoding="utf-8")
    print(f"wrote {out_path}: {len(BLOCKS)} blocks, {len(POINTS)} points")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
