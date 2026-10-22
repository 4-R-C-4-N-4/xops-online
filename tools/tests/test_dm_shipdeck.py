"""SHIPDECK authored source, packaged assets, and real-engine acceptance."""
import hashlib
import importlib.util
import json
from pathlib import Path
import shutil
import tempfile
import unittest

from tools.mapgen import author
from tools.artgen.briefing_pictures import generate_dmshipdeck
from tools.tests.test_dm_check import run

ROOT = Path(__file__).resolve().parents[2]
DATA = ROOT / 'assets/pack/data'
SOURCE = ROOT / 'tools/mapgen/examples/dm_shipdeck.py'


class ShipdeckTests(unittest.TestCase):
    def test_source_and_binary_regeneration(self):
        spec = importlib.util.spec_from_file_location('dm_shipdeck', SOURCE)
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
        doc = json.loads(SOURCE.with_suffix('.json').read_text())
        self.assertEqual(module.build_doc(), doc)
        built = author.build(doc)
        self.assertEqual(built.bd1_bytes, (DATA / 'dmship/temp.bd1').read_bytes())
        self.assertEqual(built.pd1_bytes, (DATA / 'dmship/ship.pd1').read_bytes())
        spawns = [p for p in doc['points'] if p.get('p1') == 60]
        self.assertEqual(sorted(p['p4'] for p in spawns), list(range(12)))
        self.assertEqual({p['pos'][1] for p in spawns}, {2, 34, 66})
        self.assertEqual(len([p for p in doc['points'] if p.get('p1') == 61]), 3)
        for texture in doc['textures']:
            self.assertEqual((DATA / 'dmship' / texture).read_bytes(),
                             (DATA / 'ufoescape' / texture).read_bytes())

    def test_briefing_and_mission_assets(self):
        with tempfile.TemporaryDirectory() as tmp:
            generated = generate_dmshipdeck(Path(tmp))
            self.assertEqual(generated.read_bytes(), (DATA / 'briefing/dmship.bmp').read_bytes())
        lines = (DATA / 'dmship/ship.mif').read_text().splitlines()
        for index in (2, 3, 7):
            self.assertTrue((DATA.parent / lines[index].replace('\\', '/')).is_file())
        self.assertLessEqual(len(lines[9:]), 14)
        self.assertTrue(all(len(line) <= 52 for line in lines[9:]))
        records = [json.loads(line) for line in (ROOT / 'assets/provenance.jsonl').read_text().splitlines()]
        for path in list((DATA / 'dmship').iterdir()) + [DATA / 'briefing/dmship.bmp']:
            record = next(r for r in records if r['path'] == path.relative_to(DATA.parent).as_posix())
            self.assertEqual(record['sha256'], hashlib.sha256(path.read_bytes()).hexdigest())

    @unittest.skipUnless(shutil.which('g++'), 'g++ required for real-engine gate')
    def test_sealed_engine_gate(self):
        code, output = run('data/dmship', 'ship.pd1', '--sealed')
        self.assertEqual(code, 0, output)
        self.assertIn('DMCHECK OK', output)
