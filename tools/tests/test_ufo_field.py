"""Tests for the UFO FIELD mission content (docs/MISSION-UFO-FIELD.md).

Covers: deterministic build from JSON, block/point limits, a closed
outer farm boundary (with internal pasture gates), the player HUMANINFO/type, the 6/8/6 staged
wave groups, the event chain shape, the 8 cow small objects, and the
addon/mif/msg/cow.txt file formats.
"""
import json
import struct
import unittest
from collections import Counter
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
JSON_SRC = ROOT / "tools/mapgen/examples/ufo_field.json"
BD1_PATH = ROOT / "assets/pack/data/ufofield/temp.bd1"
PD1_PATH = ROOT / "assets/pack/data/ufofield/ufo.pd1"
MSG_PATH = ROOT / "assets/pack/data/ufofield/ufo.msg"
MIF_PATH = ROOT / "assets/pack/data/ufofield/ufo.mif"
MISSIONS_PATH = ROOT / "assets/pack/data/missions.txt"
COW_PATH = ROOT / "assets/pack/addon/cow.txt"
BRIEFING_BMP = ROOT / "assets/pack/data/briefing/ufofield.bmp"

import sys
sys.path.insert(0, str(ROOT))
from tools.mapgen import author, bd1, pd1  # noqa: E402


def _covered(intervals, lo, hi):
    pos = lo
    for a, b in sorted(intervals):
        if a > pos + 1e-6:
            return False
        pos = max(pos, b)
    return pos >= hi - 1e-6


class DeterministicBuildTests(unittest.TestCase):
    def setUp(self):
        self.doc = json.loads(JSON_SRC.read_text())
        self.result = author.build(self.doc)

    def test_matches_committed_binaries(self):
        self.assertEqual(self.result.bd1_bytes, BD1_PATH.read_bytes())
        self.assertEqual(self.result.pd1_bytes, PD1_PATH.read_bytes())

    def test_generator_matches_json(self):
        from tools.mapgen.examples.ufo_field import build_doc
        self.assertEqual(build_doc(), self.doc)

    def test_mission_outdoor_gate(self):
        import subprocess
        run = subprocess.run([sys.executable, str(ROOT / "tools/dmcheck.py"),
                              "data/ufofield", "ufo.pd1", "--mission", "--outdoor"],
                             cwd=ROOT, capture_output=True, text=True, timeout=180)
        self.assertEqual(run.returncode, 0, run.stdout + run.stderr)
        self.assertIn("METRIC death_pit_edges 0", run.stdout)
        self.assertIn("DMCHECK OK", run.stdout)

    def test_within_limits(self):
        blocks = bd1.loads(self.result.bd1_bytes).blocks
        points = pd1.loads(self.result.pd1_bytes).points
        self.assertLessEqual(len(blocks), 160)
        self.assertLessEqual(len(points), 256)
        self.assertLessEqual(len(self.doc["textures"]), 10)


class PerimeterTests(unittest.TestCase):
    """Same approach as tools/tests/test_starter_perimeter.py."""

    def setUp(self):
        self.blocks = json.loads(JSON_SRC.read_text())["blocks"]
        floor = self.blocks[0]
        self.x0, _, self.z0 = floor["min"]
        self.x1, _, self.z1 = floor["max"]

    def _walls(self, axis, plane, outward):
        out = []
        for b in self.blocks:
            if "min" not in b or "max" not in b:
                continue  # raw-vertex UFO blocks are not perimeter walls
            lo, hi = b["min"], b["max"]
            if hi[1] - max(lo[1], 0) < 24:
                continue
            face = lo[axis] if outward > 0 else hi[axis]
            if abs(face - plane) < 1e-6 or (lo[axis] <= plane <= hi[axis]):
                other = 0 if axis == 2 else 2
                out.append((lo[other], hi[other]))
        return out

    def test_every_floor_edge_is_walled(self):
        for name, axis, plane, outward, lo, hi in [
            ("south", 2, self.z1, 1, self.x0, self.x1),
            ("north", 2, self.z0, -1, self.x0, self.x1),
            ("east", 0, self.x1, 1, self.z0, self.z1),
            ("west", 0, self.x0, -1, self.z0, self.z1),
        ]:
            with self.subTest(edge=name):
                self.assertTrue(_covered(self._walls(axis, plane, outward), lo, hi),
                                 f"{name} perimeter has a gap")


class PointContentTests(unittest.TestCase):
    def setUp(self):
        self.points = pd1.loads(PD1_PATH.read_bytes()).points

    def test_player_is_humaninfo_43(self):
        human2 = [p for p in self.points if p.p1 == pd1.P1_HUMAN2 and p.p4 == 0]
        self.assertEqual(len(human2), 1, "exactly one player HUMAN2 (p4=0)")
        info_by_id = {p.p4: p for p in self.points if p.p1 == pd1.P1_HUMANINFO}
        info = info_by_id[human2[0].p2]
        self.assertEqual(info.p2, 43, "player HUMANINFO type must be 43 (rancher)")

    def test_staged_waves_6_8_6_with_types_44_45_46(self):
        staged = [p for p in self.points if p.p1 == pd1.P1_STAGED_HUMAN]
        self.assertEqual(len(staged), 20)
        info_by_id = {p.p4: p for p in self.points if p.p1 == pd1.P1_HUMANINFO}
        by_wave = Counter(p.p4 for p in staged)
        self.assertEqual(by_wave, {1: 6, 2: 8, 3: 6})
        type_by_wave = {}
        for p in staged:
            info = info_by_id[p.p2]
            type_by_wave.setdefault(p.p4, set()).add(info.p2)
            self.assertNotEqual(info.p3, 0, "staged aliens must not share the player's team 0")
        self.assertEqual(type_by_wave, {1: {44}, 2: {45}, 3: {46}})

    def test_eight_cow_small_objects(self):
        cows = [p for p in self.points if p.p1 == pd1.P1_SMALLOBJ and p.p2 == 12]
        self.assertEqual(len(cows), 8)

    def test_event_chain_reachable_from_entry_minus100_in_order(self):
        events = {p.p4: p for p in self.points if 10 <= p.p1 <= 19 or p.p1 in (40, 41)}
        # Walk the chain starting at the entry id -100 (EventControl::Reset).
        seen = []
        current = -100
        guard = 0
        while current in events and guard < 50:
            p = events[current]
            seen.append((p.p1, p.p2))
            current = p.p3
            guard += 1
        expected = [
            (17, 5), (18, 0), (41, 1), (40, 0), (17, 4),
            (18, 1), (41, 2), (40, 0), (17, 4), (18, 2), (41, 3),
        ]
        self.assertEqual(seen, expected)


class MsgFormatTests(unittest.TestCase):
    def test_three_lines_within_byte_limit(self):
        lines = MSG_PATH.read_text().splitlines()
        self.assertEqual(len(lines), 3)
        for line in lines:
            # MAX_POINTMESSAGEBYTE = 73 (datafile.h); 71 chars + CRLF.
            self.assertLessEqual(len(line) + 2, 73)
        self.assertIn("WAVE 1", lines[0])
        self.assertIn("WAVE 2", lines[1])
        self.assertIn("FINAL WAVE", lines[2])


class CowAddonFormatTests(unittest.TestCase):
    def test_six_lines_within_limits(self):
        lines = COW_PATH.read_text().splitlines()
        self.assertEqual(len(lines), 6)
        model, texture, decide, hp, sound, jump = lines
        self.assertTrue(model.endswith(".x"))
        self.assertTrue(texture.endswith(".bmp"))
        self.assertEqual(int(decide), 9)
        self.assertEqual(int(hp), 127)
        self.assertTrue(sound.endswith(".wav"))
        self.assertEqual(int(jump), 0)
        for line in lines:
            self.assertLess(len(line), 32)  # fgets buffers are 16 or _MAX_PATH


class MainMenuListingTests(unittest.TestCase):
    def test_ufo_field_is_first_main_menu_mission_not_an_addon(self):
        entries = [line.split("|") for line in MISSIONS_PATH.read_text().splitlines()
                   if line and not line.startswith(("#", ";"))]
        self.assertEqual(entries[0], ["UFO FIELD", "UFO FIELD - alien wave defence",
                                      "data\\ufofield\\", "ufo.mif", "0", "0"])
        self.assertEqual(entries[1][0], "UFO ESCAPE")
        self.assertEqual(entries[2][0], "STARTER")
        self.assertEqual(list((ROOT / "assets/pack/addon").glob("*.mif")), [])


class MifFormatTests(unittest.TestCase):
    def test_header_fields_and_briefing_limits(self):
        lines = MIF_PATH.read_text().splitlines()
        name, fullname, bd1_path, pd1_path, sky, flags, addobj, picA, picB = lines[:9]
        self.assertLessEqual(len(name), 23)
        self.assertLessEqual(len(fullname), 63)
        # Briefing draws the title at 18 px per glyph on a 640 px screen.
        self.assertLessEqual(len(fullname) * 18, 640, "briefing title would not fit")
        self.assertEqual(bd1_path, r"data\ufofield\temp.bd1")
        self.assertEqual(pd1_path, r"data\ufofield\ufo.pd1")
        self.assertEqual(sky, "5")
        self.assertEqual(addobj, r"addon\cow.txt")
        self.assertEqual(picA, r"data\briefing\ufofield.bmp")
        self.assertEqual(picB, "!")
        briefing = lines[9:]
        self.assertLessEqual(len(briefing), 17)
        for line in briefing:
            self.assertLessEqual(len(line), 49)


class BriefingPictureTests(unittest.TestCase):
    def test_bmp_header(self):
        self.assertTrue(BRIEFING_BMP.exists())
        data = BRIEFING_BMP.read_bytes()
        self.assertEqual(data[0:2], b"BM")
        w, h = struct.unpack_from("<ii", data, 0x12)
        self.assertEqual((w, h), (256, 256))
        comp = struct.unpack_from("<I", data, 0x1E)[0]
        self.assertEqual(comp, 0, "must be uncompressed (BI_RGB)")


class PackIncludesAddonTests(unittest.TestCase):
    def test_zip_contains_addon_mif(self):
        import importlib.util
        import tempfile
        import zipfile

        spec = importlib.util.spec_from_file_location(
            "build_pack", str(ROOT / "tools/build_pack.py"))
        build_pack = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(build_pack)

        with tempfile.TemporaryDirectory() as tmp:
            staged_data = build_pack.stage_data(str(Path(tmp) / "staging"))
            out_path, _digest = build_pack.build_zip(
                staged_data, str(Path(tmp) / "out"),
                addon_dir=str(Path(tmp) / "staging" / "addon"))
            with zipfile.ZipFile(out_path) as zf:
                names = zf.namelist()
                self.assertIn("data/ufofield/ufo.mif", names)
                self.assertNotIn("addon/ufo.mif", names)
                self.assertIn("addon/cow.txt", names)
                self.assertIn("addon/cow.txt.cover", names)
                data_names = [n for n in names if n.startswith("data/")]
                addon_names = [n for n in names if n.startswith("addon/")]
                self.assertEqual(data_names, sorted(data_names))
                self.assertEqual(addon_names, sorted(addon_names))
                self.assertEqual(names, data_names + addon_names,
                                  "deterministic ordering: all data/ then all addon/")


class UfoPathTests(unittest.TestCase):
    """AI paths are looked up by p4 across ALL points (ai.cpp ~2069,
    SearchPointdata flag 0x08), so path ids must be unique map-wide and every
    route must stay on waypoints."""

    def test_every_route_resolves_to_unique_waypoints(self):
        pts = pd1.loads(PD1_PATH.read_bytes()).points
        by_p4 = {}
        for p in pts:
            by_p4.setdefault(p.p4, []).append(p)
        starts = {p.p3 for p in pts if p.p1 in (1, 6, 50)}
        starts.discard(0)  # Player has no patrol path.
        self.assertTrue(starts)
        for s in sorted(starts):
            with self.subTest(route=s):
                seen = set()
                cur = s
                while cur not in seen:
                    seen.add(cur)
                    hits = by_p4.get(cur, [])
                    self.assertEqual(len(hits), 1, f"path id {cur} must be unique map-wide")
                    self.assertEqual(hits[0].p1, 3, f"path id {cur} must be a waypoint")
                    cur = hits[0].p3

    def test_hay_cover_spread_across_both_sides(self):
        blocks = json.loads(JSON_SRC.read_text())["blocks"]
        for side in (-1, 1):
            full = [b for b in blocks if b.get("texture") == "hay.bmp"
                    and "min" in b and b["min"][1] == 0 and b["max"][1] >= 24
                    and b["max"][0] - b["min"][0] >= 28
                    and side * (b["min"][0]+b["max"][0])/2 > 40]
            self.assertGreaterEqual(len(full), 2)

    def test_full_height_cover_in_front_of_spawn(self):
        blocks = json.loads(JSON_SRC.read_text())["blocks"]
        self.assertTrue(any(
            b.get("texture") == "hay.bmp"
            and b["min"][0] <= -20 and b["max"][0] >= 20
            and b["max"][1] >= 24 and 160 <= b["min"][2] < b["max"][2] < 200
            for b in blocks))


if __name__ == "__main__":
    unittest.main()
