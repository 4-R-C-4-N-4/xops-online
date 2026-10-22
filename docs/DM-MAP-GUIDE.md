# Deathmatch map guide (XOPS ONLINE)

Read this before building or changing a DM arena. Every arena must pass the
real-engine gate:

    python3 tools/dmcheck.py data/<dir> <file>.pd1 [--sealed]

`--sealed` is for indoor maps (no view into the void). The gate builds the
actual engine collision/physics code natively and measures the map; it is
the acceptance test, not a suggestion. `python3 tools/dmcheck.py --jumpprobe`
re-measures the jump limit.

## Engine envelope (measured, not guessed)

| Quantity | Value | Source |
|---|---|---|
| Player height / eye | 20 / ~17 units | `HUMAN_HEIGHT`, camera |
| Collision radius | 2.8 | `HUMAN_MAPCOLLISION_R` |
| Walk-up step | 3.2 (treat 3.5 as the limit on ramps) | `HUMAN_MAPCOLLISION_CLIMBHEIGHT` |
| Steepest walkable slope | 50 deg; build ramps at <= 0.4 rise per unit | `HUMAN_MAPCOLLISION_SLOPEANGLE` |
| Jump onto a ledge (walking + jump) | 16 clears, 17 does not | `dmcheck --jumpprobe` |
| Safe drop | <= 40 | fall damage starts beyond |
| Gravity lift climb | ~1.2 units/tick (30 ticks/s) | `GRAVITYLIFT_SPEED` |
| Max blocks per map | 400 (raised from 160 in this fork) | `MAX_BLOCKS` |
| Max points per map | 256 | `MAX_POINTS` |

Heights therefore fall into clear bands; avoid 14.5..17.5 (players cannot
tell whether they can climb it, and the gate rejects it):

- <= 3: steps, walk over.
- 8..14: low cover / jumpable crates (protects legs and body, you shoot over it).
- >= 18 (use 22+ for full cover): walls, pillars, tall crates.
- Ceilings: >= 28 above any walking surface (jumping into a 20-unit-high
  ceiling feels awful). Hanging cover may come down to 26 above a catwalk.

## Point contract

| p1 | Meaning | Fields |
|---|---|---|
| 60 | DM spawn | p4 = index 0..N-1, rotation faces an open lane |
| 61 | Gravity lift | x/y/z = foot of the column (on the floor), rotation = exit direction (rotation r faces (sin r, cos r) in x/z), p2 = radius (5..8), p3 = height (top floor y minus foot y) |
| 1 | Player spawn | exactly one, human_type 43 (the map also loads as a mission) |
| 2 | Weapon pickup | p2 weapon id, p3 ammo |

A lift raises anyone standing in its column at a steady speed and pushes
them toward its exit direction near the top. Put the top floor's edge within
`radius + 1` of the column centre, on the exit side, with its surface at
`y + p3` (+-1). The gate rides every lift with real physics and fails it if
the player is not left standing on the top floor.

## Layout rules (why the first DM HANGAR failed)

The first hangar was one 200x200 box: 87% of random standing pairs could
see each other, every spawn was visible from 40-50% of the map, and the
catwalk was cut off from the floor. Movement in XOPS is limited (no crouch,
no sprint, slow strafe), so sight lines decide the game.

Inspired by classic arena design (Halo's Lockout, Quake 3's DM maps, Unreal's
Deck16) — principles only, never their geometry:

1. Several connected spaces, not one hall: rooms, corridors, a central
   arena, upper and lower levels. Every space has at least two exits; no dead
   ends longer than ~30 units.
2. Levels: at least two (three is better), joined by ramps, stairs, drops
   and at least one gravity lift (fast vertical travel). Hallways and ramps
   link upstairs and downstairs in more than one place.
3. Cover everywhere, but never from all angles: each piece of cover should be
   flankable from another route. Break every long lane after ~80-120 units.
4. Catwalks get cover too: hanging bulkheads/pipes/vats from the ceiling,
   railings 8..12 high, pillars.
5. Spawns tucked into side rooms or alcoves, facing out, not visible from the
   middle of the map; spread them so no two are within 40 units.
6. Weapons pull players through the map: power weapons (plasma 26, lever 23,
   grenades 13) in contested spots (top of a lift, middle of the arena),
   basic weapons near spawns.
7. Size: play area roughly 60k..200k square units for 2-8 players (the
   gate prints `play_area`). Ceilings and wall heights large enough that
   rooms feel open.

## Gate thresholds

| Metric | Requirement |
|---|---|
| `trapped_fraction` | <= 0.5% (spots you can reach but never leave) |
| spawns, pickups | each on a standing spot, reachable from spawn 0 and back |
| lifts | real physics leaves the player on the top floor |
| `sightline_fraction` | 0.10 .. 0.45 of random standing pairs see each other |
| spawn exposure | each spawn visible from <= 35% of the map |
| `ambiguous_ledges` | 0 (no ledges 14.5..17.5 above an adjacent floor) |
| `void_leaks` (`--sealed`) | 0 |
| ramps | every sloped block top (rise >= 2, slope <= 0.9, width >= 6): both ends open onto play area that is not the ramp, and the real player walks it up and down |
| `death_pit_edges` (`--outdoor`) | 0: no play spot can walk or jump (over anything lower than the jump) into a floorless column within 48 units |

`--mission` runs the same gate on single-player maps from the player's own
spawn and skips the deathmatch-only gates (spawn count, sight lines, spawn
exposure, ambiguous ledges are reported only).

## Rendering hygiene

Faces that share a plane and overlap flicker (z-fighting). `tools.mapgen
build` now pushes the smaller face of every such pair out by 0.08 units
(so flush details such as a dirt patch on grass stay visible)
(tools/mapgen/zfight.py), so overlapping corners and flush trims are safe to
author; `python3 tools/geomcheck.py --all` audits the committed binaries and
`python3 tools/rebuild_maps.py` rebuilds every pack map from its source.

## Content

- Textures load from the map's own directory: copy every texture named in
  temp.bd1 beside it (the pack test fails otherwise). Reuse the generated
  free-pack textures (ufoescape, ufofield sets); add provenance records.
- Wall textures are upright now (mapgen side-face UVs fixed); text-like
  textures (labpanel, hazard) read correctly.
- Write the map as a Python generator in `tools/mapgen/examples/dm_<name>.py`
  emitting JSON, built with `python3 -m tools.mapgen build`.
- Each DM map also gets a .mif/.msg, a briefing picture
  (`tools/artgen/briefing_pictures.py`) and an entry in
  `web/net/game-net.js` `DM_MAPS` plus the lobby map `<select>` in
  `web/index.html` (the parent wires the last two).
