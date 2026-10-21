# DM SHIPDECK

Original clean-room alien ship arena, authored in
`tools/mapgen/examples/dm_shipdeck.py`. No upstream map/archive data used.
Textures are byte-identical copies from the free-pack `data/ufoescape` set;
per-asset hashes and sources are in the root `PROVENANCE-ADD.jsonl` sidecar.
The shared provenance ledger and manifest are deliberately untouched.

Engineering at y=0, labs at y=32, and upper deck/crossing catwalks at y=64
surround a goo reactor. Three ramps and three gravity lifts connect the
floors; corridor partitions have end routes and central cross passages.
Goo troughs, lab crates, pipe pillars and roof-hung bulkheads provide cover.
Twelve spawns and nine weapon pickups are distributed across the decks.
The map is available through both the mission list and multiplayer lobby.

Rebuild:

```sh
python3 tools/mapgen/examples/dm_shipdeck.py
python3 -m tools.mapgen build tools/mapgen/examples/dm_shipdeck.json -o assets/pack/data/dmship --pd1-name ship.pd1
python3 tools/dmcheck.py data/dmship ship.pd1 --sealed
```

The sealed real-engine gate reports DMCHECK OK: 134 blocks, 16,170 standing
spots, play area 258,720, trapped fraction 0, sightline fraction 0.129,
longest sightline 344, worst spawn exposure 0.19, ambiguous ledges 0,
void leaks 0 (5,390 sampled spots). Lift resting heights are 32.3, 64.5,
and 64.5. The stacked play area exceeds the guide's suggested 200,000;
longest lanes also exceed its suggested 80–120 units, while all enforced
visibility, connectivity, collision and sealing gates pass.

Deterministic source/JSON/binary and briefing regeneration, packaged texture
identity, mission paths, sidecar hashes, and the sealed engine gate are
covered in `tools/tests/test_dm_shipdeck.py`. Browser verification was not
performed.

Full validation: `python3 -W ignore -m unittest discover -s tools/tests`
ran 306 tests with two failures: LeakSanitizer cannot operate under sandbox
ptrace in `test_protocol_roundtrip_and_fuzz`, and manifest synchronization
reports `data/missions.txt` as generated → missing because its updated hash
has not been merged into the shared provenance ledger. These shared files
are outside this task's permitted edits. All 15 focused DM map tests pass.

Protocol test passes with `ASAN_OPTIONS=detect_leaks=0`. Commit is blocked:
Git cannot create `/home/ivy/Work/xops-online/.git/worktrees/dm-shipdeck/index.lock`
because the shared worktree metadata is read-only in this sandbox. Nothing
was pushed.
