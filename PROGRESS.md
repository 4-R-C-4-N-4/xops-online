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
