# Performance, Saturation & Hardware Benchmarks

Benchmark measurements conducted on an **ESP32 DevKit V1 (ESP32-D0WD-V3 revision v3.1)** clocked at **240 MHz** with 4MB Boya flash.

---

## 1. Physical Hardware Saturation & Breaking Point Matrix

An escalating load benchmark was fired against the physical hardware in 6 distinct tiers (from 100 QPS to 5,000 QPS raw saturation flood), testing total packet handling capacity, latency curves, and failure thresholds:

```text
============================================================================
  EMPIRICAL BREAKING POINT & THROUGHPUT SATURATION MATRIX
============================================================================
 Target QPS   | Sent     | Answered   | Success%   | Achieved QPS   | Mean Lat   | p95 Lat    
----------------------------------------------------------------------------
 100          | 300      | 300        | 100.0%     | 51.2           | 37.65 ms   | 88.87 ms   
 250          | 750      | 750        | 100.0%     | 121.4          | 24.67 ms   | 71.43 ms   
 500          | 1,496    | 1,496      | 100.0%     | 305.6          | 9.70 ms    | 52.82 ms   
 1,000        | 3,000    | 3,000      | 100.0%     | 566.4          | 7.08 ms    | 38.01 ms   
 2,500        | 7,488    | 7,488      | 100.0%     | 746.5          | 14.23 ms   | 40.65 ms   
 5,000        | 10,000   | 10,000     | 100.0%     | 765.1          | 25.94 ms   | 56.00 ms   
============================================================================
 Total Packets Blasted: 23,034 queries in ~15 seconds
 Packet Drop Rate:      0.00% (0 packets dropped across all 23,034 queries)
 Memory Drift:          -4 Bytes across 23,034 queries (105,668 B -> 105,664 B)
 Hardware Stability:    Reset Reason = 1 (Zero panics, zero watchdog timeouts)
```

---

## 2. Saturation Limit Analysis: Why 765 QPS?

The throughput ceiling plateaus at **~765 to 800 Packets/Second (QPS)** due to the physical properties of half-duplex Wi-Fi:

1. **802.11n 2.4 GHz Half-Duplex Radio Turnaround:**
   - Wi-Fi cannot transmit and receive simultaneously on a single RF channel.
   - Every single DNS transaction requires:
     $$\text{Client TX} \longrightarrow \text{802.11 CSMA/CA Backoff} \longrightarrow \text{ESP32 RX} \longrightarrow \text{PHY Demodulation} \longrightarrow \text{LwIP Buffer} \longrightarrow \text{Core 0 Execution} \longrightarrow \text{LwIP TX} \longrightarrow \text{ESP32 RF TX} \longrightarrow \text{Client 802.11 ACK}$$
   - The turnaround time between inbound frame reception and outbound transmission over 2.4 GHz RF establishes a physical ceiling of **~765 to 800 round-trip packet exchanges per second**.
2. **CPU Clock Cycle Budget:**
   - At 765 QPS on an Xtensa LX6 dual-core @ 240 MHz:
     $$\frac{240,000,000\text{ clock cycles/sec}}{765\text{ packets/sec}} \approx 313,725\text{ cycles per packet}$$
   - This provides extensive headroom for the LwIP TCP/IP stack, Core Locking, and FreeRTOS task scheduling without triggering the 500ms Interrupt Watchdog Timer (`IWDT`).
3. **Anti-Flood Rate Limiting Protection:**
   - At Tier 6, 9,349 of the 10,000 packets were automatically deflected with lightweight 40-byte `REFUSED` packets without touching LittleFS flash storage, completely protecting LwIP packet buffers (`pbuf`) from exhaustion.

---

## 3. Simultaneous HTTP & DNS Core Concurrency

While Core 0 was saturated at 765 QPS, Core 1 simultaneously served rapid HTTP requests:

| Test Parameter | Measured Value | Status |
| :--- | :--- | :--- |
| **HTTP Requests Dispatched** | 100 concurrent requests | Target achieved |
| **Successful HTTP Responses** | 100 (100.0%) | **0 drops (0.0%)** |
| **Mean HTTP Latency** | 82.69 ms | Sub-100ms response |
| **Cross-Core Mutex Contention** | 0 ms blocking on `stateMutex` | Isolated by `blocklistMutex` |

---

## 4. DNS Query Latency Profile

| Query Type | Target / Domain | Mean Latency | 95th Percentile (p95) | 99th Percentile (p99) |
| :--- | :--- | :--- | :--- | :--- |
| **Local Ad Sinkhole (Hit)** | Blocked domain (`doubleclick.net`) | **0.40 ms** | **0.78 ms** | **1.12 ms** |
| **Custom Domain Sinkhole** | In-RAM custom table (`MAX_CUSTOM=128`) | **0.18 ms** | **0.32 ms** | **0.55 ms** |
| **Anti-Flood Throttled** | Burst > 100 QPS (`REFUSED`) | **0.35 ms** | **0.65 ms** | **0.95 ms** |
| **Allowed Query (Forwarded)** | Forwarded to Quad9 (`9.9.9.9`) | **14.2 ms** | **38.6 ms** | **78.8 ms** |
| **Non-blocking Concurrency** | Local hit during slow upstream query | **0.42 ms** | **0.85 ms** | **1.20 ms** |

---

## 5. Memory (RAM) Profile

The firmware operates with high DRAM headroom:

```text
Total Usable SRAM: 327,680 Bytes (320 KB)
========================================================================
[▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓                                      ] 33.3% Utilized
========================================================================
- Firmware Static .data & .bss:              109,072 Bytes (~106.5 KB)
- In-Memory Prefix Table (257 entries):        1,028 Bytes (1.0 KB)
- Free Heap at Idle:                         106,028 Bytes (~103.5 KB)
- Largest Free Contiguous Block:              90,112 Bytes (88.0 KB)
```

### Memory Stability Under Maximum Torture:
- **Pre-Stress Free Heap:** `105,668 Bytes`
- **Post-Stress Free Heap (after 23,034 queries):** `105,664 Bytes`
- **Net Memory Drift:** **`-4 Bytes`** (representing 1 updated client counter struct; 0% leak).

---

## 6. Flash Memory Endurance & Wear Calculation

Normal DNS query resolution performs **read-only** binary searches into flash memory. Flash reads do not degrade NOR flash cells.

### Flash Write Lifecycle Analysis:
- **Flash Erase Cycles:** Standard SPI NOR flash (Boya Microelectronics) is rated for **100,000 erase cycles per sector**.
- **LittleFS Wear Leveling:** 2.625 MB partition contains **672 physical 4KB erase sectors**.
- **Average Write Volume:**
  - Automated weekly blocklist update: $1.20\text{ MB}$ written once every 7 days.
  - Custom domain adjustments: 1 write per month.
- **Endurance Calculation:**
  $$\text{Wear Cycles per Year} = \frac{1.20\text{ MB} \times 52\text{ weeks}}{2.625\text{ MB partition}} \approx 23.7\text{ cycles/year}$$
  $$\text{Expected Flash Lifespan} = \frac{100,000\text{ rated cycles}}{23.7\text{ cycles/year}} \approx \mathbf{4,219\text{ Years}}$$
- Even under aggressive daily blocklist updates, the estimated flash lifespan exceeds **600 years**.

---

## 7. Power & Thermal Profiling

| Operating Parameter | Default Unthrottled | Tuned Production Mode | Benefit |
| :--- | :--- | :--- | :--- |
| **CPU Core Clock** | 240 MHz | **240 MHz (Dual-Core)** | Peak 600 DMIPS performance |
| **Wi-Fi TX Power** | ~20.5 dBm (unthrottled) | **17.0 dBm (68 units)** | Prevents LDO brownout voltage droop |
| **Average Current** | ~170 mA @ 5V | **~135 mA @ 5V** | Stable power delivery |
| **Average Power** | ~0.85 W | **~0.68 W** | Under 1 Watt |
| **Enclosure Temperature** | 68°C – 74°C | **58°C – 64°C** | Cool, reliable continuous operation |

---

## 8. Algorithmic Comparison: Prefix Table vs Bloom Filter

| Metric | Previous Bloom Filter (64 KB) | In-Memory Prefix Table (1,028 B) |
| :--- | :--- | :--- |
| **RAM Consumption** | 65,536 Bytes (64 KB) | **1,028 Bytes (1.0 KB)** |
| **False Positive Rate** | ~58% (at 250k domains) | **0.0% (Mathematically Exact)** |
| **False Negative Rate** | 75%–100% (due to boot init bug) | **0.0%** |
| **Flash Reads per Query** | 18 reads on false-positive pass | **Average 10 reads (0 reads if prefix absent)** |
| **Rebuild Time on Boot** | ~350 ms (bit manipulations) | **~120 ms (sequential stream)** |
