import { test } from 'node:test';
import assert from 'node:assert/strict';
import { mountLobby } from '../../web/net/lobby.js';

// Small DOM harness exercises the actual lobby controller without a dependency.
class Element {
  constructor(tag) { this.tag = tag; this.children = []; this.attrs = {}; this.events = {}; this.value = ''; this.classList = { toggle() {} }; }
  setAttribute(k, v) { this.attrs[k] = v; if (k === 'value') this.value = v; }
  append(...nodes) { this.children.push(...nodes); }
  addEventListener(k, fn) { this.events[k] = fn; }
  set innerHTML(v) { this.children = []; }
  focus() { this.focused = true; }
}
function descendants(node) { return [node, ...node.children.flatMap(descendants)]; }
function setup(blockStorage = false) {
  globalThis.Node = Element;
  globalThis.document = { createElement: t => new Element(t), createTextNode: t => Object.assign(new Element('#text'), { textContent: t }), getElementById: () => null };
  globalThis.localStorage = { getItem() { if (blockStorage) throw Error('blocked'); return 'Alice'; }, setItem() { if (blockStorage) throw Error('blocked'); } };
  globalThis.location = { hash: '#room=abc234', origin: 'https://example.test', pathname: '/play/' };
  const root = new Element('main'), calls = [];
  const controller = mountLobby(root, { onHost: (...args) => calls.push(['host', ...args]), onJoin: (...args) => calls.push(['join', ...args]) });
  const all = descendants(root);
  const byId = id => all.find(n => n.attrs.id === id);
  const button = label => all.find(n => n.tag === 'button' && n.children[0]?.attrs.alt === label);
  byId('xops-lobby-transport').value = 'webrtc';
  return { root, calls, controller, byId, button, all };
}
test('share URL prefills join, preserves callback contract and validates room', () => {
  const ui = setup();
  assert.equal(ui.byId('xops-lobby-room').value, 'abc234');
  ui.button('Join').events.click();
  assert.deepEqual(ui.calls, [['join', 'abc234', 'Alice', 'webrtc']]);
  ui.byId('xops-lobby-room').value = 'bad';
  ui.button('Join').events.click();
  assert.equal(ui.calls.length, 1);
  assert.ok(ui.byId('xops-lobby-room').focused);
});
test('host and join share one name, with map in host section and code in join section', () => {
  const ui = setup(true);
  assert.equal(ui.all.filter(n => n.attrs.id === 'xops-lobby-name').length, 1);
  const host = ui.all.find(n => n.className === 'host-action');
  const join = ui.all.find(n => n.className === 'join-action');
  assert.ok(descendants(host).includes(ui.byId('mp-map')));
  assert.ok(descendants(join).includes(ui.byId('xops-lobby-room')));
  ui.byId('xops-lobby-name').value = 'Bob';
  ui.button('Host').events.click();
  assert.equal(ui.calls[0][0], 'host');
  assert.equal(ui.calls[0][2], 'Bob');
  assert.match(location.hash, /host=1/);
  ui.button('Join').events.click();
  assert.equal(ui.calls[1][2], 'Bob');
});
test('copy link produces a join URL and exposes it if clipboard is unavailable', async () => {
  const ui = setup();
  let copied;
  Object.defineProperty(globalThis, 'navigator', { configurable: true, value: { clipboard: { writeText: async text => { copied = text; } } } });
  await ui.button('Copy link').events.click();
  assert.equal(copied, 'https://example.test/play/#room=abc234');
  navigator.clipboard.writeText = async () => { throw Error('blocked'); };
  await ui.button('Copy link').events.click();
  const status = ui.all.find(n => n.className === 'xops-status');
  assert.equal(status.textContent, copied);
});
