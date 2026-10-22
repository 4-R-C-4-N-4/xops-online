# DM SPIRE

Original sealed reactor outpost: west and east towers bracket a courtyard;
a north spire carries a high gallery. Ten protected spawns and eight pickups
span y=0, 32 and 64. Three gravity lifts reach the middle level. Four ramps
provide alternate travel: two tower ramps, the north ground ramp, and the
stacked upper ramp. North dogleg halls connect the towers to railed bridges;
hanging baffles interrupt views from above. Drop one band at a time (32
units) for a safe descent. The ground dividers have openings at both ends
and doorways through their middle, providing routes around the reactors.

General arena principles (vertical loops, alternate approaches, contested
weapons, flankable cover) inspired the design. No Lockout geometry, external
reference map, proprietary archive, xopsdata, or proprietary derivative was
used. Textures are byte-identical copies of the existing generated CC0
`data/ufoescape` textures. Asset hashes and source paths are recorded only
in the root `PROVENANCE-ADD.jsonl` integration sidecar.

![Height-sliced top-down plans](img/dm/spire_layout.png)

Gold is ramp geometry, yellow is a DM spawn, orange is a pickup, and cyan
is a gravity lift. Plans show the three height bands; grey includes walls
and overhead structures intersecting player height.

Regenerate from the repository root:

```sh
python3 tools/mapgen/examples/dm_spire.py
python3 -m tools.mapgen build tools/mapgen/examples/dm_spire.json -o assets/pack/data/dmspire --pd1-name spire.pd1
python3 -c 'from pathlib import Path; from tools.artgen.briefing_pictures import generate_dmspire, generate_spire_layout; generate_dmspire(Path("assets/pack/data/briefing")); generate_spire_layout(Path("docs/img/dm/spire_layout.png"))'
python3 tools/dmcheck.py data/dmspire spire.pd1 --sealed
python3 -W ignore -m unittest discover -s tools/tests
```

Measured native engine gate: 72 blocks, 7,596 standing spots, 97,808 units
of connected play area; all three lifts settle at y=32.3. Sightline fraction
0.199, worst spawn exposure 0.21, trapped fraction 0.0000, ambiguous ledges
0 and void leaks 0 (2,038 samples). Result: `DMCHECK OK`.

The native gate validates collision, lift physics and connectivity. Browser
play and visual verification were not performed.

Full requested unittest discovery on 2026-10-21 ran 300 tests in 32.844s:
`FAILED (failures=2, errors=1)`. The blockers are external to the arena:

- `test_char_atlas.CharAtlasTest.setUpClass`: Shinonome font tarball download
  fails with `Temporary failure in name resolution`.
- `test_net_protocol`: LeakSanitizer reports a fatal sandbox/ptrace error.
- `test_sync_manifest_status`: `data/missions.txt` has a stale generated hash
  because this mission changes the list. The shared manifest is deliberately
  untouched; its hash needs updating during integration alongside the sidecar.

No thresholds or engine/gate sources were changed. The engine harness's
mission-count assertion remains exact, now six.

Targeted rerun with `ASAN_OPTIONS=detect_leaks=0` passed all 18 tests in
2.199s (`test_net_protocol`, `test_dm_maps`, `test_dm_spire`). This verifies
protocol assertions but does not establish leak freedom. The sealed gate
was rerun afterward and again printed `DMCHECK OK`.

Commit was attempted but the sandbox mounts this worktree's Git metadata
read-only: Git cannot create `index.lock`. Changes remain uncommitted; no
push was attempted.
