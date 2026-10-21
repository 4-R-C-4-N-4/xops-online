// game-net.js — glue between a transport (transport.js) and the engine's
// network session (src/net/exports.cpp). The engine never sees the network;
// JS moves opaque protocol messages in and out through a shared 64 KiB
// staging buffer.
//
// JS-level pre-game message (never reaches the engine): the host sends
// [0x40, mapIndex] to every peer that joins, so a client can start the same
// map before the engine's own HELLO/WELCOME handshake runs.

export const MAP_MSG = 0x40;
export const DM_MAPS = [
  { id: 'hangar', title: 'DM HANGAR', mif: 'data/dmhangar/hangar.mif' },
  { id: 'field', title: 'DM FIELD', mif: 'data/dmfield/field.mif' },
];

export function mapIndexById(id) {
  const i = DM_MAPS.findIndex((m) => m.id === id);
  return i < 0 ? 0 : i;
}

const enc = new TextEncoder();

function staging(Module) {
  const ptr = Module._net_buffer();
  return Module.HEAPU8.subarray(ptr, ptr + Module._net_buffer_size());
}

// Arm the engine session before callMain starts the mission.
export function armEngine(Module, role, name) {
  const bytes = enc.encode(String(name || '').slice(0, 15));
  const buf = staging(Module);
  buf.set(bytes);
  buf[bytes.length] = 0;
  Module._net_arm_buffer(role === 'host' ? 1 : 2);
}

// Wire transport <-> engine. Messages that arrive before the engine is
// running are queued; pump() runs on a short timer.
export function connectEngine(Module, transport, { role, mapIndex = 0, onMap } = {}) {
  const inbox = [];
  let stopped = false;
  transport.onMessage((peer, bytes) => {
    if (bytes.length === 2 && bytes[0] === MAP_MSG) {
      if (role === 'client' && typeof onMap === 'function') onMap(bytes[1]);
      return;
    }
    inbox.push([peer, bytes]);
  });
  // A transport can report a join before the peer is ready to receive
  // (loopback emits join before its welcome), so the host repeats the map
  // announcement: once shortly after each join and every second to everyone.
  const announce = (peer) => transport.send(peer, new Uint8Array([MAP_MSG, mapIndex]), true);
  let mapTimer = null;
  if (role === 'host') {
    transport.onPeerJoin((peer) => { setTimeout(() => announce(peer), 50); });
    mapTimer = setInterval(() => { if (transport.peers().length) announce(null); }, 1000);
  }
  transport.onPeerLeave((peer) => {
    if (typeof Module._net_peer_left === 'function') Module._net_peer_left(role === 'host' ? peer : -1);
  });

  function pump() {
    if (stopped || typeof Module._net_poll !== 'function') return;
    for (const [peer, bytes] of inbox.splice(0)) {
      if (bytes.length === 0 || bytes.length > Module._net_buffer_size()) continue;
      staging(Module).set(bytes);
      Module._net_deliver(bytes.length, peer);
    }
    let n;
    while ((n = Module._net_poll()) > 0) {
      const out = staging(Module).slice(0, n);
      const peer = Module._net_last_peer();
      transport.send(peer < 0 ? null : peer, out, Module._net_last_reliable() !== 0);
    }
  }
  const timer = setInterval(pump, 8);
  return {
    pump,
    stop() {
      stopped = true;
      clearInterval(timer);
      if (mapTimer) clearInterval(mapTimer);
      try { transport.close(); } catch (_) { /* already closed */ }
    },
  };
}

export function readNetState(Module) {
  if (typeof Module._net_state_json !== 'function') return null;
  try { return JSON.parse(Module.UTF8ToString(Module._net_state_json())); } catch (_) { return null; }
}
