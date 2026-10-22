"""Coplanar-face (z-fighting) detection and resolution for authored maps.

Two drawn faces of different blocks that face the same way, share a plane
(within PLANE_EPS) and overlap where nothing else covers them fight in the
depth buffer: the renderer draws both and the visible one flickers per pixel
as the camera moves. RenderMapdata() culls back faces and only draws texture
ids 0..TOTAL_BLOCKTEXTURE-1, so opposite-facing or untextured faces never
fight.

resolve() pushes the smaller face of each fighting pair OUT by INSET units
along its normal (moving that face's four vertices), repeating until nothing
fights. The smaller surface is the detail the author meant to see (a dirt
patch flush on grass, a trim post flush with a wall), so it wins; the larger
surface stays exactly where it was authored. Moving outward never opens a
crack into the void. Collision changes by less than a tenth of a unit.
Idempotent: a resolved map has no fights left.
"""
from __future__ import annotations

from . import bd1

INSET = 0.08

FACE_VERTS = [(1, 0, 3, 2), (6, 7, 4, 5), (5, 4, 0, 1), (6, 5, 1, 2), (7, 6, 2, 3), (4, 7, 3, 0)]
FACE_NAMES = ["top", "bottom", "front", "right", "back", "left"]
PLANE_EPS = 0.05     # world units; 24-bit depth at 800 far resolves ~0.003
MIN_AREA = 0.25      # square units of overlap worth reporting


def sub(a, b): return (a[0] - b[0], a[1] - b[1], a[2] - b[2])
def dot(a, b): return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]
def cross(a, b): return (a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0])


def norm(a):
    n = dot(a, a) ** 0.5
    return (a[0] / n, a[1] / n, a[2] / n) if n > 1e-9 else None


def block_faces(block, index):
    pts = [(block.x[i], block.y[i], block.z[i]) for i in range(8)]
    centre = tuple(sum(p[k] for p in pts) / 8 for k in range(3))
    out = []
    for f, vids in enumerate(FACE_VERTS):
        if not (0 <= block.faces[f].texture_id < bd1.TOTAL_BLOCKTEXTURE):
            continue
        poly = [pts[v] for v in vids]
        # Newell normal, oriented away from the block centre.
        n = [0.0, 0.0, 0.0]
        for i in range(4):
            a, b = poly[i], poly[(i + 1) % 4]
            n[0] += (a[1] - b[1]) * (a[2] + b[2])
            n[1] += (a[2] - b[2]) * (a[0] + b[0])
            n[2] += (a[0] - b[0]) * (a[1] + b[1])
        n = norm(n)
        if n is None:
            continue  # degenerate face (collapsed edge)
        fc = tuple(sum(p[k] for p in poly) / 4 for k in range(3))
        if dot(n, sub(fc, centre)) < 0:
            n = (-n[0], -n[1], -n[2])
        # Non-planar quads are drawn as two triangles; skip warped ones.
        if max(abs(dot(n, sub(p, poly[0]))) for p in poly) > 0.01:
            continue
        out.append(dict(block=index, face=f, poly=poly, n=n, d=dot(n, poly[0])))
    return out


def planes_of(block):
    """Inward-safe half-spaces (n, d) with n pointing out, for a convex block."""
    pts = [(block.x[i], block.y[i], block.z[i]) for i in range(8)]
    centre = tuple(sum(p[k] for p in pts) / 8 for k in range(3))
    planes = []
    for vids in FACE_VERTS:
        poly = [pts[v] for v in vids]
        n = None
        for i in range(4):
            n = norm(cross(sub(poly[(i + 1) % 4], poly[i]), sub(poly[(i + 2) % 4], poly[i])))
            if n:
                break
        if n is None:
            continue
        if dot(n, sub(poly[0], centre)) < 0:
            n = (-n[0], -n[1], -n[2])
        planes.append((n, dot(n, poly[0])))
    return planes


def inside(planes, p, eps=1e-3):
    return all(dot(n, p) < d - eps for n, d in planes)


def basis(n):
    a = (1.0, 0.0, 0.0) if abs(n[0]) < 0.9 else (0.0, 1.0, 0.0)
    u = norm(cross(n, a))
    return u, cross(n, u)


def clip(subject, clipper):
    """Sutherland-Hodgman for convex 2D polygons (either winding)."""
    def area2(poly):
        return sum(poly[i][0] * poly[(i + 1) % len(poly)][1] - poly[(i + 1) % len(poly)][0] * poly[i][1]
                   for i in range(len(poly)))
    if area2(clipper) < 0:
        clipper = clipper[::-1]
    out = subject
    for i in range(len(clipper)):
        a, b = clipper[i], clipper[(i + 1) % len(clipper)]
        side = lambda p: (b[0] - a[0]) * (p[1] - a[1]) - (b[1] - a[1]) * (p[0] - a[0])
        src, out = out, []
        if not src:
            break
        for j in range(len(src)):
            p, q = src[j], src[(j + 1) % len(src)]
            sp, sq = side(p), side(q)
            if sp >= 0:
                out.append(p)
            if (sp >= 0) != (sq >= 0):
                t = sp / (sp - sq)
                out.append((p[0] + t * (q[0] - p[0]), p[1] + t * (q[1] - p[1])))
    return out


def area(poly):
    return abs(sum(poly[i][0] * poly[(i + 1) % len(poly)][1] - poly[(i + 1) % len(poly)][0] * poly[i][1]
                   for i in range(len(poly)))) / 2 if len(poly) >= 3 else 0.0


def fights(blocks, names=None):
    faces = [f for i, b in enumerate(blocks) for f in block_faces(b, i)]
    solids = [planes_of(b) for b in blocks]
    found = []
    for i in range(len(faces)):
        a = faces[i]
        for j in range(i + 1, len(faces)):
            b = faces[j]
            if a["block"] == b["block"] or dot(a["n"], b["n"]) < 0.9999 or abs(a["d"] - b["d"]) > PLANE_EPS:
                continue
            u, v = basis(a["n"])
            pa = [(dot(p, u), dot(p, v)) for p in a["poly"]]
            pb = [(dot(p, u), dot(p, v)) for p in b["poly"]]
            ov = clip(pa, pb)
            ar = area(ov)
            if ar < MIN_AREA:
                continue
            # Visible unless every sample just in front of the overlap is inside some other block.
            cx = sum(p[0] for p in ov) / len(ov)
            cy = sum(p[1] for p in ov) / len(ov)
            samples = [(cx, cy)] + [((p[0] * 3 + cx) / 4, (p[1] * 3 + cy) / 4) for p in ov]
            visible = False
            for s in samples:
                w = tuple(a["n"][k] * a["d"] + u[k] * s[0] + v[k] * s[1] + a["n"][k] * 0.02 for k in range(3))
                if not any(inside(solids[k], w) for k in range(len(blocks)) if k not in (a["block"], b["block"])):
                    visible = True
                    break
            if visible:
                found.append((a, b, ar))
    return found




def face_area(poly):
    u, v = basis(_poly_normal(poly))
    return area([(dot(p, u), dot(p, v)) for p in poly])


def _poly_normal(poly):
    n = [0.0, 0.0, 0.0]
    for i in range(4):
        a, b = poly[i], poly[(i + 1) % 4]
        n[0] += (a[1] - b[1]) * (a[2] + b[2])
        n[1] += (a[2] - b[2]) * (a[0] + b[0])
        n[2] += (a[0] - b[0]) * (a[1] + b[1])
    return norm(n) or (0.0, 1.0, 0.0)


def resolve(blocks, max_rounds=8):
    """Return (blocks, moved) with every visible coplanar overlap removed."""
    blocks = list(blocks)
    moved = 0
    for _ in range(max_rounds):
        found = fights(blocks)
        if not found:
            break
        done = set()
        for a, b, _area in found:
            small = a if face_area(a["poly"]) < face_area(b["poly"]) or (
                face_area(a["poly"]) == face_area(b["poly"]) and a["block"] > b["block"]) else b
            key = (small["block"], small["face"])
            if key in done:
                continue
            done.add(key)
            blk = blocks[small["block"]]
            xs, ys, zs = list(blk.x), list(blk.y), list(blk.z)
            n = small["n"]
            for vi in FACE_VERTS[small["face"]]:
                xs[vi] += n[0] * INSET
                ys[vi] += n[1] * INSET
                zs[vi] += n[2] * INSET
            r = lambda vals: tuple(round(v, 4) for v in vals)
            blocks[small["block"]] = bd1.Block(x=r(xs), y=r(ys), z=r(zs), faces=blk.faces)
            moved += 1
    return blocks, moved
