// loopback.js — BroadcastChannel transport for phase-1 two-tab testing.
//
// Two or more tabs of the same origin join a BroadcastChannel named after
// the room code. Topology is a star, same as the WebRTC transport:
// clients only exchange "data" messages with the host; the host can reach
// any single client or broadcast to all.
//
// Handshake: a client sends HELLO; the host (and only the host) answers
// with WELCOME carrying the integer peerId it assigns to that client. The
// client always refers to the host with the fixed local peerId `1` (there
// is only ever one remote peer from a client's point of view). Everyone
// broadcasts a HEARTBEAT once a second; a peer not heard from for 4
// seconds is dropped and reported via onPeerLeave.
//
// Every control message carries a random session id (`sid`) used to tell
// instances apart; "own" messages are ignored (BroadcastChannel does not
// deliver to the sender, but we also check explicitly for safety, e.g. for
// polyfills that do).
//
// Payload bytes travel as a structured-clone ArrayBuffer (BroadcastChannel
// supports postMessage's structured clone, so no base64 encoding needed);
// `send()` always copies its input into a fresh ArrayBuffer before posting.

const HEARTBEAT_MS = 1000;
const PEER_TIMEOUT_MS = 4000;
const SWEEP_MS = 1000;

function randomSid() {
  const buf = new Uint32Array(4);
  crypto.getRandomValues(buf);
  return Array.from(buf, (n) => n.toString(36)).join('-');
}

function randomSmallId() {
  const buf = new Uint32Array(1);
  crypto.getRandomValues(buf);
  return 1 + (buf[0] % 65535);
}

function toArrayBufferCopy(bytes) {
  const u8 = bytes instanceof Uint8Array ? bytes : new Uint8Array(bytes);
  const copy = new Uint8Array(u8.length);
  copy.set(u8);
  return copy.buffer;
}

export function createLoopbackTransport({ room, role, name } = {}) {
  if (role !== 'host' && role !== 'client') {
    throw new Error('createLoopbackTransport: role must be "host" or "client"');
  }
  const channelName = `xops-net-loopback-${room}`;
  const channel = new BroadcastChannel(channelName);
  const mySid = randomSid();
  const selfId = randomSmallId();
  const HOST_PEER_ID = 1; // fixed local id a client uses to refer to the host

  let closed = false;
  const messageCbs = [];
  const joinCbs = [];
  const leaveCbs = [];

  // host: sid -> { peerId, lastSeen, name }
  // client: just one entry for the host, keyed by its real sid once known.
  const peersBySid = new Map();
  // reverse lookup: peerId -> sid, used by send()
  const sidByPeerId = new Map();
  let nextPeerId = 2; // 1 is reserved for "the host" from a client's perspective

  let hostSid = null; // client-side: the sid of the host once welcomed
  let welcomed = false; // client-side: have we been assigned a peerId?

  function emit(cbs, ...args) {
    for (const cb of cbs.slice()) {
      try {
        cb(...args);
      } catch (err) {
        console.error('[xops-net loopback] listener error', err);
      }
    }
  }

  function post(msg) {
    if (closed) return;
    channel.postMessage(msg);
  }

  function addPeer(sid, peerId, peerName) {
    if (peersBySid.has(sid)) {
      peersBySid.get(sid).lastSeen = Date.now();
      return;
    }
    peersBySid.set(sid, { peerId, lastSeen: Date.now(), name: peerName });
    sidByPeerId.set(peerId, sid);
    emit(joinCbs, peerId);
  }

  function removePeer(sid) {
    const entry = peersBySid.get(sid);
    if (!entry) return;
    peersBySid.delete(sid);
    sidByPeerId.delete(entry.peerId);
    emit(leaveCbs, entry.peerId);
  }

  channel.onmessage = (ev) => {
    const msg = ev.data;
    if (!msg || msg.from === mySid) return;

    if (role === 'host') {
      if (msg.type === 'hello') {
        let entry = peersBySid.get(msg.from);
        if (!entry) {
          const peerId = nextPeerId++;
          peersBySid.set(msg.from, { peerId, lastSeen: Date.now(), name: msg.name });
          sidByPeerId.set(peerId, msg.from);
          emit(joinCbs, peerId);
          entry = peersBySid.get(msg.from);
        } else {
          entry.lastSeen = Date.now();
        }
        post({ type: 'welcome', from: mySid, to: msg.from, assignedId: entry.peerId });
      } else if (msg.type === 'heartbeat' || msg.type === 'data') {
        const entry = peersBySid.get(msg.from);
        if (entry) entry.lastSeen = Date.now();
        if (msg.type === 'data' && (msg.to === mySid || msg.to === undefined)) {
          emit(messageCbs, entry ? entry.peerId : null, new Uint8Array(msg.bytes));
        }
      } else if (msg.type === 'bye') {
        removePeer(msg.from);
      }
    } else {
      // client: only the host matters; ignore chatter between host and
      // other clients that we happen to overhear on the shared channel.
      if (msg.type === 'welcome' && msg.to === mySid) {
        hostSid = msg.from;
        welcomed = true;
        addPeer(hostSid, HOST_PEER_ID, null);
      } else if (msg.from === hostSid) {
        if (msg.type === 'heartbeat') {
          const entry = peersBySid.get(hostSid);
          if (entry) entry.lastSeen = Date.now();
        } else if (msg.type === 'data' && (msg.to === mySid || msg.to === undefined)) {
          const entry = peersBySid.get(hostSid);
          if (entry) entry.lastSeen = Date.now();
          emit(messageCbs, HOST_PEER_ID, new Uint8Array(msg.bytes));
        } else if (msg.type === 'bye') {
          removePeer(hostSid);
          hostSid = null;
          welcomed = false;
        }
      }
    }
  };

  // Handshake kick-off (client) + heartbeat loop (both roles).
  if (role === 'client') {
    post({ type: 'hello', from: mySid, name });
  }

  const helloRetry = role === 'client'
    ? setInterval(() => {
        if (!welcomed) post({ type: 'hello', from: mySid, name });
      }, HEARTBEAT_MS)
    : null;

  const heartbeatTimer = setInterval(() => {
    post({ type: 'heartbeat', from: mySid });
  }, HEARTBEAT_MS);

  const sweepTimer = setInterval(() => {
    const now = Date.now();
    for (const [sid, entry] of peersBySid) {
      if (now - entry.lastSeen > PEER_TIMEOUT_MS) {
        removePeer(sid);
        if (role === 'client' && sid === hostSid) {
          hostSid = null;
          welcomed = false;
        }
      }
    }
  }, SWEEP_MS);

  function send(peerId, bytes, reliable) {
    if (closed) return;
    const payload = toArrayBufferCopy(bytes);
    if (role === 'host') {
      if (peerId === null || peerId === undefined) {
        post({ type: 'data', from: mySid, to: undefined, reliable: !!reliable, bytes: payload });
      } else {
        const sid = sidByPeerId.get(peerId);
        if (!sid) return;
        post({ type: 'data', from: mySid, to: sid, reliable: !!reliable, bytes: payload });
      }
    } else {
      if (!hostSid) return; // not connected yet
      post({ type: 'data', from: mySid, to: hostSid, reliable: !!reliable, bytes: payload });
    }
  }

  return {
    selfId,
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
      return Array.from(sidByPeerId.keys());
    },
    close() {
      if (closed) return;
      closed = true;
      post({ type: 'bye', from: mySid });
      clearInterval(heartbeatTimer);
      clearInterval(sweepTimer);
      if (helloRetry) clearInterval(helloRetry);
      channel.onmessage = null;
      channel.close();
    },
    // Test-only hook: simulate a crashed peer (no BYE sent) so the 4s
    // timeout/sweep path can be exercised without a real close().
    __simulateCrashForTest() {
      closed = true;
      clearInterval(heartbeatTimer);
      clearInterval(sweepTimer);
      if (helloRetry) clearInterval(helloRetry);
      channel.onmessage = null;
      channel.close();
    },
  };
}
