// webrtc.js — WebRTC transport using a vendored, pinned Trystero build
// (Nostr strategy: serverless signaling over public Nostr relays).
//
// See web/vendor/trystero/VENDOR.md for exact version/source/hashes.
//
// Topology: star, same contract as loopback.js — clients only exchange
// data with the host, the host can reach any single client or broadcast.
//
// Trystero 0.26.0's room/action objects use *property-assignment*
// listeners rather than listener-registration function calls, e.g.
// `room.onPeerJoin = (id) => {...}` (not `room.onPeerJoin(cb)`), and
// `makeAction(ns)` returns a single `{ send, onMessage, ... }` object
// (not the older `[sendAction, getAction]` tuple). Verified directly
// against the vendored build before writing this file.
//
// Channels: Trystero's `makeAction(namespace)` creates one named action
// (effectively one logical channel) multiplexed over the underlying
// RTCPeerConnection's data channel(s); it does not expose a knob for
// unreliable/unordered delivery per action, so **both** 'r' (reliable)
// and 'u' (unreliable) logical channels required by NET-PROTOCOL.md end
// up carried over the same underlying reliable, ordered transport. This
// is a documented limitation: there is currently no packet loss/reorder
// tolerance gained from marking a message unreliable with this vendored
// build. If a future Trystero release exposes RTCDataChannelInit options
// through makeAction, 'u' should be switched to a true unreliable channel.
//
// ICE: STUN from Google and Cloudflare, no TURN by default (symmetric NAT
// will fail to connect peer-to-peer in that case — see ONLINE-PLAN.md).
// Pass {turn: [{urls, username, credential}, ...]} to add TURN servers.

import { joinRoom } from '../vendor/trystero/nostr.mjs';

const APP_ID = 'xops-online-v1';

const DEFAULT_ICE_SERVERS = [
  { urls: 'stun:stun.l.google.com:19302' },
  { urls: 'stun:stun.cloudflare.com:3478' },
];

export function createWebrtcTransport({ room, role, name, turn } = {}) {
  if (role !== 'host' && role !== 'client') {
    throw new Error('createWebrtcTransport: role must be "host" or "client"');
  }
  if (!room) throw new Error('createWebrtcTransport: room is required');

  const iceServers = DEFAULT_ICE_SERVERS.concat(Array.isArray(turn) ? turn : []);
  const trystero = joinRoom({ appId: APP_ID, rtcConfig: { iceServers } }, room);

  const reliableAction = trystero.makeAction('r');
  const unreliableAction = trystero.makeAction('u');

  let closed = false;
  const messageCbs = [];
  const joinCbs = [];
  const leaveCbs = [];

  // Trystero peer ids are opaque strings; map them to small local ints.
  const localIdByRemote = new Map();
  const remoteByLocalId = new Map();
  let nextLocalId = 1;

  function localIdFor(remoteId) {
    let id = localIdByRemote.get(remoteId);
    if (id === undefined) {
      id = nextLocalId++;
      localIdByRemote.set(remoteId, id);
      remoteByLocalId.set(id, remoteId);
    }
    return id;
  }

  function emit(cbs, ...args) {
    for (const cb of cbs.slice()) {
      try {
        cb(...args);
      } catch (err) {
        console.error('[xops-net webrtc] listener error', err);
      }
    }
  }

  // Trystero 0.26.0 room listeners are property setters, not registration
  // functions — see the module header note above.
  trystero.onPeerJoin = (remoteId) => {
    const id = localIdFor(remoteId);
    emit(joinCbs, id);
  };

  trystero.onPeerLeave = (remoteId) => {
    const id = localIdByRemote.get(remoteId);
    if (id === undefined) return;
    localIdByRemote.delete(remoteId);
    remoteByLocalId.delete(id);
    emit(leaveCbs, id);
  };

  function handleIncoming(payload, metadata) {
    const remoteId = metadata && metadata.peerId;
    const bytes = payload instanceof Uint8Array ? payload : new Uint8Array(payload);
    emit(messageCbs, localIdFor(remoteId), bytes);
  }

  reliableAction.onMessage = handleIncoming;
  unreliableAction.onMessage = handleIncoming;

  function send(peerId, bytes, reliable) {
    if (closed) return;
    const payload = bytes instanceof Uint8Array ? bytes : new Uint8Array(bytes);
    const action = reliable ? reliableAction : unreliableAction;
    const options = {};
    if (peerId !== null && peerId !== undefined) {
      const remoteId = remoteByLocalId.get(peerId);
      if (!remoteId) return;
      options.target = remoteId;
    }
    // action.send() is async; failures are logged by Trystero itself, and
    // callers of this transport treat send() as fire-and-forget (matches
    // loopback.js and the NET-PROTOCOL.md contract).
    action.send(payload, options).catch((err) => {
      console.error('[xops-net webrtc] send failed', err);
    });
  }

  return {
    // Trystero's own selfId is a string; expose a small local int that
    // matches the convention used for peer ids on this transport.
    selfId: 0,
    send,
    onMessage(cb) {
      messageCbs.push(cb);
    },
    onPeerJoin(cb) {
      joinCbs.push(cb);
    },
    onPeerLeave(cb) {
      leaveCbs.push(cb);
    },
    peers() {
      return Array.from(localIdByRemote.values());
    },
    close() {
      if (closed) return;
      closed = true;
      trystero.leave();
    },
  };
}
