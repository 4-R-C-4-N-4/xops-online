import { test } from 'node:test';
import assert from 'node:assert/strict';
import {
  ALPHABET,
  ROOM_CODE_LENGTH,
  generateRoomCode,
  isValidRoomCode,
  normalizeRoomCode,
  formatHash,
  parseHash,
} from '../../web/net/room.js';

test('generateRoomCode produces valid codes of the right length and alphabet', () => {
  for (let i = 0; i < 200; i++) {
    const code = generateRoomCode();
    assert.equal(code.length, ROOM_CODE_LENGTH);
    for (const ch of code) assert.ok(ALPHABET.includes(ch), `unexpected char ${ch}`);
    assert.ok(isValidRoomCode(code));
  }
});

test('generateRoomCode excludes confusable characters', () => {
  for (const ch of ['0', 'o', '1', 'i', 'l']) {
    assert.ok(!ALPHABET.includes(ch), `alphabet should not contain ${ch}`);
  }
});

test('generateRoomCode respects custom length', () => {
  assert.equal(generateRoomCode(10).length, 10);
});

test('isValidRoomCode rejects bad input', () => {
  assert.equal(isValidRoomCode(''), false);
  assert.equal(isValidRoomCode('abc'), false); // too short
  assert.equal(isValidRoomCode('abcdefg'), false); // too long
  assert.equal(isValidRoomCode('ab01il'), false); // confusables
  assert.equal(isValidRoomCode(null), false);
  assert.equal(isValidRoomCode(undefined), false);
  assert.equal(isValidRoomCode(42), false);
});

test('isValidRoomCode accepts valid codes case-insensitively', () => {
  assert.ok(isValidRoomCode('abc234'));
  assert.ok(isValidRoomCode('ABC234'));
  assert.ok(isValidRoomCode('  abc234  '));
});

test('normalizeRoomCode lowercases and trims, empty string for invalid', () => {
  assert.equal(normalizeRoomCode(' ABC234 '), 'abc234');
  assert.equal(normalizeRoomCode('bad'), '');
  assert.equal(normalizeRoomCode(null), '');
});

test('formatHash builds the expected hash string', () => {
  assert.equal(formatHash('abc234', true), '#room=abc234&host=1');
  assert.equal(formatHash('ABC234', false), '#room=abc234');
});

test('formatHash rejects invalid room codes', () => {
  assert.throws(() => formatHash('bad', true));
});

test('parseHash round-trips formatHash output', () => {
  assert.deepEqual(parseHash(formatHash('abc234', true)), { room: 'abc234', host: true });
  assert.deepEqual(parseHash(formatHash('abc234', false)), { room: 'abc234', host: false });
});

test('parseHash handles missing/garbage hash', () => {
  assert.deepEqual(parseHash(''), { room: '', host: false });
  assert.deepEqual(parseHash('#nonsense'), { room: '', host: false });
  assert.deepEqual(parseHash('#room=bad&host=1'), { room: '', host: true });
});
