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
