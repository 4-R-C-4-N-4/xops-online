"""Every free-pack map must find its textures: the engine loads the names
stored in temp.bd1 from the map's own directory, so a missing file renders
as untextured white (found in the first DM HANGAR multiplayer test)."""
import pathlib
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[2]
PACK = ROOT / "assets" / "pack" / "data"


def texture_names(bd1: pathlib.Path):
    data = bd1.read_bytes()
    names = []
    for i in range(10):
        raw = data[i * 31:(i + 1) * 31].split(b"\0")[0]
        if raw:
            names.append(raw.decode("ascii"))
    return names


class MapTextureTests(unittest.TestCase):
    def test_every_bd1_texture_exists_beside_the_map(self):
        maps = sorted(PACK.glob("*/temp.bd1"))
        self.assertTrue(maps)
        missing = []
        for bd1 in maps:
            present = {p.name.lower() for p in bd1.parent.iterdir()}
            for name in texture_names(bd1):
                if name.lower() not in present:
                    missing.append(f"{bd1.parent.name}/{name}")
        self.assertEqual(missing, [], "textures missing beside their map")


if __name__ == "__main__":
    unittest.main()
