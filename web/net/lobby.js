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

  let savedName = '';
  try { savedName = (localStorage.getItem(NAME_KEY) || '').slice(0, MAX_NAME_LEN); } catch {}
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

  const bitmap = (file, label) => el('img', { src: new URL(`../ui/${file}.svg`, import.meta.url).href, alt: label, height: '32' });
  const hostBtn = el('button', { type: 'button' }, [bitmap('host', 'Host')]);
  const joinBtn = el('button', { type: 'button' }, [bitmap('join', 'Join')]);
  const copyBtn = el('button', { type: 'button' }, [bitmap('copy', 'Copy link')]);

  const status = el('p', { class: 'xops-status', role: 'status' }, ['']);
  const peerList = el('ul', { class: 'xops-peers' }, []);

  function persistName() {
    const v = nameInput.value.slice(0, MAX_NAME_LEN);
    try { localStorage.setItem(NAME_KEY, v); } catch {}
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

  const mapSelect = document.getElementById('mp-map') || el('select', { id: 'mp-map' },
    ['hangar', 'field', 'shipdeck', 'spire'].map(id => el('option', { value: id }, ['DM ' + id.toUpperCase()])));
  const root = el('div', { class: 'xops-lobby' }, [
    el('div', { class: 'identity-row' }, [
      el('label', { for: 'xops-lobby-name' }, ['Player name', nameInput]),
      el('label', { for: 'xops-lobby-transport' }, ['Connection', transportSelect]),
    ]),
    el('div', { class: 'play-actions' }, [
      el('section', { class: 'host-action', 'aria-label': 'Host a room' }, [
        el('h3', {}, ['01 / CREATE ROOM']),
        el('label', { for: 'mp-map' }, ['Arena', mapSelect]), hostBtn,
      ]),
      el('section', { class: 'join-action', 'aria-label': 'Join a room' }, [
        el('h3', {}, ['02 / ENTER ROOM']),
        el('label', { for: 'xops-lobby-room' }, ['Room code', roomInput]), joinBtn,
      ]),
    ]),
    el('div', { class: 'room-feedback' }, [copyBtn, status]),
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
