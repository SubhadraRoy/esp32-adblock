# Security Policy & Threat Model

## Supported Versions

Only the current `main` branch running native ESP-IDF v6.x is actively supported with security updates and patches:

| Version / Branch | Supported | Architecture | Runtime Engine |
| :--- | :--- | :--- | :--- |
| `main` (Native ESP-IDF v6.x) | :white_check_mark: | Dual-Core Xtensa LX6 AMP | Zero-Allocation BSD Sockets + FreeRTOS |
| Legacy Arduino-based releases | :x: | Single-Core Arduino Wrapper | Deprecated / Unsupported |

---

## High-Assurance Threat Model & Defense-in-Depth Architecture

ESP32 AdBlock operates as a dedicated network security appliance inside a Local Area Network (LAN). Because it processes untrusted UDP datagrams on port 53 and HTTP REST administrative commands on port 80, it implements a comprehensive 7-tier defensive shield:

```
[Untrusted Network / Inbound Datagrams]
                 │
                 ▼
┌────────────────────────────────────────────────────────┐
│  Tier 1: Socket Buffer Clamping & Sanitization         │  Max 1,472 B MTU, 0-allocation stack safety
└────────────────────────┬───────────────────────────────┘
                         │
                         ▼
┌────────────────────────────────────────────────────────┐
│  Tier 2: Anti-Flood Rate Limiting Shield               │  100 QPS threshold per client IP
│                                                        │  RFC 5625 REFUSED deflection (0 flash hits)
└────────────────────────┬───────────────────────────────┘
                         │
                         ▼
┌────────────────────────────────────────────────────────┐
│  Tier 3: Hardware TRNG Upstream Nonces                 │  esp_random() 16-bit TxID matching
│                                                        │  Protects against Kaminsky cache poisoning
└────────────────────────┬───────────────────────────────┘
                         │
                         ▼
┌────────────────────────────────────────────────────────┐
│  Tier 4: DNS Rebinding Defense Shield                  │  RFC 1918 / RFC 4193 private IP interception
│                                                        │  Blocks intranet pivot attacks (0.0.0.0 sinkhole)
└────────────────────────┬───────────────────────────────┘
                         │
                         ▼
┌────────────────────────────────────────────────────────┐
│  Tier 5: 5-Strike Brute-Force Lockout Defense          │  Tracks failed admin tokens per client IP
│                                                        │  30-second sliding window lockout (HTTP 429)
└────────────────────────┬───────────────────────────────┘
                         │
                         ▼
┌────────────────────────────────────────────────────────┐
│  Tier 6: 64-Iteration Constant-Time Auth Verification  │  constantTimeCompare() eliminates timing leaks
│                                                        │  Enforces non-default X-Admin-Token
└────────────────────────┬───────────────────────────────┘
                         │
                         ▼
┌────────────────────────────────────────────────────────┐
│  Tier 7: Strict CSRF & Origin Validation               │  Host, Origin, and Referer header matching
└────────────────────────────────────────────────────────┘
```

---

## Detailed Shield Specifications

### 1. DNS Rebinding Protection (RFC 1918 / RFC 4193 Sinkhole)
- **Threat:** An external malicious website (e.g. `attacker.com`) sets a very low DNS TTL and later alters its DNS A record to resolve to an internal private IPv4/IPv6 address (such as `192.168.1.1`, `192.168.1.50`, or `10.0.0.1`). When a LAN browser accesses the domain, it bypasses Same-Origin Policy (SOP) to attack intranet routers, IoT devices, or the ESP32 administrative API.
- **Mitigation:**
  - Upstream DNS responses are parsed before being forwarded to the client.
  - If any A record resolves to an RFC 1918 private range (`10.0.0.0/8`, `172.16.0.0/12`, `192.168.0.0/16`), loopback (`127.0.0.0/8`), link-local (`169.254.0.0/16`), or private IPv6 (`fc00::/7`), the response is flagged as a rebind exploit.
  - The record is immediately sanitized and rewritten to `0.0.0.0` (or returned with `NOERROR` / 0 answers), neutralising the intranet pivot vector.
  - All rebinding events are logged to the in-memory circular telemetry buffer with action `REBIND_DEFENSE`.

### 2. Anti-Flood Rate Limiting & DoS Shield
- **Threat:** Malicious LAN devices, malware bots, or amplification attacks blast high-frequency UDP DNS floods (e.g. 5,000+ QPS), exhausting socket buffers, starving FreeRTOS tasks, or thrashing SPI Flash memory.
- **Mitigation:**
  - In-memory rate limiting tracks query frequency on a per-client IP sliding window.
  - Baseline threshold is set to **100 QPS per client IP**.
  - When an IP exceeds 100 QPS:
    1. The query bypasses LittleFS flash search entirely, eliminating disk read amplification.
    2. The engine immediately generates a lightweight, 40-byte standard **RFC 5625 `REFUSED`** datagram (RCODE 5) directly from a static stack template with zero heap allocation.
    3. Normal clients continue to be served without packet drops or buffer congestion.
  - Hardware benchmarks prove zero packet drops across 23,034 queries at up to 5,000 QPS saturation.

### 3. 5-Strike Brute-Force Lockout Defense
- **Threat:** Automated credential stuffing or brute-force dictionary attacks against mutating REST administrative endpoints (`/ban`, `/unban`, `/addblock`, `/delblock`, `/setupdate`, `/setname`).
- **Mitigation:**
  - Every mutating endpoint tracks failed authentication attempts keyed by client IPv4.
  - If a client IP accumulates **5 consecutive invalid `X-Admin-Token` attempts**, the IP is placed in a **30-second lockout state**.
  - During the lockout window, all administrative requests from that IP receive an immediate `HTTP 429 Too Many Requests` response without evaluating token comparisons or accessing flash storage.
  - Successful authentication resets the failure counter to zero.

### 4. 64-Iteration Constant-Time Authentication Verification
- **Threat:** Remote timing side-channel attacks measuring microseconds or CPU clock cycles of string comparison functions (`strcmp`/`memcmp`) to incrementally deduce the secret administrative token character by character.
- **Mitigation:**
  - Token authentication uses a dedicated `constantTimeCompare()` function:
    ```c
    static bool constantTimeCompare(const char* a, const char* b, size_t maxLen = 64) {
        size_t lenA = strlen(a);
        size_t lenB = strlen(b);
        uint8_t diff = (lenA ^ lenB);
        for (size_t i = 0; i < maxLen; ++i) {
            char ca = (i < lenA) ? a[i] : 0;
            char cb = (i < lenB) ? b[i] : 0;
            diff |= (ca ^ cb);
        }
        return diff == 0;
    }
    ```
  - The loop always executes for exactly 64 iterations regardless of the length or content of the input strings, eliminating execution-time variance.
  - Default token lockdown: If `ADMIN_TOKEN` remains configured as `"changeme"`, mutating endpoints refuse execution to prevent deployment with factory defaults.

### 5. Cross-Site Request Forgery (CSRF) & Strict Origin Validation
- **Threat:** A LAN user visits a third-party website with hidden JavaScript or `<img>`/`<form>` tags designed to trigger `POST /ban` or `POST /delblock` on `http://<device-ip>/`.
- **Mitigation:**
  - All mutating endpoints enforce strict HTTP header validation:
    1. `Origin` and `Referer` headers are extracted and parsed.
    2. Requests lacking valid local origin headers or containing external hostnames/IPs are rejected with `HTTP 403 Forbidden`.
    3. Custom administrative requests require the explicit HTTP header `X-Admin-Token`, which cannot be forged across origins by simple HTML forms.

### 6. Upstream Nonce Entropy & Kaminsky Cache Poisoning Defense
- **Threat:** Attackers spoof authoritative DNS responses by guessing transaction IDs (TxID) and UDP source ports.
- **Mitigation:**
  - Upstream query transaction IDs are generated using the ESP32's hardware True Random Number Generator (`esp_random()`), which samples thermal noise from the Wi-Fi/Bluetooth RF ADC.
  - Upstream responses are strictly verified against the randomized 16-bit TxID, socket descriptor, and question section byte sequence. Unmatched replies are discarded immediately.

### 7. Memory Bounds, Zero Allocation, & FreeRTOS Stack Safety
- **Buffer Overflow Protection:**
  - UDP receive buffers are bounded to the 1,472-byte Ethernet MTU.
  - DNS domain name decoders strictly enforce RFC 1035 bounds: label length $\le 63$ octets, total domain length $\le 253$ octets.
  - Compression pointer decoders track recursion depth ($\le 8$) and offset monotonicity to prevent infinite loop pointer traps (`0xC00C` / `0xC0FF`).
- **Zero-Allocation Hot Paths:**
  - The `dnsTask` operates with **0 dynamic heap allocations** (`malloc`, `calloc`, `new`, `free`, `delete`).
  - Web server telemetry responses stream via `JsonChunker` with a fixed 1,400-byte stack buffer, preventing LwIP heap fragmentation.

---

## Automated Security Verification Suite

Every security feature is verified on hardware using the automated Python test suite (`tools/verify_security.py`).

Run the suite against your live ESP32:
```powershell
python tools/verify_security.py --ip <device-ip> --token <your-token>
```

### Automated Security Test Coverage (13/13 Pass):
1. `test_unauthenticated_ban`: Confirms mutating endpoints reject requests without token (`HTTP 401`).
2. `test_invalid_token_ban`: Confirms invalid token rejection.
3. `test_brute_force_lockout`: Confirms 5 consecutive failed tokens trigger `HTTP 429 Too Many Requests`.
4. `test_csrf_rejection`: Confirms external `Origin` (`http://malicious-site.com`) is rejected (`HTTP 403`).
5. `test_constant_time_comparison`: Tests constant-time verification across length variants.
6. `test_anti_flood_rate_limiting`: Blasts 120 queries from single IP; verifies `REFUSED` (RCODE 5) throttling.
7. `test_dns_rebinding_defense`: Injects mock RFC 1918 upstream response; verifies sanitization to `0.0.0.0`.
8. `test_telemetry_schema`: Validates strict JSON schema, uptime, RSSI, and DRAM telemetry metrics.
9. `test_client_rename_api`: Verifies device renaming and connection persistence (`conn=lan` / `conn=wifi`).
10. `test_rule_add_delete_lifecycle`: Tests custom block rule addition and idempotent deletion.
11. `test_malformed_dns_fuzzing`: Sends circular compression pointers (`0xC00C`) and verifies 0 crashes.
12. `test_dual_core_responsiveness`: Verifies HTTP latency $\le 25\text{ ms}$ while DNS is under heavy UDP load.
13. `test_memory_leak_soak`: Verifies zero heap drift over high-frequency query bursts.

---

## Reporting a Vulnerability

If you discover a security vulnerability in ESP32 AdBlock, please do not disclose it publicly in issues or pull requests.

Instead, report it via:
- **GitHub Private Vulnerability Reporting:** Submit an advisory on the project repository.
- **Maintainer Email:** Contact the lead maintainer with:
  1. Detailed description of the vulnerability and attack vector.
  2. Proof-of-concept exploit script or curl command sequence.
  3. ESP32 hardware revision, flash configuration, and firmware commit hash.

We acknowledge receipt within 48 hours and release patches in coordinated advisories.
