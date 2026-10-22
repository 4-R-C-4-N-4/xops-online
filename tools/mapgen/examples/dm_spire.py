#!/usr/bin/env python3
"""Original CC0 DM SPIRE: three tower courts, stacked bridges and lift loops.

General arena principles only: alternate routes, contested vertical travel,
flankable cover and protected spawns. No reference geometry or external assets.
Run this file, then build the emitted JSON with tools.mapgen.
"""
from __future__ import annotations

import json
import math
from pathlib import Path
from typing import Dict, List

HERE = Path(__file__).resolve().parent

# Free-pack sci-fi textures only (assets/pack/data/ufoescape), same set DM
# HANGAR uses, fitting a "cold research outpost" theme.
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
    if z0 > z1:
        z0, z1, y_at_z0, y_at_z1 = z1, z0, y_at_z1, y_at_z0
    thick = 4.0
    verts = [
        [x0, y_at_z0, z0], [x1, y_at_z0, z0], [x1, y_at_z1, z1], [x0, y_at_z1, z1],
        [x0, y_at_z0 - thick, z0], [x1, y_at_z0 - thick, z0], [x1, y_at_z1 - thick, z1], [x0, y_at_z1 - thick, z1],
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


# Sealed footprint, three walkable bands separated by a safe 32-unit drop.
GND, MID, TOP = 0, 32, 64
floor_slab("foundation", -160,160,-150,150,0)
ceiling_slab("ceiling", -160,160,-150,150,108)
wall_x("west shell",-160,-150,150,0,108)
wall_x("east shell",160,-150,150,0,108)
wall_z("north shell",-150,-160,160,0,108)
wall_z("south shell",150,-160,160,0,108)

# Two lateral towers: broad raised rooms and independent ground access ramps.
for side in (-1,1):
    a,b = (92,156) if side==1 else (-156,-92)
    floor_slab(f"tower {side} mid floor",a,b,-60,60,MID)
    # Ramp rises northward into the room, no floor above the ramp throat.
    ramp_block_z(f"tower {side} ramp",a+12,b-12,140,60,0,MID)
    wall_x(f"tower {side} ramp left",a+10,60,140,0,108)
    wall_x(f"tower {side} ramp right",b-10,60,140,0,108)
    wall_x(f"tower {side} outer wall",side*156,-60,60,0,68)
    wall_z(f"tower {side} north wall",-60,a,b,0,68,gap=(a+20,b-20))
    wall_x(f"tower {side} inner wall",side*92,-60,60,0,68,gap=(-14,14))
    ceiling_slab(f"tower {side} ceiling",a,b,-60,60,66)
    # Lift sits beside the inner edge, pushes outward onto a flat floor.
    point("raw",(side*87,0,0),rotation=face(side,0),p1=61,p2=6,p3=MID)
    box_obstacle(f"tower {side} room cover",a+24,a+36,MID,MID+10,25,37,"console.bmp")
    # Enclosed upper north bridge, a dogleg into the spire landing.
    floor_slab(f"bridge {side} north leg",a+20,b-20,-112,-60,MID)
    wall_x(f"bridge {side} outside",a+20,-112,-60,MID,66)
    wall_x(f"bridge {side} inside",b-20,-112,-60,MID,66)
    ceiling_slab(f"bridge {side} roof",a+20,b-20,-112,-60,66)
    x0,x1=(24,136) if side==1 else (-136,-24)
    floor_slab(f"bridge {side} cross leg",x0,x1,-128,-104,MID)
    wall_z(f"bridge {side} north rail",-128,x0,x1,MID,MID+10)
    wall_z(f"bridge {side} south rail",-104,x0,x1,MID,MID+10,gap=(a+20,b-20))
    box_obstacle(f"bridge {side} hanging baffle",side*64-6,side*64+6,58,86,-128,-104,"bulkhead.bmp")

# North tower landing and high gallery: ramp, lift and safe drop choices.
floor_slab("spire mid landing",-28,28,-146,-92,MID)
ramp_block_z("spire ground ramp",-16,16,-12,-92,0,MID)
# Side walls stop at the landing so both bridges connect without obstructions.
wall_x("spire ramp west",-22,-90,-20,0,108)
wall_x("spire ramp east",22,-90,-20,0,108)
floor_slab("spire high gallery",-28,28,-146,-112,TOP)
ramp_block_z("spire upper ramp",-16,16,-32,-112,MID,TOP)
# Mid ramp toe is served by an open elevated courtyard spur.
floor_slab("courtyard mid spur",-16,16,-32,-16,MID)
point("raw",(0,0,-11),rotation=math.pi,p1=61,p2=6,p3=MID)
wall_x("spire top west",-28,-146,-112,TOP,104)
wall_x("spire top east",28,-146,-112,TOP,104)
# Ground route dividers form four connected courts; doorways provide loops.
for side in (-1,1):
    wall_z(f"south court divider {side}",50,side*80-30,side*80+30,0,28,gap=(side*80-9,side*80+9))
    wall_z(f"north court divider {side}",-70,side*75-35,side*75+35,0,28,gap=(side*75-10,side*75+10))
    wall_x(f"south court elbow {side}",side*40,70,126,0,28)
    box_obstacle(f"courtyard reactor {side}",side*50-10,side*50+10,0,28,-18,18,"labpanel.bmp")
    box_obstacle(f"south court crate {side}",side*125-8,side*125+8,0,10,98,114,"hazard.bmp")
# Offset tall cover in the central south court.
box_obstacle("south central screen",-16,16,0,28,76,84,"bulkhead.bmp")

for side in (-1,1):
    wall_z(f"north spawn screen {side}",-112,side*138-16,side*138+16,0,28)
    wall_x(f"south spawn screen {side}",side*120,118,146,0,28)

DM_SPAWNS = [(-140,2,-130),(140,2,-130),(-150,2,130),(150,2,130),
             (-60,2,110),(60,2,110),(-140,34,40),(140,34,40),
             (-60,34,-116),(0,66,-132)]
for idx,pos in enumerate(DM_SPAWNS):
    point("raw",pos,rotation=face(-pos[0],-pos[2]),p1=60,p4=idx)
point("player_spawn",(-150,2,130),rotation=math.pi,human_type=43)
for pos,wid,ammo in [((-130,2,-120),1,90),((130,2,-120),9,90),
                      ((-150,2,110),10,90),((150,2,110),3,60),
                      ((0,2,40),13,4),((60,34,-116),26,50),
                      ((0,66,-132),23,20),((-124,34,0),24,50)]:
    point("weapon_pickup",pos,weapon_id=wid,ammo=ammo)

def build_doc():
    return {"schema":1,"textures":TEXTURES,"blocks":BLOCKS,"points":POINTS}

def main():
    path=HERE / "dm_spire.json"
    path.write_text(json.dumps(build_doc(),indent=2)+"\n")
    print(f"wrote {path}: {len(BLOCKS)} blocks, {len(POINTS)} points")

if __name__ == "__main__":
    main()
