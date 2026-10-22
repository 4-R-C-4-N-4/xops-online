# Launcher bitmap lettering

The SVG labels are generated offline by tools/fonts/build_launcher_labels.py
from the public-domain Shinonome ASCII glyphs in src/platform/text/font_atlas.inc.
The font source and public-domain declaration are documented in
assets/fonts/README.md and assets/fonts/SHINONOME-LICENSE.txt in this repository.
The label layouts and generator additions are dedicated under CC0-1.0:
https://creativecommons.org/publicdomain/zero/1.0/ .

No original X operations assets are used. The classic results screen uses
scaled texture lettering (result::Render2D in src/gamemain.cpp). The launcher
uses the same public-domain glyph family as our generated game texture atlas,
with a magenta accent; it is not a reproduction of the proprietary original font.

Body and changing status text use the device's monospace font. SVG labels
have text alternatives and load locally, with no CDN or font dependency.
