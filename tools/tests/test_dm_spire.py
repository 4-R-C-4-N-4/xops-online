"""DM SPIRE source, rendered plans, wiring and clean-room asset provenance."""
import hashlib
import json
import runpy
import tempfile
import unittest
from pathlib import Path

from tools.artgen import briefing_pictures
from tools.tests.test_dm_check import run

ROOT = Path(__file__).resolve().parents[2]


class SpireTests(unittest.TestCase):
    def test_generator_matches_json(self):
        module = runpy.run_path(str(ROOT / 'tools/mapgen/examples/dm_spire.py'))
        doc = json.loads((ROOT / 'tools/mapgen/examples/dm_spire.json').read_text())
        self.assertEqual(module['build_doc'](), doc)
        self.assertEqual(len([p for p in doc['points'] if p.get('p1') == 61]), 3)
        self.assertEqual({p['pos'][1] for p in doc['points'] if p.get('p1') == 60}, {2,34,66})

    def test_sealed_engine_gate(self):
        code, output = run('data/dmspire', 'spire.pd1', '--sealed')
        self.assertEqual(code, 0, output)
        self.assertIn('DMCHECK OK', output)

    def test_deterministic_pictures(self):
        with tempfile.TemporaryDirectory() as tmp:
            picture = briefing_pictures.generate_dmspire(Path(tmp))
            self.assertEqual(picture.read_bytes(), (ROOT / 'assets/pack/data/briefing/dmspire.bmp').read_bytes())
            plan = briefing_pictures.generate_spire_layout(Path(tmp) / 'layout.png')
            self.assertEqual(plan.read_bytes(), (ROOT / 'docs/img/dm/spire_layout.png').read_bytes())

    def test_provenance_covers_every_asset(self):
        records = {r['path']: r for r in map(json.loads, (ROOT / 'PROVENANCE-ADD.jsonl').read_text().splitlines())}
        assets = list((ROOT / 'assets/pack/data/dmspire').iterdir())
        assets += [ROOT / 'assets/pack/data/briefing/dmspire.bmp', ROOT / 'docs/img/dm/spire_layout.png']
        for path in assets:
            key = str(path.relative_to(ROOT / 'assets/pack')) if path.is_relative_to(ROOT / 'assets/pack') else str(path.relative_to(ROOT))
            rec = records[key]
            self.assertEqual(rec['license'], 'CC0-1.0')
            self.assertEqual(rec['sha256'], hashlib.sha256(path.read_bytes()).hexdigest())
            if rec['source_path']:
                self.assertEqual(path.read_bytes(), (ROOT / 'assets/pack' / rec['source_path']).read_bytes())

    def test_lobby_and_briefing_wiring(self):
        self.assertIn("id: 'spire', title: 'DM SPIRE', mif: 'data/dmspire/spire.mif'", (ROOT / 'web/net/game-net.js').read_text())
        self.assertIn('<option value="spire">DM SPIRE</option>', (ROOT / 'web/index.html').read_text())
        mif = (ROOT / 'assets/pack/data/dmspire/spire.mif').read_text().splitlines()
        self.assertEqual(mif[7], 'data\\briefing\\dmspire.bmp')
        self.assertLessEqual(len(mif[9:]), 14)
        self.assertTrue(all(len(line) <= 52 for line in mif[9:]))
