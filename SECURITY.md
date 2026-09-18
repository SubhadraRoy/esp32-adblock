# Security Policy & Threat Model

## Supported Versions

Only the current main branch running native ESP-IDF is actively supported with security fixes:

| Version / Branch | Supported |
| :--- | :--- |
| `main` (Pure ESP-IDF v6.x) | :white_check_mark: |
| Legacy Arduino-based releases | :x: |

---

## Threat Model & Security Design

ESP32 AdBlock is designed as an embedded network appliance operating within a private, trusted Local Area Network (LAN).

### 1. Cross-Site Request Forgery (CSRF)
- **Threat:** A user browsing the internet from a LAN client visits a malicious website that issues background POST requests to `http://<device-ip>/ban` or `/addblock`.
- **Mitigation:** The HTTP server validates incoming `Origin` and `Referer` headers. Any request originating from an external domain is rejected with `401 Unauthorized` / `403 Forbidden`. Furthermore, all mutating endpoints require an `X-Admin-Token` header.

### 2. Administrative Token & Timing Attacks
- **Threat:** LAN attackers attempt brute-force token guessing or timing analysis on authentication endpoints.
- **Mitigation:**
  - Token matching is performed via `constantTimeCompare()`, which checks all bytes regardless of length mismatches, eliminating timing side-channels.
  - If `ADMIN_TOKEN` remains the default value `"changeme"`, all mutating endpoints are permanently locked out until changed in firmware.

### 3. DNS Rebinding & Cache Poisoning
- **Threat:** Malicious upstream actors or compromised local devices attempt DNS cache poisoning or ID spoofing.
- **Mitigation:**
  - Upstream query transaction IDs are cryptographically randomized using ESP32's hardware True Random Number Generator (`esp_random()`).
  - Responses from upstream resolvers must match the exact randomized ID, the destination socket, and the original question section bytes.

### 4. Memory Bounds & Heap Safety
- **Buffer Overflow Protection:** Socket packet sizes are clamped to the 1,472-byte Ethernet MTU payload. Hostname parsers strictly enforce label lengths ($\le 63$) and domain lengths ($\le 253$).
- **Zero-Allocation APIs:** Query string parsers and JSON builders use fixed stack buffers rather than heap `malloc`, preventing heap fragmentation and memory exhaustion denial-of-service.

---

## Reporting a Vulnerability

If you discover a security vulnerability, please do not open a public issue.

Instead, please report it privately via GitHub Security Advisories or by emailing the project maintainers with:
1. Description of the vulnerability and attack vector.
2. Proof-of-concept steps or curl commands.
3. Affected hardware or environment configurations.

We will review, verify, and address reported vulnerabilities promptly.

