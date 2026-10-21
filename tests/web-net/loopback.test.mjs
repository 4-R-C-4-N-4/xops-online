import { test } from 'node:test';
import assert from 'node:assert/strict';
import { createLoopbackTransport } from '../../web/net/loopback.js';

function once(transport, event) {
  return new Promise((resolve) => {
    const register = event === 'join' ? transport.onPeerJoin : transport.onPeerLeave;
    register((id) => resolve(id));
  });
}

function sleep(ms) {
  return new Promise((resolve) => setTimeout(resolve, ms));
}

async function waitUntil(fn, { timeoutMs = 5000, stepMs = 20 } = {}) {
  const deadline = Date.now() + timeoutMs;
  while (Date.now() < deadline) {
    if (fn()) return;
    await sleep(stepMs);
  }
  throw new Error('waitUntil: condition never became true');
}

test('loopback: host sees two clients join, bytes arrive intact', async () => {
  const room = `test-room-${Math.random().toString(36).slice(2)}`;
  const host = createLoopbackTransport({ room, role: 'host', name: 'Host' });
  const clientA = createLoopbackTransport({ room, role: 'client', name: 'A' });
  const clientB = createLoopbackTransport({ room, role: 'client', name: 'B' });

  try {
    await waitUntil(() => host.peers().length === 2);
    await waitUntil(() => clientA.peers().length === 1);
    await waitUntil(() => clientB.peers().length === 1);

    const [hostPeerA, hostPeerB] = host.peers();
    assert.notEqual(hostPeerA, hostPeerB);

    const received = [];
    host.onMessage((peerId, bytes) => received.push([peerId, bytes]));

    const payload = new Uint8Array([1, 2, 3, 250, 251]);
    clientA.send(null, payload, true);

    await waitUntil(() => received.length === 1);
    const [fromPeer, bytes] = received[0];
    assert.ok(host.peers().includes(fromPeer));
    assert.deepEqual(Array.from(bytes), [1, 2, 3, 250, 251]);
    assert.ok(bytes instanceof Uint8Array);

    // mutate the sender's buffer after sending — receiver must have its own copy
    payload[0] = 99;
    assert.equal(bytes[0], 1);
  } finally {
    host.close();
    clientA.close();
    clientB.close();
  }
});

test('loopback: host broadcast reaches all clients, clients do not see each other directly', async () => {
  const room = `test-room-${Math.random().toString(36).slice(2)}`;
  const host = createLoopbackTransport({ room, role: 'host', name: 'Host' });
  const clientA = createLoopbackTransport({ room, role: 'client', name: 'A' });
  const clientB = createLoopbackTransport({ room, role: 'client', name: 'B' });

  try {
    await waitUntil(() => host.peers().length === 2);
    await waitUntil(() => clientA.peers().length === 1);
    await waitUntil(() => clientB.peers().length === 1);

    const gotA = [];
    const gotB = [];
    clientA.onMessage((peerId, bytes) => gotA.push([peerId, bytes]));
    clientB.onMessage((peerId, bytes) => gotB.push([peerId, bytes]));

    host.send(null, new Uint8Array([7, 7, 7]), true);

    await waitUntil(() => gotA.length === 1 && gotB.length === 1);
    assert.deepEqual(Array.from(gotA[0][1]), [7, 7, 7]);
    assert.deepEqual(Array.from(gotB[0][1]), [7, 7, 7]);

    // Client A talks directly to the host only; it must never receive a
    // message directly from client B (star topology).
    clientB.send(null, new Uint8Array([9]), true); // addressed to host, not A
    await sleep(300);
    assert.equal(gotA.length, 1, 'client A should not see client B traffic directly');
  } finally {
    host.close();
    clientA.close();
    clientB.close();
  }
});

test('loopback: peer leave fires after close() (bye message)', async () => {
  const room = `test-room-${Math.random().toString(36).slice(2)}`;
  const host = createLoopbackTransport({ room, role: 'host', name: 'Host' });
  const client = createLoopbackTransport({ room, role: 'client', name: 'C' });

  try {
    await waitUntil(() => host.peers().length === 1);
    const leavePromise = once(host, 'leave');
    client.close();
    const leftId = await leavePromise;
    assert.equal(host.peers().length, 0);
    assert.equal(typeof leftId, 'number');
  } finally {
    host.close();
  }
});

test('loopback: peer timeout fires onPeerLeave without an explicit close()', { timeout: 10000 }, async () => {
  const room = `test-room-${Math.random().toString(36).slice(2)}`;
  const host = createLoopbackTransport({ room, role: 'host', name: 'Host' });
  const client = createLoopbackTransport({ room, role: 'client', name: 'C' });

  try {
    await waitUntil(() => host.peers().length === 1);
    // Simulate the client vanishing without sending BYE (e.g. a crash):
    // kill its heartbeat/handshake timers and channel without close().
    client.__simulateCrashForTest();

    const leftId = await once(host, 'leave');
    assert.equal(typeof leftId, 'number');
    assert.equal(host.peers().length, 0);
  } finally {
    host.close();
  }
});

test('loopback: close() stops further message delivery', async () => {
  const room = `test-room-${Math.random().toString(36).slice(2)}`;
  const host = createLoopbackTransport({ room, role: 'host', name: 'Host' });
  const client = createLoopbackTransport({ room, role: 'client', name: 'C' });

  try {
    await waitUntil(() => host.peers().length === 1);
    host.close();

    const received = [];
    client.onMessage((peerId, bytes) => received.push(bytes));
    // host is closed, send should be a no-op and nothing should arrive.
    host.send(null, new Uint8Array([1]), true);
    await sleep(200);
    assert.equal(received.length, 0);
  } finally {
    client.close();
  }
});
