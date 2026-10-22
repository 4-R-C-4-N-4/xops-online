"""Launcher asset/structure contracts; viewport acceptance remains a browser gate."""
from html.parser import HTMLParser
from pathlib import Path
import importlib.util
import unittest
ROOT = Path(__file__).resolve().parents[2]

class Tags(HTMLParser):
    def __init__(self, text):
        super().__init__(); self.tags = []; self.feed(text)
    def handle_starttag(self, tag, attrs):
        self.tags.append((tag, dict(attrs)))

class LauncherTests(unittest.TestCase):
    def test_ids_and_collapsed_secondary_controls(self):
        tags = Tags((ROOT/'web/index.html').read_text()).tags
        ids = [a['id'] for _, a in tags if 'id' in a]
        self.assertEqual(len(ids), len(set(ids)))
        for name in ['play', 'archive', 'forget', 'preset', 'controls-setup', 'status', 'log', 'canvas', 'fullscreen', 'resume-game', 'mp-lobby']:
            self.assertIn(name, ids)
        for name in ['classic-options', 'diagnostics']:
            attrs = next(a for t, a in tags if a.get('id') == name)
            self.assertNotIn('open', attrs)
        self.assertIn(('meta', {'name':'viewport', 'content':'width=device-width, initial-scale=1'}), tags)
        self.assertNotIn('one original starter mission', (ROOT/'web/index.html').read_text())

    def test_bitmap_labels_reproducible_and_staged_by_explicit_allowlist(self):
        spec = importlib.util.spec_from_file_location('labels', ROOT/'tools/fonts/build_launcher_labels.py')
        labels = importlib.util.module_from_spec(spec); spec.loader.exec_module(labels)
        stage = (ROOT/'tools/stage_pages.py').read_text()
        for name, text in labels.LABELS.items():
            self.assertEqual((ROOT/f'web/ui/{name}.svg').read_text(), labels.render(text))
            self.assertIn(f"'{name}.svg'", stage)
        self.assertIn("ROOT / 'web/ui'", (ROOT/'tools/package_web.py').read_text())
        self.assertIn('min-height:44px', (ROOT/'web/ui/launcher.css').read_text())
