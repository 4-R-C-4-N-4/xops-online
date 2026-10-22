#!/usr/bin/env python3
"""Stage only the exact fingerprinted engine/page/data-pack files for public
hosting. Never copy the repository, xopsdata, user archives, or raw build
directory.
"""
from pathlib import Path
import hashlib
import json
import re
import shutil
import zipfile

ROOT = Path(__file__).resolve().parents[1]


def _original_md5s():
    reference = json.loads((ROOT / 'tools/manifest/xops096_reference.json').read_text())
    records = reference.get('files', []) if isinstance(reference, dict) else reference
    return {r['md5'] for r in records if r.get('md5')}


def _check_zip_members(zip_path, originals):
    """Raise if any member of zip_path (by content MD5) matches an original
    game asset, even if the member/entry is renamed inside the zip."""
    with zipfile.ZipFile(zip_path) as zf:
        for info in zf.infolist():
            if info.is_dir():
                continue
            data = zf.read(info)
            if hashlib.md5(data).hexdigest() in originals:
                raise ValueError(
                    f'Original game asset detected inside {zip_path.name}: {info.filename}')


def stage(build, destination):
    html = (build / 'index.html').read_text()
    scripts = re.findall(r'<script src="(openxops\.[0-9a-f]{16}\.js)"', html)
    wasm = re.findall(r"'?(openxops\.[0-9a-f]{16}\.wasm)'?", html)
    if len(scripts) != 1 or len(wasm) != 1:
        raise ValueError('Expected exactly one fingerprinted engine script and WASM reference')
    if scripts[0].split('.')[1] != wasm[0].split('.')[1]:
        raise ValueError('Engine fingerprints differ')
    engine_js = (build / scripts[0]).read_text()
    if '_qa_' in engine_js:
        raise ValueError(
            'Refusing to publish a diagnostic/test-hook engine build '
            '(engine JS exports a _qa_* QA hook symbol)')
    packs = re.findall(r'(openxops-data\.[0-9a-f]{16}\.zip)', html)
    if len(set(packs)) != 1:
        raise ValueError('Expected exactly one data-pack zip reference')
    pack_name = packs[0]
    names = ['index.html', scripts[0], wasm[0], pack_name, 'LICENSE']
    if 'ui/launcher.css' in html:
        names.extend('ui/' + name for name in (
            'launcher.css', 'title.svg', 'operate.svg', 'host.svg',
            'join.svg', 'copy.svg', 'controls.svg', 'fullscreen.svg',
            'resume.svg', 'close.svg', 'forget.svg', 'start.svg', 'FONT-LICENSE.md'))
    # Detect accidental original assets even if renamed; metadata only is committed.
    originals = _original_md5s()
    for name in names:
        path = build / name
        data = path.read_bytes()
        if hashlib.md5(data).hexdigest() in originals:
            raise ValueError('Original game asset detected: ' + name)
        if name.endswith('.zip'):
            _check_zip_members(path, originals)
    if destination.exists():
        shutil.rmtree(destination)
    destination.mkdir(parents=True)
    for name in names:
        (destination / name).parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(build / name, destination / name)
    shutil.copyfile(ROOT / 'NOTICE.md', destination / 'NOTICE.md')
    # XOPS ONLINE: static network modules (allowlisted file types only)
    allowed = ('.js', '.mjs', '.css', '.html', '.md') 
    for sub in ('net', 'vendor'):
        for path in sorted((build / sub).rglob('*')):
            if path.is_file() and (path.suffix in allowed or path.name.startswith('LICENSE')):
                target = destination / path.relative_to(build)
                target.parent.mkdir(parents=True, exist_ok=True)
                shutil.copyfile(path, target)
                names.append(str(path.relative_to(build)))
    (destination / '.nojekyll').touch()
    return names

if __name__ == '__main__':
    print('Public files:', stage(ROOT / 'build-web', ROOT / 'build-pages'))
