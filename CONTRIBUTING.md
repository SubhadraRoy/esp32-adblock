# Contributing to ESP32 AdBlock

Thank you for your interest in contributing to ESP32 AdBlock! We welcome pull requests, bug reports, and performance optimizations that uphold our standards for high-assurance, 24/7/365 embedded production environments.

---

## 50-Persona Technical Quality Gates

Every pull request is evaluated against the 50 specialized engineering personas established during the comprehensive technical audit:

```
┌────────────────────────────────────────────────────────────────────────┐
│                      50-Persona Quality Framework                      │
├──────────────────────────┬─────────────────────────────────────────────┤
│ 1. Embedded Systems & C  │ Static stack bounds, 0-allocation hot paths │
│ 2. Network Protocols     │ RFC 1035 / RFC 5625 compliance, RCODE rules │
│ 3. RTOS & Concurrency    │ Dual-core Xtensa AMP task isolation (Core 0/1)│
│ 4. Memory & Silicon      │ Zero heap drift, 0 malloc/free in dnsTask   │
│ 5. Web Architecture      │ 1,400 B JsonChunker streaming, Canvas 2D UI │
│ 6. Cryptography & Sec    │ Constant-time auth (64 iter), CSRF shields  │
│ 7. Hardware & Reliability│ Zero brownout, WDT resets, 24k soak safety  │
└──────────────────────────┴─────────────────────────────────────────────┘
```

### Core Architecture Rules:
1. **Pure ESP-IDF Native APIs Only:**
   - **No Arduino compatibility wrappers.** Do not include `WiFi.h`, `WiFiUDP`, `WebServer.h`, or the Arduino `String` class.
   - Use raw BSD sockets (`lwip/sockets.h`), `esp_wifi`, `esp_http_server`, `esp_http_client`, and POSIX VFS (`fopen`/`fread`).
2. **Dual-Core Xtensa AMP Task Pinning:**
   - Core 0 is reserved strictly for high-throughput networking (`dnsTask`, Wi-Fi stack, LwIP worker).
   - Core 1 handles user-space workloads: HTTP REST API, LittleFS flash transactions, and housekeeping.
3. **Lock Hierarchy & Fine-Grained Locking:**
   - Global metrics are protected by `stateMutex`.
   - LittleFS flash lookups are protected by `blocklistMutex`.
   - **Never** perform socket I/O, flash reads/writes, or FreeRTOS task delays while holding `stateMutex`.
4. **Zero Dynamic Allocation in Hot Paths:**
   - The DNS request path (`dnsTask`) must run with **zero heap allocations** (`malloc`, `calloc`, `strdup`, `new`, `delete`).
   - Query buffers, client structures, and transaction tables must remain statically sized or allocated at boot.
5. **Stack Safety & Static Buffers:**
   - Do not allocate large buffers ($>256\text{ B}$) on task stacks. Use static file-scope buffers or pre-allocated heap structures allocated during initialization.
6. **Streaming JSON Chunking:**
   - Telemetry responses and large JSON payloads must stream via `JsonChunker` with fixed 1,400-byte buffers to avoid allocating monolithic heap strings.

---

## Development & Testing Workflow

### 1. Build Verification
Before submitting changes, ensure the firmware builds cleanly with zero errors and zero warnings:

```powershell
pio run
```

### 2. Hardware Flashing & Monitor
To test your changes on physical hardware:

```powershell
# Flash firmware at 921600 baud
pio run --target upload

# Verify boot logs on serial monitor
pio device monitor --baud 115200
```

### 3. Automated Security & Performance Verification
All PRs must achieve a **100% pass rate (13/13 tests)** on the automated test suite before merging:

```powershell
python tools/verify_security.py --ip <YOUR_ESP32_IP> --token <YOUR_ADMIN_TOKEN>
```

#### Test Suite Matrix:
- `test_unauthenticated_ban`: Rejects missing auth (`HTTP 401`).
- `test_invalid_token_ban`: Rejects invalid tokens (`HTTP 401`).
- `test_brute_force_lockout`: Enforces 5-strike lockout (`HTTP 429`).
- `test_csrf_rejection`: Enforces local Origin/Referer matching (`HTTP 403`).
- `test_constant_time_comparison`: Validates 64-iteration timing safety.
- `test_anti_flood_rate_limiting`: Verifies 100 QPS RFC 5625 `REFUSED` throttling.
- `test_dns_rebinding_defense`: Verifies RFC 1918 / RFC 4193 private IP sinkholing.
- `test_telemetry_schema`: Validates JSON schema, uptime, RSSI, and DRAM metrics.
- `test_client_rename_api`: Validates device renaming and interface persistence.
- `test_rule_add_delete_lifecycle`: Tests custom rule add and idempotent delete.
- `test_malformed_dns_fuzzing`: Verifies resilience against corrupted DNS packets.
- `test_dual_core_responsiveness`: Verifies HTTP latency $\le 25\text{ ms}$ under heavy UDP DNS load.
- `test_memory_leak_soak`: Verifies zero heap drift over high-frequency query bursts.

### 4. Code Formatting & Cleanliness
- Maintain standard C++17 formatting.
- Preserve all existing comments, docstrings, and licensing headers.
- Never hardcode or commit Wi-Fi SSIDs or passwords (`src/wifi_credentials.h` is gitignored).

---

## Submitting Pull Requests

1. Fork the repository and create your feature branch:
   ```bash
   git checkout -b feature/hardware-bloom-filter
   ```
2. Commit your changes with clear, imperative commit messages:
   ```bash
   git commit -m "Optimize DNS label decoder recursion bounds"
   ```
3. Push to your fork:
   ```bash
   git push origin feature/hardware-bloom-filter
   ```
4. Open a Pull Request on GitHub:
   - Provide a concise description of the change and the rationale.
   - Include output logs from `pio run` and `tools/verify_security.py`.
   - Detail any hardware variations tested (e.g., ESP32-WROOM-32, ESP32-WROVER, ESP32-S3).
