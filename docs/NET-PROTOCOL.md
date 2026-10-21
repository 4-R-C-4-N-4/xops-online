# XOPS ONLINE network protocol (v1)

Contract between the engine (`src/net/`), the C exports, and the JS
transports. All multi-byte values little-endian. Every message starts with a
1-byte type and the 1-byte protocol version (`NET_PROTOCOL_VERSION = 1`).
Transports deliver whole messages (no framing needed); unreliable/unordered
delivery is allowed for `CMD` and `SNAP`, reliable ordered for the rest.

## Message types

| Type | Name | Dir | Reliable | Body |
|------|------|-----|----------|------|
| 1 | HELLO | client->host | yes | name: u8 len + UTF-8 (<= 15 bytes) |
| 2 | WELCOME | host->client | yes | slot u8, map id u8 (index into DM map list), tick u32, seed u32 |
| 3 | CMD | client->host | no | count u8 (1..4), then `count` NetCmd records (newest last; redundancy against loss) |
| 4 | SNAP | host->client | no | Snapshot (below) |
| 5 | EVENT | host->client | yes | count u8, then events |
| 6 | BYE | either | yes | reason u8 |
| 7 | SCORE | host->client | yes | count u8, then {slot u8, frags i16, deaths i16, name len u8 + bytes} |

## NetCmd (11 bytes)

| Field | Type | Notes |
|-------|------|-------|
| seq | u32 | client tick counter, increasing |
| buttons | u16 | bit0 forward, 1 back, 2 left, 3 right, 4 walk, 5 jump, 6 fire, 7 reload, 8 switch weapon, 9 weapon slot 1, 10 weapon slot 2, 11 drop weapon, 12 scope toggle, 13 fire-mode toggle (shotmode) |
| rx | i16 | yaw, radians * 10430.378 (2^16 / 2pi), wrapped |
| ry | i16 | pitch, radians * 10430.378, clamped to +-70 deg |
| weapon_hint | u8 | reserved 0 |

Every `buttons` bit carries the HELD key state for that tick (not an edge).
The host derives press edges itself by comparing a slot's current command
against the last command it applied for that slot; it tracks the last
applied seq per slot and replays missed redundant commands in order.

## Snapshot

Header: tick u32, ack_seq u32 (last applied seq of the receiving client),
human_count u8, then per human (only enabled humans):

| Field | Type | Notes |
|-------|------|-------|
| id | u8 | human slot 0..95 |
| x, y, z | i16 each | world units * 8 (range +-4096) |
| rx | i16 | as NetCmd |
| ry | i16 | as NetCmd |
| hp | i16 | |
| flags | u8 | bit0 dead, bit1 dead-motion finished, bit2 scope, bit3 invincible (spawn protection) |
| team | i8 | |
| param | u8 | human type id |
| weapon | u8 | selected weapon param id (255 none) |
| move | u8 | move mode/flags for animation (`GetMovemode(true)`) |

## Events

Each event: type u8 then body.

| Type | Name | Body |
|------|------|------|
| 1 | SHOT | shooter u8, weapon param u8, rx i16, ry i16 (client replays effect and sound, no damage) |
| 2 | KILL | killer u8, victim u8, weapon param u8, headshot u8 |
| 3 | RESPAWN | slot u8 |
| 4 | PICKUP | slot u8, weapon param u8 |
| 5 | MATCH | state u8 (0 warmup, 1 playing, 2 ended), seconds_left u16 |

## C exports (engine side)

- `int net_host_start(int max_players)`; `int net_client_start(void)`
- `int net_on_message(const uint8_t* buf, int len, int peer)` — JS -> engine
- `int net_poll_outgoing(uint8_t* buf, int cap, int* peer, int* reliable)` —
  engine -> JS, call until it returns 0; peer -1 = broadcast
- `void net_peer_left(int peer)`

JS side: `web/net/transport.js` exposes `{ send(peer, bytes, reliable), onMessage(cb), onPeerJoin(cb), onPeerLeave(cb), close() }`.
