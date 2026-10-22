#!/usr/bin/env python3
"""Publish matching fingerprinted JS/WASM names and atomically replace the page.
Run after em++ and after tools/build_pack.py. Injects the engine fingerprint
and the data-pack zip filename into the page; no game assets are packaged
here (tools/build_pack.py already wrote the pack zip into `build`).
"""
from pathlib import Path
import hashlib
import os
import shutil

ROOT = Path(__file__).resolve().parents[1]

def find_pack_name(build):
    packs = sorted(build.glob('openxops-data.*.zip'))
    if not packs:
        raise FileNotFoundError(
            'No openxops-data.*.zip in build dir; run tools/build_pack.py first')
    if len(packs) > 1:
        raise ValueError(f'Expected exactly one data pack zip, found {len(packs)}')
    return packs[0].name

def package(build, template):
    js = (build / 'openxops.js').read_bytes()
    wasm = (build / 'openxops.wasm').read_bytes()
    version = hashlib.sha256(js + wasm).hexdigest()[:16]
    js_name, wasm_name = f'openxops.{version}.js', f'openxops.{version}.wasm'
    shutil.copyfile(build / 'openxops.js', build / js_name)
    shutil.copyfile(build / 'openxops.wasm', build / wasm_name)
    pack_name = find_pack_name(build)
    html = template.read_text()
    for token in ('__ENGINE_SCRIPT__', '__ENGINE_WASM__', '__DATA_PACK__'):
        if token not in html:
            raise ValueError(f'Missing build token {token}')
    html = (html.replace('__ENGINE_SCRIPT__', js_name)
                .replace('__ENGINE_WASM__', wasm_name)
                .replace('__DATA_PACK__', pack_name))
    # Fixed, reviewed launcher assets only; no user or game data.
    shutil.copytree(ROOT / 'web/ui', build / 'ui', dirs_exist_ok=True)
    temporary = build / 'index.html.next'
    temporary.write_text(html)
    os.replace(temporary, build / 'index.html')
    return version, pack_name

if __name__ == '__main__':
    version, pack_name = package(ROOT / 'build-web', ROOT / 'web/index.html')
    print('Packaged engine build:', version, 'data pack:', pack_name)
