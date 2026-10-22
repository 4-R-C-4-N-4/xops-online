#!/usr/bin/env python3
"""Offline SVG bitmap labels from public-domain Shinonome ASCII atlas entries.
No proprietary assets, downloads, or font dependencies. Output: CC0-1.0.
"""
from pathlib import Path
import re
ROOT = Path(__file__).resolve().parents[2]
LABELS = {'title': 'XOPS ONLINE', 'operate': 'DEATHMATCH', 'host': 'HOST',
          'join': 'JOIN', 'copy': 'COPY LINK', 'controls': 'CONTROLS',
          'fullscreen': 'FULLSCREEN', 'resume': 'RESUME GAME', 'close': 'CLOSE',
          'forget': 'FORGET ZIP', 'start': 'START GAME'}


def render(text):
    source = (ROOT / 'src/platform/text/font_atlas.inc').read_text()
    glyphs = {int(code, 16): bytes(int(v, 16) for v in values.split(','))
              for code, values in re.findall(r'\{0x([0-9a-fA-F]+),\{([^}]+)\}\}', source)}
    cells = []
    for i, ch in enumerate(text):
        if ch == ' ':
            continue
        rows = glyphs[ord(ch)]
        for y in range(16):
            for x in range(8):
                if rows[y * 2] & (128 >> x):
                    cells.append(f'M{i*9+x} {y}h1v1h-1z')
    return (f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {len(text)*9} 16" '
            f'shape-rendering="crispEdges"><path fill="#f5f2f5" d="{"".join(cells)}"/></svg>\n')


def main():
    out = ROOT / 'web/ui'
    out.mkdir(exist_ok=True)
    for name, text in LABELS.items():
        (out / f'{name}.svg').write_text(render(text))


if __name__ == '__main__':
    main()
