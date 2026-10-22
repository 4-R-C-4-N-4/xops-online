"""Tests for tools/stage_pages.py: original-asset MD5 guard over zip members."""
import hashlib
import importlib.util
import json
import tempfile
import unittest
import zipfile
from pathlib import Path

SCRIPT = Path(__file__).resolve().parents[1] / 'stage_pages.py'
spec = importlib.util.spec_from_file_location('stage_pages', SCRIPT)
stage_pages = importlib.util.module_from_spec(spec)
spec.loader.exec_module(stage_pages)


def _write_build(build, pack_name='openxops-data.aaaaaaaaaaaaaaaa.zip', pack_bytes=None):
    build.mkdir(parents=True, exist_ok=True)
    fp = 'cafebabecafebabe'
    js_name, wasm_name = f'openxops.{fp}.js', f'openxops.{fp}.wasm'
    (build / js_name).write_text('// engine js, no diagnostic hook here')
    (build / wasm_name).write_bytes(b'wasm-bytes')
    if pack_bytes is None:
        with zipfile.ZipFile(build / pack_name, 'w') as zf:
            zf.writestr('data/model/cube.x', b'cube-model-bytes')
    else:
        (build / pack_name).write_bytes(pack_bytes)
    html = (f'<script src="{js_name}"></script>\n'
            f"Module={{locateFile:p=>'{wasm_name}'}}\n"
            f'fetch("{pack_name}")')
    (build / 'index.html').write_text(html)
    (build / 'LICENSE').write_text('license text')
    return js_name, wasm_name, pack_name


class StagePagesTests(unittest.TestCase):
    def setUp(self):
        self._real_root = stage_pages.ROOT
        self._real_lookup = stage_pages._original_md5s
        self.addCleanup(self._restore)

    def _restore(self):
        stage_pages.ROOT = self._real_root
        stage_pages._original_md5s = self._real_lookup

    def test_stages_expected_files_when_clean(self):
        with tempfile.TemporaryDirectory() as t:
            root = Path(t)
            build = root / 'build-web'
            js_name, wasm_name, pack_name = _write_build(build)
            (root / 'NOTICE.md').write_text('notice')
            real_originals = stage_pages._original_md5s()
            stage_pages._original_md5s = lambda: real_originals
            stage_pages.ROOT = root
            dest = root / 'build-pages'
            names = stage_pages.stage(build, dest)
            self.assertIn(pack_name, names)
            self.assertTrue((dest / pack_name).exists())
            self.assertTrue((dest / 'NOTICE.md').exists())

    def test_launcher_assets_staged_without_unlisted_neighbors(self):
        with tempfile.TemporaryDirectory() as t:
            root = Path(t)
            build = root / 'build-web'
            _write_build(build)
            with (build / 'index.html').open('a') as page:
                page.write('<link href="ui/launcher.css" rel="stylesheet">')
            ui = build / 'ui'
            ui.mkdir()
            expected = ['launcher.css', 'title.svg', 'operate.svg', 'host.svg',
                        'join.svg', 'copy.svg', 'controls.svg', 'fullscreen.svg',
                        'resume.svg', 'close.svg', 'forget.svg', 'start.svg', 'FONT-LICENSE.md']
            for name in expected:
                (ui / name).write_text('synthetic launcher asset ' + name)
            (ui / 'private.bin').write_bytes(b'never publish')
            (root / 'NOTICE.md').write_text('notice')
            stage_pages._original_md5s = lambda: set()
            stage_pages.ROOT = root
            destination = root / 'pages'
            stage_pages.stage(build, destination)
            self.assertEqual(sorted(p.name for p in (destination / 'ui').iterdir()),
                             sorted(expected))

    def test_rejects_zip_member_matching_known_original_md5(self):
        # Synthetic bytes can't collide with the real reference MD5 list, so
        # monkeypatch _original_md5s to include a hash of our own test bytes.
        with tempfile.TemporaryDirectory() as t:
            root = Path(t)
            build = root / 'build-web'
            original_bytes = b'this-stands-in-for-an-original-asset'
            original_md5 = hashlib.md5(original_bytes).hexdigest()
            pack_name = 'openxops-data.bbbbbbbbbbbbbbbb.zip'
            build.mkdir(parents=True)
            fp = 'deadbeefdeadbeef'
            (build / f'openxops.{fp}.js').write_text('// clean engine js')
            (build / f'openxops.{fp}.wasm').write_bytes(b'wasm')
            with zipfile.ZipFile(build / pack_name, 'w') as zf:
                zf.writestr('data/model/renamed_but_same_bytes.x', original_bytes)
            html = (f'<script src="openxops.{fp}.js"></script>\n'
                    f"'openxops.{fp}.wasm'\n"
                    f'"{pack_name}"')
            (build / 'index.html').write_text(html)
            (build / 'LICENSE').write_text('license')
            (root / 'NOTICE.md').write_text('notice')

            real_lookup = stage_pages._original_md5s
            stage_pages._original_md5s = lambda: {original_md5}
            stage_pages.ROOT = root
            try:
                with self.assertRaises(ValueError):
                    stage_pages.stage(build, root / 'build-pages')
            finally:
                stage_pages._original_md5s = real_lookup

    def test_rejects_engine_js_with_qa_hook_export(self):
        with tempfile.TemporaryDirectory() as t:
            root = Path(t)
            build = root / 'build-web'
            _write_build(build)
            fp = 'cafebabecafebabe'
            js_name = f'openxops.{fp}.js'
            # Simulate a build compiled with OPENXOPS_TEST_HOOKS=1 leaking
            # into the engine JS via an exported QA hook wrapper.
            (build / js_name).write_text(
                '// engine js\nvar _qa_pose_preview = Module["_qa_pose_preview"];\n')
            (root / 'NOTICE.md').write_text('notice')
            real_originals = stage_pages._original_md5s()
            stage_pages._original_md5s = lambda: real_originals
            stage_pages.ROOT = root
            with self.assertRaises(ValueError):
                stage_pages.stage(build, root / 'build-pages')

    def test_reference_json_shape_is_readable(self):
        data = json.loads((stage_pages.ROOT / 'tools/manifest/xops096_reference.json').read_text())
        self.assertIn('files', data)


if __name__ == '__main__':
    unittest.main()
