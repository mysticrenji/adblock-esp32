#pragma once

#include <Arduino.h>

static const char kPortalPage[] PROGMEM = R"PORTAL(<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>ESP32 Adblock</title>
<style>
:root{color-scheme:dark;font:16px system-ui,sans-serif;background:#101923;color:#e6edf3}
*{box-sizing:border-box}body{margin:0}main{max-width:960px;margin:auto;padding:32px 20px}
header{display:flex;align-items:center;justify-content:space-between;gap:16px;flex-wrap:wrap}
h1{margin:4px 0;font-size:2rem}h2{font-size:1rem;color:#a9bbca;margin:0 0 16px}
.eyebrow{font-size:.75rem;letter-spacing:.16em;color:#61d9bd}p{color:#a9bbca;line-height:1.6}
.badge{border:1px solid #365466;border-radius:99px;padding:8px 14px;color:#61d9bd}
.grid{display:grid;grid-template-columns:repeat(4,minmax(0,1fr));gap:14px;margin:28px 0 14px}
article,section{border:1px solid #2b3e4e;border-radius:14px;background:#172431;padding:22px}
.value{font-size:2rem;font-weight:650;font-variant-numeric:tabular-nums}
.details{display:grid;grid-template-columns:1fr 1fr;gap:14px}dl{margin:0}
dl div{display:flex;justify-content:space-between;gap:16px;padding:10px 0;border-bottom:1px solid #2b3e4e}
dt{color:#a9bbca}dd{margin:0;text-align:right;overflow-wrap:anywhere;min-width:0}
.domain{overflow-wrap:anywhere;font-family:monospace;font-size:1.1rem;min-height:2.8em}
footer{margin-top:20px;color:#a9bbca;font-size:.85rem}#notice{color:#f3cf83;min-height:1.6em}
@media(max-width:650px){.grid{grid-template-columns:1fr 1fr}.details{grid-template-columns:1fr}main{padding:24px 16px}.value{font-size:1.65rem}}
</style>
</head>
<body><main>
<header><div><div class="eyebrow">T-DISPLAY S3 / LOCAL PORTAL</div><h1>ESP32 Adblock</h1>
<p>Your DNS appliance, at a glance.</p></div><span class="badge" id="connection" role="status">Connecting…</span></header>
<div class="grid">
<article><h2>Queries</h2><div class="value" id="queries">—</div></article>
<article><h2>Blocked</h2><div class="value" id="blocked">—</div></article>
<article><h2>Forwarded</h2><div class="value" id="forwarded">—</div></article>
<article><h2>Rejected</h2><div class="value" id="rejected">—</div></article>
</div>
<div class="details"><section><h2>Device health</h2><dl>
<div><dt>IP address</dt><dd id="ip">—</dd></div>
<div><dt>DNS service</dt><dd id="dns">—</dd></div>
<div><dt>Blocklist</dt><dd id="list">—</dd></div>
<div><dt>Wi-Fi signal</dt><dd id="rssi">—</dd></div>
<div><dt>Uptime</dt><dd id="uptime">—</dd></div>
<div><dt>Free memory</dt><dd id="heap">—</dd></div>
</dl></section><section><h2>Latest DNS query</h2><div class="domain" id="domain">Waiting for queries…</div>
<p>Statistics refresh every 5 seconds. DNS counters restart when the device reboots.</p>
<p id="notice" role="status"></p></section></div>
<footer>Authenticated local access · Close your browser session to clear saved login credentials.</footer>
</main><script>
const set=(id,value)=>{document.getElementById(id).textContent=value;};
async function refresh(){
  try{
    const response=await fetch('/api/status',{cache:'no-store',credentials:'same-origin'});
    if(!response.ok)throw new Error('Status unavailable. Reload to sign in again.');
    const s=await response.json();
    for(const key of ['queries','blocked','forwarded','rejected'])set(key,s[key].toLocaleString());
    set('ip',s.ip);set('connection',s.wifi_connected?'Connected':'Wi-Fi disconnected');
    set('dns',s.dns_enabled?'Enabled':'Disabled');
    set('list',s.blocklist_ready?s.blocklist_entries.toLocaleString()+' entries':'Missing / invalid');
    set('rssi',s.rssi+' dBm');set('heap',Math.round(s.free_heap/1024)+' KB');
    const minutes=Math.floor(s.uptime_seconds/60);
    set('uptime',Math.floor(minutes/60)+'h '+minutes%60+'m');
    set('domain',s.last_domain||'No queries yet');
    set('notice',!s.dns_enabled?'DNS is disabled in firmware configuration.':!s.blocklist_ready?'Blocklist unavailable: queries are forwarded without blocking.':'');
  }catch(error){set('connection','Connection lost');set('notice',error.message);}
  finally{setTimeout(refresh,5000);}
}
refresh();
</script></body></html>)PORTAL";
