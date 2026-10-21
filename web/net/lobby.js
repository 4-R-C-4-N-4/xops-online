// lobby.js — framework-free DOM module for the Host/Join lobby screen.
//
// mountLobby(container, { onHost(room, name, kind), onJoin(room, name, kind) })
// renders name/room inputs, Host/Join buttons, a copy-link button, a
// transport picker (WebRTC vs same-browser loopback) and status/peer-list
// areas. It does not know about transport.js: the host page wires onHost/
// onJoin to createTransport() and feeds status/peer updates back in via
// the returned controller's setStatus/setPeers.
//
// Returns a controller: { setStatus(text, isError), setPeers(list), destroy() }
// where list is an array of { id, name, pingMs } objects.

import { generateRoomCode, isValidRoomCode, normalizeRoomCode, formatHash, parseHash } from './room.js';

const NAME_KEY = 'xops.lobby.name';
const MAX_NAME_LEN = 15;

function el(tag, attrs = {}, children = []) {
  const node = document.createElement(tag);
  for (const [k, v] of Object.entries(attrs)) {
    if (k === 'class') node.className = v;
    else if (k.startsWith('on') && typeof v === 'function') node.addEventListener(k.slice(2), v);
    else node.setAttribute(k, v);
  }
  for (const child of children) {
    node.append(child instanceof Node ? child : document.createTextNode(child));
  }
  return node;
}

export function mountLobby(container, { onHost, onJoin } = {}) {
  if (!container) throw new Error('mountLobby: container is required');

  const savedName = (localStorage.getItem(NAME_KEY) || '').slice(0, MAX_NAME_LEN);
  const { room: hashRoom } = parseHash(location.hash);

  const nameInput = el('input', {
    type: 'text',
    id: 'xops-lobby-name',
    maxlength: String(MAX_NAME_LEN),
    placeholder: 'Your name',
    value: savedName,
    'aria-label': 'Your name',
  });

  const roomInput = el('input', {
    type: 'text',
    id: 'xops-lobby-room',
    maxlength: '6',
    placeholder: 'Room code',
    value: hashRoom || '',
    'aria-label': 'Room code',
  });

  const transportSelect = el(
    'select',
    { id: 'xops-lobby-transport', 'aria-label': 'Transport' },
    [
      el('option', { value: 'webrtc' }, ['Online (WebRTC)']),
      el('option', { value: 'loopback' }, ['Same-browser test (tabs)']),
    ]
  );

  const hostBtn = el('button', { type: 'button' }, ['Host']);
  const joinBtn = el('button', { type: 'button' }, ['Join']);
  const copyBtn = el('button', { type: 'button' }, ['Copy link']);

  const status = el('p', { class: 'xops-status', role: 'status' }, ['']);
  const peerList = el('ul', { class: 'xops-peers' }, []);

  function persistName() {
    const v = nameInput.value.slice(0, MAX_NAME_LEN);
    localStorage.setItem(NAME_KEY, v);
    return v || 'Player';
  }

  function setStatus(text, isError) {
    status.textContent = text || '';
    status.classList.toggle('xops-error', !!isError);
  }

  function setPeers(list) {
    peerList.innerHTML = '';
    for (const p of list || []) {
      const ping = p.pingMs === undefined || p.pingMs === null ? '—' : `${Math.round(p.pingMs)} ms`;
      peerList.append(
        el('li', {}, [
          el('span', {}, [p.name || `peer ${p.id}`]),
          el('span', { class: 'xops-peer-ping' }, [ping]),
        ])
      );
    }
  }

  hostBtn.addEventListener('click', () => {
    const name = persistName();
    const room = generateRoomCode();
    roomInput.value = room;
    location.hash = formatHash(room, true);
    setStatus(`Hosting room ${room}…`);
    if (typeof onHost === 'function') onHost(room, name, transportSelect.value);
  });

  joinBtn.addEventListener('click', () => {
    const name = persistName();
    const room = normalizeRoomCode(roomInput.value);
    if (!room) {
      setStatus('Enter a valid 6-character room code.', true);
      roomInput.focus();
      return;
    }
    location.hash = formatHash(room, false);
    setStatus(`Joining room ${room}…`);
    if (typeof onJoin === 'function') onJoin(room, name, transportSelect.value);
  });

  copyBtn.addEventListener('click', async () => {
    const room = normalizeRoomCode(roomInput.value);
    if (!room) {
      setStatus('No room code to copy yet.', true);
      return;
    }
    const url = `${location.origin}${location.pathname}${formatHash(room, false)}`;
    try {
      await navigator.clipboard.writeText(url);
      setStatus('Link copied to clipboard.');
    } catch {
      setStatus(url); // fall back to showing it so it can be selected manually
    }
  });

  const root = el('div', { class: 'xops-lobby' }, [
    el('h2', {}, ['XOPS Online']),
    el('label', { for: 'xops-lobby-name' }, ['Name']),
    nameInput,
    el('label', { for: 'xops-lobby-room' }, ['Room code']),
    roomInput,
    el('label', { for: 'xops-lobby-transport' }, ['Transport']),
    transportSelect,
    el('div', { class: 'xops-row' }, [hostBtn, joinBtn]),
    el('div', { class: 'xops-row' }, [copyBtn]),
    status,
    peerList,
  ]);

  container.innerHTML = '';
  container.append(root);

  if (hashRoom && isValidRoomCode(hashRoom)) {
    setStatus(`Room ${hashRoom} ready to join.`);
  }

  return {
    setStatus,
    setPeers,
    destroy() {
      container.innerHTML = '';
    },
  };
}
