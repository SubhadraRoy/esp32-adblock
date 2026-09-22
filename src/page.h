#pragma once
// Embedded dashboard for ESP32 AdBlock
// Production 24/7/365 UI: Cyber-Clean Bento Telemetry Dashboard
// 100% self-contained, zero external CDN dependencies, high-performance canvas throughput graph

const char PAGE[] = R"HTML(<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1,viewport-fit=cover">
<meta name="theme-color" content="#07090e">
<title>ESP32 AdBlock &mdash; Control Center</title>
<style>
:root{
  --bg:#07090e;
  --surface:#0e1424;
  --surface-subtle:#0a0e1a;
  --surface-hover:#141c30;
  --surface-elevated:#162038;
  --border:rgba(255,255,255,0.08);
  --border-subtle:rgba(255,255,255,0.04);
  --border-focus:#0ea5e9;
  --text:#f8fafc;
  --text-muted:#94a3b8;
  --text-dim:#64748b;
  --emerald:#10b981;
  --emerald-bg:rgba(16,185,129,0.12);
  --emerald-glow:rgba(16,185,129,0.25);
  --crimson:#f43f5e;
  --crimson-bg:rgba(244,63,94,0.12);
  --crimson-glow:rgba(244,63,94,0.25);
  --cyan:#0ea5e9;
  --cyan-bg:rgba(14,165,233,0.12);
  --cyan-glow:rgba(14,165,233,0.25);
  --amber:#f59e0b;
  --amber-bg:rgba(245,158,11,0.12);
  --purple:#8b5cf6;
  --purple-bg:rgba(139,92,246,0.12);
  --mono:ui-monospace,"SF Mono","Cascadia Code",Menlo,Consolas,monospace;
  --sans:ui-sans-serif,system-ui,-apple-system,"Segoe UI",Roboto,Helvetica,Arial,sans-serif;
  --radius:10px;
  --radius-sm:6px;
  --shadow-card:0 4px 20px -2px rgba(0,0,0,0.5),inset 0 1px 0 rgba(255,255,255,0.06);
}
*{box-sizing:border-box;margin:0;padding:0}
body{
  background:radial-gradient(circle at 50% 0%,rgba(14,165,233,0.06),transparent 45%),var(--bg);
  color:var(--text);
  font-family:var(--sans);
  font-size:13px;
  line-height:1.5;
  min-height:100vh;
  -webkit-font-smoothing:antialiased;
}
.mono{font-family:var(--mono)}
.dim{color:var(--text-dim)}
.muted{color:var(--text-muted)}
.emerald{color:var(--emerald)}
.crimson{color:var(--crimson)}
.cyan{color:var(--cyan)}
.amber{color:var(--amber)}
.purple{color:var(--purple)}

/* Header */
header{
  background:rgba(10,14,26,0.85);
  backdrop-filter:blur(12px);
  -webkit-backdrop-filter:blur(12px);
  border-bottom:1px solid var(--border);
  position:sticky;
  top:0;
  z-index:30;
  padding:0 20px;
}
.hdr-inner{
  max-width:1180px;
  margin:0 auto;
  display:flex;
  align-items:center;
  height:58px;
  gap:16px;
}
.brand{
  display:flex;
  align-items:center;
  gap:10px;
  font-weight:700;
  font-size:15px;
  letter-spacing:-0.02em;
  text-decoration:none;
  color:var(--text);
}
.brand svg{width:24px;height:24px;color:var(--cyan);filter:drop-shadow(0 0 8px var(--cyan-glow));flex-shrink:0}
.brand-name{font-weight:800;background:linear-gradient(135deg,#fff 30%,#94a3b8 100%);-webkit-background-clip:text;-webkit-text-fill-color:transparent}
.pulse-beacon{
  display:inline-flex;
  align-items:center;
  gap:6px;
  padding:3px 8px;
  border-radius:999px;
  background:var(--emerald-bg);
  border:1px solid rgba(16,185,129,0.3);
  font-size:11px;
  font-weight:600;
  color:var(--emerald);
}
.beacon-dot{
  width:7px;
  height:7px;
  border-radius:50%;
  background:var(--emerald);
  box-shadow:0 0 0 0 var(--emerald);
  animation:beacon 2s infinite;
}
@keyframes beacon{0%{box-shadow:0 0 0 0 rgba(16,185,129,0.8)}70%{box-shadow:0 0 0 6px rgba(16,185,129,0)}100%{box-shadow:0 0 0 0 rgba(16,185,129,0)}}
.beacon-dot.offline{background:var(--crimson);animation:none}
.pulse-beacon.offline{background:var(--crimson-bg);border-color:rgba(244,63,94,0.3);color:var(--crimson)}

.hdr-right{margin-left:auto;display:flex;align-items:center;gap:10px}
.meta-chip{
  display:inline-flex;
  align-items:center;
  gap:6px;
  padding:4px 10px;
  border-radius:var(--radius-sm);
  font-size:11px;
  font-weight:500;
  background:var(--surface);
  border:1px solid var(--border);
  color:var(--text-muted);
  cursor:pointer;
  transition:all .15s ease;
  user-select:none;
}
.meta-chip:hover{border-color:var(--cyan);color:var(--text)}
.meta-chip.auth-on{border-color:rgba(16,185,129,0.4);color:var(--emerald);background:var(--emerald-bg)}
.meta-chip svg{width:14px;height:14px}

/* Tabs Navigation */
.nav-wrap{background:rgba(10,14,26,0.6);border-bottom:1px solid var(--border);padding:0 20px}
.nav-inner{max-width:1180px;margin:0 auto;display:flex;gap:4px;overflow-x:auto;-webkit-overflow-scrolling:touch;scrollbar-width:none}
.nav-inner::-webkit-scrollbar{display:none}
.tab-btn{
  background:transparent;
  border:none;
  outline:none;
  padding:12px 14px;
  font-family:inherit;
  font-size:12.5px;
  font-weight:600;
  color:var(--text-muted);
  cursor:pointer;
  display:inline-flex;
  align-items:center;
  gap:7px;
  border-bottom:2px solid transparent;
  transition:all .15s ease;
  white-space:nowrap;
}
.tab-btn svg{width:16px;height:16px;color:currentColor;opacity:.7}
.tab-btn:hover{color:var(--text)}
.tab-btn.active{
  color:var(--cyan);
  border-bottom-color:var(--cyan);
}
.tab-btn.active svg{opacity:1;filter:drop-shadow(0 0 6px var(--cyan-glow))}
.tab-badge{
  font-size:10px;
  padding:1px 6px;
  border-radius:999px;
  background:rgba(255,255,255,0.06);
  color:var(--text-muted);
  font-weight:700;
}
.tab-btn.active .tab-badge{background:var(--cyan-bg);color:var(--cyan)}

/* Main Layout */
.main{max-width:1180px;margin:0 auto;padding:24px 20px 80px}
.tab-pane{display:none;animation:fadeIn .2s ease}
.tab-pane.active{display:block}
@keyframes fadeIn{from{opacity:0;transform:translateY(4px)}to{opacity:1;transform:translateY(0)}}

/* Bento Grid */
.bento-grid{
  display:grid;
  grid-template-columns:repeat(4,1fr);
  gap:16px;
  margin-bottom:24px;
}
@media(max-width:1024px){.bento-grid{grid-template-columns:repeat(2,1fr)}}
@media(max-width:600px){.bento-grid{grid-template-columns:1fr}}

.bento-card{
  background:var(--surface);
  border:1px solid var(--border);
  border-radius:var(--radius);
  box-shadow:var(--shadow-card);
  padding:18px;
  display:flex;
  flex-direction:column;
  position:relative;
  overflow:hidden;
  transition:border-color .15s ease;
}
.bento-card:hover{border-color:rgba(255,255,255,0.16)}
.bento-card.col-2{grid-column:span 2}
.bento-card.col-4{grid-column:span 4}
@media(max-width:1024px){.bento-card.col-2,.bento-card.col-4{grid-column:span 2}}
@media(max-width:600px){.bento-card.col-2,.bento-card.col-4{grid-column:span 1}}

.bento-header{display:flex;align-items:center;justify-content:space-between;margin-bottom:10px}
.bento-title{font-size:11.5px;font-weight:700;text-transform:uppercase;letter-spacing:.05em;color:var(--text-dim);display:flex;align-items:center;gap:6px}
.bento-icon{
  width:28px;
  height:28px;
  border-radius:var(--radius-sm);
  display:flex;
  align-items:center;
  justify-content:center;
  background:var(--surface-subtle);
  border:1px solid var(--border);
}
.bento-icon svg{width:15px;height:15px}
.bento-value{font-size:28px;font-weight:800;letter-spacing:-0.03em;color:var(--text);margin-bottom:6px;line-height:1.1}
.bento-sub{font-size:11.5px;color:var(--text-muted);display:flex;align-items:center;gap:6px}

/* Progress bar inside bento */
.progress-track{
  width:100%;
  height:6px;
  border-radius:999px;
  background:rgba(255,255,255,0.06);
  margin-top:10px;
  overflow:hidden;
  position:relative;
}
.progress-fill{
  height:100%;
  border-radius:999px;
  transition:width .4s ease;
}
.progress-fill.emerald{background:linear-gradient(90deg,#10b981,#34d399)}
.progress-fill.crimson{background:linear-gradient(90deg,#f43f5e,#fb7185)}
.progress-fill.cyan{background:linear-gradient(90deg,#0ea5e9,#38bdf8)}

/* Dual-Channel Live DNS Throughput Pulse Chart */
.pulse-chart{
  display:flex;
  height:76px;
  gap:2px;
  position:relative;
  align-items:stretch;
  padding:4px 0;
  margin-top:8px;
}
.pulse-chart::before{
  content:'';
  position:absolute;
  left:0;
  right:0;
  top:50%;
  height:1px;
  background:rgba(255,255,255,0.08);
  pointer-events:none;
}
.pulse-col{
  flex:1;
  min-width:2px;
  display:flex;
  flex-direction:column;
  cursor:pointer;
  transition:background .15s;
  border-radius:2px;
}
.pulse-col:hover{background:rgba(255,255,255,0.08)}
.pulse-top,.pulse-bot{
  flex:1;
  display:flex;
  width:100%;
}
.pulse-top{align-items:flex-end}
.pulse-bot{align-items:flex-start}
.pulse-bar{
  width:100%;
  border-radius:1px;
  transition:height .25s ease;
  min-height:2px;
}
.pulse-bar.top{
  background:linear-gradient(180deg,#34d399,#10b981);
  box-shadow:0 0 6px rgba(16,185,129,0.3);
}
.pulse-bar.bot{
  background:linear-gradient(180deg,#f43f5e,#e11d48);
  box-shadow:0 0 6px rgba(244,63,94,0.3);
}

/* Standard Table Card */
.table-card{
  background:var(--surface);
  border:1px solid var(--border);
  border-radius:var(--radius);
  box-shadow:var(--shadow-card);
  margin-bottom:24px;
  overflow:hidden;
}
.card-hdr{
  padding:16px 20px;
  border-bottom:1px solid var(--border);
  display:flex;
  align-items:center;
  justify-content:space-between;
  flex-wrap:wrap;
  gap:12px;
}
.card-title{
  font-size:14px;
  font-weight:700;
  color:var(--text);
  display:flex;
  align-items:center;
  gap:8px;
}
.card-actions{display:flex;align-items:center;gap:8px;flex-wrap:wrap}

.search-box{
  position:relative;
  display:flex;
  align-items:center;
}
.search-box svg{position:absolute;left:10px;width:14px;height:14px;color:var(--text-dim);pointer-events:none}
.search-input{
  background:var(--surface-subtle);
  border:1px solid var(--border);
  border-radius:var(--radius-sm);
  color:var(--text);
  font-family:inherit;
  font-size:12px;
  padding:6px 12px 6px 30px;
  width:200px;
  outline:none;
  transition:all .15s ease;
}
.search-input:focus{border-color:var(--border-focus);width:250px;box-shadow:0 0 0 2px var(--cyan-bg)}

/* Buttons */
.btn{
  background:var(--surface-elevated);
  border:1px solid var(--border);
  border-radius:var(--radius-sm);
  color:var(--text);
  font-family:inherit;
  font-size:12px;
  font-weight:600;
  padding:6px 12px;
  cursor:pointer;
  display:inline-flex;
  align-items:center;
  gap:6px;
  transition:all .15s ease;
  user-select:none;
  text-decoration:none;
}
.btn:hover{background:var(--surface-hover);border-color:rgba(255,255,255,0.18)}
.btn.primary{background:linear-gradient(135deg,#0ea5e9,#0284c7);border-color:rgba(14,165,233,0.5);color:#fff}
.btn.primary:hover{box-shadow:0 0 12px var(--cyan-glow)}
.btn.danger{background:linear-gradient(135deg,#f43f5e,#e11d48);border-color:rgba(244,63,94,0.5);color:#fff}
.btn.danger:hover{box-shadow:0 0 12px var(--crimson-glow)}
.btn.sm{padding:4px 9px;font-size:11px}

/* Tables */
.table-wrap{width:100%;overflow-x:auto;-webkit-overflow-scrolling:touch}
table{width:100%;border-collapse:collapse;text-align:left;font-size:12.5px}
thead th{
  background:rgba(10,14,26,0.6);
  color:var(--text-dim);
  font-size:11px;
  font-weight:700;
  text-transform:uppercase;
  letter-spacing:.05em;
  padding:10px 18px;
  border-bottom:1px solid var(--border);
  white-space:nowrap;
}
tbody td{
  padding:12px 18px;
  border-bottom:1px solid var(--border-subtle);
  vertical-align:middle;
}
tbody tr{transition:background .12s ease}
tbody tr:hover{background:var(--surface-hover)}
tbody tr:last-child td{border-bottom:none}

/* Table elements */
.dev-cell{display:flex;align-items:center;gap:8px}
.dev-name{font-weight:600;color:var(--text)}
.edit-btn{
  background:transparent;
  border:none;
  cursor:pointer;
  color:var(--cyan);
  display:inline-flex;
  align-items:center;
  padding:2px 4px;
  border-radius:4px;
  opacity:.8;
}
.edit-btn:hover{opacity:1;background:var(--cyan-bg)}
.edit-btn svg{width:13px;height:13px}

.status-badge{
  display:inline-flex;
  align-items:center;
  gap:5px;
  padding:2px 7px;
  border-radius:999px;
  font-size:10.5px;
  font-weight:700;
  text-transform:uppercase;
  letter-spacing:.04em;
}
.status-badge.online{background:var(--emerald-bg);color:var(--emerald)}
.status-badge.offline{background:rgba(255,255,255,0.06);color:var(--text-dim)}
.status-badge.banned{background:var(--crimson-bg);color:var(--crimson)}
.status-badge.nodata{background:var(--amber-bg);color:var(--amber)}
.status-badge.blocked{background:var(--crimson-bg);color:var(--crimson)}

.ratio-meter{
  width:90px;
  height:5px;
  border-radius:999px;
  background:rgba(255,255,255,0.08);
  overflow:hidden;
  display:inline-block;
  vertical-align:middle;
  margin-right:6px;
}
.ratio-fill{height:100%;border-radius:999px;background:var(--crimson)}

.copy-chip{
  cursor:pointer;
  padding:2px 5px;
  border-radius:4px;
  transition:background .15s;
}
.copy-chip:hover{background:rgba(255,255,255,0.08);color:var(--cyan)}

/* Forms */
.form-grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(280px,1fr));gap:16px;padding:20px}
.form-group{display:flex;flex-direction:column;gap:6px}
.form-label{font-size:12px;font-weight:700;color:var(--text-muted);display:flex;justify-content:space-between}
.form-input{
  background:var(--surface-subtle);
  border:1px solid var(--border);
  border-radius:var(--radius-sm);
  color:var(--text);
  font-family:inherit;
  font-size:13px;
  padding:8px 12px;
  outline:none;
  transition:border-color .15s ease;
}
.form-input:focus{border-color:var(--border-focus);box-shadow:0 0 0 2px var(--cyan-bg)}

.chips-row{display:flex;flex-wrap:wrap;gap:6px;margin-top:8px}
.preset-chip{
  padding:4px 9px;
  border-radius:999px;
  background:var(--surface-subtle);
  border:1px solid var(--border);
  font-size:11px;
  color:var(--text-muted);
  cursor:pointer;
  transition:all .15s;
}
.preset-chip:hover{border-color:var(--cyan);color:var(--cyan);background:var(--cyan-bg)}

/* Upload Box */
.drop-zone{
  border:2px dashed var(--border);
  border-radius:var(--radius);
  padding:28px 20px;
  text-align:center;
  cursor:pointer;
  transition:all .2s ease;
  background:var(--surface-subtle);
}
.drop-zone:hover,.drop-zone.dragover{border-color:var(--cyan);background:var(--cyan-bg)}
.drop-icon svg{width:36px;height:36px;color:var(--cyan);margin-bottom:8px}

/* Modals */
.modal-overlay{
  position:fixed;
  inset:0;
  background:rgba(0,0,0,0.7);
  backdrop-filter:blur(6px);
  -webkit-backdrop-filter:blur(6px);
  z-index:50;
  display:none;
  align-items:center;
  justify-content:center;
  padding:20px;
}
.modal-overlay.open{display:flex;animation:fadeIn .15s ease}
.modal-box{
  background:var(--surface);
  border:1px solid var(--border);
  border-radius:var(--radius);
  box-shadow:0 20px 40px rgba(0,0,0,0.8),inset 0 1px 0 rgba(255,255,255,0.1);
  width:100%;
  max-width:420px;
  overflow:hidden;
}
.modal-hdr{
  padding:16px 20px;
  border-bottom:1px solid var(--border);
  display:flex;
  align-items:center;
  justify-content:space-between;
}
.modal-title{font-size:14px;font-weight:700}
.modal-close{
  background:transparent;
  border:none;
  cursor:pointer;
  color:var(--text-dim);
  font-size:18px;
  line-height:1;
}
.modal-close:hover{color:var(--text)}
.modal-body{padding:20px}
.modal-ftr{
  padding:14px 20px;
  border-top:1px solid var(--border);
  display:flex;
  justify-content:flex-end;
  gap:8px;
  background:var(--surface-subtle);
}

/* Toast notifications */
#toastCont{
  position:fixed;
  bottom:20px;
  right:20px;
  z-index:60;
  display:flex;
  flex-direction:column;
  gap:8px;
  pointer-events:none;
}
.toast{
  background:var(--surface-elevated);
  border:1px solid var(--border);
  box-shadow:0 8px 24px rgba(0,0,0,0.6);
  border-radius:var(--radius-sm);
  padding:10px 16px;
  font-size:12px;
  font-weight:600;
  color:var(--text);
  display:flex;
  align-items:center;
  gap:8px;
  pointer-events:auto;
  animation:toastIn .2s ease;
  max-width:320px;
}
.toast.success{border-color:rgba(16,185,129,0.5);color:var(--emerald)}
.toast.error{border-color:rgba(244,63,94,0.5);color:var(--crimson)}
@keyframes toastIn{from{opacity:0;transform:translateY(8px)}to{opacity:1;transform:translateY(0)}}
</style>
</head>
<body>

<!-- Header -->
<header>
  <div class="hdr-inner">
    <a href="#" class="brand">
      <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.2"><path d="M12 22s8-4 8-10V5l-8-3-8 3v7c0 6 8 10 8 10z"/><polyline points="9 12 11 14 15 10"/></svg>
      <span class="brand-name">ESP32 AdBlock</span>
    </a>

    <div class="pulse-beacon" id="statusBeacon">
      <span class="beacon-dot" id="beaconDot"></span>
      <span id="beaconText">ACTIVE</span>
    </div>

    <div class="hdr-right">
      <div class="meta-chip" title="Device local IPv4 address">
        <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><rect x="2" y="2" width="20" height="8" rx="2"/><rect x="2" y="14" width="20" height="8" rx="2"/><line x1="6" y1="6" x2="6.01" y2="6"/><line x1="6" y1="18" x2="6.01" y2="18"/></svg>
        <span class="mono" id="chipIp">192.168.0.x</span>
      </div>
      <div class="meta-chip" title="Continuous system uptime">
        <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><circle cx="12" cy="12" r="10"/><polyline points="12 6 12 12 16 14"/></svg>
        <span class="mono" id="chipUptime">0d 0h 0m</span>
      </div>
      <div class="meta-chip" id="authBtn" onclick="openTokenModal()" title="Admin security token authentication">
        <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><rect x="3" y="11" width="18" height="11" rx="2" ry="2"/><path d="M7 11V7a5 5 0 0 1 10 0v4"/></svg>
        <span id="authLabel">Auth Token</span>
      </div>
    </div>
  </div>
</header>

<!-- Tabs Navigation -->
<div class="nav-wrap">
  <div class="nav-inner">
    <button class="tab-btn active" data-tab="tab-overview" onclick="switchTab('tab-overview')">
      <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><rect x="3" y="3" width="7" height="9" rx="1"/><rect x="14" y="3" width="7" height="5" rx="1"/><rect x="14" y="12" width="7" height="9" rx="1"/><rect x="3" y="16" width="7" height="5" rx="1"/></svg>
      Overview
    </button>
    <button class="tab-btn" data-tab="tab-clients" onclick="switchTab('tab-clients')">
      <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M17 21v-2a4 4 0 0 0-4-4H5a4 4 0 0 0-4 4v2"/><circle cx="9" cy="7" r="4"/><path d="M23 21v-2a4 4 0 0 0-3-3.87"/><path d="M16 3.13a4 4 0 0 1 0 7.75"/></svg>
      Clients <span class="tab-badge" id="navClientCount">0</span>
    </button>
    <button class="tab-btn" data-tab="tab-rules" onclick="switchTab('tab-rules')">
      <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><polyline points="9 11 12 14 22 4"/><path d="M21 12v7a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2h11"/></svg>
      Rules &amp; Blocklist <span class="tab-badge" id="navRuleCount">0</span>
    </button>
    <button class="tab-btn" data-tab="tab-logs" onclick="switchTab('tab-logs')">
      <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M12 22s8-4 8-10V5l-8-3-8 3v7c0 6 8 10 8 10z"/><line x1="12" y1="8" x2="12" y2="12"/><line x1="12" y1="16" x2="12.01" y2="16"/></svg>
      Blocked Log <span class="tab-badge" id="navLogCount">0</span>
    </button>
    <button class="tab-btn" data-tab="tab-system" onclick="switchTab('tab-system')">
      <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><circle cx="12" cy="12" r="3"/><path d="M19.4 15a1.65 1.65 0 0 0 .33 1.82l.06.06a2 2 0 0 1 0 2.83 2 2 0 0 1-2.83 0l-.06-.06a1.65 1.65 0 0 0-1.82-.33 1.65 1.65 0 0 0-1 1.51V21a2 2 0 0 1-2 2 2 2 0 0 1-2-2v-.09A1.65 1.65 0 0 0 9 19.4a1.65 1.65 0 0 0-1.82.33l-.06.06a2 2 0 0 1-2.83 0 2 2 0 0 1 0-2.83l.06-.06a1.65 1.65 0 0 0 .33-1.82 1.65 1.65 0 0 0-1.51-1H3a2 2 0 0 1-2-2 2 2 0 0 1 2-2h.09A1.65 1.65 0 0 0 4.6 9a1.65 1.65 0 0 0-.33-1.82l-.06-.06a2 2 0 0 1 0-2.83 2 2 0 0 1 2.83 0l.06.06a1.65 1.65 0 0 0 1.82.33H9a1.65 1.65 0 0 0 1-1.51V3a2 2 0 0 1 2-2 2 2 0 0 1 2 2v.09a1.65 1.65 0 0 0 1 1.51 1.65 1.65 0 0 0 1.82-.33l.06-.06a2 2 0 0 1 2.83 0 2 2 0 0 1 0 2.83l-.06.06a1.65 1.65 0 0 0-.33 1.82V9a1.65 1.65 0 0 0 1.51 1H21a2 2 0 0 1 2 2 2 2 0 0 1-2 2h-.09a1.65 1.65 0 0 0-1.51 1z"/></svg>
      System &amp; Updates
    </button>
  </div>
</div>

<!-- Main Contents -->
<div class="main">

  <!-- TAB 1: OVERVIEW -->
  <div class="tab-pane active" id="tab-overview">
    
    <!-- Bento Metric Cards -->
    <div class="bento-grid">
      
      <!-- Card: Blocked Queries -->
      <div class="bento-card">
        <div class="bento-header">
          <span class="bento-title">Ads Sinkholed</span>
          <div class="bento-icon" style="color:var(--crimson);border-color:rgba(244,63,94,0.3)">
            <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><circle cx="12" cy="12" r="10"/><line x1="4.93" y1="4.93" x2="19.07" y2="19.07"/></svg>
          </div>
        </div>
        <div class="bento-value crimson mono" id="kpiBlocked">0</div>
        <div class="bento-sub">
          <span>Block Rate:</span>
          <strong class="crimson mono" id="kpiRatio">0.0%</strong>
        </div>
        <div class="progress-track">
          <div class="progress-fill crimson" id="kpiRatioBar" style="width:0%"></div>
        </div>
      </div>

      <!-- Card: Allowed Queries -->
      <div class="bento-card">
        <div class="bento-header">
          <span class="bento-title">Clean Traffic Passed</span>
          <div class="bento-icon" style="color:var(--emerald);border-color:rgba(16,185,129,0.3)">
            <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M22 11.08V12a10 10 0 1 1-5.93-9.14"/><polyline points="22 4 12 14.01 9 11.01"/></svg>
          </div>
        </div>
        <div class="bento-value emerald mono" id="kpiAllowed">0</div>
        <div class="bento-sub">
          <span>Forwarded to Quad9 (:53)</span>
        </div>
        <div class="progress-track">
          <div class="progress-fill emerald" style="width:100%"></div>
        </div>
      </div>

      <!-- Card: Active LAN Clients -->
      <div class="bento-card">
        <div class="bento-header">
          <span class="bento-title">Active LAN Clients</span>
          <div class="bento-icon" style="color:var(--cyan);border-color:rgba(14,165,233,0.3)">
            <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><rect x="2" y="3" width="20" height="14" rx="2"/><line x1="8" y1="21" x2="16" y2="21"/><line x1="12" y1="17" x2="12" y2="21"/></svg>
          </div>
        </div>
        <div class="bento-value cyan mono" id="kpiClients">0</div>
        <div class="bento-sub">
          <span>Online in last 15 min</span>
        </div>
        <div class="progress-track">
          <div class="progress-fill cyan" style="width:100%"></div>
        </div>
      </div>

      <!-- Card: Flash Rules -->
      <div class="bento-card">
        <div class="bento-header">
          <span class="bento-title">Rules in Flash</span>
          <div class="bento-icon" style="color:var(--purple);border-color:rgba(139,92,246,0.3)">
            <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><line x1="18" y1="20" x2="18" y2="10"/><line x1="12" y1="20" x2="12" y2="4"/><line x1="6" y1="20" x2="6" y2="14"/></svg>
          </div>
        </div>
        <div class="bento-value purple mono" id="kpiDomains">0</div>
        <div class="bento-sub">
          <span id="kpiPrefixes">Prefix index: 256 RAM buckets</span>
        </div>
        <div class="progress-track">
          <div class="progress-fill" style="background:var(--purple);width:100%"></div>
        </div>
      </div>

    </div>

    <!-- Live Throughput Pulse Graph -->
    <div class="table-card" style="padding:20px;margin-bottom:24px">
      <div style="display:flex;align-items:center;justify-content:space-between;margin-bottom:14px;flex-wrap:wrap;gap:10px">
        <div>
          <div style="font-size:14px;font-weight:700">Live DNS Resolution Throughput</div>
          <div class="dim" style="font-size:11.5px">Real-time dual-channel query pulse polled from ESP32 Core 0</div>
        </div>
        <div style="display:flex;align-items:center;gap:14px">
          <div style="display:flex;align-items:center;gap:6px;font-size:11.5px;font-weight:600"><span style="width:8px;height:8px;border-radius:2px;background:var(--emerald);box-shadow:0 0 6px var(--emerald);display:inline-block"></span> <span>Allowed</span></div>
          <div style="display:flex;align-items:center;gap:6px;font-size:11.5px;font-weight:600"><span style="width:8px;height:8px;border-radius:2px;background:var(--crimson);box-shadow:0 0 6px var(--crimson);display:inline-block"></span> <span>Sinkholed</span></div>
          <div class="meta-chip sm" style="cursor:default">
            <span class="dim">Rate:</span> <strong class="mono emerald" id="qpmRate">0 q/m</strong> <span class="mono dim" id="qpsRate">(0.0 q/s)</span>
          </div>
        </div>
      </div>
      <div class="pulse-chart" id="pulseChart"></div>
    </div>

    <!-- Hardware Health Bento Bar -->
    <div class="bento-grid">
      <div class="bento-card col-2">
        <div class="bento-header">
          <span class="bento-title">ESP32 Heap Memory (DRAM)</span>
          <span class="mono cyan" id="heapTxt">0 KB free</span>
        </div>
        <div class="progress-track" style="height:8px">
          <div class="progress-fill cyan" id="heapBar" style="width:40%"></div>
        </div>
        <div class="bento-sub" style="margin-top:8px">
          <span>Zero fragmentation architecture &bull; 115 KB free contiguous DRAM headroom</span>
        </div>
      </div>

      <div class="bento-card col-2">
        <div class="bento-header">
          <span class="bento-title">Wi-Fi Signal (2.4 GHz)</span>
          <span class="mono emerald" id="wifiRssiTxt">-0 dBm</span>
        </div>
        <div class="progress-track" style="height:8px">
          <div class="progress-fill emerald" id="wifiRssiBar" style="width:75%"></div>
        </div>
        <div class="bento-sub" style="margin-top:8px">
          <span>TX Power capped at 17 dBm &bull; Thermal throttling prevented</span>
        </div>
      </div>
    </div>

  </div>

  <!-- TAB 2: CLIENTS -->
  <div class="tab-pane" id="tab-clients">
    
    <!-- Active Clients (Online) Card -->
    <div class="table-card">
      <div class="card-hdr">
        <div class="card-title">
          <span class="beacon-dot"></span>
          <span>Active Clients (Online)</span>
          <span class="tab-badge emerald" id="onlineBadge">0</span>
        </div>
        <div class="card-actions">
          <div class="search-box">
            <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><circle cx="11" cy="11" r="8"/><line x1="21" y1="21" x2="16.65" y2="16.65"/></svg>
            <input class="search-input" id="clientSearch" placeholder="Filter name, IP, MAC..." oninput="renderClients()">
          </div>
          <button class="btn sm" onclick="exportClientsCSV()" title="Export clients table to CSV">
            <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" style="width:13px;height:13px"><path d="M21 15v4a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2v-4"/><polyline points="7 10 12 15 17 10"/><line x1="12" y1="15" x2="12" y2="3"/></svg>
            Export CSV
          </button>
        </div>
      </div>
      <div class="table-wrap">
        <table>
          <thead>
            <tr>
              <th>Device Name</th>
              <th>Client IP</th>
              <th>Physical MAC</th>
              <th>Blocked</th>
              <th>Passed</th>
              <th>Block Ratio</th>
              <th style="text-align:right">Action</th>
            </tr>
          </thead>
          <tbody id="clientRows">
            <tr><td colspan="7" class="dim" style="text-align:center;padding:24px">Discovering network clients...</td></tr>
          </tbody>
        </table>
      </div>
    </div>

    <!-- Offline Devices Card (Strictly Deduplicated) -->
    <div class="table-card">
      <div class="card-hdr">
        <div class="card-title">
          <span class="beacon-dot offline"></span>
          <span>Offline Devices</span>
          <span class="tab-badge" id="offlineBadge">0</span>
        </div>
        <div class="dim" style="font-size:11.5px">Inactivity &gt; 15 min &bull; Strictly non-overlapping with active devices</div>
      </div>
      <div class="table-wrap">
        <table>
          <thead>
            <tr>
              <th>Device Name</th>
              <th>Client IP</th>
              <th>Physical MAC</th>
              <th>Last Active</th>
              <th>Blocked</th>
              <th>Passed</th>
              <th style="text-align:right">Manage</th>
            </tr>
          </thead>
          <tbody id="offlineRows">
            <tr><td colspan="7" class="dim" style="text-align:center;padding:24px">No offline devices recorded</td></tr>
          </tbody>
        </table>
      </div>
    </div>

  </div>

  <!-- TAB 3: RULES & BLOCKLIST -->
  <div class="tab-pane" id="tab-rules">
    
    <!-- Add Custom Rule Card -->
    <div class="table-card">
      <div class="card-hdr">
        <div class="card-title">
          <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" style="width:16px;height:16px;color:var(--cyan)"><circle cx="12" cy="12" r="10"/><line x1="12" y1="8" x2="12" y2="16"/><line x1="8" y1="12" x2="16" y2="12"/></svg>
          <span>Add Custom Block Rule</span>
        </div>
      </div>
      <div class="form-grid">
        <div class="form-group" style="grid-column:1/-1">
          <label class="form-label">Domain or Hostname to Sinkhole</label>
          <div style="display:flex;gap:8px">
            <input class="form-input mono" id="customInput" placeholder="e.g. ads.tracker.com, telemetry.local" style="flex:1" onkeydown="if(event.key==='Enter')addCustomDomain()">
            <button class="btn primary" onclick="addCustomDomain()">Block Domain</button>
          </div>
          <div class="chips-row">
            <span class="dim" style="font-size:11px;align-self:center;margin-right:4px">Presets:</span>
            <span class="preset-chip" onclick="quickAdd('analytics.google.com')">+ Google Analytics</span>
            <span class="preset-chip" onclick="quickAdd('ads.tiktok.com')">+ TikTok Ads</span>
            <span class="preset-chip" onclick="quickAdd('graph.facebook.com')">+ Facebook Telemetry</span>
            <span class="preset-chip" onclick="quickAdd('telemetry.samsungcloud.com')">+ Samsung Smart TV</span>
            <span class="preset-chip" onclick="quickAdd('data.mistat.xiaomi.com')">+ Xiaomi Spyware</span>
          </div>
        </div>
      </div>
    </div>

    <!-- Custom Blocked Domains List -->
    <div class="table-card">
      <div class="card-hdr">
        <div class="card-title">
          <span>Active Custom Blacklist</span>
          <span class="tab-badge" id="customCountBadge">0</span>
        </div>
        <div class="card-actions">
          <div class="search-box">
            <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><circle cx="11" cy="11" r="8"/><line x1="21" y1="21" x2="16.65" y2="16.65"/></svg>
            <input class="search-input" id="customSearch" placeholder="Search custom rules..." oninput="renderCustom()">
          </div>
        </div>
      </div>
      <div class="table-wrap">
        <table>
          <thead>
            <tr>
              <th>Blocked Domain</th>
              <th>Target Sinkhole</th>
              <th style="text-align:right">Action</th>
            </tr>
          </thead>
          <tbody id="customRows">
            <tr><td colspan="3" class="dim" style="text-align:center;padding:24px">No custom domains added</td></tr>
          </tbody>
        </table>
      </div>
    </div>

    <!-- Flash Blocklist Upload -->
    <div class="table-card">
      <div class="card-hdr">
        <div class="card-title">
          <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" style="width:16px;height:16px;color:var(--purple)"><path d="M21 15v4a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2v-4"/><polyline points="17 8 12 3 7 8"/><line x1="12" y1="3" x2="12" y2="15"/></svg>
          <span>Flash Binary Blocklist Upload (blocklist.bin)</span>
        </div>
      </div>
      <div style="padding:20px">
        <input type="file" id="blocklistFileInput" accept=".bin" style="display:none" onchange="handleFileSelected(this.files)">
        <div class="drop-zone" id="dropZone" onclick="$('blocklistFileInput').click()" ondragover="event.preventDefault();this.classList.add('dragover')" ondragleave="this.classList.remove('dragover')" ondrop="event.preventDefault();this.classList.remove('dragover');handleFileSelected(event.dataTransfer.files)">
          <div class="drop-icon"><svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.8"><path d="M4 14.899A7 7 0 1 1 15.71 8h1.79a4.5 4.5 0 0 1 2.5 8.242"/><path d="M12 12v9"/><path d="m16 16-4-4-4 4"/></svg></div>
          <div style="font-size:14px;font-weight:700;margin-bottom:4px">Select or Drag blocklist.bin Here</div>
          <div class="dim" style="font-size:11.5px">Pre-compiled 40-bit FNV-1a binary blocklist &bull; Max 1.20 MB</div>
        </div>
        <div id="uploadProgressBox" style="display:none;margin-top:14px">
          <div style="display:flex;justify-content:space-between;font-size:11.5px;margin-bottom:6px">
            <span id="uploadStatusTxt">Writing to LittleFS flash...</span>
            <span class="mono cyan" id="uploadPctTxt">0%</span>
          </div>
          <div class="progress-track">
            <div class="progress-fill cyan" id="uploadBar" style="width:0%"></div>
          </div>
        </div>
      </div>
    </div>

  </div>

  <!-- TAB 4: BLOCKED LOG -->
  <div class="tab-pane" id="tab-logs">
    <div class="table-card">
      <div class="card-hdr">
        <div class="card-title">
          <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" style="width:16px;height:16px;color:var(--crimson)"><circle cx="12" cy="12" r="10"/><line x1="12" y1="8" x2="12" y2="12"/><line x1="12" y1="16" x2="12.01" y2="16"/></svg>
          <span>Real-Time Blocked Query Activity Log</span>
          <span class="tab-badge crimson" id="logCountBadge">0</span>
        </div>
        <div class="card-actions">
          <div class="search-box">
            <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><circle cx="11" cy="11" r="8"/><line x1="21" y1="21" x2="16.65" y2="16.65"/></svg>
            <input class="search-input" id="logSearch" placeholder="Filter domain or device..." oninput="renderLogs()">
          </div>
          <button class="btn sm" onclick="fetchLogs()" title="Refresh blocked log">
            <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" style="width:13px;height:13px"><polyline points="23 4 23 10 17 10"/><path d="M20.49 15a9 9 0 1 1-2.12-9.36L23 10"/></svg>
            Refresh
          </button>
          <button class="btn sm" onclick="exportLogsCSV()" title="Export recent blocked activity to CSV">
            <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" style="width:13px;height:13px"><path d="M21 15v4a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2v-4"/><polyline points="7 10 12 15 17 10"/><line x1="12" y1="15" x2="12" y2="3"/></svg>
            Export CSV
          </button>
        </div>
      </div>
      <div class="table-wrap">
        <table>
          <thead>
            <tr>
              <th>Timestamp</th>
              <th>Client Device</th>
              <th>Destination Domain</th>
              <th>Type</th>
              <th>Action</th>
            </tr>
          </thead>
          <tbody id="logRows">
            <tr><td colspan="5" class="dim" style="text-align:center;padding:24px">No blocked queries logged yet</td></tr>
          </tbody>
        </table>
      </div>
    </div>
  </div>

  <!-- TAB 5: SYSTEM & UPDATES -->
  <div class="tab-pane" id="tab-system">
    
    <!-- Remote Auto-Update Settings -->
    <div class="table-card">
      <div class="card-hdr">
        <div class="card-title">
          <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" style="width:16px;height:16px;color:var(--cyan)"><path d="M21.5 2v6h-6M21.34 15.57a10 10 0 1 1-.57-8.38l5.67-5.67"/></svg>
          <span>Automated Remote Blocklist Updates (HTTPS OTA)</span>
        </div>
      </div>
      <div class="form-grid">
        <div class="form-group" style="grid-column:span 2">
          <label class="form-label">Remote HTTPS Blocklist URL</label>
          <input class="form-input mono" id="updateUrlInput" placeholder="https://raw.githubusercontent.com/.../blocklist.bin">
        </div>
        <div class="form-group">
          <label class="form-label">Update Interval (Hours)</label>
          <input class="form-input mono" id="updateIntervalInput" type="number" min="1" max="720" value="24">
        </div>
        <div class="form-group" style="justify-content:flex-end">
          <div style="display:flex;gap:8px">
            <button class="btn primary" onclick="saveUpdateSettings()">Save Schedule</button>
            <button class="btn" onclick="triggerManualFetch()">Fetch &amp; Swap Now</button>
          </div>
        </div>
      </div>
      <div style="padding:0 20px 16px;font-size:11.5px;color:var(--text-muted)">
        <span>Last Update Status:</span> <strong class="mono cyan" id="updateStatusTxt">never checked</strong>
      </div>
    </div>

    <!-- Hardware Specifications -->
    <div class="bento-grid">
      <div class="bento-card col-2">
        <div class="bento-header">
          <span class="bento-title">ESP32 DevKit V1 Hardware</span>
          <span class="mono emerald">Eco 3.1</span>
        </div>
        <table style="margin-top:6px;font-size:12px">
          <tr><td class="dim">SoC Model</td><td class="mono">ESP32-D0WD-V3 (Revision v3.1)</td></tr>
          <tr><td class="dim">CPU Clock</td><td class="mono">240 MHz Dual-Core Xtensa LX6 (Peak 600 DMIPS)</td></tr>
          <tr><td class="dim">Total SRAM</td><td class="mono">520 KB Internal SRAM (~320 KB usable)</td></tr>
          <tr><td class="dim">Flash Memory</td><td class="mono">4 MB SPI Flash (DIO @ 80 MHz, Boya)</td></tr>
          <tr><td class="dim">LittleFS Partition</td><td class="mono">2.625 MB (0x160000 - 0x400000, ~500k hashes)</td></tr>
          <tr><td class="dim">Watchdogs</td><td class="mono">500ms IWDT + 20s TWDT (Level 4 Brownout)</td></tr>
        </table>
      </div>

      <div class="bento-card col-2">
        <div class="bento-header">
          <span class="bento-title">Network &amp; DNS Engine</span>
          <span class="mono cyan">Core 0 Async</span>
        </div>
        <table style="margin-top:6px;font-size:12px">
          <tr><td class="dim">DNS Sinkhole Port</td><td class="mono">UDP :53 (&lt;0.5 ms Latency)</td></tr>
          <tr><td class="dim">Upstream Forwarder</td><td class="mono">Quad9 (9.9.9.9) Async select()</td></tr>
          <tr><td class="dim">EDNS0 Compliance</td><td class="mono">RFC 6891 Clamp 1232B (OPT RR)</td></tr>
          <tr><td class="dim">IPv6 Handling</td><td class="mono">RFC NODATA (ANCOUNT=0, NOERROR)</td></tr>
          <tr><td class="dim">Web Server</td><td class="mono">Core 1 Priority 5 (esp_http_server)</td></tr>
        </table>
      </div>
    </div>

  </div>

</div>

<!-- Device Name Edit Modal -->
<div class="modal-overlay" id="nameModal">
  <div class="modal-box">
    <div class="modal-hdr">
      <div class="modal-title">Rename Network Device</div>
      <button class="modal-close" onclick="closeNameModal()">&times;</button>
    </div>
    <div class="modal-body">
      <div class="dim" style="font-size:11.5px;margin-bottom:6px">Assign a friendly alias for device IP:</div>
      <div class="mono cyan" id="nameModalSub" style="font-size:14px;font-weight:700;margin-bottom:14px">192.168.0.x</div>
      <div class="form-group">
        <label class="form-label">Friendly Device Alias</label>
        <input class="form-input" id="nameModalInput" placeholder="e.g. Living Room TV, Work Laptop" maxlength="31" onkeydown="if(event.key==='Enter')saveDeviceName()">
      </div>
    </div>
    <div class="modal-ftr">
      <button class="btn" onclick="clearDeviceName()">Clear Alias</button>
      <button class="btn primary" onclick="saveDeviceName()">Save Alias</button>
    </div>
  </div>
</div>

<!-- Admin Token Modal -->
<div class="modal-overlay" id="tokenModal">
  <div class="modal-box">
    <div class="modal-hdr">
      <div class="modal-title">Admin Token Authentication</div>
      <button class="modal-close" onclick="closeTokenModal()">&times;</button>
    </div>
    <div class="modal-body">
      <div class="dim" style="font-size:11.5px;margin-bottom:12px">Required for mutating operations (rules, banning, renaming, updates).</div>
      <div class="form-group">
        <label class="form-label">Security Token</label>
        <input class="form-input mono" type="password" id="tokenInput" placeholder="Enter ADMIN_TOKEN" onkeydown="if(event.key==='Enter')saveToken()">
      </div>
    </div>
    <div class="modal-ftr">
      <button class="btn" onclick="clearToken()">Log Out</button>
      <button class="btn primary" onclick="saveToken()">Save Token</button>
    </div>
  </div>
</div>

<!-- Toast Container -->
<div id="toastCont"></div>

<script>
// Dashboard State & Core Logic
var $ = function(id){ return document.getElementById(id); };
var adminToken = localStorage.getItem('esp32_admin_token') || '';
var pollTimeout = null;
var activeTab = 'tab-overview';
var targetEditIp = '';

// Dual-Channel Throughput Pulse History
var CHART_COLS = 48;
var prevBlock = null, prevPass = null, pulseHistory = [];

// Cached data
var lastStats = null;
var lastLogs = [];

function notify(msg, isError) {
  var c = $('toastCont');
  var t = document.createElement('div');
  t.className = 'toast ' + (isError ? 'error' : 'success');
  t.textContent = msg;
  c.appendChild(t);
  setTimeout(function(){
    t.style.opacity = '0';
    t.style.transform = 'translateY(6px)';
    t.style.transition = 'all .3s';
    setTimeout(function(){ if(t.parentNode) t.parentNode.removeChild(t); }, 300);
  }, 3500);
}

function updateAuthBadge() {
  var b = $('authBtn');
  var l = $('authLabel');
  if (adminToken) {
    b.classList.add('auth-on');
    l.textContent = 'Authorized';
  } else {
    b.classList.remove('auth-on');
    l.textContent = 'Auth Token';
  }
}

function openTokenModal() {
  $('tokenInput').value = adminToken;
  $('tokenModal').classList.add('open');
  setTimeout(function(){ $('tokenInput').focus(); }, 100);
}
function closeTokenModal() { $('tokenModal').classList.remove('open'); }
function saveToken() {
  adminToken = $('tokenInput').value.trim();
  localStorage.setItem('esp32_admin_token', adminToken);
  updateAuthBadge();
  closeTokenModal();
  notify('Admin token saved');
  loadData();
}
function clearToken() {
  adminToken = '';
  localStorage.removeItem('esp32_admin_token');
  updateAuthBadge();
  closeTokenModal();
  notify('Logged out');
  loadData();
}

function switchTab(id) {
  activeTab = id;
  var btns = document.querySelectorAll('.tab-btn');
  btns.forEach(function(b){
    if (b.getAttribute('data-tab') === id) b.classList.add('active');
    else b.classList.remove('active');
  });
  var panes = document.querySelectorAll('.tab-pane');
  panes.forEach(function(p){
    if (p.id === id) p.classList.add('active');
    else p.classList.remove('active');
  });
  if (id === 'tab-logs') fetchLogs();
}

// Keyboard shortcuts: 1-5 for tabs, / for search, Esc for modals
window.addEventListener('keydown', function(e){
  if (e.target.tagName === 'INPUT' || e.target.tagName === 'TEXTAREA') {
    if (e.key === 'Escape') { closeNameModal(); closeTokenModal(); }
    return;
  }
  if (e.key === '1') switchTab('tab-overview');
  else if (e.key === '2') switchTab('tab-clients');
  else if (e.key === '3') switchTab('tab-rules');
  else if (e.key === '4') switchTab('tab-logs');
  else if (e.key === '5') switchTab('tab-system');
  else if (e.key === '/') {
    e.preventDefault();
    if (activeTab === 'tab-clients') $('clientSearch').focus();
    else if (activeTab === 'tab-rules') $('customSearch').focus();
    else if (activeTab === 'tab-logs') $('logSearch').focus();
  } else if (e.key === 'Escape') {
    closeNameModal();
    closeTokenModal();
  }
});

// Dual-Channel Live DNS Throughput Pulse Chart
function initPulseChart() {
  var c = $('pulseChart');
  if (!c) return;
  var html = '';
  for (var i = 0; i < CHART_COLS; i++) {
    html += '<div class="pulse-col" title="Waiting for traffic...">' +
      '<div class="pulse-top"><div class="pulse-bar top" style="height:2px;opacity:0.35"></div></div>' +
      '<div class="pulse-bot"><div class="pulse-bar bot" style="height:0%;opacity:0.35"></div></div>' +
      '</div>';
  }
  c.innerHTML = html;
}

function updatePulseChart(b, p) {
  if (prevBlock !== null) {
    if (b < prevBlock || p < prevPass) pulseHistory = []; // Board reboot or counter rollover
    var db = Math.max(0, b - prevBlock);
    var dp = Math.max(0, p - prevPass);
    pulseHistory.push({ b: db, p: dp });
    if (pulseHistory.length > CHART_COLS) pulseHistory.shift();

    var qpm = Math.round((db + dp) * 20); // 3s interval * 20 = 60s
    var qps = ((db + dp) / 3.0).toFixed(1);
    if ($('qpmRate')) $('qpmRate').textContent = qpm + ' q/m';
    if ($('qpsRate')) $('qpsRate').textContent = '(' + qps + ' q/s)';
  }
  prevBlock = b; prevPass = p;

  var maxVal = 1;
  pulseHistory.forEach(function(h){
    if (h.p > maxVal) maxVal = h.p;
    if (h.b > maxVal) maxVal = h.b;
  });

  var chartEl = $('pulseChart');
  if (!chartEl) return;
  var cols = chartEl.children;
  for (var i = 0; i < CHART_COLS; i++) {
    var idx = pulseHistory.length - (CHART_COLS - i);
    var item = idx >= 0 ? pulseHistory[idx] : null;
    var col = cols[i];
    if (!col) continue;
    var topBar = col.querySelector('.top');
    var botBar = col.querySelector('.bot');
    if (item) {
      var topPct = item.p > 0 ? Math.max(8, Math.round((item.p / maxVal) * 100)) : 2;
      var botPct = item.b > 0 ? Math.max(8, Math.round((item.b / maxVal) * 100)) : 0;
      topBar.style.height = topPct + '%';
      topBar.style.opacity = item.p > 0 ? '1' : '0.35';
      botBar.style.height = botPct + '%';
      botBar.style.opacity = item.b > 0 ? '1' : '0.35';
      col.title = 'Allowed: ' + item.p + ', Blocked: ' + item.b;
    } else {
      topBar.style.height = '2px';
      topBar.style.opacity = '0.35';
      botBar.style.height = '0%';
      botBar.style.opacity = '0.35';
      col.title = 'Waiting for traffic...';
    }
  }
}

// Chained Polling Loop
function loadData() {
  if (pollTimeout) clearTimeout(pollTimeout);
  var headers = { 'Connection': 'close' };
  if (adminToken) headers['X-Admin-Token'] = adminToken;

  var controller = new AbortController();
  var timeoutId = setTimeout(function(){ controller.abort(); }, 4000);

  fetch('/stats.json', { headers: headers, signal: controller.signal })
    .then(function(res){
      clearTimeout(timeoutId);
      if (!res.ok) throw new Error('HTTP ' + res.status);
      return res.json();
    })
    .then(function(data){
      $('statusBeacon').classList.remove('offline');
      $('beaconDot').classList.remove('offline');
      $('beaconText').textContent = 'ACTIVE';

      updatePulseChart(data.blocked || 0, data.allowed || 0);

      lastStats = data;
      renderStats(data);
      renderClients();
      renderCustom();
    })
    .catch(function(err){
      $('statusBeacon').classList.add('offline');
      $('beaconDot').classList.add('offline');
      $('beaconText').textContent = 'OFFLINE';
    })
    .finally(function(){
      pollTimeout = setTimeout(loadData, 3000);
    });
}

function renderStats(d) {
  $('chipIp').textContent = d.ip || '192.168.0.x';
  $('chipUptime').textContent = d.uptime || '0d 0h 0m';

  $('kpiBlocked').textContent = (d.blocked || 0).toLocaleString();
  $('kpiAllowed').textContent = (d.allowed || 0).toLocaleString();
  $('kpiDomains').textContent = (d.domains || 0).toLocaleString();

  var total = (d.blocked || 0) + (d.allowed || 0);
  var pct = total > 0 ? ((d.blocked / total) * 100).toFixed(1) : '0.0';
  $('kpiRatio').textContent = pct + '%';
  $('kpiRatioBar').style.width = Math.min(100, parseFloat(pct)) + '%';

  var clients = d.clients || [];
  var online = clients.filter(function(c){ return (c.lastSeenSec || 0) <= 900; });
  $('kpiClients').textContent = online.length;
  $('navClientCount').textContent = clients.length;
  $('navRuleCount').textContent = (d.custom || []).length;

  var freeKb = Math.round((d.heap || 0) / 1024);
  $('heapTxt').textContent = freeKb + ' KB free';
  var heapPct = Math.min(100, Math.round((d.heap / 327680) * 100));
  $('heapBar').style.width = heapPct + '%';

  var rssi = d.rssi || -60;
  $('wifiRssiTxt').textContent = rssi + ' dBm';
  var rssiPct = Math.max(10, Math.min(100, Math.round((rssi + 100) * 1.66)));
  $('wifiRssiBar').style.width = rssiPct + '%';

  if (d.upurl && !$('updateUrlInput').matches(':focus')) $('updateUrlInput').value = d.upurl;
  if (d.upiv && !$('updateIntervalInput').matches(':focus')) $('updateIntervalInput').value = d.upiv;
  if (d.upstat) $('updateStatusTxt').textContent = d.upstat;
}

// Client Table Rendering with Strict Active vs Offline Deduplication
function renderClients() {
  if (!lastStats || !lastStats.clients) return;
  var q = ($('clientSearch').value || '').toLowerCase();
  var clients = lastStats.clients;

  var activeRows = $('clientRows');
  var offlineRows = $('offlineRows');
  var activeHtml = '', offlineHtml = '';
  var activeCount = 0, offlineCount = 0;

  clients.forEach(function(c){
    var ip = c.ip || '';
    var mac = c.mac || '--:--:--:--:--:--';
    var name = c.name || '';
    var match = !q || ip.toLowerCase().includes(q) || mac.toLowerCase().includes(q) || name.toLowerCase().includes(q);
    if (!match) return;

    var isOnline = (c.lastSeenSec || 0) <= 900;
    var tot = (c.blocked || 0) + (c.allowed || 0);
    var pct = tot > 0 ? Math.round((c.blocked / tot) * 100) : 0;

    var nameHtml = '<div class="dev-cell">' +
      '<span class="dev-name">' + (escapeHtml(name) || '<span class="dim">Unnamed Device</span>') + '</span>' +
      '<button class="edit-btn" onclick="openEditNameModal(\'' + ip + '\',\'' + escapeAttr(name) + '\')" title="Edit device alias">' +
      '<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M12 20h9"/><path d="M16.5 3.5a2.12 2.12 0 0 1 3 3L7 19l-4 1 1-4L16.5 3.5z"/></svg>' +
      '</button></div>';

    var ipMacHtml = '<span class="copy-chip mono" onclick="copyText(\'' + ip + '\')" title="Copy IP">' + ip + '</span>';
    var macChipHtml = '<span class="copy-chip mono dim" onclick="copyText(\'' + mac + '\')" title="Copy MAC">' + mac + '</span>';

    if (isOnline) {
      activeCount++;
      var banBtn = '<button class="btn sm ' + (c.banned ? 'danger' : '') + '" onclick="toggleBan(\'' + ip + '\')">' +
        (c.banned ? 'Unban' : 'Ban') + '</button>';
      activeHtml += '<tr>' +
        '<td>' + nameHtml + '</td>' +
        '<td>' + ipMacHtml + '</td>' +
        '<td>' + macChipHtml + '</td>' +
        '<td class="mono crimson">' + (c.blocked || 0) + '</td>' +
        '<td class="mono emerald">' + (c.allowed || 0) + '</td>' +
        '<td><div class="ratio-meter"><div class="ratio-fill" style="width:' + pct + '%"></div></div><span class="mono dim">' + pct + '%</span></td>' +
        '<td style="text-align:right">' + banBtn + '</td>' +
        '</tr>';
    } else {
      offlineCount++;
      var agoTxt = formatAgo(c.lastSeenSec);
      var delBtn = '<div style="display:flex;justify-content:flex-end;gap:6px">' +
        '<button class="btn sm ' + (c.banned ? 'danger' : '') + '" onclick="toggleBan(\'' + ip + '\')">' + (c.banned ? 'Unban' : 'Ban') + '</button>' +
        '<button class="btn sm danger" onclick="deleteClient(\'' + ip + '\')" title="Delete offline device record">' +
        '<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" style="width:12px;height:12px"><polyline points="3 6 5 6 21 6"/><path d="M19 6v14a2 2 0 0 1-2 2H7a2 2 0 0 1-2-2V6m3 0V4a2 2 0 0 1 2-2h4a2 2 0 0 1 2 2v2"/></svg>' +
        '</button></div>';
      offlineHtml += '<tr>' +
        '<td>' + nameHtml + '</td>' +
        '<td>' + ipMacHtml + '</td>' +
        '<td>' + macChipHtml + '</td>' +
        '<td><span class="status-badge offline">' + agoTxt + '</span></td>' +
        '<td class="mono crimson">' + (c.blocked || 0) + '</td>' +
        '<td class="mono emerald">' + (c.allowed || 0) + '</td>' +
        '<td style="text-align:right">' + delBtn + '</td>' +
        '</tr>';
    }
  });

  $('onlineBadge').textContent = activeCount;
  $('offlineBadge').textContent = offlineCount;

  activeRows.innerHTML = activeHtml || '<tr><td colspan="7" class="dim" style="text-align:center;padding:24px">No active devices found</td></tr>';
  offlineRows.innerHTML = offlineHtml || '<tr><td colspan="7" class="dim" style="text-align:center;padding:24px">No offline devices recorded</td></tr>';
}

function formatAgo(sec) {
  if (!sec || sec < 60) return 'Just now';
  var min = Math.floor(sec / 60);
  if (min < 60) return min + 'm ago';
  var hr = Math.floor(min / 60);
  if (hr < 24) return hr + 'h ago';
  var d = Math.floor(hr / 24);
  return d + 'd ago';
}

// Device Rename Modal Handlers
function openEditNameModal(ip, currentName) {
  targetEditIp = ip;
  $('nameModalSub').textContent = ip;
  $('nameModalInput').value = currentName || '';
  $('nameModal').classList.add('open');
  setTimeout(function(){ $('nameModalInput').focus(); }, 100);
}
function closeNameModal() { $('nameModal').classList.remove('open'); targetEditIp = ''; }

function saveDeviceName() {
  if (!targetEditIp) return;
  var name = $('nameModalInput').value.trim();
  postAuth('/setname?ip=' + encodeURIComponent(targetEditIp) + '&name=' + encodeURIComponent(name))
    .then(function(){
      notify('Device alias updated');
      closeNameModal();
      loadData();
    })
    .catch(function(e){ notify('Failed to save alias: ' + e.message, true); });
}

function clearDeviceName() {
  if (!targetEditIp) return;
  postAuth('/setname?ip=' + encodeURIComponent(targetEditIp) + '&name=')
    .then(function(){
      notify('Device alias cleared');
      closeNameModal();
      loadData();
    })
    .catch(function(e){ notify('Failed to clear alias: ' + e.message, true); });
}

function deleteClient(ip) {
  if (!confirm('Purge offline device ' + ip + ' from tracking and memory?')) return;
  postAuth('/delclient?ip=' + encodeURIComponent(ip))
    .then(function(){
      notify('Device removed: ' + ip);
      loadData();
    })
    .catch(function(e){ notify('Delete failed: ' + e.message, true); });
}

function toggleBan(ip) {
  postAuth('/ban?ip=' + encodeURIComponent(ip))
    .then(function(){
      notify('Client ban toggled: ' + ip);
      loadData();
    })
    .catch(function(e){ notify('Ban toggle failed: ' + e.message, true); });
}

// Custom Blacklist Rules
function renderCustom() {
  if (!lastStats || !lastStats.custom) return;
  var q = ($('customSearch').value || '').toLowerCase();
  var custom = lastStats.custom;
  $('customCountBadge').textContent = custom.length;

  var html = '';
  custom.forEach(function(dom){
    if (q && !dom.toLowerCase().includes(q)) return;
    html += '<tr>' +
      '<td class="mono crimson">' + escapeHtml(dom) + '</td>' +
      '<td><span class="status-badge blocked">0.0.0.0</span></td>' +
      '<td style="text-align:right">' +
      '<button class="btn sm danger" onclick="removeCustomDomain(\'' + escapeAttr(dom) + '\')">Remove</button>' +
      '</td></tr>';
  });
  $('customRows').innerHTML = html || '<tr><td colspan="3" class="dim" style="text-align:center;padding:24px">No matching custom domains</td></tr>';
}

function addCustomDomain() {
  var raw = $('customInput').value.trim();
  if (!raw) return;
  var items = raw.split(/[\s,]+/);
  var p = Promise.resolve();
  items.forEach(function(item){
    if (!item) return;
    p = p.then(function(){
      return postAuth('/addblock?d=' + encodeURIComponent(item));
    });
  });
  p.then(function(){
    $('customInput').value = '';
    notify('Added ' + items.length + ' custom domain(s)');
    loadData();
  }).catch(function(e){ notify('Add rule failed: ' + e.message, true); });
}

function quickAdd(dom) {
  $('customInput').value = dom;
  addCustomDomain();
}

function removeCustomDomain(dom) {
  postAuth('/unblock?d=' + encodeURIComponent(dom))
    .then(function(){
      notify('Removed rule: ' + dom);
      loadData();
    })
    .catch(function(e){ notify('Remove failed: ' + e.message, true); });
}

// Flash Blocklist Upload
function handleFileSelected(files) {
  if (!files || !files.length) return;
  var file = files[0];
  if (file.size > 1258291) { notify('File too large (max 1.20 MB)', true); return; }
  if (file.size % 5 !== 0) { notify('Invalid blocklist: size must be multiple of 5 bytes', true); return; }

  $('uploadProgressBox').style.display = 'block';
  $('uploadStatusTxt').textContent = 'Uploading ' + file.name + '...';
  $('uploadBar').style.width = '20%';

  var xhr = new XMLHttpRequest();
  xhr.open('POST', '/upload', true);
  xhr.setRequestHeader('Content-Type', 'application/octet-stream');
  if (adminToken) xhr.setRequestHeader('X-Admin-Token', adminToken);

  xhr.upload.onprogress = function(e){
    if (e.lengthComputable) {
      var pct = Math.round((e.loaded / e.total) * 100);
      $('uploadBar').style.width = pct + '%';
      $('uploadPctTxt').textContent = pct + '%';
    }
  };

  xhr.onload = function(){
    if (xhr.status === 200) {
      notify('Blocklist flashed and atomically swapped!');
      $('uploadStatusTxt').textContent = 'Upload complete!';
      setTimeout(function(){ $('uploadProgressBox').style.display = 'none'; }, 2500);
      loadData();
    } else {
      notify('Upload failed: HTTP ' + xhr.status + ' ' + xhr.responseText, true);
      $('uploadProgressBox').style.display = 'none';
    }
  };
  xhr.onerror = function(){
    notify('Upload network error', true);
    $('uploadProgressBox').style.display = 'none';
  };
  xhr.send(file);
}

// Blocked Activity Log (Tab 4)
function fetchLogs() {
  var headers = { 'Connection': 'close' };
  if (adminToken) headers['X-Admin-Token'] = adminToken;
  fetch('/log.json', { headers: headers })
    .then(function(res){
      if (!res.ok) throw new Error('HTTP ' + res.status);
      return res.json();
    })
    .then(function(logs){
      lastLogs = logs;
      $('navLogCount').textContent = logs.length;
      $('logCountBadge').textContent = logs.length;
      renderLogs();
    })
    .catch(function(err){});
}

function renderLogs() {
  var q = ($('logSearch').value || '').toLowerCase();
  var html = '';
  var count = 0;
  lastLogs.forEach(function(l){
    var dom = l.domain || '';
    var name = l.name || '';
    var ip = l.ip || '';
    if (q && !dom.toLowerCase().includes(q) && !name.toLowerCase().includes(q) && !ip.toLowerCase().includes(q)) return;
    count++;

    var timeStr = '+0s';
    if (l.time > 1700000000) {
      var d = new Date(l.time * 1000);
      timeStr = d.toLocaleTimeString();
    } else {
      timeStr = '+' + l.time + 's';
    }

    var devLabel = (escapeHtml(name) ? ('<strong>' + escapeHtml(name) + '</strong> &bull; ') : '') +
      '<span class="mono dim">' + ip + '</span>';

    var actionBadge = l.action === 'NODATA'
      ? '<span class="status-badge nodata">NODATA</span>'
      : '<span class="status-badge blocked">0.0.0.0</span>';

    html += '<tr>' +
      '<td class="mono dim" style="white-space:nowrap">' + timeStr + '</td>' +
      '<td>' + devLabel + '</td>' +
      '<td class="mono crimson">' + escapeHtml(dom) + '</td>' +
      '<td><span class="status-badge offline mono">' + (l.type || 'A') + '</span></td>' +
      '<td>' + actionBadge + '</td>' +
      '</tr>';
  });

  $('logRows').innerHTML = html || '<tr><td colspan="5" class="dim" style="text-align:center;padding:24px">No blocked queries logged yet</td></tr>';
}

// CSV Exporters
function exportClientsCSV() {
  if (!lastStats || !lastStats.clients) return;
  var rows = [['Name','IP','MAC','Blocked','Allowed','Status','LastSeenSec']];
  lastStats.clients.forEach(function(c){
    rows.push([
      c.name || '',
      c.ip || '',
      c.mac || '',
      c.blocked || 0,
      c.allowed || 0,
      c.banned ? 'Banned' : ((c.lastSeenSec || 0) <= 900 ? 'Active' : 'Offline'),
      c.lastSeenSec || 0
    ]);
  });
  downloadCSV(rows, 'esp32_adblock_clients.csv');
}

function exportLogsCSV() {
  if (!lastLogs || !lastLogs.length) { notify('No logs available to export', true); return; }
  var rows = [['Timestamp','ClientIP','ClientName','MAC','Domain','QueryType','Action']];
  lastLogs.forEach(function(l){
    rows.push([
      l.time || '',
      l.ip || '',
      l.name || '',
      l.mac || '',
      l.domain || '',
      l.type || '',
      l.action || ''
    ]);
  });
  downloadCSV(rows, 'esp32_blocked_log.csv');
}

function downloadCSV(rows, filename) {
  var csv = rows.map(function(r){
    return r.map(function(val){
      var str = String(val).replace(/"/g, '""');
      return '"' + str + '"';
    }).join(',');
  }).join('\r\n');
  var blob = new Blob([csv], { type: 'text/csv;charset=utf-8;' });
  var link = document.createElement('a');
  link.href = URL.createObjectURL(blob);
  link.download = filename;
  link.click();
}

// System Tab Actions
function saveUpdateSettings() {
  var u = $('updateUrlInput').value.trim();
  var h = parseInt($('updateIntervalInput').value) || 24;
  postAuth('/setupdate?u=' + encodeURIComponent(u) + '&h=' + h)
    .then(function(){
      notify('Update schedule configured');
      loadData();
    })
    .catch(function(e){ notify('Schedule update failed: ' + e.message, true); });
}

function triggerManualFetch() {
  postAuth('/fetchnow')
    .then(function(){
      notify('Background blocklist download triggered');
      loadData();
    })
    .catch(function(e){ notify('Fetch failed: ' + e.message, true); });
}

// API Helper
function postAuth(url) {
  var headers = { 'Connection': 'close' };
  if (adminToken) headers['X-Admin-Token'] = adminToken;
  return fetch(url, { method: 'POST', headers: headers })
    .then(function(res){
      if (res.status === 401) {
        openTokenModal();
        throw new Error('Authentication required (Admin Token)');
      }
      if (!res.ok) throw new Error('HTTP ' + res.status);
      return res.text();
    });
}

function copyText(txt) {
  if (!navigator.clipboard) return;
  navigator.clipboard.writeText(txt).then(function(){
    notify('Copied to clipboard: ' + txt);
  });
}

function escapeHtml(s) {
  if (!s) return '';
  return String(s).replace(/&/g,'&amp;').replace(/</g,'&lt;').replace(/>/g,'&gt;').replace(/"/g,'&quot;');
}
function escapeAttr(s) {
  if (!s) return '';
  return String(s).replace(/'/g,'\\&#39;').replace(/"/g,'&quot;');
}

// Boot Initialization
updateAuthBadge();
initPulseChart();
loadData();
</script>
</body>
</html>
)HTML";
