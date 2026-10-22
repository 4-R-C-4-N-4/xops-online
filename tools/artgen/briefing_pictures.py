#!/usr/bin/env python3
"""Procedural briefing-picture generator (CC0, no copied content).

Renders a stylised top-down tactical map for a mission briefing picture
directly from the authored map source JSON (tools/mapgen/examples/*.json),
so the image always matches the actual level layout.

The engine draws the briefing picture with Draw2DTextureScaling into a
160x150 on-screen box (src/gamemain.cpp ~1726-1732) whenever the mission
script's picture line (src/datafile.cpp:803-824) names a
data/briefing/<name>.bmp file instead of "!". We render at 256x256 (a
power-of-two size consistent with the other bmp8 texture slots in
docs/ASSET-CONTRACT.md / assets/manifest.json) and let the engine scale it
down into the briefing box; this avoids baking in a non-POT texture while
still rendering a legible map at generation time.

Orientation: the player spawns at +Z, facing into the compound (toward
-Z), so the generated picture puts the spawn marker near the bottom of the
image and the rest of the compound above it, matching that south-facing
view.
"""
from __future__ import annotations

import json
import sys
from pathlib import Path
from typing import Dict, List, Tuple

from PIL import Image, ImageDraw

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
sys.path.insert(0, str(ROOT))

from tools.artgen.skybox import write_bmp8  # noqa: E402

SIZE = 256

# World-space XZ bounds the map is rendered within (matches the starter
# compound's outer perimeter walls with a small margin).
WORLD_MIN = (-220.0, -220.0)
WORLD_MAX = (220.0, 220.0)

BG = (46, 50, 28)          # dark olive background
FLOOR = (74, 80, 46)       # compound floor
WALL = (120, 112, 84)      # buildings / walls
WALL_EDGE = (40, 36, 24)
GRID = (58, 62, 36)
SPAWN = (235, 230, 90)
ENEMY = (198, 42, 36)
PICKUP = (72, 168, 210)
COW = (235, 235, 235)
HATCH = (170, 90, 220)
TEXT = (230, 226, 210)


def world_to_px(x: float, z: float, world_min=WORLD_MIN, world_max=WORLD_MAX) -> Tuple[int, int]:
    """Map world XZ to image pixels.

    North (-Z, away from the player) is up; south (+Z, the player's
    spawn side) is down, matching the in-game spawn-facing-north view.
    """
    wx0, wz0 = world_min
    wx1, wz1 = world_max
    u = (x - wx0) / (wx1 - wx0)
    v = (z - wz0) / (wz1 - wz0)
    px = int(round(u * (SIZE - 1)))
    py = int(round(v * (SIZE - 1)))  # +Z (south/spawn) -> bottom of image
    return px, py


def load_map(json_path: Path) -> Dict:
    with open(json_path, "r", encoding="utf-8") as f:
        return json.load(f)


def draw_grid(draw: ImageDraw.ImageDraw, step: int = 32) -> None:
    for x in range(0, SIZE, step):
        draw.line([(x, 0), (x, SIZE - 1)], fill=GRID, width=1)
    for y in range(0, SIZE, step):
        draw.line([(0, y), (SIZE - 1, y)], fill=GRID, width=1)


def draw_blocks(draw: ImageDraw.ImageDraw, blocks: List[Dict],
                 world_min=WORLD_MIN, world_max=WORLD_MAX) -> None:
    for i, blk in enumerate(blocks):
        if "min" not in blk or "max" not in blk:
            continue  # raw-vertex (non-box) blocks, e.g. the UFO hull, are skipped
        mn = blk["min"]
        mx = blk["max"]
        x0, z0 = mn[0], mn[2]
        x1, z1 = mx[0], mx[2]
        p0 = world_to_px(x0, z0, world_min, world_max)
        p1 = world_to_px(x1, z1, world_min, world_max)
        rect = [min(p0[0], p1[0]), min(p0[1], p1[1]),
                max(p0[0], p1[0]), max(p0[1], p1[1])]
        if i == 0:
            # Floor slab: fill the whole footprint as the base ground.
            draw.rectangle(rect, fill=FLOOR)
        else:
            draw.rectangle(rect, fill=WALL, outline=WALL_EDGE, width=1)


def draw_marker_arrow(draw: ImageDraw.ImageDraw, cx: int, cy: int,
                       rotation: float, color, size: int = 7) -> None:
    import math
    # rotation 0 points toward -Z (up/north) in the engine's convention
    # used by the other mapgen tools; the starter spawn uses pi, i.e.
    # facing north into the compound. Draw a simple triangle arrow.
    dx = -math.sin(rotation)
    dz = -math.cos(rotation)
    tip = (cx + dx * size, cy + dz * size)
    back = (cx - dx * size * 0.6, cy - dz * size * 0.6)
    perp = (-dz, dx)
    left = (back[0] + perp[0] * size * 0.5, back[1] + perp[1] * size * 0.5)
    right = (back[0] - perp[0] * size * 0.5, back[1] - perp[1] * size * 0.5)
    draw.polygon([tip, left, right], fill=color, outline=(0, 0, 0))


def draw_dot(draw: ImageDraw.ImageDraw, cx: int, cy: int, color,
             r: int = 4) -> None:
    draw.ellipse([cx - r, cy - r, cx + r, cy + r], fill=color,
                 outline=(0, 0, 0))


def draw_pickup(draw: ImageDraw.ImageDraw, cx: int, cy: int, color,
                 r: int = 4) -> None:
    draw.rectangle([cx - r, cy - r, cx + r, cy + r], fill=color,
                   outline=(0, 0, 0))


def _text(draw: ImageDraw.ImageDraw, xy, s: str, color) -> None:
    draw.text(xy, s, fill=color)


def render_briefing_picture(map_json: Path, label: str,
                             world_min=WORLD_MIN, world_max=WORLD_MAX) -> Image.Image:
    data = load_map(map_json)
    blocks = data["blocks"]
    points = data.get("points", [])

    img = Image.new("RGB", (SIZE, SIZE), BG)
    draw = ImageDraw.Draw(img)

    draw_grid(draw)
    draw_blocks(draw, blocks, world_min, world_max)

    for pt in points:
        t = pt.get("type")
        pos = pt.get("pos", [0.0, 0.0, 0.0])
        px, py = world_to_px(pos[0], pos[2], world_min, world_max)
        if t == "player_spawn":
            draw_marker_arrow(draw, px, py, pt.get("rotation", 0.0), SPAWN)
        elif t == "human" and pt.get("team") == 2:
            draw_dot(draw, px, py, ENEMY)
        elif t == "weapon_pickup":
            draw_pickup(draw, px, py, PICKUP)
        elif t == "small_object":
            draw_dot(draw, px, py, COW, r=3)
        elif t == "staged_human":
            draw_dot(draw, px, py, HATCH, r=2)

    draw.rectangle([0, 0, SIZE - 1, SIZE - 1], outline=(0, 0, 0), width=2)
    _text(draw, (8, 6), label, TEXT)

    # Build an 8-bit paletted image deterministically from the small fixed
    # palette of colors used above, so the same inputs always produce the
    # same bytes.
    p_img = img.convert(
        "P", palette=Image.Palette.ADAPTIVE, colors=256, dither=Image.NONE
    )
    return p_img


def generate_starter(out_dir: Path) -> Path:
    map_json = ROOT / "tools" / "mapgen" / "examples" / "starter_compound.json"
    img = render_briefing_picture(map_json, "STARTER COMPOUND")
    out_path = out_dir / "starter.bmp"
    out_dir.mkdir(parents=True, exist_ok=True)
    write_bmp8(img, out_path)
    return out_path


def generate_for(map_json: Path, label: str, out_path: Path,
                  world_min=WORLD_MIN, world_max=WORLD_MAX) -> Path:
    """Generic entry point: any mapgen author JSON -> a briefing .bmp."""
    img = render_briefing_picture(map_json, label, world_min, world_max)
    out_path.parent.mkdir(parents=True, exist_ok=True)
    write_bmp8(img, out_path)
    return out_path


def generate_ufofield(out_dir: Path) -> Path:
    out_dir.mkdir(parents=True, exist_ok=True)
    image = render_ufofield_plan(768)
    write_bmp8(image.resize((SIZE, SIZE), Image.Resampling.LANCZOS).convert(
        "P", palette=Image.Palette.ADAPTIVE, colors=256), out_dir / "ufofield.bmp")
    return out_dir / "ufofield.bmp"


def render_ufofield_plan(size=768):
    """North-up roof-cutaway plan: show barn doors, loft and hay steps."""
    doc = load_map(ROOT / "tools/mapgen/examples/ufo_field.json")
    img = Image.new("RGB", (size, size + 70), (25, 30, 25))
    draw = ImageDraw.Draw(img)
    def project(x, z):
        return (int((x+310)/620*size), int((z+430)/740*size))
    colors = {"grass.bmp": (76,105,57), "dirt.bmp": (133,111,77),
              "hay.bmp": (208,179,87), "barn_red.bmp": (150,65,53),
              "fence.bmp": (168,139,104), "trim_white.bmp": (223,215,192)}
    for b in doc['blocks']:
        name = b.get('comment', '')
        if name == 'barn roof' or name == 'barn door lintel' or name == 'ufo dome':
            continue
        color = colors.get(b.get('texture'), (122,155,163))
        if 'min' in b:
            lo,hi=b['min'],b['max']
            draw.rectangle([project(lo[0],lo[2]),project(hi[0],hi[2])], fill=color)
        else:
            vs=b['vertices'][:4]
            draw.polygon([project(v[0],v[2]) for v in vs], fill=color)
    for pt in doc['points']:
        if pt['type'] not in ('small_object','player_spawn'):
            continue
        x,y=project(pt['pos'][0],pt['pos'][2])
        color=(245,245,225) if pt['type']=='small_object' else (80,235,255)
        draw.ellipse((x-4,y-4,x+4,y+4),fill=color,outline=(20,20,20))
    for text,x,z in [('BARN / LOFT 36',-68,-375),('GATE',-158,-238),
                      ('GATE',105,-238),('UFO / 32',-40,-165),('SPAWN',12,204)]:
        draw.text(project(x,z),text,fill=(255,255,255),stroke_width=1,stroke_fill=(25,25,25))
    draw.text((12,size+8),'UFO FIELD | NORTH UP | barn roof omitted to show interior',fill='white')
    draw.text((12,size+27),'Red: 96-high boundary stores / Gold: hay, 12-unit steps and 36-high loft',fill='white')
    draw.text((12,size+46),'White: cows / Cyan: spawn / Continuous ground beneath every route',fill='white')
    return img


def generate_ufofield_layout(path):
    path.parent.mkdir(parents=True, exist_ok=True)
    render_ufofield_plan().save(path)
    return path


def generate_ufoescape(out_dir: Path) -> Path:
    map_json = ROOT / "tools" / "mapgen" / "examples" / "ufo_escape.json"
    # The route runs roughly x:[-100,580] z:[-200,420]; pad a little.
    return generate_for(map_json, "UFO ESCAPE", out_dir / "ufoescape.bmp",
                         world_min=(-120.0, -210.0), world_max=(600.0, 430.0))


def generate_dmhangar(out_dir: Path) -> Path:
    map_json = ROOT / "tools" / "mapgen" / "examples" / "dm_hangar.json"
    # Include the rebuilt hangar and all four side rooms.
    return generate_for(map_json, "DM HANGAR", out_dir / "dmhangar.bmp",
                         world_min=(-215.0, -215.0), world_max=(215.0, 215.0))


def generate_dmfield(out_dir: Path) -> Path:
    map_json = ROOT / "tools" / "mapgen" / "examples" / "dm_field.json"
    # Same footprint as UFO FIELD.
    return generate_for(map_json, "DM FIELD", out_dir / "dmfield.bmp",
                         world_min=(-310.0, -430.0), world_max=(310.0, 310.0))


def generate_dmshipdeck(out_dir: Path) -> Path:
    map_json = ROOT / "tools" / "mapgen" / "examples" / "dm_shipdeck.json"
    # Footprint is 360x300 (+-180 x, +-150 z); pad a little.
    return generate_for(map_json, "DM SHIPDECK", out_dir / "dmship.bmp",
                         world_min=(-200.0, -170.0), world_max=(200.0, 170.0))
def render_spire_plan(level=32, size=384):
    """Height-sliced original plan; omit the shell roof to reveal routes."""
    doc = load_map(ROOT / "tools/mapgen/examples/dm_spire.json")
    img = Image.new("RGB", (size, size), (18, 25, 36))
    draw = ImageDraw.Draw(img)
    def project(x, z):
        return (int((x + 172) / 344 * size), int((z + 172) / 344 * size))
    for block in doc["blocks"]:
        if "vertices" in block:
            vs = block["vertices"][:4]
            if min(v[1] for v in vs) <= level <= max(v[1] for v in vs):
                draw.polygon([project(v[0], v[2]) for v in vs], fill=(155, 130, 62))
            continue
        lo, hi = block["min"], block["max"]
        if hi[1] == level and hi[1] - lo[1] <= 2:
            color = (55, 100, 126)
        elif lo[1] <= level + 18 and hi[1] > level + 2:
            color = (120, 134, 145)
        else:
            continue
        draw.rectangle([project(lo[0], lo[2]), project(hi[0], hi[2])], fill=color)
    for pt in doc["points"]:
        x,y,z = pt["pos"]
        px,py = project(x,z)
        if pt.get("p1") == 61 and y <= level <= y + pt["p3"]:
            draw.ellipse((px-5,py-5,px+5,py+5), outline=(60,255,220), width=2)
        elif abs(y-level) <= 3:
            color = (255,220,90) if pt.get("p1") == 60 else (255,125,80)
            draw.ellipse((px-3,py-3,px+3,py+3), fill=color)
    draw.text((10, 8), f"DM SPIRE / Y={level}", fill=(240,240,240))
    return img


def generate_dmspire(out_dir: Path) -> Path:
    out_dir.mkdir(parents=True, exist_ok=True)
    image = render_spire_plan().resize((SIZE, SIZE), Image.Resampling.NEAREST)
    path = out_dir / "dmspire.bmp"
    write_bmp8(image.convert("P", palette=Image.Palette.ADAPTIVE, colors=256), path)
    return path


def generate_spire_layout(path: Path) -> Path:
    image = Image.new("RGB", (1152, 416), (18,25,36))
    for i, level in enumerate((0,32,64)):
        image.paste(render_spire_plan(level), (i*384,0))
    ImageDraw.Draw(image).text((12,395),
        "NORTH UP | gold: ramps / yellow: spawns / orange: weapons / cyan: lifts | safe drops: 32 units",
        fill=(240,240,240))
    path.parent.mkdir(parents=True, exist_ok=True)
    image.save(path)
    return path


def main() -> int:
    out_dir = ROOT / "assets" / "pack" / "data" / "briefing"
    path = generate_starter(out_dir)
    print(f"wrote {path}")
    generate_ufofield_layout(ROOT / "docs/img/ufofield_layout.png")
    path2 = generate_ufofield(out_dir)
    print(f"wrote {path2}")
    path3 = generate_ufoescape(out_dir)
    print(f"wrote {path3}")
    path4 = generate_dmhangar(out_dir)
    print(f"wrote {path4}")
    path5 = generate_dmfield(out_dir)
    print(f"wrote {path5}")
    path6 = generate_dmshipdeck(out_dir)
    print(f"wrote {path6}")
    print(f"wrote {generate_dmspire(out_dir)}")
    print(f"wrote {generate_spire_layout(ROOT / 'docs/img/dm/spire_layout.png')}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
