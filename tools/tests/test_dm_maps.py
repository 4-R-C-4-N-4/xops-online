"""Tests for the DM HANGAR and DM FIELD deathmatch maps."""
import json
import math
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]

import sys
sys.path.insert(0, str(ROOT))
from tools.mapgen import author, bd1, pd1  # noqa: E402

PLAYER_HEIGHT = 20.0

MAPS = {
    "spire": dict(
        json_src=ROOT / "tools/mapgen/examples/dm_spire.json",
        bd1=ROOT / "assets/pack/data/dmspire/temp.bd1",
        pd1=ROOT / "assets/pack/data/dmspire/spire.pd1",
        mif=ROOT / "assets/pack/data/dmspire/spire.mif",
        msg=ROOT / "assets/pack/data/dmspire/spire.msg",
    ),
    "hangar": dict(
        json_src=ROOT / "tools/mapgen/examples/dm_hangar.json",
        bd1=ROOT / "assets/pack/data/dmhangar/temp.bd1",
        pd1=ROOT / "assets/pack/data/dmhangar/hangar.pd1",
        mif=ROOT / "assets/pack/data/dmhangar/hangar.mif",
        msg=ROOT / "assets/pack/data/dmhangar/hangar.msg",
    ),
    "field": dict(
        json_src=ROOT / "tools/mapgen/examples/dm_field.json",
        bd1=ROOT / "assets/pack/data/dmfield/temp.bd1",
        pd1=ROOT / "assets/pack/data/dmfield/field.pd1",
        mif=ROOT / "assets/pack/data/dmfield/field.mif",
        msg=ROOT / "assets/pack/data/dmfield/field.msg",
    ),
}
MISSIONS_PATH = ROOT / "assets/pack/data/missions.txt"


def _bounds(b):
    if "min" in b:
        return b["min"], b["max"]
    vs = b["vertices"]
    return [min(v[i] for v in vs) for i in range(3)], [max(v[i] for v in vs) for i in range(3)]


def _dm_spawns(points):
    return sorted((p for p in points if p["type"] == "raw" and p.get("p1") == 60),
                  key=lambda p: p["p4"])


def _weapon_pickups(points):
    return [p for p in points if p["type"] == "weapon_pickup"]


class DeterministicBuildTests(unittest.TestCase):
    def test_matches_committed_binaries(self):
        for name, cfg in MAPS.items():
            with self.subTest(name=name):
                doc = json.loads(cfg["json_src"].read_text())
                result = author.build(doc)
                self.assertEqual(result.bd1_bytes, cfg["bd1"].read_bytes())
                self.assertEqual(result.pd1_bytes, cfg["pd1"].read_bytes())


class LimitsTests(unittest.TestCase):
    def test_within_block_and_texture_limits(self):
        for name, cfg in MAPS.items():
            with self.subTest(name=name):
                doc = json.loads(cfg["json_src"].read_text())
                blocks = bd1.loads(cfg["bd1"].read_bytes()).blocks
                self.assertLessEqual(len(blocks), 400)
                self.assertLessEqual(len(doc["textures"]), 10)


class SpawnTests(unittest.TestCase):
    """10 DM spawns, unique p4 0..9, on a floor with headroom, not inside
    a block, and spread out (min pairwise distance >= 40)."""

    def _blocked(self, blocks, x, y, z):
        for b in blocks:
            lo, hi = _bounds(b)
            if lo[0] - 0.5 <= x <= hi[0] + 0.5 and lo[2] - 0.5 <= z <= hi[2] + 0.5:
                if lo[1] <= y <= hi[1]:
                    return True
        return False

    def test_spawn_count_and_unique_ids(self):
        for name, cfg in MAPS.items():
            with self.subTest(name=name):
                doc = json.loads(cfg["json_src"].read_text())
                spawns = _dm_spawns(doc["points"])
                self.assertEqual(len(spawns), 10)
                ids = sorted(p["p4"] for p in spawns)
                self.assertEqual(ids, list(range(10)))

    def test_spawns_have_headroom_and_are_not_inside_blocks(self):
        for name, cfg in MAPS.items():
            with self.subTest(name=name):
                doc = json.loads(cfg["json_src"].read_text())
                blocks = [b for b in doc["blocks"]]
                for p in _dm_spawns(doc["points"]):
                    x, y, z = p["pos"]
                    with self.subTest(spawn=p["p4"]):
                        # feet-level column must be clear...
                        self.assertFalse(self._blocked(blocks, x, y, z),
                                          f"{name} spawn {p['p4']} is inside a block")
                        # ...and the 20-unit headroom above it must be clear too.
                        self.assertFalse(self._blocked(blocks, x, y + PLAYER_HEIGHT - 1.0, z),
                                          f"{name} spawn {p['p4']} lacks headroom")

    def test_minimum_pairwise_spawn_distance(self):
        for name, cfg in MAPS.items():
            with self.subTest(name=name):
                doc = json.loads(cfg["json_src"].read_text())
                coords = [tuple(p["pos"]) for p in _dm_spawns(doc["points"])]
                mind = min(math.dist(coords[i], coords[j])
                           for i in range(len(coords)) for j in range(i + 1, len(coords)))
                self.assertGreaterEqual(mind, 40.0 - 1e-6)

    def test_exactly_one_player_spawn_and_no_hostile_humans(self):
        for name, cfg in MAPS.items():
            with self.subTest(name=name):
                doc = json.loads(cfg["json_src"].read_text())
                spawns = [p for p in doc["points"] if p["type"] == "player_spawn"]
                self.assertEqual(len(spawns), 1)
                hostiles = [p for p in doc["points"] if p["type"] in ("human", "staged_human")]
                self.assertEqual(hostiles, [])


class PickupTests(unittest.TestCase):
    def test_at_least_six_weapon_pickups_not_inside_blocks(self):
        for name, cfg in MAPS.items():
            with self.subTest(name=name):
                doc = json.loads(cfg["json_src"].read_text())
                pickups = _weapon_pickups(doc["points"])
                self.assertGreaterEqual(len(pickups), 6)
                blocks = doc["blocks"]
                for p in pickups:
                    x, y, z = p["pos"]
                    with self.subTest(weapon_id=p["weapon_id"]):
                        for b in blocks:
                            lo, hi = _bounds(b)
                            inside = (lo[0] <= x <= hi[0] and lo[2] <= z <= hi[2]
                                      and lo[1] <= y <= hi[1])
                            self.assertFalse(inside, f"{name} pickup {p['weapon_id']} inside {b['comment']}")


def _flood_fill_reaches(doc, targets, cell=4.0):
    """Coarse grid flood fill from DM spawn 0 to every target XZ, clearing
    a column if some sampled Y band is open (not blocked) at both feet and
    head height."""
    blocks = doc["blocks"]

    def bounds_list():
        return [(_bounds(b)) for b in blocks]

    bl = bounds_list()

    def blocked(x, y, z):
        for lo, hi in bl:
            if lo[0] - 1 <= x <= hi[0] + 1 and lo[2] - 1 <= z <= hi[2] + 1 and lo[1] <= y <= hi[1]:
                return True
        return False

    spawn0 = next(p for p in _dm_spawns(doc["points"]) if p["p4"] == 0)
    x0, y0, z0 = spawn0["pos"]
    xs = [x0] + [t[0] for t in targets]
    zs = [z0] + [t[2] for t in targets]
    xmin, xmax = min(xs) - 40, max(xs) + 40
    zmin, zmax = min(zs) - 40, max(zs) + 40

    y_samples = sorted({round(y0, 1), 2.0, 12.0, 24.0, 32.0, 44.0})

    def cell_of(x, z):
        return (round(x / cell), round(z / cell))

    start = cell_of(x0, z0)
    seen = {start}
    frontier = [start]
    while frontier:
        cx, cz = frontier.pop()
        for dx, dz in ((1, 0), (-1, 0), (0, 1), (0, -1)):
            nx, nz = cx + dx, cz + dz
            if (nx, nz) in seen:
                continue
            wx, wz = nx * cell, nz * cell
            if not (xmin <= wx <= xmax and zmin <= wz <= zmax):
                continue
            clear = any(not blocked(wx, y, wz) and not blocked(wx, y + PLAYER_HEIGHT - 2, wz)
                        for y in y_samples)
            if clear:
                seen.add((nx, nz))
                frontier.append((nx, nz))
    unreached = [t for t in targets if cell_of(t[0], t[2]) not in seen]
    return unreached


class WalkabilityFloodFillTests(unittest.TestCase):
    def test_spawn_0_reaches_every_spawn_and_pickup(self):
        for name, cfg in MAPS.items():
            with self.subTest(name=name):
                doc = json.loads(cfg["json_src"].read_text())
                targets = [tuple(p["pos"]) for p in _dm_spawns(doc["points"])]
                targets += [tuple(p["pos"]) for p in _weapon_pickups(doc["points"])]
                unreached = _flood_fill_reaches(doc, targets)
                self.assertEqual(unreached, [], f"{name}: unreachable targets {unreached}")


class EnclosureTests(unittest.TestCase):
    """DM HANGAR must be fully sealed: every floor sample has a ceiling
    above it (no view into the void). DM FIELD is an open-air pasture by
    design (inherited from UFO FIELD) so this only applies to the hangar.
    """

    def test_hangar_every_floor_sample_has_a_ceiling_overhead(self):
        doc = json.loads(MAPS["hangar"]["json_src"].read_text())
        blocks = doc["blocks"]
        floors = [b for b in blocks if "min" in b and (b["max"][1] - b["min"][1]) <= 2.5
                  and "floor" in b.get("comment", "")]
        ceilings = [b for b in blocks if "min" in b and (b["max"][1] - b["min"][1]) <= 2.5
                    and "ceiling" in b.get("comment", "")]
        self.assertTrue(floors and ceilings)
        for floor in floors:
            fx0, _, fz0 = floor["min"]
            fx1, _, fz1 = floor["max"]
            covering = [c for c in ceilings
                        if c["min"][0] <= fx0 + 2 and c["max"][0] >= fx1 - 2
                        and c["min"][2] <= fz0 + 2 and c["max"][2] >= fz1 - 2
                        and c["min"][1] > floor["max"][1]]
            self.assertTrue(covering, f"no ceiling over floor {floor.get('comment')}")


class MissionWiringTests(unittest.TestCase):
    def test_missions_txt_lists_dm_maps_after_existing_entries(self):
        lines = [l for l in MISSIONS_PATH.read_text().splitlines() if l and not l.startswith(("#", ";"))]
        names = [l.split("|")[0] for l in lines]
        self.assertIn("UFO FIELD", names)
        self.assertIn("UFO ESCAPE", names)
        self.assertIn("STARTER", names)
        self.assertEqual(names[-4:], ["DM HANGAR", "DM FIELD", "DM SHIPDECK", "DM SPIRE"])
        for l in lines[-3:]:
            parts = l.split("|")
            self.assertLessEqual(len(parts[1]), 35)

    def test_mif_files_present_and_within_limits(self):
        for name, cfg in MAPS.items():
            with self.subTest(name=name):
                lines = cfg["mif"].read_text().splitlines()
                self.assertGreaterEqual(len(lines), 9)
                name_field, fullname, bd1_path, pd1_path = lines[0], lines[1], lines[2], lines[3]
                self.assertLessEqual(len(name_field), 23)
                self.assertLessEqual(len(fullname), 35)
                self.assertTrue(bd1_path.endswith("temp.bd1"))
                self.assertTrue(pd1_path.endswith(".pd1"))

    def test_msg_files_present(self):
        for name, cfg in MAPS.items():
            with self.subTest(name=name):
                self.assertTrue(cfg["msg"].exists())


class HangarRebuildTests(unittest.TestCase):
    def test_ramps_are_gentle_and_clear_above_their_centrelines(self):
        doc = json.loads(MAPS["hangar"]["json_src"].read_text())
        ramps = [b for b in doc["blocks"] if b.get("comment", "").startswith("ramp ") and "vertices" in b]
        self.assertEqual(len(ramps), 3)
        for ramp in ramps:
            top = ramp["vertices"][:4]
            lo, hi = _bounds(ramp)
            run = max(hi[0] - lo[0], hi[2] - lo[2])
            self.assertLessEqual((max(v[1] for v in top) - min(v[1] for v in top)) / run, 0.4)
            # Check the actual sloped walking surface, rather than its bounding box.
            a, b = (top[0], top[1]) if top[0][1] != top[1][1] else (top[0], top[3])
            for t in (0.1, 0.3, 0.5, 0.7, 0.9):
                x, y, z = [a[i] + t * (b[i] - a[i]) for i in range(3)]
                if a[0] == b[0]:
                    x = (lo[0] + hi[0]) / 2
                else:
                    z = (lo[2] + hi[2]) / 2
                for block in doc["blocks"]:
                    if block is ramp or "min" not in block:
                        continue
                    low, high = _bounds(block)
                    if low[0] < x < high[0] and low[2] < z < high[2]:
                        self.assertFalse(low[1] < y + 20 and high[1] > y + 1,
                                         f"{ramp['comment']} blocked by {block['comment']}")

    def test_sidecar_hashes_and_local_textures(self):
        import hashlib
        records = [json.loads(line) for line in (ROOT / "assets/provenance.jsonl").read_text().splitlines()]
        records = [r for r in records if r['path'].startswith('data/dmhangar/') or r['path'] == 'data/briefing/dmhangar.bmp']
        expected = set((ROOT / "assets/pack/data/dmhangar").iterdir())
        expected.add(ROOT / "assets/pack/data/briefing/dmhangar.bmp")
        self.assertEqual({r["path"] for r in records},
                         {p.relative_to(ROOT / "assets/pack").as_posix() for p in expected})
        for record in records:
            self.assertEqual(record["license"], "CC0-1.0")
            self.assertEqual(record["sha256"], hashlib.sha256(
                (ROOT / "assets/pack" / record["path"]).read_bytes()).hexdigest())
        doc = json.loads(MAPS["hangar"]["json_src"].read_text())
        for texture in doc["textures"]:
            self.assertTrue((MAPS["hangar"]["bd1"].parent / texture).is_file())


if __name__ == "__main__":
    unittest.main()
