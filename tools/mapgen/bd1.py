"""Reader/writer for OpenXOPS ``.bd1`` block-data files.

Byte layout derived from ``BlockDataInterface::LoadFiledata``
(src/datafile.cpp:109-176) and the ``blockdata``/``b_material`` structs
(src/datafile.h:60-82). All offsets below cite that function.

File layout (little-endian throughout):

  offset 0        10 * 31 bytes   texture names, each a fixed 31-byte
                                   field read with a raw ``fread`` (no
                                   length prefix) and later used with
                                   ``strcpy`` (datafile.cpp:128-130), so
                                   each name must be NUL-terminated
                                   within its 31 bytes (<=30 chars).
  offset 310      2 bytes         block count, uint16 LE
                                   (datafile.cpp:133-135), clamped to
                                   MAX_BLOCKS=160 (datafile.h:35) on load.
  offset 312+     316 bytes each  one record per block (see below).

Each 316-byte block record is two back-to-back freads
(datafile.cpp:140-141):

  * 72 * float32 LE (288 bytes), consumed into local array
    ``bdata_main[80]`` (only the first 72 slots are used):
      [0:8]   x[0..7]          vertex X (datafile.cpp:148)
      [8:16]  y[0..7]          vertex Y (datafile.cpp:149)
      [16:24] z[0..7]          vertex Z (datafile.cpp:150)
      [24:48] u[face][0..3]    4 U texcoords per face, face-major
                                (datafile.cpp:156-163: u[0] at
                                24+face*4, u[1] at 25+face*4, etc.)
      [48:72] v[face][0..3]    4 V texcoords per face, same layout
                                offset by 24 (datafile.cpp:157-163)
  * 28 * int8 (28 bytes), consumed into ``bdata_mainb[30]``:
      textureID for face j is the signed byte at offset j*4
      (datafile.cpp:155: ``bdata_mainb[j*4]``); the other 3 bytes of
      each 4-byte slot, and the trailing 4 bytes, are never read back
      (unused padding here).

Vertex layout (ASCII cube, from the blockdataface() comment,
datafile.cpp:303-323)::

        3 .----. 2
         /|    /|
        0 .----. 1 |
      7 . -|  --. 6
        |  /   | /
        4 .----. 5

Face -> vertex index mapping, and the (fixed) UV-slot mapping that
every face uses, both from blockdataface() (datafile.cpp:324-381):

  face 0 top     vertices (1,0,3,2)   uv slots (2,3,0,1)
  face 1 bottom  vertices (6,7,4,5)   uv slots (2,3,0,1)
  face 2 front   vertices (5,4,0,1)   uv slots (2,3,0,1)
  face 3 right   vertices (6,5,1,2)   uv slots (2,3,0,1)
  face 4 back    vertices (7,6,2,3)   uv slots (2,3,0,1)
  face 5 left    vertices (4,7,3,0)   uv slots (2,3,0,1)

"uv slots (2,3,0,1)" means: the texcoord applied to the face's first
listed vertex is u[face][2],v[face][2]; to the second vertex,
u[face][3],v[face][3]; to the third, u[face][0],v[face][0]; to the
fourth, u[face][1],v[face][1]. This module stores/accepts UVs already
in file-slot order (u[face][0..3], v[face][0..3]) -- callers that think
in per-vertex order must apply the same uv-slot permutation themselves;
the ``blockgeom`` helper below does this for the box-authoring path.
"""

from __future__ import annotations

import struct
from dataclasses import dataclass, field
from typing import List, Sequence, Tuple

TOTAL_BLOCKTEXTURE = 10       # datafile.h:36
MAX_BLOCKS = 400              # datafile.h:35 (XOPS ONLINE raised 160 -> 400)
TEXNAME_FIELD_LEN = 31        # datafile.cpp:129 (fread(..., 1, 31, fp))
TEXNAME_MAX_CHARS = TEXNAME_FIELD_LEN - 1  # must leave room for the NUL

HEADER_SIZE = TOTAL_BLOCKTEXTURE * TEXNAME_FIELD_LEN  # 310
BLOCK_RECORD_SIZE = 72 * 4 + 28  # 288 + 28 = 316

# Face -> (vertex indices, uv-slot indices), datafile.cpp:324-381.
FACE_VERTEX_MAP: Tuple[Tuple[int, int, int, int], ...] = (
    (1, 0, 3, 2),   # 0 top
    (6, 7, 4, 5),   # 1 bottom
    (5, 4, 0, 1),   # 2 front
    (6, 5, 1, 2),   # 3 right
    (7, 6, 2, 3),   # 4 back
    (4, 7, 3, 0),   # 5 left
)
FACE_UVSLOT_MAP: Tuple[int, int, int, int] = (2, 3, 0, 1)
FACE_NAMES = ("top", "bottom", "front", "right", "back", "left")


class BD1Error(ValueError):
    """Raised for malformed .bd1 data or author input."""


@dataclass
class FaceMaterial:
    """One block face: texture id + the 4 file-order UV pairs.

    ``texture_id`` is a *signed* byte (datafile.cpp:155,
    ``(int)bdata_mainb[j*4]``) so valid values are -128..127; the engine
    treats <0 or an out-of-range id as "no texture" at render time
    (not decoded further here -- see docs/MAPGEN.md).
    ``u``/``v`` are each a 4-tuple in *file slot order* (slot 0..3), not
    per-vertex order; see module docstring.
    """

    texture_id: int = -1
    u: Tuple[float, float, float, float] = (0.0, 0.0, 1.0, 1.0)
    v: Tuple[float, float, float, float] = (0.0, 0.0, 1.0, 1.0)

    def __post_init__(self):
        if not (-128 <= self.texture_id <= 127):
            raise BD1Error(f"texture_id {self.texture_id} out of signed-byte range")
        if len(self.u) != 4 or len(self.v) != 4:
            raise BD1Error("u/v must each have exactly 4 values")


@dataclass
class Block:
    """One 8-vertex block with 6 faces (datafile.h:74-82)."""

    x: Tuple[float, float, float, float, float, float, float, float]
    y: Tuple[float, float, float, float, float, float, float, float]
    z: Tuple[float, float, float, float, float, float, float, float]
    faces: List[FaceMaterial] = field(default_factory=list)

    def __post_init__(self):
        for name, arr in (("x", self.x), ("y", self.y), ("z", self.z)):
            if len(arr) != 8:
                raise BD1Error(f"block {name} must have exactly 8 vertices")
        if len(self.faces) != 6:
            raise BD1Error("block must have exactly 6 faces")


@dataclass
class BD1File:
    textures: List[str] = field(default_factory=list)  # up to 10 entries
    blocks: List[Block] = field(default_factory=list)

    def __post_init__(self):
        if len(self.textures) > TOTAL_BLOCKTEXTURE:
            raise BD1Error(f"at most {TOTAL_BLOCKTEXTURE} texture slots")
        for name in self.textures:
            encoded = name.encode("ascii")
            if len(encoded) > TEXNAME_MAX_CHARS:
                raise BD1Error(
                    f"texture name {name!r} too long "
                    f"(max {TEXNAME_MAX_CHARS} chars + NUL in a {TEXNAME_FIELD_LEN}-byte field)"
                )
        if len(self.blocks) > MAX_BLOCKS:
            raise BD1Error(f"at most {MAX_BLOCKS} blocks (MAX_BLOCKS, datafile.h:35)")


def _pack_texname(name: str) -> bytes:
    raw = name.encode("ascii")
    if len(raw) > TEXNAME_MAX_CHARS:
        raise BD1Error(f"texture name {name!r} exceeds {TEXNAME_MAX_CHARS} chars")
    return raw + b"\x00" * (TEXNAME_FIELD_LEN - len(raw))


def _unpack_texname(raw: bytes) -> str:
    nul = raw.find(b"\x00")
    if nul == -1:
        raise BD1Error("texture name field has no NUL terminator")
    return raw[:nul].decode("ascii", errors="replace")


def dumps(bd1: BD1File) -> bytes:
    """Serialize a BD1File to exact .bd1 bytes."""
    out = bytearray()
    textures = list(bd1.textures) + [""] * (TOTAL_BLOCKTEXTURE - len(bd1.textures))
    for name in textures[:TOTAL_BLOCKTEXTURE]:
        out += _pack_texname(name)

    out += struct.pack("<H", len(bd1.blocks))

    for block in bd1.blocks:
        floats = list(block.x) + list(block.y) + list(block.z)
        # u-block (24 floats) then v-block (24 floats), face-major,
        # matching bdata_main[j*4+24.."27"] / [j*4+48.."51"].
        u_block = [0.0] * 24
        v_block = [0.0] * 24
        for j, face in enumerate(block.faces):
            for k in range(4):
                u_block[j * 4 + k] = face.u[k]
                v_block[j * 4 + k] = face.v[k]
        floats += u_block + v_block
        if len(floats) != 72:
            raise BD1Error("internal error: expected 72 floats per block")
        out += struct.pack("<72f", *floats)

        texids = bytearray(28)
        for j, face in enumerate(block.faces):
            texids[j * 4] = face.texture_id & 0xFF
        out += bytes(texids)

    return bytes(out)


def loads(data: bytes) -> BD1File:
    """Parse exact .bd1 bytes into a BD1File.

    Mirrors BlockDataInterface::LoadFiledata (datafile.cpp:109-176),
    including the MAX_BLOCKS clamp -- block counts above 160 in the
    header are clamped, and any trailing bytes beyond the clamped
    block count are simply not read (matching the engine).
    """
    if len(data) < HEADER_SIZE + 2:
        raise BD1Error("file too short for .bd1 header")

    textures = []
    pos = 0
    for _ in range(TOTAL_BLOCKTEXTURE):
        textures.append(_unpack_texname(data[pos:pos + TEXNAME_FIELD_LEN]))
        pos += TEXNAME_FIELD_LEN

    (count,) = struct.unpack_from("<H", data, pos)
    pos += 2
    if count > MAX_BLOCKS:
        count = MAX_BLOCKS  # datafile.cpp:135

    blocks = []
    for _ in range(count):
        if pos + BLOCK_RECORD_SIZE > len(data):
            raise BD1Error("truncated block record")
        floats = struct.unpack_from("<72f", data, pos)
        pos += 288
        texids = data[pos:pos + 28]
        pos += 28

        x = tuple(floats[0:8])
        y = tuple(floats[8:16])
        z = tuple(floats[16:24])
        faces = []
        for j in range(6):
            u = tuple(floats[24 + j * 4: 24 + j * 4 + 4])
            v = tuple(floats[48 + j * 4: 48 + j * 4 + 4])
            tid = struct.unpack_from("<b", texids, j * 4)[0]
            faces.append(FaceMaterial(texture_id=tid, u=u, v=v))
        blocks.append(Block(x=x, y=y, z=z, faces=faces))

    return BD1File(textures=[t for t in textures if t or True][:TOTAL_BLOCKTEXTURE], blocks=blocks)


# ---------------------------------------------------------------------------
# Authoring convenience: axis-aligned box -> Block
# ---------------------------------------------------------------------------

#: vertex positions for a box, matching the ASCII cube diagram at
#: datafile.cpp:303-323 (0..3 is the "near/top" ring as drawn there,
#: 4..7 the "far/bottom" ring). We map min/max corners onto it so that
#: faces come out with outward-facing winding for the face/vertex table
#: above. This placement is an authoring choice (the source does not
#: mandate any particular absolute vertex order beyond the face/vertex
#: *relationship* used by blockdataface()); any consistent assignment
#: that keeps that relationship intact produces a loader-correct block.
def box_vertices(vmin: Sequence[float], vmax: Sequence[float]):
    x0, y0, z0 = vmin
    x1, y1, z1 = vmax
    # Index layout chosen to match the diagram: 0,1,2,3 is the top face
    # (y1) going (x0,z0)-(x1,z0)-(x1,z1)-(x0,z1); 4,5,6,7 is the bottom
    # face (y0) in the same x/z order.
    xs = (x0, x1, x1, x0, x0, x1, x1, x0)
    ys = (y1, y1, y1, y1, y0, y0, y0, y0)
    zs = (z0, z0, z1, z1, z0, z0, z1, z1)
    return xs, ys, zs


def box_block(vmin: Sequence[float], vmax: Sequence[float],
              face_textures: Sequence[int], uv_scale: Sequence[float] = (1.0, 1.0)) -> Block:
    """Build a Block for an axis-aligned box.

    face_textures: 6 signed texture ids, in FACE_NAMES order
                    (top, bottom, front, right, back, left).
    uv_scale: (su, sv) tiling multiplier for the box's own width/height
              per face, applied to a 0..1 UV square written into file
              slot order via FACE_UVSLOT_MAP so that slot k holds the
              per-vertex UV of the face's k-th listed vertex.
    """
    if len(face_textures) != 6:
        raise BD1Error("face_textures must have exactly 6 entries")
    xs, ys, zs = box_vertices(vmin, vmax)
    su, sv = uv_scale
    # Per-vertex UV square (vertex order 0,1,2,3 as seen by the loader
    # for each face): (0,0) (su,0) (su,sv) (0,sv). This is an authoring
    # choice; the loader only cares that slot k == vertex k's UV via
    # FACE_UVSLOT_MAP (fixed by blockdataface(), not a free choice).
    per_vertex_uv = ((0.0, 0.0), (su, 0.0), (su, sv), (0.0, sv))
    # Side faces list their vertices bottom-right, bottom-left, top-left,
    # top-right as seen from outside, and v=0 is the top row of the image.
    # Rotating the square 180 degrees puts the image upright and unmirrored
    # (the plain square showed every wall texture upside down and mirrored).
    side_uv = ((su, sv), (0.0, sv), (0.0, 0.0), (su, 0.0))
    faces = []
    for j in range(6):
        tid = face_textures[j]
        uvs = side_uv if j >= 2 else per_vertex_uv
        u_slots = [0.0, 0.0, 0.0, 0.0]
        v_slots = [0.0, 0.0, 0.0, 0.0]
        for vertex_pos in range(4):
            slot = FACE_UVSLOT_MAP[vertex_pos]
            u_slots[slot], v_slots[slot] = uvs[vertex_pos]
        faces.append(FaceMaterial(texture_id=tid, u=tuple(u_slots), v=tuple(v_slots)))
    return Block(x=xs, y=ys, z=zs, faces=faces)
