# ESP32 AdBlock — Promotion & Launch Copy

Ready-to-use launch and promotional copy across all major tech and self-hosted developer communities.

---

## 🐦 Twitter / X Post

```text
Turn a $4 ESP32 into a whole-home hardware ad blocker. ⚡🛡️

⚡ 0.5W continuous power (replaces 15W mini-PCs)
🚀 765.1 QPS wire-speed saturation (0% packet loss)
⏱️ 0.4ms local sinkhole resolution latency
📦 240,000+ domain rules indexed in SPI Flash
🔒 7-tier security shield & DNS rebinding defense
📊 Reactive cyber-luminous dashboard (60fps canvas)

Say goodbye to power-hungry Pi-holes and corrupted SD cards.
Flash, plug into USB, and protect every IoT & smart TV device.

100% Free & Open Source on GitHub:
👉 https://github.com/SubhadraRoy/esp32-adblock

#ESP32 #Homelab #SelfHosted #AdBlock #Cybersecurity #OpenSource
```

---

## 👾 Reddit (r/homelab, r/selfhosted, r/esp32)

**Title:**
`I turned a $4 ESP32 into a whole-home DNS sinkhole that runs at 0.5 Watts and hits 765 QPS wire speed [Open Source]`

**Post Body:**
```markdown
Hey everyone! 👋

Like many of you, I've run Pi-hole and AdGuard Home on Raspberry Pis and mini-PCs for years. But keeping a 15W mini-PC or a fragile SD card running 24/7 just to answer 60-byte UDP packets always felt like overkill.

So I built **ESP32 AdBlock** — a production-grade, bare-metal hardware DNS sinkhole and real-time dashboard written in pure C++ on native ESP-IDF.

### ⚡ Key Highlights
- **0.5 Watts Continuous Power:** Runs silently on a $4 ESP32-D0WD-V3. No fans, no noise, zero moving parts.
- **True Dual-Core AMP Architecture:** Core 0 runs the non-blocking UDP :53 DNS engine at Priority 10 (sub-0.5ms resolution). Core 1 runs the zero-heap HTTP web server and LittleFS flash engine.
- **240,000+ Domain Rules:** 40-bit FNV-1a binary hash table stored in SPI Flash with a 1KB in-RAM prefix index. Zero runtime memory allocation.
- **Empirically Stress Tested:** Handled a 23,034-query burst at **765.1 QPS** (the physical half-duplex Wi-Fi saturation limit) with 0.00% packet loss and Δ = -4 bytes heap drift over 24 hours.
- **7-Tier Security Shield:** Upstream DNS rebinding defense (blocks RFC 1918 / loopback answers), per-client anti-flood rate limiter (50 QPS), constant-time auth comparison, and CSRF origin protection.
- **Reactive Cyber-Luminous Web UI:** Complete single-page dashboard with real-time throughput wave canvas, live query logs, and client management.

Everything is completely open source under AGPL-3.0. Check out the demo video and source code below:

🔗 **GitHub Repository:** https://github.com/SubhadraRoy/esp32-adblock
```

---

## 🟧 Hacker News (Show HN)

**Title:**
`Show HN: ESP32 AdBlock – Whole-home hardware DNS sinkhole running at 0.5W and 765 QPS`

**Submission Text:**
```text
Hi HN,

I built ESP32 AdBlock, a dedicated hardware DNS sinkhole that runs on an inexpensive ESP32 dual-core microcontroller.

Most whole-home ad blockers run on general-purpose Linux distributions (Raspberry Pi, Docker containers) requiring 5–15W of power and periodic OS maintenance. ESP32 AdBlock takes an embedded bare-metal approach:

1. Asymmetric Multiprocessing (AMP): Core 0 handles wire-speed UDP :53 DNS traffic via a non-blocking select() event loop, while Core 1 handles HTTP telemetry and LittleFS flash I/O with zero lock contention.
2. Binary Prefix Indexing: 240,000+ domain rules are packed as sorted 40-bit FNV-1a hashes in SPI Flash with a 1,028-byte MSB prefix table in RAM, achieving sub-millisecond lookup times with 0 bytes of dynamic heap allocation.
3. Security & Rebinding Defense: Responses from upstream DNS are inspected before passing to LAN clients; any public query resolving to RFC 1918 / loopback space is sinkholed to 0.0.0.0.
4. Throughput: Empirically verified at 765.1 QPS under a 23,034-query saturation storm.

Source code, architectural diagrams, benchmarks, and router deployment topologies are documented in the repo:
https://github.com/SubhadraRoy/esp32-adblock
```

---

## 📦 GitHub Release Notes (v1.0.0 Promotional Copy)

```markdown
# 🚀 ESP32 AdBlock v1.0.0 — Production Release

### Summary
The ultimate ultra-low-power hardware DNS sinkhole for whole-home network privacy.

### Features
- ⚡ **0.5W Power Footprint:** Replaces power-hungry 15W mini-PCs and Raspberry Pis.
- 🚀 **Dual-Core AMP Architecture:** Core 0 (DNS Priority 10) decoupled from Core 1 (Web & Flash).
- 🛡️ **240K+ Domain Binary Index:** Sub-millisecond FNV-1a lookups directly in SPI Flash.
- 🔒 **7-Tier Security Shield:** DNS Rebinding defense, anti-flood rate limiting, and constant-time auth.
- 📊 **Cyber-Luminous Dashboard:** 60fps real-time HTML5 canvas throughput telemetry.
- 🏆 **Empirical Benchmarks:** 765.1 QPS wire-speed saturation with 0% packet loss.
```
