"""Map geometry hygiene: no z-fighting faces, binaries match their sources."""
import pathlib
import subprocess
import sys
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT))
from tools.mapgen import author, bd1, zfight  # noqa: E402


def box(lo, hi):
    return {"min": lo, "max": hi, "texture": "t.bmp"}


class ZFightTests(unittest.TestCase):
    def test_overlapping_walls_fight_then_resolve(self):
        # Two walls overlapping at a corner share their outer faces.
        doc = {"textures": ["t.bmp"], "blocks": [box([-40, 0, -4], [40, 20, 0]), box([36, 0, -4], [40, 20, 40])],
               "resolve_zfighting": False}
        raw = bd1.loads(author.build(doc).bd1_bytes).blocks
        # right (x=40), front (z=-4), top and (floorless, so visible) bottom of the corner
        self.assertEqual(len(zfight.fights(raw)), 4)
        doc["resolve_zfighting"] = True
        fixed = author.build(doc).bd1_bytes
        self.assertEqual(zfight.fights(bd1.loads(fixed).blocks), [])
        # Idempotent: rebuilding the resolved geometry changes nothing.
        again = author.build(author.dump_bd1(fixed)).bd1_bytes
        self.assertEqual(again, fixed)

    def test_buried_and_back_to_back_faces_are_not_fights(self):
        # A crate standing on a floor: crate bottom faces down, floor top faces up.
        doc = {"textures": ["t.bmp"], "blocks": [box([-50, -4, -50], [50, 0, 50]), box([-5, 0, -5], [5, 10, 5])],
               "resolve_zfighting": False}
        self.assertEqual(zfight.fights(bd1.loads(author.build(doc).bd1_bytes).blocks), [])

    def test_every_pack_map_is_free_of_zfighting(self):
        for path in sorted((ROOT / "assets/pack/data").glob("*/temp.bd1")):
            with self.subTest(map=path.parent.name):
                found = zfight.fights(bd1.loads(path.read_bytes()).blocks)
                self.assertEqual(found, [], f"{path}: {len(found)} coplanar overlaps; run tools/rebuild_maps.py")

    def test_pack_maps_match_their_sources(self):
        r = subprocess.run([sys.executable, str(ROOT / "tools/rebuild_maps.py"), "--check"], cwd=ROOT,
                           capture_output=True, text=True)
        self.assertEqual(r.returncode, 0, r.stdout + r.stderr)


if __name__ == "__main__":
    unittest.main()
