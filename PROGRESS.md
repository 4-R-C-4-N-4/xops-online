# XOPS ONLINE progress

Plan: docs/ONLINE-PLAN.md. Protocol: docs/NET-PROTOCOL.md.
Forked from OpenXOPS-web at b4d1676 (history kept; its log is docs/UPSTREAM-PROGRESS.md).

## Phase 0 — repo
- Created from OpenXOPS-web with history; Pages deploy under /xops-online/.

## Phase 1 — engine seam (parent)
- `HumanCommand` (src/humancommand.h): held/pressed button bits + aim. `ObjectManager::ApplyHumanCommand` is now the only path from keyboard to a human; `maingame::InputPlayer` builds a command from the bindings (same calls, same order as before) and stores it in `LastPlayerCommand` for clients to send.
- Net control: `SetNetControlled(id)` makes the AI loop skip a human (driven by a peer).
- Puppet worlds: `SetNetPuppet(true)` keeps bullet/grenade/zombie hit effects but never changes HP.
- Replication: `NetHumanState` + `GetNetHumanState` (host) / `ApplyNetHumanState` (client: creates the human in the same slot, mirrors death, respawn, team, held weapon model; keeps the local player's own aim) / `ApplyNetPuppetMoves` (keeps puppets walking between snapshots). Host helpers `AddHumanIndexAt`, `RespawnHuman` (same slot, weapons released), `RemoveHuman`.
- Native harness (real engine): command movement equals direct control to 1e-3; semi-auto fires once per press, full-auto 15 shots/60 ticks; puppet headshot leaves HP 200 while host drops to 61; two ObjectManagers in one process stay in sync (position exact, team, weapon, death, same-slot respawn, removal). 287 tests OK; browser single-player still moves.
- Known: a puppet accelerates from rest after each snap (0.14 vs ~0.8 units/tick); phase 3 sends snapshots every tick, phase 6 smooths.

## In flight (Sonnet workers, deleg_ba3ca583)
- net-proto: src/net/protocol.{h,cpp} + native tests (phase 2).
- net-web: web/net transports (BroadcastChannel loopback, Trystero/Nostr WebRTC), lobby shell, node tests.
- dm-maps: DM HANGAR (new two-level arena) and DM FIELD (UFO FIELD variant), DM spawn points p1=60.
- Next for the parent: src/net/session (host/client loop on top of the protocol and the engine seam), C exports, direct mission launch via the existing `.mif -NoBriefing` command line, two-tab spike.

## Phases 2-3 — protocol, transports, session, two-window match (integrated)
- Merged workers: net-proto (b223ad5: protocol module, sanitizer + fuzz tests), net-web (c30325a: loopback + Trystero/Nostr WebRTC transports, lobby, 15 node tests; Trystero 0.26.0 vendored with sha256s), dm-maps (c3eb26f: DM HANGAR + DM FIELD, 10 DM spawns each, pickups, tests). DM provenance folded into the ledger.
- Parent: `ObjectManager::SpawnShot` split out of ShotWeapon (clients replay host shots without ammo/cooldown), shot log, last-attacker + headshot tracking for frag credit.
- Parent: `src/net/session.{h,cpp}` host/client session (join -> AddHumanIndex slot with own FFA team, per-peer command queue with press edges, snapshot per tick, SHOT/KILL/RESPAWN events, frags/deaths, 3 s same-slot respawn at the DM spawn farthest from players, 1.5 s spawn protection, HELLO retry, leave). `src/net/exports.cpp` C exports + JSON state for the HUD.
- maingame: armed session attaches at mission start; host runs pre/post tick around ObjectManager::Process; clients aim locally, send commands, apply snapshots, run no AI; DM disables mission events and win/lose; direct-launch debug overlay off in matches.
- Page: multiplayer panel (lobby + map choice), `web/net/game-net.js` glue (64 KiB staging buffer, JS-level map announcement so clients load the host's map), HTML scoreboard + kill feed overlay, no pause-on-blur during a match. Pages staging allowlists net/ and vendor/ files.
- Fixed from the first two-window run: DM maps rendered white (textures load from the map's own directory) -> textures copied beside the maps + a pack-wide test; map announcement raced the loopback welcome -> repeated announcement.
- Verified natively (harness): in-memory host+client session: join, command-driven movement replicated (view within 1.42 units), 65 snapshots in 2 s at ~55 B each, frag + headshot kill feed + mirrored scores, same-slot respawn, shot replay, leave.
- Verified in the browser (two windows, same-browser transport, DM HANGAR): client joins as slot 1, walks (host sees it), host headshots it (HUD on both: HOSTY 1/0, ALICE 0/1, "HOSTY [HEADSHOT] ALICE"), client shows STATE DEAD then respawns at a DM spawn after 3 s.
- Known gaps: no client-side prediction (own movement waits one host tick), ammo HUD on clients is not replicated, a hidden host tab stalls the match (browsers throttle background tabs), no bots/match timer yet, WebRTC not yet exercised inside the game.
- Live (deploy run 39688813518, engine 8228fab618f5ef7b, no QA exports): two browser windows on https://4-r-c-4-n-4.github.io/xops-online/ connected over the WebRTC transport (Trystero/Nostr signaling) in about 1 s; client joined DM FIELD as slot 1; scores mirrored on both HUDs. Both windows ran on one machine, so cross-NAT connectivity is still untested.
- CI fix: the manifest generator reads the pinned `upstream-1.080` tag; tags were not pushed to the new repo (pushed now).

## Phase 7a — better arenas (in progress)
User playtest: "the map is bad" (DM HANGAR): too small/cramped, hard to get around; wants more room, verified jump heights, hallways linking up and down, hanging cover on catwalks, Lockout-style levels + fast vertical travel + cover everywhere but not from all angles, and enough maps to play with a friend. Next arena: multi-level alien ship deck.
- Engine: gravity lifts (pd1 p1=61: radius p2, height p3, exit = rotation): steady climb ~1.2 units/tick, push toward the exit near the top; host/single-player only (puppets follow snapshots). Native test: rises 32 in ~31 ticks, exits along +x.
- Engine: MAX_BLOCKS 160 -> 400 (bigger arenas; classic data unaffected).
- `tools/dmcheck.py` + `tests/dm/dmcheck.cpp`: real-engine map gate (standing-spot graph with walk/jump/drop/lift edges, lift physics ride, one-way traps, sight-line fraction, spawn exposure, ambiguous ledge heights, void leaks). Jump limit measured with a step fixture: walking + jump clears 16, not 17.
- The original DM HANGAR fails the gate exactly as the playtest said: play area 42.6k (the catwalk is cut off from the floor), 87% of pairs see each other, spawns visible from up to 96% of the map. DM FIELD (open field) also fails sight lines (64%).
- Rules in docs/DM-MAP-GUIDE.md. Workers dispatched: DM HANGAR rebuild, DM SHIPDECK (new), DM SPIRE (new, Lockout-inspired towers + lifts).

### DM SHIPDECK integration acceptance
- Sonnet workers exhausted provider rate limits; preserved all partial work and resumed each in isolated Codex CLI processes.
- Integrated SHIPDECK: 134 blocks, three decks (0/32/64), 12 spawns, 9 pickups, 3 lifts. Parent reran DMCHECK: sightline fraction 0.129, worst spawn exposure 0.19, zero traps/ambiguous ledges/void leaks.
- Folded provenance sidecar into authoritative ledger, recomputed hashes, added required curation fields, retargeted asset test to ledger. Full suite: 306 tests OK; validator: 0 failures, 347 warnings.
- Parent browser acceptance on local test build: textured upper catwalk/reactor view inspected, host simulation running; rode lift from (30,0.5,20) to (37.05,64.48,20), HP 200. Not yet deployed; other two maps still in progress.

### DM HANGAR rebuilt and integrated locally
- 106 blocks, play area 96,448 (previously 42,624), three ramps and two gravity lifts, side rooms, sheltered spawns, hanging cover. Parent DMCHECK OK: sightline fraction 0.221, worst spawn exposure 0.24; zero trapped spots, ambiguous ledges or void leaks.
- Fixed native session test fixture: random spawn facing hit new cover, so use a known clear ground-floor lane for the command replication check; retained original movement/drift assertions. Full integrated suite: 309 tests OK. Validator: 0 failures/347 warnings.
- Browser: hosted rebuilt hangar, inspected textured ground/upper routes; gravity lift raised player from y0.5 to upper platform y~30 with HP 200. No deployment yet, awaiting SPIRE integration.

### Four-arena release candidate
- Integrated SPIRE: 72 blocks, three lifts, 10 spawns/8 pickups, 97,808 play area. Parent reran DMCHECK OK: sightline fraction 0.199, worst spawn exposure 0.21, zero traps/ambiguous ledges/void leaks.
- Combined suite: 314 tests OK. Pack validator: 0 failures, 360 warnings. Four lobby entries share stable map indices (hangar/field/shipdeck/spire).
- Parent browser SPIRE check: textured routes/bridges, running host; lift carried player from (-87,0.5,0) to (-94.03,32.32,0), HP 200. All three indoor arenas have local browser rendering/lift checks, not full multiplayer combat playthroughs.
- Building clean production candidate without QA exports, then deploying and checking live.

### Four arenas live
- Deployment https://github.com/4-R-C-4-N-4/xops-online/actions/runs/39697792675 succeeded for 345ed05. Live engine 73b17dcd0bf70bc5, no QA exports; all four arena choices present.
- Parent opened live SPIRE host and joined a second browser window over WebRTC; client role 2/local slot 1 and both players in scoreboard, correct map rendered. Same machine only, not cross-NAT. Local clean production SHIPDECK host/client join also passed.
- Play: https://4-r-c-4-n-4.github.io/xops-online/ . Remaining networking limits unchanged: no prediction, client ammo HUD incomplete, hidden host stalls, no TURN. Numeric layout gates and lift rides are not full combat playtesting; further map feel feedback remains useful.

### Launcher Operate screen pass (local worktree; parent acceptance pending)
- Replaced the development-preview wall of text with a flat charcoal/magenta Operate menu: one player name/connection selector, adjacent Host + arena and Join + room-code controls, bitmap headings/action labels, visible status/errors and keyboard focus. Responsive short-desktop and narrow-screen rules retain 44px targets.
- Moved classic archive/single-player, original download/privacy information and saved-ZIP feedback into an explicit secondary disclosure; diagnostics log is independently collapsed. Removed obsolete one-starter-mission copy. Existing launch, archive, preset, pause/dialog, fullscreen, engine and transport contracts retained; no C++ or networking changes.
- Added offline SVG bitmap-label generation from our authored ASCII entries, CC0/provenance notes, packaging copy support and an explicit Pages staging allowlist. No original game assets or Downloads archives inspected; fontTools unavailable, so no WOFF dependency added. Launcher images also resolve correctly from the net demo page.
- Added actual lobby DOM/controller regressions for share-link prefill, validation, Host/Join callback contracts, shared name/map placement, blocked storage and clipboard fallback. Added page-ID/disclosure/glyph reproducibility checks and a staging test that excludes unlisted neighboring files. Design and parent browser checklist: docs/LAUNCHER-DESIGN.md.
- Verification: node --test tests/web-net/*.test.mjs passes all 3 test files. Targeted Python launcher/staging/packaging/browser-controls checks: 16 passed. Full requested Python suite was run; after fixing regressions and using LSAN_OPTIONS=detect_leaks=0 for the sandbox ptrace limitation, final run completed 309 tests with one setup error: test_char_atlas cannot download the uncached pinned public-domain Shinonome tarball (DNS/network restricted). No test source was weakened or skipped to conceal this blocker.
- Production build succeeded with PATH pointing at the installed Emscripten and EM_CACHE=/tmp/xops-launcher-emcache (copied from the read-only installed cache); the emsdk setup script otherwise clears EM_CACHE. Engine ddf440be7cc08601, pack 7b58dcba92fbaea6. Existing resource.cpp unknown escape warning remains. python3 tools/stage_pages.py succeeded with all launcher assets/license staged; no QA engine exports. Final page/assets repackaged after the UI edits. git diff --check clean.
- No commit, push or deployment. Parent owns 1280x720 / 1280x577 / 390px browser geometry, visual and gameplay acceptance; these are documented targets, not claimed browser results.

### Launcher look-and-feel acceptance
- Parent tightened worker design: actual DEATHMATCH label instead of internal Operate wording, larger title, removed decorative action rails; corrected font provenance to public-domain Shinonome glyphs (same family as free-pack results text), not hand-authored ASCII.
- 317 Python tests OK, 18 Node tests pass. Clean production package/staging succeeds. No QA exports.
- Browser: 1280x577 fits without scrolling; 390x844 no horizontal overflow/broken images; Controls dialog and invalid code feedback work; share URL prefills room; WebRTC host and second-window Join launched SPIRE with both names on scoreboard.
- Original proprietary font/data never inspected; web labels are local bitmap SVGs, regenerated offline without font runtime dependencies. Publishing launcher pass.
