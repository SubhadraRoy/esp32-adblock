#pragma once
// Embedded dashboard for ESP32 AdBlock
// Production 24/7/365 UI: Lightweight, zero external CDN dependencies, modern moderate-dark aesthetic.

const char PAGE[] = R"HTML(<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1,viewport-fit=cover">
<meta name="theme-color" content="#0b0f19">
<title>ESP32 AdBlock &mdash; Control Center</title>
<style>
:root{
  --bg:#0b0f19;
  --card:#111827;
  --card-subtle:#0e1626;
  --card-hover:#162032;
  --border:#1e293b;
  --border-focus:#38bdf8;
  --text:#f8fafc;
  --text-muted:#94a3b8;
  --text-dim:#64748b;
  --emerald:#10b981;
  --emerald-bg:rgba(16,185,129,0.12);
  --rose:#f43f5e;
  --rose-bg:rgba(244,63,94,0.12);
  --sky:#0ea5e9;
  --sky-bg:rgba(14,165,233,0.12);
  --amber:#f59e0b;
  --amber-bg:rgba(245,158,11,0.12);
  --indigo:#6366f1;
  --indigo-bg:rgba(99,102,241,0.12);
  --mono:ui-monospace,"SF Mono","Cascadia Code",Menlo,Consolas,monospace;
  --sans:ui-sans-serif,system-ui,-apple-system,"Segoe UI",Roboto,Helvetica,Arial,sans-serif;
  --radius:8px;
}
*{box-sizing:border-box;margin:0;padding:0}
body{background:var(--bg);color:var(--text);font-family:var(--sans);font-size:13px;line-height:1.5;min-height:100vh;-webkit-font-smoothing:antialiased}
.mono{font-family:var(--mono)}
.dim{color:var(--text-dim)}
.muted{color:var(--text-muted)}
.emerald{color:var(--emerald)}
.rose{color:var(--rose)}
.sky{color:var(--sky)}
.amber{color:var(--amber)}

/* Header */
header{background:rgba(17,24,39,0.85);backdrop-filter:blur(8px);-webkit-backdrop-filter:blur(8px);border-bottom:1px solid var(--border);position:sticky;top:0;z-index:20;padding:0 16px}
.hdr-inner{max-width:1100px;margin:0 auto;display:flex;align-items:center;height:54px;gap:12px}
.logo-box{display:flex;align-items:center;gap:8px;font-weight:700;font-size:14px;letter-spacing:-0.01em;text-decoration:none;color:var(--text)}
.logo-box svg{width:20px;height:20px;color:var(--sky);flex-shrink:0}
.pulse-dot{width:8px;height:8px;border-radius:50%;background:var(--emerald);display:inline-block;transition:background .3s}
.pulse-dot.live{box-shadow:0 0 0 0 rgba(16,185,129,0.7);animation:radar 2s infinite}
.pulse-dot.offline{background:var(--rose);animation:none}
@keyframes radar{0%{box-shadow:0 0 0 0 rgba(16,185,129,0.7)}70%{box-shadow:0 0 0 6px rgba(16,185,129,0)}100%{box-shadow:0 0 0 0 rgba(16,185,129,0)}}
.badge-version{font-size:10px;padding:2px 6px;border-radius:999px;background:var(--sky-bg);color:var(--sky);font-weight:600}
.hdr-right{margin-left:auto;display:flex;align-items:center;gap:10px}
.chip{display:inline-flex;align-items:center;gap:6px;padding:4px 9px;border-radius:6px;font-size:11px;font-weight:500;background:var(--card-subtle);border:1px solid var(--border);color:var(--text-muted);cursor:pointer;transition:all .15s}
.chip:hover{border-color:var(--border-focus);color:var(--text)}
.chip.auth-on{border-color:rgba(16,185,129,0.4);color:var(--emerald);background:var(--emerald-bg)}

/* Offline Alert */
#offline-alert{display:none;background:var(--rose-bg);color:var(--rose);border-bottom:1px solid var(--rose);padding:8px 16px;font-size:12px;text-align:center;font-weight:600}

/* Navigation Tabs */
nav.tabs{display:flex;gap:4px;max-width:1100px;margin:12px auto 0;padding:0 16px;border-bottom:1px solid var(--border)}
.tab-btn{background:transparent;border:none;border-bottom:2px solid transparent;color:var(--text-muted);padding:8px 14px;font-size:13px;font-weight:600;cursor:pointer;display:inline-flex;align-items:center;gap:7px;transition:all .15s}
.tab-btn:hover{color:var(--text)}
.tab-btn.active{color:var(--sky);border-bottom-color:var(--sky)}
.tab-btn svg{width:15px;height:15px}

/* Main Container */
.main{max-width:1100px;margin:0 auto;padding:16px 16px 48px}
.tab-pane{display:none}
.tab-pane.active{display:block;animation:paneIn .2s ease}
@keyframes paneIn{from{opacity:0;transform:translateY(4px)}to{opacity:1;transform:translateY(0)}}

/* Metric Cards */
.metrics{display:grid;grid-template-columns:repeat(auto-fit,minmax(160px,1fr));gap:12px;margin-bottom:16px}
.card{background:var(--card);border:1px solid var(--border);border-radius:var(--radius);padding:14px;position:relative;overflow:hidden}
.card .title{font-size:11px;text-transform:uppercase;letter-spacing:.05em;color:var(--text-muted);font-weight:600;display:flex;align-items:center;justify-content:space-between}
.card .val{font-size:22px;font-weight:700;line-height:1.2;margin:6px 0 2px;font-family:var(--mono)}
.card .sub{font-size:11px;color:var(--text-dim)}
.ratio-bar{height:4px;background:var(--border);border-radius:2px;overflow:hidden;margin-top:8px}
.ratio-bar-fill{height:100%;background:var(--rose);transition:width .4s ease}

/* Pulse Chart Container */
.pulse-box{background:var(--card);border:1px solid var(--border);border-radius:var(--radius);padding:14px 16px;margin-bottom:16px}
.pulse-header{display:flex;align-items:center;justify-content:space-between;margin-bottom:12px}
.pulse-header h3{font-size:12px;text-transform:uppercase;letter-spacing:.05em;color:var(--text-muted);font-weight:600}
.pulse-legend{display:flex;gap:14px;font-size:11px}
.pulse-legend span{display:inline-flex;align-items:center;gap:5px}
.pulse-legend i{width:7px;height:7px;border-radius:2px;display:inline-block}
.pulse-chart{display:flex;height:68px;gap:2px;position:relative;align-items:stretch}
.pulse-chart::before{content:'';position:absolute;left:0;right:0;top:50%;height:1px;background:var(--border)}
.pulse-col{flex:1;min-width:2px;display:flex;flex-direction:column}
.pulse-top,.pulse-bot{flex:1;display:flex}
.pulse-top{align-items:flex-end}
.pulse-bot{align-items:flex-start}
.pulse-bar{width:100%;border-radius:1px;transition:height .25s ease}
.pulse-bar.top{background:var(--emerald);opacity:.9}
.pulse-bar.bot{background:var(--rose);opacity:.9}

/* Tables & Controls */
.table-card{background:var(--card);border:1px solid var(--border);border-radius:var(--radius);overflow:hidden;margin-bottom:16px}
.table-header{padding:12px 14px;border-bottom:1px solid var(--border);display:flex;align-items:center;gap:10px;flex-wrap:wrap}
.table-title{font-size:13px;font-weight:600;display:flex;align-items:center;gap:8px}
.table-title .badge{font-size:11px;background:var(--card-subtle);padding:2px 7px;border-radius:999px;border:1px solid var(--border);color:var(--text-muted)}
.table-actions{margin-left:auto;display:flex;gap:8px}
.search-input{background:var(--bg);border:1px solid var(--border);color:var(--text);padding:6px 10px;font-size:12px;border-radius:6px;width:180px;transition:border-color .15s}
.search-input:focus{outline:none;border-color:var(--border-focus)}
.table-wrap{width:100%;overflow-x:auto;-webkit-overflow-scrolling:touch}
table{width:100%;border-collapse:collapse;white-space:nowrap;font-size:12px}
th{background:var(--card-subtle);color:var(--text-muted);text-transform:uppercase;font-size:10px;letter-spacing:.05em;padding:8px 12px;text-align:left;font-weight:600;border-bottom:1px solid var(--border)}
td{padding:9px 12px;border-bottom:1px solid var(--border);color:var(--text)}
tbody tr:hover{background:var(--card-hover)}
tbody tr:last-child td{border-bottom:none}
.btn{background:var(--card-subtle);border:1px solid var(--border);color:var(--text);padding:6px 12px;font-size:12px;font-weight:500;border-radius:6px;cursor:pointer;display:inline-flex;align-items:center;gap:6px;transition:all .15s;line-height:1}
.btn:hover{background:var(--card-hover);border-color:var(--text-dim)}
.btn.primary{background:var(--sky);border-color:var(--sky);color:#081018;font-weight:600}
.btn.primary:hover{opacity:.9}
.btn.danger{color:var(--rose);border-color:rgba(244,63,94,0.3)}
.btn.danger:hover{background:var(--rose-bg);border-color:var(--rose)}
.btn.sm{padding:4px 8px;font-size:11px}
.btn:disabled{opacity:.4;cursor:not-allowed}

/* Forms & Rows */
.form-row{display:flex;gap:8px;align-items:center;flex-wrap:wrap;padding:12px 14px}
.form-input{background:var(--bg);border:1px solid var(--border);color:var(--text);padding:8px 11px;font-size:13px;border-radius:6px;font-family:var(--mono)}
.form-input:focus{outline:none;border-color:var(--border-focus)}
.grow{flex:1 1 200px}
.hint-text{font-size:11px;color:var(--text-dim);padding:0 14px 12px}

/* Chips & Presets */
.preset-wrap{display:flex;gap:6px;flex-wrap:wrap;padding:0 14px 12px}
.preset-chip{font-size:11px;padding:3px 8px;border-radius:4px;background:var(--card-subtle);border:1px solid var(--border);color:var(--text-muted);cursor:pointer;transition:all .15s}
.preset-chip:hover{border-color:var(--sky);color:var(--sky);background:var(--sky-bg)}

/* Modal */
.modal-backdrop{display:none;position:fixed;inset:0;background:rgba(0,0,0,0.65);backdrop-filter:blur(4px);z-index:99;align-items:center;justify-content:center}
.modal-box{background:var(--card);border:1px solid var(--border);border-radius:10px;width:90%;max-width:380px;padding:18px;box-shadow:0 16px 36px rgba(0,0,0,0.5)}
.modal-box h3{font-size:14px;font-weight:700;margin-bottom:6px}
.modal-box p{font-size:12px;color:var(--text-muted);margin-bottom:14px}
.modal-footer{display:flex;gap:8px;justify-content:flex-end;margin-top:14px}

/* Toast */
#toast{position:fixed;bottom:20px;right:20px;max-width:360px;padding:10px 14px;border-radius:8px;font-size:12px;font-weight:500;z-index:100;display:none;box-shadow:0 8px 24px rgba(0,0,0,0.4);animation:pop .2s ease}
#toast.err{background:#2a1117;border:1px solid var(--rose);color:#ff9bb4}
#toast.ok{background:#0e2421;border:1px solid var(--emerald);color:#a7f3d0}
@keyframes pop{from{opacity:0;transform:scale(.95)}to{opacity:1;transform:scale(1)}}

/* Responsive */
@media (max-width:640px){
  .hdr-inner{height:auto;padding:10px 0;flex-wrap:wrap}
  .hdr-right{margin-left:0;width:100%;justify-content:space-between}
  .search-input{width:100%}
  .metrics{grid-template-columns:repeat(2,1fr)}
  .pulse-chart{height:54px}
}
</style>
</head>
<body>

<div id="offline-alert">Device disconnected &mdash; attempting reconnection in background...</div>

<header>
  <div class="hdr-inner">
    <a href="/" class="logo-box">
      <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M12 22s8-4 8-10V5l-8-3-8 3v7c0 6 8 10 8 10z"/></svg>
      <span>ESP32 ADBLOCK</span>
      <span class="badge-version">v1.0.0</span>
    </a>
    <div class="hdr-right">
      <span class="pulse-dot live" id="dot" title="Connection status"></span>
      <span class="chip mono" id="hostChip" onclick="copyText(this.textContent,'IP address copied')" title="Click to copy IP">0.0.0.0</span>
      <span class="chip" id="authChip" onclick="openAuthModal()">Auth: Token</span>
    </div>
  </div>
</header>

<nav class="tabs">
  <button class="tab-btn active" data-tab="tab-overview" onclick="switchTab('tab-overview')">
    <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><rect x="3" y="3" width="7" height="9"/><rect x="14" y="3" width="7" height="5"/><rect x="14" y="12" width="7" height="9"/><rect x="3" y="16" width="7" height="5"/></svg>
    Overview
  </button>
  <button class="tab-btn" data-tab="tab-clients" onclick="switchTab('tab-clients')">
    <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M17 21v-2a4 4 0 0 0-4-4H5a4 4 0 0 0-4 4v2"/><circle cx="9" cy="7" r="4"/><path d="M23 21v-2a4 4 0 0 0-3-3.87"/><path d="M16 3.13a4 4 0 0 1 0 7.75"/></svg>
    Clients <span class="badge" id="tabClientCount" style="font-size:10px;margin-left:2px;color:var(--text-dim)">(0)</span>
  </button>
  <button class="tab-btn" data-tab="tab-rules" onclick="switchTab('tab-rules')">
    <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M12 22s8-4 8-10V5l-8-3-8 3v7c0 6 8 10 8 10z"/></svg>
    Rules &amp; Blocklist
  </button>
  <button class="tab-btn" data-tab="tab-system" onclick="switchTab('tab-system')">
    <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><circle cx="12" cy="12" r="3"/><path d="M19.4 15a1.65 1.65 0 0 0 .33 1.82l.06.06a2 2 0 0 1 0 2.83 2 2 0 0 1-2.83 0l-.06-.06a1.65 1.65 0 0 0-1.82-.33 1.65 1.65 0 0 0-1 1.51V21a2 2 0 0 1-2 2 2 2 0 0 1-2-2v-.09A1.65 1.65 0 0 0 9 19.4a1.65 1.65 0 0 0-1.82.33l-.06.06a2 2 0 0 1-2.83 0 2 2 0 0 1 0-2.83l.06-.06a1.65 1.65 0 0 0 .33-1.82 1.65 1.65 0 0 0-1.51-1H3a2 2 0 0 1-2-2 2 2 0 0 1 2-2h.09A1.65 1.65 0 0 0 4.6 9a1.65 1.65 0 0 0-.33-1.82l-.06-.06a2 2 0 0 1 0-2.83 2 2 0 0 1 2.83 0l.06.06a1.65 1.65 0 0 0 1.82.33H9a1.65 1.65 0 0 0 1-1.51V3a2 2 0 0 1 2-2 2 2 0 0 1 2 2v.09a1.65 1.65 0 0 0 1 1.51 1.65 1.65 0 0 0 1.82-.33l.06-.06a2 2 0 0 1 2.83 0 2 2 0 0 1 0 2.83l-.06.06a1.65 1.65 0 0 0-.33 1.82V9a1.65 1.65 0 0 0 1.51 1H21a2 2 0 0 1 2 2 2 2 0 0 1-2 2h-.09a1.65 1.65 0 0 0-1.51 1z"/></svg>
    System &amp; Updates
  </button>
</nav>

<div class="main">

  <!-- Tab 1: Overview -->
  <div class="tab-pane active" id="tab-overview">
    <div class="metrics">
      <div class="card">
        <div class="title"><span>Blocked Queries</span><span class="rose" id="blockRate">0%</span></div>
        <div class="val rose" id="v-block">0</div>
        <div class="sub">Sinkholed locally</div>
        <div class="ratio-bar"><div class="ratio-bar-fill" id="blockBar" style="width:0%"></div></div>
      </div>
      <div class="card">
        <div class="title"><span>Passed Queries</span><span class="emerald" id="qpm">0 q/m</span></div>
        <div class="val emerald" id="v-pass">0</div>
        <div class="sub">Forwarded to Quad9</div>
      </div>
      <div class="card">
        <div class="title"><span>Blocklist Rules</span><span class="sky">MSB Prefix</span></div>
        <div class="val sky" id="v-bl">0</div>
        <div class="sub">Sorted 40-bit hashes in flash</div>
      </div>
      <div class="card">
        <div class="title"><span>Active Clients</span><span class="indigo">ARP Sync</span></div>
        <div class="val" id="v-cli">0</div>
        <div class="sub">Monitored LAN devices</div>
      </div>
      <div class="card">
        <div class="title"><span>Wi-Fi Signal</span><span id="wifiPct">100%</span></div>
        <div class="val" id="v-wifi">0 dBm</div>
        <div class="sub">802.11 b/g/n @ 17 dBm</div>
      </div>
      <div class="card">
        <div class="title"><span>Free DRAM</span><span class="amber">160 MHz</span></div>
        <div class="val amber" id="v-ram">0 KB</div>
        <div class="sub" id="v-up">Uptime: 0d 0h 0m</div>
      </div>
    </div>

    <!-- Live Activity Pulse Chart -->
    <div class="pulse-box">
      <div class="pulse-header">
        <h3>Live Network Activity (Dual-Channel Throughput)</h3>
        <div class="pulse-legend">
          <span><i style="background:var(--emerald)"></i>Passed</span>
          <span><i style="background:var(--rose)"></i>Blocked</span>
        </div>
      </div>
      <div class="pulse-chart" id="pulseChart"></div>
    </div>
  </div>

  <!-- Tab 2: Clients -->
  <div class="tab-pane" id="tab-clients">
    <div class="table-card">
      <div class="table-header">
        <div class="table-title">
          <span>Connected Network Clients</span>
          <span class="badge" id="clientBadge">0</span>
        </div>
        <div class="table-actions">
          <input class="search-input" id="clientSearch" placeholder="Search IP or MAC..." oninput="renderClients()">
          <button class="btn sm" onclick="exportClientsCSV()" title="Export clients to CSV">Export CSV</button>
        </div>
      </div>
      <div class="table-wrap">
        <table id="clientTable">
          <thead>
            <tr>
              <th>Client IP</th>
              <th>MAC Address</th>
              <th>Blocked</th>
              <th>Passed</th>
              <th>Block Ratio</th>
              <th>Action</th>
            </tr>
          </thead>
          <tbody>
            <tr><td colspan="6" class="dim" style="text-align:center;padding:18px">Discovering network clients...</td></tr>
          </tbody>
        </table>
      </div>
    </div>
  </div>

  <!-- Tab 3: Rules & Blocklist -->
  <div class="tab-pane" id="tab-rules">
    <!-- Custom Block Rules -->
    <div class="table-card">
      <div class="table-header">
        <div class="table-title">
          <span>Custom Blocked Domains</span>
          <span class="badge" id="customBadge">0</span>
        </div>
        <div class="table-actions">
          <input class="search-input" id="customSearch" placeholder="Filter rules..." oninput="renderCustom()">
        </div>
      </div>
      <div class="form-row">
        <input class="form-input grow" id="domInput" placeholder="ad-tracker.example.com (comma/space separated for bulk)" autocomplete="off" spellcheck="false">
        <button class="btn primary" id="addDomBtn" onclick="handleAddDomain()">Block Domain</button>
      </div>
      <div class="preset-wrap">
        <span class="dim" style="font-size:11px;align-self:center;margin-right:2px">Popular trackers:</span>
        <span class="preset-chip" onclick="addPreset('telemetry.microsoft.com')">+ telemetry.microsoft.com</span>
        <span class="preset-chip" onclick="addPreset('fls-na.amazon.com')">+ fls-na.amazon.com</span>
        <span class="preset-chip" onclick="addPreset('ads.tiktok.com')">+ ads.tiktok.com</span>
        <span class="preset-chip" onclick="addPreset('doubleclick.net')">+ doubleclick.net</span>
        <span class="preset-chip" onclick="addPreset('graph.instagram.com')">+ graph.instagram.com</span>
      </div>
      <div class="table-wrap">
        <table id="customTable">
          <thead>
            <tr><th>Blocked Domain Name</th><th style="text-align:right">Action</th></tr>
          </thead>
          <tbody>
            <tr><td colspan="2" class="dim" style="text-align:center;padding:18px">No custom blocked domains added</td></tr>
          </tbody>
        </table>
      </div>
    </div>

    <!-- Binary Blocklist Upload -->
    <div class="table-card">
      <div class="table-header">
        <div class="table-title"><span>Upload Flash Blocklist</span></div>
      </div>
      <form id="uploadForm" class="form-row">
        <input type="file" id="fileInput" accept=".bin" class="form-input grow">
        <button type="submit" class="btn primary" id="uploadBtn">Upload to LittleFS</button>
        <span id="uploadMsg" class="dim mono" style="font-size:12px"></span>
      </form>
      <div class="hint-text">
        Upload pre-sorted <code>blocklist.bin</code> generated via <code>tools/build_blocklist.py</code> (Max 1.20 MB / 245k domains). Swapped atomically with zero downtime.
      </div>
    </div>
  </div>

  <!-- Tab 4: System & Updates -->
  <div class="tab-pane" id="tab-system">
    <!-- Auto-Update Configuration -->
    <div class="table-card">
      <div class="table-header">
        <div class="table-title"><span>Automated Remote Updates</span></div>
      </div>
      <div class="form-row">
        <input class="form-input grow" id="updateUrl" placeholder="https://raw.githubusercontent.com/user/repo/main/blocklist.bin" autocomplete="off">
        <span class="dim">every</span>
        <input class="form-input" id="updateInterval" style="width:60px;text-align:center" value="24" inputmode="numeric">
        <span class="dim">hours</span>
        <button class="btn" id="saveUpdBtn" onclick="handleSaveUpdate()">Save Schedule</button>
        <button class="btn" id="fetchNowBtn" onclick="handleFetchNow()">Fetch Now</button>
      </div>
      <div class="hint-text">
        Status: <span id="updateStatusTxt" class="mono emerald">&mdash;</span>
      </div>
    </div>

    <!-- Hardware & Telemetry Details -->
    <div class="table-card">
      <div class="table-header">
        <div class="table-title"><span>Hardware &amp; Firmware Specifications</span></div>
      </div>
      <div class="table-wrap">
        <table>
          <tbody>
            <tr><td class="dim">Microcontroller &amp; Silicon</td><td class="mono">ESP32-D0WD-V3 (Revision v3.1, Dual-Core Xtensa LX6 @ 160 MHz)</td></tr>
            <tr><td class="dim">Memory Footprint</td><td class="mono">320 KB Usable SRAM &bull; ~115 KB Contiguous Free DRAM &bull; Zero PSRAM Needed</td></tr>
            <tr><td class="dim">Flash Storage &amp; Filesystem</td><td class="mono">4 MB SPI Flash (DIO @ 80 MHz) &bull; 2.625 MB LittleFS VFS &bull; &gt;600 Yr Cell Endurance</td></tr>
            <tr><td class="dim">Network Sockets &amp; Queue</td><td class="mono">16 KB UDP Receive Buffer &bull; 32-slot Async DnsTx Queue (Zero HoL Blocking)</td></tr>
            <tr><td class="dim">RFC Standards Compliance</td><td class="mono">RFC 1035 Standard DNS &bull; RFC 6891 EDNS0 (1232B Clamped) &bull; RFC NODATA AAAA</td></tr>
            <tr><td class="dim">Watchdogs &amp; Protection</td><td class="mono">500ms Interrupt Watchdog (IWDT) &bull; 20s Task Watchdog &bull; Brownout Level 4 (2.67V)</td></tr>
          </tbody>
        </table>
      </div>
    </div>
  </div>

</div>

<!-- Admin Token Modal -->
<div class="modal-backdrop" id="authModal" onclick="if(event.target===this)closeAuthModal()">
  <div class="modal-box">
    <h3>Admin Authentication</h3>
    <p>Enter the <code>ADMIN_TOKEN</code> set in firmware to authorize client bans, custom rules, and blocklist updates.</p>
    <div style="display:flex;gap:8px;align-items:center">
      <input type="password" id="modalTokenInput" class="form-input grow" placeholder="Enter admin token">
      <button class="btn sm" type="button" onclick="toggleTokenVisibility()">Show</button>
    </div>
    <div class="modal-footer">
      <button class="btn" onclick="clearToken()">Clear</button>
      <button class="btn" onclick="closeAuthModal()">Cancel</button>
      <button class="btn primary" onclick="saveToken()">Save Token</button>
    </div>
  </div>
</div>

<div id="toast"></div>

<script>
const $ = id => document.getElementById(id);
const fmt = n => (n == null ? '0' : Number(n).toLocaleString());

function esc(s) {
  if (s == null) return '';
  return String(s).replace(/&/g,'&amp;').replace(/</g,'&lt;').replace(/>/g,'&gt;').replace(/"/g,'&quot;').replace(/'/g,'&#39;');
}

function copyText(txt, msg = 'Copied to clipboard') {
  navigator.clipboard.writeText(txt).then(() => notify(msg)).catch(() => {});
}

let toastTimer = null;
function notify(msg, isErr = false) {
  const t = $('toast');
  t.textContent = msg;
  t.className = isErr ? 'err' : 'ok';
  t.style.display = 'block';
  clearTimeout(toastTimer);
  toastTimer = setTimeout(() => { t.style.display = 'none'; }, 3800);
}

// Tab Switching
function switchTab(tabId) {
  document.querySelectorAll('.tab-btn').forEach(b => b.classList.toggle('active', b.dataset.tab === tabId));
  document.querySelectorAll('.tab-pane').forEach(p => p.classList.toggle('active', p.id === tabId));
}

// Token State Management
function getToken() { return (sessionStorage.getItem('adminToken') || '').trim(); }
function updateAuthBadge() {
  const t = getToken();
  const b = $('authChip');
  if (t) {
    b.textContent = 'Admin: Ready';
    b.classList.add('auth-on');
  } else {
    b.textContent = 'Auth: Required';
    b.classList.remove('auth-on');
  }
}

function openAuthModal() {
  $('modalTokenInput').value = getToken();
  $('authModal').style.display = 'flex';
  setTimeout(() => $('modalTokenInput').focus(), 50);
}

function closeAuthModal() {
  $('authModal').style.display = 'none';
}

function toggleTokenVisibility() {
  const inp = $('modalTokenInput');
  inp.type = inp.type === 'password' ? 'text' : 'password';
}

function saveToken() {
  const val = $('modalTokenInput').value.trim();
  if (val) {
    sessionStorage.setItem('adminToken', val);
    notify('Admin token saved for this session');
  } else {
    sessionStorage.removeItem('adminToken');
    notify('Admin token cleared');
  }
  updateAuthBadge();
  closeAuthModal();
  loadData();
}

function clearToken() {
  sessionStorage.removeItem('adminToken');
  $('modalTokenInput').value = '';
  updateAuthBadge();
  closeAuthModal();
  notify('Admin token cleared');
  loadData();
}

// Generic Backend Request Wrapper
async function apiPost(path, params = null) {
  const q = params ? ('?' + new URLSearchParams(params).toString()) : '';
  const tok = getToken();
  const ctrl = new AbortController();
  const timer = setTimeout(() => ctrl.abort(), 6000);
  try {
    const r = await fetch(path + q, {
      method: 'POST',
      headers: { 'X-Admin-Token': tok },
      signal: ctrl.signal
    });
    clearTimeout(timer);
    if (r.status === 401) {
      const errTxt = await r.text();
      sessionStorage.removeItem('adminToken');
      updateAuthBadge();
      if (errTxt.includes('changeme')) {
        notify('Blocked: Firmware ADMIN_TOKEN is set to "changeme" placeholder', true);
      } else {
        notify('401 Unauthorized: Invalid admin token', true);
        openAuthModal();
      }
      throw new Error('unauthorized');
    }
    if (!r.ok) {
      const msg = await r.text();
      notify('Failed: ' + (msg || r.statusText), true);
      throw new Error(msg);
    }
    return r;
  } catch(e) {
    clearTimeout(timer);
    if (e.message !== 'unauthorized' && !e.message.startsWith('Failed:')) {
      notify('Request failed: ' + (e.name === 'AbortError' ? 'timeout' : e.message), true);
    }
    throw e;
  }
}

// Pulse Chart Engine
const CHART_COLS = 44;
let prevBlock = null, prevPass = null, pulseHistory = [];

function initChart() {
  const c = $('pulseChart');
  let html = '';
  for (let i = 0; i < CHART_COLS; i++) {
    html += '<div class="pulse-col"><div class="pulse-top"><div class="pulse-bar top" style="height:0%"></div></div><div class="pulse-bot"><div class="pulse-bar bot" style="height:0%"></div></div></div>';
  }
  c.innerHTML = html;
}
initChart();

function updateChart(b, p) {
  if (prevBlock !== null) {
    if (b < prevBlock || p < prevPass) pulseHistory = []; // Board reboot detected
    const db = Math.max(0, b - prevBlock);
    const dp = Math.max(0, p - prevPass);
    pulseHistory.push({ b: db, p: dp });
    if (pulseHistory.length > CHART_COLS) pulseHistory.shift();

    // Calculate Queries Per Minute
    const qpm = Math.round((db + dp) * 20); // 3s interval * 20 = 60s rate
    $('qpm').textContent = qpm + ' q/m';
  }
  prevBlock = b; prevPass = p;
  const maxVal = Math.max(1, ...pulseHistory.map(h => Math.max(h.b, h.p)));
  const cols = $('pulseChart').children;
  for (let i = 0; i < CHART_COLS; i++) {
    const idx = pulseHistory.length - (CHART_COLS - i);
    const item = idx >= 0 ? pulseHistory[idx] : null;
    const topEl = cols[i].querySelector('.top');
    const botEl = cols[i].querySelector('.bot');
    if (item) {
      topEl.style.height = Math.round((item.p / maxVal) * 100) + '%';
      botEl.style.height = Math.round((item.b / maxVal) * 100) + '%';
    } else {
      topEl.style.height = '0%';
      botEl.style.height = '0%';
    }
  }
}

// Data State Cache
let rawClients = [], rawCustom = [];
let lastClientsJSON = '', lastCustomJSON = '';
let isPolling = false, failCount = 0;

async function loadData() {
  if (isPolling) return;
  isPolling = true;
  const ctrl = new AbortController();
  const timer = setTimeout(() => ctrl.abort(), 4000);

  try {
    const r = await fetch('/stats.json', {
      headers: { 'X-Admin-Token': getToken() },
      signal: ctrl.signal
    });
    clearTimeout(timer);
    if (!r.ok) throw new Error('HTTP ' + r.status);
    const s = await r.json();

    failCount = 0;
    $('dot').className = 'pulse-dot live';
    $('offline-alert').style.display = 'none';

    if (s.ip) $('hostChip').textContent = s.ip;
    updateChart(s.blocked || 0, s.allowed || 0);

    // Update KPI Metric Cards
    const b = s.blocked || 0;
    const p = s.allowed || 0;
    const total = b + p;
    const pct = total > 0 ? ((b / total) * 100).toFixed(1) : '0.0';
    $('v-block').textContent = fmt(b);
    $('v-pass').textContent = fmt(p);
    $('blockRate').textContent = pct + '%';
    $('blockBar').style.width = pct + '%';

    $('v-bl').textContent = fmt(s.domains);
    $('v-cli').textContent = s.clients ? s.clients.length : 0;
    $('tabClientCount').textContent = `(${s.clients ? s.clients.length : 0})`;
    $('v-wifi').textContent = (s.rssi || 0) + ' dBm';

    // WiFi Quality Estimation
    const rVal = s.rssi || -100;
    const qPct = Math.min(100, Math.max(0, Math.round(2 * (rVal + 100))));
    $('wifiPct').textContent = qPct + '%';

    $('v-ram').textContent = Math.round((s.heap || 0) / 1024) + ' KB';
    $('v-up').textContent = 'Uptime: ' + (s.uptime || '-');

    // Client Table Cache & Render
    rawClients = s.clients || [];
    const clientStr = JSON.stringify(rawClients);
    if (clientStr !== lastClientsJSON) {
      lastClientsJSON = clientStr;
      renderClients();
    }

    // Custom Rules Cache & Render
    rawCustom = s.custom || [];
    const customStr = JSON.stringify(rawCustom);
    if (customStr !== lastCustomJSON) {
      lastCustomJSON = customStr;
      renderCustom();
    }

    // System Settings Fields (only update if not currently focused by user)
    if (document.activeElement !== $('updateUrl')) $('updateUrl').value = s.upurl || '';
    if (document.activeElement !== $('updateInterval')) $('updateInterval').value = s.upiv || 24;
    $('updateStatusTxt').textContent = s.upstat || '—';

  } catch(err) {
    clearTimeout(timer);
    failCount++;
    $('dot').className = 'pulse-dot offline';
    $('offline-alert').style.display = 'block';
  } finally {
    isPolling = false;
    setTimeout(loadData, failCount > 0 ? 5000 : 3000);
  }
}

// Client Table Renderer with Live Search Filter
function renderClients() {
  const q = ($('clientSearch').value || '').toLowerCase().trim();
  const filtered = rawClients.filter(c => !q || (c.ip && c.ip.includes(q)) || (c.mac && c.mac.toLowerCase().includes(q)));
  filtered.sort((a,b) => (b.blocked + b.allowed) - (a.blocked + a.allowed));

  $('clientBadge').textContent = filtered.length;
  const tbody = $('clientTable').tBodies[0];
  if (!filtered.length) {
    tbody.innerHTML = `<tr><td colspan="6" class="dim" style="text-align:center;padding:18px">${q ? 'No clients match "' + esc(q) + '"' : 'No clients connected yet'}</td></tr>`;
    return;
  }

  tbody.innerHTML = filtered.map(c => {
    const tot = c.blocked + c.allowed;
    const rPct = tot > 0 ? ((c.blocked / tot) * 100).toFixed(0) : '0';
    return `<tr>
      <td class="mono font-semibold">
        <span onclick="copyText('${esc(c.ip)}','IP copied')" style="cursor:pointer" title="Click to copy">${esc(c.ip)}</span>
        ${c.banned ? ' <span class="badge-version" style="background:var(--rose-bg);color:var(--rose)">BANNED</span>' : ''}
      </td>
      <td class="mono dim">
        <span onclick="copyText('${esc(c.mac)}','MAC copied')" style="cursor:pointer" title="Click to copy">${esc(c.mac)}</span>
      </td>
      <td class="mono rose font-medium">${fmt(c.blocked)}</td>
      <td class="mono emerald font-medium">${fmt(c.allowed)}</td>
      <td style="min-width:110px">
        <div style="display:flex;align-items:center;gap:8px">
          <div class="ratio-bar grow" style="margin:0"><div class="ratio-bar-fill" style="width:${rPct}%"></div></div>
          <span class="mono dim" style="font-size:11px">${rPct}%</span>
        </div>
      </td>
      <td>
        <button class="btn sm ${c.banned ? '' : 'danger'}" data-ip="${esc(c.ip)}" onclick="handleToggleBan(this)">
          ${c.banned ? 'Unban' : 'Ban Client'}
        </button>
      </td>
    </tr>`;
  }).join('');
}

// Custom Rules Table Renderer with Live Search Filter
function renderCustom() {
  const q = ($('customSearch').value || '').toLowerCase().trim();
  const filtered = rawCustom.filter(d => !q || d.toLowerCase().includes(q));
  $('customBadge').textContent = filtered.length;

  const tbody = $('customTable').tBodies[0];
  if (!filtered.length) {
    tbody.innerHTML = `<tr><td colspan="2" class="dim" style="text-align:center;padding:18px">${q ? 'No rules match "' + esc(q) + '"' : 'No custom rules active'}</td></tr>`;
    return;
  }

  tbody.innerHTML = filtered.map(d => `<tr>
    <td class="mono font-medium">${esc(d)}</td>
    <td style="text-align:right">
      <button class="btn sm danger" data-dom="${esc(d)}" onclick="handleRemoveDomain(this)">Remove</button>
    </td>
  </tr>`).join('');
}

// Client Actions
async function handleToggleBan(btn) {
  const ip = btn.dataset.ip;
  if (!ip) return;
  btn.disabled = true;
  try {
    await apiPost('/ban', { ip });
    notify('Client ban updated: ' + ip);
    await loadData();
  } catch(_) {} finally { btn.disabled = false; }
}

async function handleAddDomain() {
  const input = $('domInput');
  const raw = input.value.trim();
  if (!raw) return;

  // Supports single or bulk comma/space separated domains
  const doms = raw.split(/[\s,]+/).map(d => d.trim().toLowerCase().replace(/^https?:\/\//,'').replace(/\/.*$/,'')).filter(Boolean);
  if (!doms.length) return;

  const btn = $('addDomBtn');
  btn.disabled = true;
  let added = 0;
  for (const d of doms) {
    try {
      await apiPost('/addblock', { d });
      added++;
    } catch(_) { break; }
  }
  btn.disabled = false;
  input.value = '';
  if (added > 0) {
    notify(`Blocked ${added} custom domain${added>1?'s':''}`);
    await loadData();
  }
}

function addPreset(dom) {
  $('domInput').value = dom;
  handleAddDomain();
}

async function handleRemoveDomain(btn) {
  const d = btn.dataset.dom;
  if (!d) return;
  btn.disabled = true;
  try {
    await apiPost('/unblock', { d });
    notify('Unblocked domain: ' + d);
    await loadData();
  } catch(_) {} finally { btn.disabled = false; }
}

// Update & Maintenance Actions
async function handleSaveUpdate() {
  const u = $('updateUrl').value.trim();
  const h = parseInt($('updateInterval').value, 10) || 24;
  if (u && !u.startsWith('https://')) {
    notify('Error: URL must begin with https://', true);
    return;
  }
  const btn = $('saveUpdBtn');
  btn.disabled = true;
  try {
    await apiPost('/setupdate', { u, h });
    notify('Update schedule saved');
    await loadData();
  } catch(_) {} finally { btn.disabled = false; }
}

async function handleFetchNow() {
  const btn = $('fetchNowBtn');
  btn.disabled = true;
  $('updateStatusTxt').textContent = 'download scheduled...';
  try {
    const r = await apiPost('/fetchnow', {});
    const txt = await r.text();
    notify(txt || 'Fetch triggered');
    $('updateStatusTxt').textContent = txt;
    await loadData();
  } catch(_) {
    $('updateStatusTxt').textContent = 'fetch failed';
  } finally { btn.disabled = false; }
}

// Binary Blocklist Upload
$('uploadForm').onsubmit = async e => {
  e.preventDefault();
  const file = $('fileInput').files[0];
  if (!file) return;
  const btn = $('uploadBtn');
  const msg = $('uploadMsg');
  btn.disabled = true;
  msg.textContent = 'Uploading ' + (file.size / 1048576).toFixed(2) + ' MB...';

  try {
    const tok = getToken();
    const r = await fetch('/upload', {
      method: 'POST',
      headers: { 'X-Admin-Token': tok },
      body: file
    });
    if (r.status === 401) {
      sessionStorage.removeItem('adminToken');
      updateAuthBadge();
      msg.textContent = '✗ Unauthorized';
      notify('Upload rejected: Incorrect admin token', true);
      openAuthModal();
    } else if (r.ok) {
      msg.textContent = '✓ Updated successfully';
      notify('Blocklist updated in flash!');
      $('fileInput').value = '';
      setTimeout(loadData, 1200);
    } else {
      const err = await r.text();
      msg.textContent = '✗ ' + err;
      notify('Upload failed: ' + err, true);
    }
  } catch(err) {
    msg.textContent = '✗ Network error';
    notify('Upload failed due to network timeout', true);
  } finally {
    btn.disabled = false;
  }
};

// Export Clients to CSV
function exportClientsCSV() {
  if (!rawClients.length) return notify('No clients to export', true);
  let csv = 'Client IP,MAC Address,Blocked Queries,Passed Queries,Banned\n';
  rawClients.forEach(c => {
    csv += `"${c.ip}","${c.mac}",${c.blocked},${c.allowed},${c.banned?'YES':'NO'}\n`;
  });
  const blob = new Blob([csv], { type: 'text/csv' });
  const url = URL.createObjectURL(blob);
  const a = document.createElement('a');
  a.href = url;
  a.download = `esp32_adblock_clients_${new Date().toISOString().slice(0,10)}.csv`;
  a.click();
  URL.revokeObjectURL(url);
}

// Keyboard Navigation
window.addEventListener('keydown', e => {
  if (e.target.tagName === 'INPUT' || e.target.tagName === 'TEXTAREA') {
    if (e.key === 'Escape') e.target.blur();
    return;
  }
  if (e.key === '1') switchTab('tab-overview');
  else if (e.key === '2') switchTab('tab-clients');
  else if (e.key === '3') switchTab('tab-rules');
  else if (e.key === '4') switchTab('tab-system');
  else if (e.key === '/') {
    e.preventDefault();
    const activeTab = document.querySelector('.tab-pane.active').id;
    if (activeTab === 'tab-clients') $('clientSearch').focus();
    else if (activeTab === 'tab-rules') $('customSearch').focus();
    else switchTab('tab-clients'), setTimeout(() => $('clientSearch').focus(), 50);
  } else if (e.key === 'Escape') {
    closeAuthModal();
  }
});

updateAuthBadge();
loadData();
</script>
</body>
</html>)HTML";
