# Contributing to ESP32 AdBlock

Thank you for your interest in contributing to ESP32 AdBlock! We welcome pull requests, bug reports, and performance optimizations that keep the project rock-solid for 24/7/365 production use.

---

## Architecture Principles & Guardrails

When modifying the codebase, adhere strictly to these engineering constraints:

1. **Pure ESP-IDF Native APIs Only:**
   - **No Arduino compatibility wrappers.** Do not use `WiFi.h`, `WiFiUDP`, `WebServer.h`, or the Arduino `String` class.
   - Use `esp_wifi`, raw BSD sockets (`lwip/sockets.h`), `esp_http_server`, `esp_http_client`, and POSIX VFS (`fopen`/`fread`).

2. **Strict Lock Discipline:**
   - Always honor the lock hierarchy documented in `src/main.cpp` and `docs/ARCHITECTURE.md`.
   - Never perform socket I/O, flash reads/writes, or delays while holding `stateMutex`.
   - Snap mutable state under lock, then process outside the lock.

3. **Zero Dynamic Allocation in Hot Paths:**
   - The DNS request path (`dnsTask`) must run with **zero heap allocations** (`malloc`/`free` or `new`/`delete`).
   - Query buffers, client structures, and transaction tables must remain statically sized.

4. **Stack Safety:**
   - Never allocate large buffers ($>256\text{ B}$) on task stacks. Use `static` file-scope buffers or pre-allocated structures.

---

## Development & Testing Workflow

### 1. Build Verification
Before opening a pull request, ensure the firmware compiles with zero errors and zero warnings:

```powershell
pio run
```

### 2. Hardware Verification
If you have an ESP32 board connected:
```powershell
# Flash firmware to board
pio run --target upload

# Verify serial boot diagnostics
pio device monitor --baud 115200
```

### 3. Verification Checklist:
- [ ] DNS forward lookups resolve correctly (`nslookup google.com <esp-ip>`).
- [ ] Blocked domains return `0.0.0.0` (`nslookup doubleclick.net <esp-ip>`).
- [ ] IPv6 `AAAA` queries return RFC NODATA (`ANCOUNT=0, NOERROR`).
- [ ] The dashboard at `http://<esp-ip>` loads and updates without socket errors.
- [ ] Free heap remains stable over sustained query bursts.

---

## Submitting Pull Requests

1. Fork the repository and create your feature branch:
   ```bash
   git checkout -b feature/my-optimization
   ```
2. Commit your changes with clear, descriptive commit messages:
   ```bash
   git commit -m "Optimize prefix table binary search bounds"
   ```
3. Push to your branch and open a Pull Request.
4. Provide a description of what was tested on hardware.

