# XOPS ONLINE launcher acceptance

The primary surface is the Operate menu: charcoal/black, white bitmap lettering,
flat borders, restrained magenta. No gradients, glass, marketing hero, or external
fonts. The results-screen reference is source-only: `result::Render2D` scales
texture text and paints RESULT magenta. We do not reproduce the original font.
Local SVG bitmap labels use the public-domain Shinonome ASCII glyphs in our atlas, generated offline
with `python3 tools/fonts/build_launcher_labels.py`; provenance and CC0 dedication
ship as `ui/FONT-LICENSE.md`. fontTools was unavailable; no dependency was added.

One shared player name and connection selector precede two parallel actions:
Host with arena; Join with code. Hash-prefilled join remains explicit (press Join),
with unchanged room/name/transport callback arguments. Storage denial no longer
prevents a lobby from mounting or launching. WebRTC/signaling contracts and all
engine code are unchanged. Classic archive, single-player, download links and
local-storage/privacy explanation are under a labeled disclosure. Diagnostics
are independently collapsed. Status/errors remain outside those disclosures.

The layout targets 1280×720 and 1280×577 with primary play controls visible without
scrolling. Short desktop viewports reduce gaps and use inline identity labels;
secondary disclosures share a row. At 390px, identity fields stack and Host/Join
remain adjacent. Inputs, selects, buttons and summaries have at least 44px targets.
Long status/errors wrap. Body text uses a readable system monospace font.

Parent browser acceptance (pending):

- At 1280×720 and 1280×577, confirm initial play controls are visible without scroll.
- At 390px, confirm no horizontal overflow, readable selects and keyboard focus.
- Check bitmap labels load from the staged site, including secondary controls.
- Host each selectable arena; join a second window from its share URL; check errors,
  room-code validation, connection choices, and clipboard fallback.
- Exercise classic/free launch, ZIP selection/forget, preset persistence, controls,
  stacked pause/resume reasons, pointer lock and fullscreen.
- Expand classic and diagnostics to verify content, download/privacy links and log.

Automated coverage exercises actual lobby DOM construction and host/join callbacks,
share URL prefilling, validation, storage denial, unique page IDs, collapsed sections,
reproducible glyph outputs, packaging, and explicit asset staging allowlists. It does
not substitute for browser geometry or multiplayer acceptance.
