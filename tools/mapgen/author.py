"""JSON authoring format for maps: compiles to .bd1 + .pd1 (+ optional
.msg) pairs, and decompiles back to the same JSON for round-tripping
("dump").

This module only *authors* convenient structures; the on-disk byte
format is entirely owned by bd1.py / pd1.py, which encode exactly what
the engine loaders expect (see their docstrings and docs/MAPGEN.md for
source citations).
"""

from __future__ import annotations

from dataclasses import dataclass
from typing import Any, Dict, List, Optional

from . import bd1, pd1, zfight

SCHEMA_VERSION = 1


class AuthorError(ValueError):
    pass


# ---------------------------------------------------------------------------
# Blocks
# ---------------------------------------------------------------------------

def _build_block(entry: Dict[str, Any], textures: List[str]) -> bd1.Block:
    if "vertices" in entry:
        verts = entry["vertices"]
        if len(verts) != 8:
            raise AuthorError("raw block 'vertices' must have exactly 8 [x,y,z] entries")
        xs = tuple(v[0] for v in verts)
        ys = tuple(v[1] for v in verts)
        zs = tuple(v[2] for v in verts)
        faces_in = entry.get("faces")
        if not faces_in or len(faces_in) != 6:
            raise AuthorError("raw block needs 'faces': 6 entries, each {texture,u,v}")
        faces = []
        for f in faces_in:
            tid = _texture_index(f.get("texture"), textures)
            u = tuple(f.get("u", (0.0, 0.0, 1.0, 1.0)))
            v = tuple(f.get("v", (0.0, 0.0, 1.0, 1.0)))
            faces.append(bd1.FaceMaterial(texture_id=tid, u=u, v=v))
        return bd1.Block(x=xs, y=ys, z=zs, faces=faces)

    if "min" in entry and "max" in entry:
        vmin = entry["min"]
        vmax = entry["max"]
        faces_spec = entry.get("faces", {})
        face_tex = []
        for name in bd1.FACE_NAMES:
            spec = faces_spec.get(name, entry.get("texture"))
            face_tex.append(_texture_index(spec, textures))
        uv_scale = entry.get("uv_scale", [1.0, 1.0])
        block = bd1.box_block(vmin, vmax, face_tex, uv_scale=uv_scale)
        if 'texture_world_units' in entry:
            import math
            units = float(entry['texture_world_units'])
            if not math.isfinite(units) or units <= 0:
                raise AuthorError('texture_world_units must be positive and finite')
            dx, dy, dz = (vmax[i] - vmin[i] for i in range(3))
            # Box faces have different extents; do not stretch one square
            # texture across a long wall or a whole courtyard floor.
            dimensions = ((dx,dz),(dx,dz),(dx,dy),(dz,dy),(dx,dy),(dz,dy))
            normalized = bd1.box_block(vmin, vmax, face_tex)
            for face, base, (width,height) in zip(block.faces, normalized.faces, dimensions):
                face.u = tuple(value * width / units for value in base.u)
                face.v = tuple(value * height / units for value in base.v)
        return block

    raise AuthorError("block entry needs either 'min'/'max' (box) or 'vertices' (raw)")


def _texture_index(spec: Any, textures: List[str]) -> int:
    if spec is None:
        return -1
    if isinstance(spec, bool):
        raise AuthorError("texture reference must be an int index or a string name")
    if isinstance(spec, int):
        return spec
    if isinstance(spec, str):
        if spec not in textures:
            raise AuthorError(f"unknown texture name {spec!r}; not in 'textures' list")
        return textures.index(spec)
    raise AuthorError(f"bad texture reference: {spec!r}")


def _block_to_json(block: bd1.Block, textures: List[str]) -> Dict[str, Any]:
    faces = []
    for name, face in zip(bd1.FACE_NAMES, block.faces):
        tex = textures[face.texture_id] if 0 <= face.texture_id < len(textures) else face.texture_id
        faces.append({"face": name, "texture": tex, "u": list(face.u), "v": list(face.v)})
    return {
        "vertices": [[block.x[i], block.y[i], block.z[i]] for i in range(8)],
        "faces": faces,
    }


# ---------------------------------------------------------------------------
# Points
# ---------------------------------------------------------------------------

# Friendly point kinds -> (p1, field order). See pd1.py docstring for
# the source citations behind each mapping.
def _build_point(entry: Dict[str, Any]) -> pd1.Point:
    kind = entry.get("type")
    pos = entry.get("pos", [0.0, 0.0, 0.0])
    x, y, z = pos
    r = entry.get("rotation", 0.0)

    if kind == "player_spawn":
        # HUMAN2 with p4=0 is the controlled player (objectmanager.cpp:269,1486).
        # p2 here is still the *authored* human_type; _synthesize_human_info()
        # rewrites it to a HUMANINFO foreign key after all points are built
        # (objectmanager.cpp:1461-1498 requires a HUMANINFO companion for the
        # real in-game team, or every human silently gets TeamID=0 -- see
        # that function's docstring for the bug this fixes).
        return pd1.Point(x, y, z, r, p1=pd1.P1_HUMAN2,
                          p2=entry.get("human_type", 0),
                          p3=entry.get("ai_path", 0),
                          p4=0)
    if kind == "human":
        # Enemy/NPC: HUMAN2, p4 != 0 so it is not auto-selected as player.
        team = entry.get("team", 1)
        if team == 0:
            raise AuthorError("human 'team' must be non-zero (0 is reserved for player_spawn)")
        return pd1.Point(x, y, z, r, p1=pd1.P1_HUMAN2,
                          p2=entry.get("human_type", 0),
                          p3=entry.get("ai_path", 0),
                          p4=team)
    if kind == "weapon_pickup":
        # WEAPON: p2 weapon id, p3 ammo count (objectmanager.cpp:384-385).
        return pd1.Point(x, y, z, r, p1=pd1.P1_WEAPON,
                          p2=entry.get("weapon_id", 0),
                          p3=entry.get("ammo", 0),
                          p4=entry.get("p4", 0))
    if kind == "waypoint":
        # AIPATH: p2 mode (event.cpp:179-181).
        return pd1.Point(x, y, z, r, p1=pd1.P1_AIPATH,
                          p2=entry.get("mode", 0),
                          p3=entry.get("p3", 0),
                          p4=entry.get("p4", 0))
    if kind == "small_object":
        # SMALLOBJ: p2 object type, p3 nonzero = extra map collision
        # snap (objectmanager.cpp:477-493).
        return pd1.Point(x, y, z, r, p1=pd1.P1_SMALLOBJ,
                          p2=entry.get("object_type", 0),
                          p3=1 if entry.get("map_collision", True) else 0,
                          p4=entry.get("p4", 0))
    if kind == "event":
        # 10..19 are the engine's recognized "event" p1 range
        # (gamemain.cpp:4022); p2/p3/p4 semantics per-event are not
        # resolved from source read for this task, so they pass through
        # as raw ints.
        event_id = entry.get("event_id")
        if event_id is None or not (pd1.P1_EVENT_MIN <= event_id <= pd1.P1_EVENT_MAX):
            raise AuthorError(f"event_id must be in [{pd1.P1_EVENT_MIN}, {pd1.P1_EVENT_MAX}]")
        return pd1.Point(x, y, z, r, p1=event_id,
                          p2=entry.get("p2", 0), p3=entry.get("p3", 0), p4=entry.get("p4", 0))
    if kind == "raw":
        return pd1.Point(x, y, z, r,
                          p1=entry["p1"], p2=entry.get("p2", 0),
                          p3=entry.get("p3", 0), p4=entry.get("p4", 0))
    if kind == "staged_human":
        # POINT_P1TYPE_STAGED_HUMAN = 50 (pd1.P1_STAGED_HUMAN):
        # docs/MISSION-UFO-FIELD.md E5 -- "same parameters as type 1":
        # p2 = HUMANINFO point id, p3 = AI path, p4 = wave group id.
        # p2 here is still the raw authored human_type; like player_spawn
        # / human it gets rewritten to a HUMANINFO foreign key by
        # _synthesize_human_info(), keyed on (human_type, team). 'team'
        # is not one of p1..p4 for this point kind (p4 is the wave group
        # id instead), so it is stashed as a plain Python attribute on
        # the Point until synthesis resolves it.
        team = entry.get("team", 1)
        if team == 0:
            raise AuthorError("staged_human 'team' must be non-zero")
        pt = pd1.Point(x, y, z, r, p1=pd1.P1_STAGED_HUMAN,
                        p2=entry.get("human_type", 0),
                        p3=entry.get("ai_path", 0),
                        p4=entry.get("wave", 1))
        setattr(pt, "_pending_team", team)  # consumed by _synthesize_human_info
        return pt

    raise AuthorError(f"unknown point type {kind!r}")


def _synthesize_human_info(points: List[pd1.Point]) -> List[pd1.Point]:
    """Give every HUMAN2 point a HUMANINFO companion point.

    Without one, ``ObjectManager::AddHumanIndex``'s point-list scan
    (objectmanager.cpp:1461-1498) fails its
    ``SearchPointdata(..., POINT_P1TYPE_HUMANINFO, ..., data.p2, ...)``
    lookup (datafile.h's mask-0x01|0x08 search matches on p1 AND p4,
    so it is really keyed by "HUMANINFO.p4 == HUMAN2.p2") and falls
    into the "mystery human" fallback (objectmanager.cpp:1468-1491),
    which calls ``AddHumanIndex(..., TeamID=0, ...)`` with a
    **hardcoded** team of 0 for every human regardless of the
    authored ``team``/p4 value. That made every spawned human
    (player and all enemies alike) share team 0, so
    ``ObjectManager::CheckGameOverorComplete`` (objectmanager.cpp:2575)
    counted zero enemies and returned "mission complete" on the very
    first frame -- the root cause of the starter mission ending
    immediately with no player control.

    This rewrites each HUMAN2 point's ``p2`` (previously the raw,
    author-facing ``human_type``) into a synthesized HUMANINFO foreign
    key, and appends one HUMANINFO point per distinct
    (human_type, team) pair: HUMANINFO.p2 = human_type (model id into
    ``ParameterInfo::Human[]``, parameter.cpp), HUMANINFO.p3 = team
    (the real team used by ``AddHumanIndex(pointdata, pointdata, bool)``
    at objectmanager.cpp:215, via ``SetParamData(infodata.p2, ...,
    infodata.p3, true)``), HUMANINFO.p4 = the synthesized key.
    """
    key_to_id: Dict[Any, int] = {}
    next_id = 1

    extra_keys: List[Any] = []
    extra_pos: Dict[Any, Any] = {}
    out: List[pd1.Point] = []
    for p in points:
        if p.p1 == pd1.P1_HUMAN2:
            key = (p.p2, p.p4)
            if key not in key_to_id:
                key_to_id[key] = next_id
                extra_keys.append(key)
                extra_pos[key] = (p.x, p.y, p.z)
                next_id += 1
            out.append(pd1.Point(p.x, p.y, p.z, p.r, p1=p.p1,
                                  p2=key_to_id[key], p3=p.p3, p4=p.p4))
        elif p.p1 == pd1.P1_STAGED_HUMAN:
            # p2 is still the raw human_type; team came through as a
            # stashed attribute (see _build_point's "staged_human" case)
            # since p3/p4 are already taken by ai_path/wave group.
            team = getattr(p, "_pending_team", 1)
            key = (p.p2, team)
            if key not in key_to_id:
                key_to_id[key] = next_id
                extra_keys.append(key)
                extra_pos[key] = (p.x, p.y, p.z)
                next_id += 1
            out.append(pd1.Point(p.x, p.y, p.z, p.r, p1=p.p1,
                                  p2=key_to_id[key], p3=p.p3, p4=p.p4))
        else:
            out.append(p)
    extra = []
    for (human_type, team) in extra_keys:
        ex, ey, ez = extra_pos[(human_type, team)]
        extra.append(pd1.Point(ex, ey, ez, 0.0, p1=pd1.P1_HUMANINFO,
                                p2=human_type, p3=team,
                                p4=key_to_id[(human_type, team)]))
    return out + extra


def _point_to_json(p: pd1.Point, info_by_id: Optional[Dict[int, pd1.Point]] = None) -> Dict[str, Any]:
    pos = [p.x, p.y, p.z]
    if p.p1 == pd1.P1_HUMAN2:
        # p.p2 is a HUMANINFO foreign key (see _synthesize_human_info);
        # resolve the authored human_type back out of its companion
        # point when one was found, else fall back to the raw value.
        info = (info_by_id or {}).get(p.p2)
        human_type = info.p2 if info is not None else p.p2
        if p.p4 == 0:
            return {"type": "player_spawn", "pos": pos, "rotation": p.r,
                    "human_type": human_type, "ai_path": p.p3}
        return {"type": "human", "pos": pos, "rotation": p.r,
                "human_type": human_type, "ai_path": p.p3, "team": p.p4}
    if p.p1 == pd1.P1_WEAPON:
        return {"type": "weapon_pickup", "pos": pos, "rotation": p.r,
                "weapon_id": p.p2, "ammo": p.p3, "p4": p.p4}
    if p.p1 == pd1.P1_AIPATH:
        return {"type": "waypoint", "pos": pos, "rotation": p.r,
                "mode": p.p2, "p3": p.p3, "p4": p.p4}
    if p.p1 == pd1.P1_SMALLOBJ:
        return {"type": "small_object", "pos": pos, "rotation": p.r,
                "object_type": p.p2, "map_collision": bool(p.p3), "p4": p.p4}
    if p.p1 == pd1.P1_STAGED_HUMAN:
        info = (info_by_id or {}).get(p.p2)
        human_type = info.p2 if info is not None else p.p2
        team = info.p3 if info is not None else 1
        return {"type": "staged_human", "pos": pos, "rotation": p.r,
                "human_type": human_type, "team": team,
                "ai_path": p.p3, "wave": p.p4}
    if pd1.P1_EVENT_MIN <= p.p1 <= pd1.P1_EVENT_MAX:
        return {"type": "event", "pos": pos, "rotation": p.r,
                "event_id": p.p1, "p2": p.p2, "p3": p.p3, "p4": p.p4}
    return {"type": "raw", "pos": pos, "rotation": p.r,
            "p1": p.p1, "p2": p.p2, "p3": p.p3, "p4": p.p4}


# ---------------------------------------------------------------------------
# Top-level build / dump
# ---------------------------------------------------------------------------

@dataclass
class BuildResult:
    bd1_bytes: bytes
    pd1_bytes: bytes


def build(doc: Dict[str, Any]) -> BuildResult:
    textures = doc.get("textures", [])
    if not isinstance(textures, list):
        raise AuthorError("'textures' must be a list of strings")

    blocks = [_build_block(b, textures) for b in doc.get("blocks", [])]
    if doc.get("resolve_zfighting", True):
        # Coplanar overlapping faces flicker in the depth buffer; inset the
        # smaller face slightly (see zfight.py). Idempotent.
        blocks, _moved = zfight.resolve(blocks)
    bd1_file = bd1.BD1File(textures=textures, blocks=blocks)

    points = [_build_point(p) for p in doc.get("points", [])]
    points = _synthesize_human_info(points)
    pd1_file = pd1.PD1File(points=points)

    return BuildResult(bd1_bytes=bd1.dumps(bd1_file), pd1_bytes=pd1.dumps(pd1_file))


def dump_bd1(data: bytes) -> Dict[str, Any]:
    f = bd1.loads(data)
    return {
        "schema": SCHEMA_VERSION,
        "textures": f.textures,
        "blocks": [_block_to_json(b, f.textures) for b in f.blocks],
    }


def dump_pd1(data: bytes) -> Dict[str, Any]:
    f = pd1.loads(data)
    # HUMANINFO points are synthesized companions (see
    # _synthesize_human_info); resolve HUMAN2 points through them and
    # drop them from the decompiled JSON so round-tripping through
    # author.build() reproduces the same authored input and bytes.
    info_by_id = {p.p4: p for p in f.points if p.p1 == pd1.P1_HUMANINFO}
    points = [_point_to_json(p, info_by_id) for p in f.points if p.p1 != pd1.P1_HUMANINFO]
    return {"schema": SCHEMA_VERSION, "points": points}
