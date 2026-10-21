// transport.js — single factory for both network transports.
//
// createTransport({kind, room, role, name}) returns:
//   {
//     selfId,                          // this peer's local id (small int, host is always 0... see below)
//     send(peerId, bytes, reliable),   // peerId === null means broadcast
//     onMessage(cb),                   // cb(peerId, bytes: Uint8Array)
//     onPeerJoin(cb),                  // cb(peerId)
//     onPeerLeave(cb),                 // cb(peerId)
//     peers(),                         // -> array of connected peerIds
//     close(),
//   }
//
// peerId is a small integer assigned by the transport the first time it
// sees a remote peer, stable for the lifetime of the session. It never
// collides with the local selfId. Internally each transport maps the
// integer to whatever id its underlying channel uses (a BroadcastChannel
// client id, or a Trystero peer id string).
//
// Star topology (both transports): clients only exchange messages with the
// host; the host can reach everyone. A client calling send(peerId, ...)
// with a peerId other than the host's is a programming error upstream (the
// transport does not enforce it — see NET-PROTOCOL.md for the host's role
// in validating senders).

import { createLoopbackTransport } from './loopback.js';
import { createWebrtcTransport } from './webrtc.js';

export function createTransport(opts) {
  const { kind } = opts || {};
  if (kind === 'loopback') return createLoopbackTransport(opts);
  if (kind === 'webrtc') return createWebrtcTransport(opts);
  throw new Error(`createTransport: unknown kind "${kind}"`);
}
