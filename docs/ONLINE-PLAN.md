# XOPS ONLINE — goals, architecture, phases

XOPS ONLINE is a multiplayer fork of OpenXOPS-web (a browser/WebAssembly port
of the BSD-licensed OpenXOPS 1.080). It does not have to stay faithful: the
single-player port keeps that promise; this repo is free to add modes,
weapons and rules. It keeps the OpenXOPS-web toolchain (Emscripten build,
mapgen/modelgen/artgen, CC0 free pack, native engine harness) and pulls fixes
from it occasionally.

## Goals

1. Fast path to deathmatch: friends open a link, join a room, and frag each
   other on a free-pack arena in the browser. No accounts, no install.
2. Zero paid infrastructure. GitHub Pages hosts the game; peers connect
   directly over WebRTC data channels; signaling uses free public relays.
3. Keep the old-timey XOPS feel: 30 Hz tick, original movement, weapons,
   hit zones, HUD. New features are additive.
4. Every phase ends at a demonstrable gate (native test + browser check).

Non-goals for the first release: anti-cheat, dedicated servers, matchmaking,
accounts, more than 8 players, classic mission co-op.

## Engine facts that shape the design (read from src/)

- Fixed tick: `GAMEFRAMEMS` (30 FPS) in `main.h`; the browser loop already
  runs fixed ticks with catch-up clamping.
- One local player: `ObjectManager::Player_HumanID`. `gamemain.cpp` and
  `objectmanager.cpp` reference the player id about 60 times (camera, HUD,
  pickups, sound listener, stats, F5 cheat).
- Player input and AI share one seam: `maingame::InputPlayer` turns keys into
  `ObjMgr.MoveForward(id)`, `ShotWeapon(id)`, `ReloadWeapon(id)`,
  `ChangeHaveWeapon(id, n)`, `MoveJump(id)`, `human::SetRxRy`; AI calls the
  same methods. A remote player is therefore "a human driven by commands".
- `maingame::Process` runs `ObjMgr.Process`, then `HumanAI[i].Process()` for
  every human except the player, then events and win checks.
- 96 human slots (`MAX_HUMAN`), team ids built in: team deathmatch is natural;
  free-for-all gives every player its own team.
- No networking code; 103 `rand()` call sites (lockstep would need a shared
  seed and is fragile, so it is rejected).

## Architecture: host-authoritative listen server in a browser tab

```
 client tab                               host tab (also a player)
 local input -> NetCmd ---- data channel ---> command queue per slot
                                              ObjMgr.Process + AI (bots)
 apply snapshot <--- Snapshot (15-30 Hz) ---- snapshot of humans/events
 render (puppet world, own camera)            render
```

- Host runs the real simulation. Each connected peer owns a human slot whose
  per-tick control comes from that peer's latest `NetCmd` instead of AI.
- Clients run the same engine in puppet mode: humans are placed from
  snapshots; no AI, no events, no damage applied locally; shot events from
  the host replay effects (tracers, sounds, muzzle flash).
- Engine/JS boundary: C exports (`net_*`) take and return byte buffers; the
  JS transport only moves opaque bytes. Protocol contract: NET-PROTOCOL.md.
- Transports behind one JS interface (`web/net/transport.js`):
  - `loopback`: BroadcastChannel between two tabs of the same origin
    (phase 1, no network).
  - `webrtc`: Trystero (MIT, vendored, pinned) for serverless signaling over
    public Nostr relays / BitTorrent trackers; STUN from Google and
    Cloudflare (`stun.cloudflare.com:3478`, free, no account). TURN (for
    symmetric NAT) is optional later: Cloudflare Realtime TURN through a tiny
    Worker that mints short-lived credentials (needs the owner's free
    Cloudflare account; the client must work without it).
- Room = random code in the URL (`#room=abc123`); first in is host
  (explicit "Host" / "Join" buttons in the lobby).

## Phases and gates

| # | Phase | Gate |
|---|-------|------|
| 0 | Repo, docs, Pages deploy under /xops-online/ | live URL loads the game |
| 1 | Engine seam: `NetCmd` built from local input and applied by one function; slots table; AI skips net-driven humans; client puppet mode | native harness: recorded commands drive a human identically to direct input; puppet mode applies a snapshot without AI/damage |
| 2 | Protocol module (`src/net/`): NetCmd + Snapshot encode/decode, quantised, versioned | native round-trip and size tests |
| 3 | Two-tab spike over BroadcastChannel | browser: tab B moves/shoots, tab A sees it; kill on host shows on both |
| 4 | Deathmatch rules: respawn at spawn points with spawn protection, frag/death counts, frag and time limit, scoreboard (Tab), bots fill empty slots | native: kill -> respawn -> score; browser scoreboard |
| 5 | WebRTC transport + lobby (Host/Join, room link, names, ping) | two machines/browsers play a round over the internet |
| 6 | Feel: client-side prediction of own movement, interpolation of others, host lag compensation for hits | measured: own movement latency 0 ticks; remote jitter hidden at 100 ms RTT |
| 7 | New features: alien weapons in DM pool, arena maps (UFO field, ship deck), low-gravity arena, team DM | per feature |

Fast-path target: phases 0-4 give a playable LAN-feel deathmatch between
tabs; phase 5 makes it online.

## Work split (at most 3 concurrent Sonnet workers, bounded files)

- Parent: engine seam (phase 1), integration glue/exports, browser verification, releases.
- Worker A: `src/net/` protocol module + native tests (phase 2).
- Worker B: `web/net/` transports (loopback + Trystero WebRTC), lobby UI shell, node tests.
- Worker C: deathmatch arena maps and DM spawn points via mapgen (content).

## Decisions

- Separate repo; BSD notices retained; unofficial.
- Host-authoritative, not lockstep (103 rand() sites, browser input delay).
- No paid servers; public relays only; TURN optional.
- Keep classic single-player missions playable offline in this fork too.
