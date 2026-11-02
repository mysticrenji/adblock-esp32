// Exercise the shipped dashboard script without browser or npm dependencies.
import assert from 'node:assert/strict';
import fs from 'node:fs';
import vm from 'node:vm';

const page = fs.readFileSync(new URL('../src/web_portal_page.h', import.meta.url), 'utf8');
const script = page.match(/<script>([\s\S]*?)<\/script>/)[1];
const status = {
  ip: '192.168.1.100', wifi_connected: true, rssi: -55,
  uptime_seconds: 3660, free_heap: 102400, dns_enabled: true,
  blocklist_ready: true, blocklist_entries: 2345,
  queries: 12, blocked: 3, forwarded: 9, rejected: 1,
  last_domain: '<img src=x onerror=alert(1)>.example',
};

async function render(data, failure = false) {
  const elements = new Map();
  for (const [, id] of page.matchAll(/id="([^"]+)"/g)) {
    const element = { textContent: '' };
    Object.defineProperty(element, 'innerHTML', {
      set() { throw new Error('Untrusted values must be rendered as text'); },
    });
    elements.set(id, element);
  }
  const timers = [];
  const context = vm.createContext({
    document: { getElementById(id) {
      assert.ok(elements.has(id), `Missing dashboard element: ${id}`);
      return elements.get(id);
    } },
    async fetch(path, options) {
      assert.equal(path, '/api/status');
      assert.equal(options.credentials, 'same-origin');
      return { ok: !failure, json: async () => data };
    },
    setTimeout: (callback, delay) => timers.push(delay),
  });
  vm.runInContext(script, context);
  // Drain the asynchronous fetch/json chain without running recurring timers.
  await new Promise(resolve => setImmediate(resolve));
  assert.deepEqual(timers, [5000]);
  return id => elements.get(id).textContent;
}

let text = await render(status);
assert.equal(text('connection'), 'Connected');
assert.equal(text('queries'), '12');
assert.equal(text('blocked'), '3');
assert.equal(text('domain'), status.last_domain);
assert.equal(text('uptime'), '1h 1m');
assert.equal(text('heap'), '100 KB');
assert.equal(text('notice'), '');

text = await render({ ...status, dns_enabled: false });
assert.equal(text('dns'), 'Disabled');
assert.match(text('notice'), /DNS is disabled/);
text = await render({ ...status, blocklist_ready: false });
assert.equal(text('list'), 'Missing / invalid');
assert.match(text('notice'), /without blocking/);
text = await render(status, true);
assert.equal(text('connection'), 'Connection lost');
assert.match(text('notice'), /Reload to sign in/);
console.log('PASS: dashboard updates, text safety, warnings, failed login, refresh scheduling');
