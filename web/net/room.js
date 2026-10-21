// room.js — room code generation/parsing helpers for XOPS ONLINE.
//
// Room codes are 6 characters from a confusable-free alphabet, generated
// with crypto.getRandomValues (uniform via rejection sampling, no modulo
// bias). The code lives in the URL hash so links can be shared directly:
//   https://example.github.io/xops-online/#room=ab3xyz&host=1
//
// Exports:
//   ALPHABET            - the allowed character set
//   generateRoomCode()  - random 6-char code
//   isValidRoomCode(s)  - true if s is a well-formed code (case-insensitive)
//   normalizeRoomCode(s)- lowercased, trimmed; '' if invalid
//   formatHash(room, isHost) -> '#room=CODE&host=1' (host flag omitted if false)
//   parseHash(hash)     -> { room, host } from a '#...' or bare query string

// Excludes confusable characters: 0/o, 1/i/l.
export const ALPHABET = '23456789abcdefghjkmnpqrstuvwxyz';
export const ROOM_CODE_LENGTH = 6;

export function generateRoomCode(length = ROOM_CODE_LENGTH) {
  const out = [];
  const buf = new Uint32Array(1);
  while (out.length < length) {
    crypto.getRandomValues(buf);
    // Rejection sampling: only accept values that divide evenly into the
    // alphabet size, so every character has equal probability.
    const limit = Math.floor(0xffffffff / ALPHABET.length) * ALPHABET.length;
    if (buf[0] >= limit) continue;
    out.push(ALPHABET[buf[0] % ALPHABET.length]);
  }
  return out.join('');
}

export function isValidRoomCode(code) {
  if (typeof code !== 'string') return false;
  const s = code.trim().toLowerCase();
  if (s.length !== ROOM_CODE_LENGTH) return false;
  for (const ch of s) {
    if (!ALPHABET.includes(ch)) return false;
  }
  return true;
}

export function normalizeRoomCode(code) {
  if (typeof code !== 'string') return '';
  const s = code.trim().toLowerCase();
  return isValidRoomCode(s) ? s : '';
}

export function formatHash(room, isHost) {
  const r = normalizeRoomCode(room);
  if (!r) throw new Error('formatHash: invalid room code');
  return isHost ? `#room=${r}&host=1` : `#room=${r}`;
}

export function parseHash(hash) {
  const s = String(hash || '').replace(/^#/, '');
  const params = new URLSearchParams(s);
  const room = normalizeRoomCode(params.get('room') || '');
  const host = params.get('host') === '1';
  return { room, host };
}
