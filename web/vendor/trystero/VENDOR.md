# Vendored Trystero (Nostr strategy)

Trystero provides serverless WebRTC signaling. XOPS ONLINE vendors the
Nostr-relay strategy only (public relays, no account, no server we run).

Pinned version: **0.26.0** (all three packages below are published in lockstep
at the same version by the Trystero project).

Source: npm registry, fetched with `npm pack <name>` on 2026-10-21 (so a
plain file copy, not a build from source — all files below are exactly the
published `dist/` output, only `nostr.mjs`'s two bare import specifiers were
rewritten to relative paths so the module works without a bundler/import-map;
no other byte was changed).

| Package | Version | Registry tarball | License |
|---|---|---|---|
| `trystero` (nostr entry only) | 0.26.0 | https://registry.npmjs.org/trystero/-/trystero-0.26.0.tgz | MIT |
| `@trystero-p2p/nostr` | 0.26.0 | https://registry.npmjs.org/@trystero-p2p/nostr/-/nostr-0.26.0.tgz | MIT |
| `@trystero-p2p/core` | 0.26.0 | https://registry.npmjs.org/@trystero-p2p/core/-/core-0.26.0.tgz | MIT |
| `@noble/secp256k1` | 3.2.0 | https://registry.npmjs.org/@noble/secp256k1/-/secp256k1-3.2.0.tgz | MIT |

## Files and sha256

```
nostr.mjs                      9b0a0df90531a09d99b99bccdff9647a055c6ba106ae99fa0f9aa5bce1c1af15
core/action-receiver.mjs       55b2c8548c7dd4118dd59eca832db6f85cacb1e26a645cbf68a37adbba536c7
core/action-wire.mjs           15f5cfcfcc13489e72a6f78b84dcd8b4f692455d0bb8bd92fbc6f808d62ece6
core/actions.mjs               1e3333876a1afb3634174acb64321e0d498c9c452668b92794a875ba3bda1be
core/crypto.mjs                ed5665176dbbc7344c93d8f4a8b2602139ed4152a1bc17d6bd8e70bbb9e24a9
core/data-limits.mjs           736fa233238dafb69f2b14ce58d8c144e16db2c97437935bf083d19bcee221c
core/handshake.mjs             07a7dab62ff5675e1e983651b6732881105d6f5ec2661049c269bee6b70920a
core/index.mjs                 48c0bf986ff3dd8d7d77292fbcbd81be6aa850209efffbb2dc36f93c70b4431
core/media.mjs                 a16a8b34969da04d1fa0bae3485b5151550c2729e0eea265f08e541a258c140
core/offer-manager.mjs         2cad766a71f6d0e8cc33c6c9d6680f102789298dd7431154325029b455caf20
core/peer.mjs                  baccc0775368af6c317d174269fb139bc3a096b144c1cfa6cabf009daeb4f49
core/room.mjs                  1439f38da220e2052f9cfbd35f148a1919520a6d79fcfa67a3e9415e4faf291
core/shared-peer.mjs           f743f0ebd78aebce5a589ce58de39271381ec239d7fb272dfec247c4f5b83a3
core/signal-handler.mjs        4f803dc0f5a9263971b5ad10bc48407c5ac90d501f05c1bfaff17d36234b7da
core/strategy.mjs              dc5d177c5db19ad672728729f2e7b42d669e6cf15a5a18df5eb7a2b02e15859
core/topic-strategy.mjs        c068735f27c8c5877c1cf64e0f515e6f3649f4468a59e240b0c6c503e3e008b
core/utils.mjs                 8da477fbc82ce08aafa8362a465fdddb9b1e68950a0558875b5fc89b3ee22f6
noble/secp256k1.js             ba5aadb469e9208f2da1c9878ab6bb93d2c894f84e6c7ebf9dbf9462741b9fb
LICENSE-nostr                  bbf91fd979faac0def9551c570e2e9f92b4e02d22f38ca5d98860b6284e1ea2
LICENSE-core                   bbf91fd979faac0def9551c570e2e9f92b4e02d22f38ca5d98860b6284e1ea2
LICENSE-noble                  394c2e6e5552e5dba202bee6390b9d6aa2754d657f5b9869e83b3d265a31550
```

(Recompute with `sha256sum nostr.mjs core/*.mjs noble/secp256k1.js LICENSE-*`
from this directory to verify.)

## What was changed from upstream

Only `nostr.mjs`'s two import lines were edited, from bare package
specifiers (`@noble/secp256k1`, `@trystero-p2p/core`) to relative paths
(`./noble/secp256k1.js`, `./core/index.mjs`), so the file loads directly as
a plain ES module with no import map or bundler. Every other vendored file
is byte-identical to its published npm `dist/` output.

## Why Nostr, not WebTorrent/IPFS/MQTT/Firebase/Supabase

Nostr relays are free, public, and require no account to use as a
signaling bus (unlike Firebase/Supabase) and are lighter-weight than
spinning up BitTorrent tracker swarms for signaling-only traffic. See
`docs/ONLINE-PLAN.md`.

## Known limitation: no transport-level unreliable channel

Trystero's `makeAction(namespace)` does not currently expose a way to
request an unreliable/unordered RTCDataChannel. `web/net/webrtc.js` creates
two logical channels ('r' and 'u') to match NET-PROTOCOL.md's reliability
split, but both are carried over Trystero's one (reliable, ordered)
default data channel. There is no loss/latency benefit from marking a
message `reliable: false` with this transport today — it is purely
forward-compatible plumbing. Revisit if a future Trystero release exposes
RTCDataChannelInit options through `makeAction`.
