# Mission spec: "UFO FIELD" (wave swarm)

Status: implemented and published. PRs #6/#7; live browser verified, 236 tests pass.
Play: https://4-r-c-4-n-4.github.io/OpenXOPS-web/ → UFO FIELD (first in the main mission list).
Engine background: docs/ARCHITECTURE.md (the section numbers below refer to it).

## 1. Concept

A UFO has landed in a fenced cow pasture at dusk. Aliens come down its ramp in
three waves. Hay bales and cows are the cover. The player is a rancher with a
scoped lever-action rifle, and can pick up the aliens' laser guns. The mission
is complete when every alien in all three waves is dead.

## 2. Player-facing requirements

| # | Requirement |
|---|---|
| R1 | Simple arena: open field, wooden fence around the edge (too tall to jump), a barn behind the UFO outside the fence, hay bales and cows for cover |
| R2 | The UFO has landed on the far (north) side with a ramp facing the player. Aliens come out of it |
| R3 | 3 waves, each with a different alien and weapon. A banner announces each wave |
| R4 | The mission completes only when all aliens in all three waves are dead |
| R5 | Alien weapons can be picked up from the bodies |
| R6 | The player starts with a scoped lever-action rifle whose damage is in line with the AK47 and AUG, and an empty second slot for alien weapons |
| R7 | Bright, creative colours for alien skins and laser bolts, distinct per weapon |
| R8 | Classic X operations data must load and play exactly as before |

## 3. Engine changes (all additive; classic data unaffected)

### E1. Append weapons (parameter.cpp; TOTAL_PARAMETERINFO_WEAPON 23 → 27)

Stats are modelled on the upstream table (values from parameter.cpp):
AK47 atk 65 / pen 1 / rate 4 / speed 11, AUG 60 / 1 / 5 / 11 with scope 1,
M1 90 / 0 / 8, DE 68 / 1 / 5, P90 42 / 1 / 4, M249 59 / 1 / 4.

| id | name | Based on (hold, flash, model offsets) | atk | pen | blazings (rate) | speed | mag | spread min-max | scope | bolt | sound |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 23 | LEVER 30-30 | M1 rifle hold | 64 | 1 | 9 (lever cycle) | 12 | 8 | 1-14 | 1 (AUG optic) | 0 (normal bullet) | bang3 (M1/DE heavy) |
| 24 | ZAP PISTOL | DE pistol hold | 30 | 0 | 5 | 16 | 12 | 3-30 | 0 | 2 pink | laser1.wav |
| 25 | PHOTON SMG | P90 hold | 15 | 0 | 5 | 16 | 40 | 3-30 | 0 | 3 cyan | laser2.wav |
| 26 | PLASMA RIFLE | M249/M4 hold | 28 | 1 | 6 | 15 | 30 | 3-28 | 0 | 4 lime | laser3.wav |

The laser weapons eject no shell casings. Files go under data/model/weapon/:
lever.x/.bmp, zap.x/.bmp, photon.x/.bmp, plasma.x/.bmp.

### E2. Bolt type per weapon (bullet table, TOTAL_PARAMETERINFO_BULLET 2 → 5)

- New `WeaponParameter.bullettype` field (default 0 = data/model/bullet.x).
  Bullets record it in `ShotWeapon`, and `bullet` rendering picks
  `Resource` bullet model/texture by type.
- Types 2/3/4: data/model/laser.x (an elongated bolt) with
  laser_pink.bmp (#ff2fd6), laser_cyan.bmp (#2ff7ff) and laser_lime.bmp
  (#9dff2f). Type 1 stays the grenade.

### E3. Laser sounds

Add laser1/2/3.wav to the weapon sound list as new sound ids that existing
weapons never use. Existing ids 0-5 and 13 keep their meaning.

### E4. Append human types (TOTAL_PARAMETERINFO_HUMAN 43 → 47, TOTAL_HUMANTEXTURE 30 → 34)

| id | name | texture | hp | AI level | Weapon[0] / [1] |
|---|---|---|---|---|---|
| 43 | RANCHER (player) | rancher.bmp (denim + straw hat band) | 200 | 2 | 23 lever / none |
| 44 | ALIEN DRONE (wave 1) | alien_teal.bmp (teal skin, silver suit, big black eyes) | 80 | 0 | 24 zap |
| 45 | ALIEN TROOPER (wave 2) | alien_violet.bmp (violet skin, orange suit) | 100 | 0 | 25 photon |
| 46 | ALIEN BRUTE (wave 3) | alien_lime.bmp (lime skin, black and gold armour) | 120 | 0 | 26 plasma |

### E5. Waves: held humans + 2 event opcodes (event.cpp reserved slot)

- New point type `POINT_P1TYPE_STAGED_HUMAN = 50`, with the same parameters as
  type 1 (p2 = HUMANINFO point id, p3 = AI path, p4 = identifier = **wave
  group id**). It loads as a held spawn record (not drawn, simulated or
  counted as alive), with its skin preloaded. A human/weapon slot is allocated
  only when deployed. Classic data never uses p1 = 50.
- Event 40, "wait for hostiles": continue only when the number of living,
  enabled humans on a team other than the player's is at most p2.
- Event 41, "deploy wave": enable every held human with p4 == p2, re-init its
  AI, then play a teleport flash + sound at its position. A failed allocation
  stays pending and cannot count as a kill. The event range
  check at event.cpp:274 is extended for 40/41.
- `CheckGameOverorComplete`: completion also requires that **no held humans
  remain**. The order inside one frame (completion check before events) is
  unchanged.

### E6. Limits

MAX_MODEL and MAX_TEXTURE go from 72 to 128 (d3dgraphics.h:49-50): the classic
ZIP plus the 4 new weapons and 3 bolt textures would exceed 72.

## 4. Content

- Mission file: `addon/ufo.mif`, which shows up in the menu's ADDON list
  (AddonList reads `addon/*.mif` through `vfs_list`). Sky 5 (dusk). The one
  addon prop type is the **cow** (cow.x/cow.bmp, decide 9, hp 127, moo.wav). Its back is 22 world units
  tall (player 20). Optional `addon/cow.txt.cover` supplies a rotated body
  hull (`slot halfwidth min_y max_y halflength`: `0 4.5 6 22 11`). Lasers
  stop on the body while it lives; shots above it pass, and dead cows no
  longer block. Without a sidecar, original prop collision is unchanged.
- Map: `data/ufofield/temp.bd1` + `ufo.pd1` + `ufo.msg`, authored as
  `tools/mapgen/examples/ufo_field.json`. Block textures (10 max): grass,
  dirt, hay, fence wood, barn red, barn roof, white trim, UFO hull, UFO
  light, dark interior.
- Layout (world units, player height 20): continuous ground x=-280..280,
  z=-400..280, enclosed by solid 96-high barn-red storage walls. These are
  deliberately tall walls, not a modeled tree line. The 24-high pasture
  fence is internal, with 60-wide north gates on both sides of the UFO.
  Ground continues outside that fence, so jumping over it is safe.
  The through barn at x=-72..72, z=-356..-260 has open north/south doors,
  a west loft at 36, two hay stair routes rising by 12 per jump,
  34 units of clearance underneath and 44 units of loft headroom.
  Yard hay provides low and tall cover with routes around either side.
  The UFO, south ramp, existing pasture hay, eight cows, spawn and alien
  paths retain their original positions. The yard adds a quieter northern
  flank; it does not move the scripted fighting out of the pasture.
  [North-up layout](img/ufofield_layout.png) omits the barn roof to show
  the interior. Source: `tools/mapgen/examples/ufo_field.py`, whose
  `build_doc()` generates the committed JSON during the map rebuild.
- Waves: wave 1 = 6 drones (p4 1), wave 2 = 8 troopers (p4 2), wave 3 =
  6 brutes (p4 3). All are held humans at the hatch top. Event line 0:
  wait 5 s → msg "WAVE 1" → deploy 1 → wait hostiles = 0 → wait 4 s →
  msg "WAVE 2" → deploy 2 → wait = 0 → wait 4 s → msg "FINAL WAVE" → deploy 3. Completion is automatic
  (E5).
- Briefing picture: generated from the map JSON (reuse
  tools/artgen/briefing_pictures.py). Briefing text ≤ 14 lines × 52 chars.

## 5. Verification gates

1. Unit/format tests for every new asset (BMP/X/WAV rules), wave and event
   logic in a native harness (staged human not counted; complete only after
   wave 3), map perimeter test (fence closed, no gaps), validate_pack 0 FAIL.
2. Classic regression: original ZIP Training Yard still loads and plays.
3. Browser (test build, QA hooks): mission appears in ADDON list. Briefing
   readable. Wave banners appear, aliens appear at the hatch with colour bolts.
   Pick up an alien weapon. Kill-all hook steps through waves 1→2→3 →
   MISSION COMPLETE only after wave 3. Close-ups of each alien and the
   rifle's scope.
4. Production build has 0 `_qa_` symbols. PR, CI and deploy, then live check.

## 6. Effort

| Stream | Effort | Owner |
|---|---|---|
| Engine E1-E6 + harness tests | M | Sonnet worker `ufo-engine` |
| Models / textures / sounds | L | Sonnet worker `ufo-assets` |
| Map, points, mif, msg, briefing | M | Sonnet worker `ufo-map` |
| Integration, browser QA, deploy | M | parent |
