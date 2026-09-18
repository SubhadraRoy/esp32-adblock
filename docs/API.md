# REST API Reference

The ESP32 AdBlock device provides an onboard HTTP REST API served on port 80. The API enables automation, integration with Home Assistant or Grafana, and remote administration.

---

## Authentication & Security

All mutating endpoints (`/ban`, `/addblock`, `/unblock`, `/fetchnow`, `/setupdate`, `/upload`) require administrative authentication.

### Authentication Methods
1. **HTTP Header (Recommended):**
   ```http
   X-Admin-Token: your_admin_password
   ```
2. **Query Parameter (Fallback):**
   ```http
   POST /endpoint?t=your_admin_password&...
   ```

### Security Policy:
- **Default Token Lockout:** If `ADMIN_TOKEN` is set to `"changeme"` in firmware, all mutating endpoints return `401 Unauthorized` until changed.
- **CSRF Protection:** The server verifies `Origin` and `Referer` headers against the device IP and `esp32adblock.local`. Cross-origin browser requests from external websites are blocked.
- **Constant-Time Verification:** Token comparison runs in constant-time to resist timing side-channel attacks.

---

## Endpoint Documentation

### 1. System Statistics (`GET /stats.json`)
Retrieves real-time throughput metrics, system telemetry, connected client states, and custom blocked domains.

- **URL:** `/stats.json`
- **Method:** `GET`
- **Auth Required:** Optional. When unauthenticated, client MAC addresses are masked as `--:--:--:--:--:--` for LAN privacy.

#### Example Request:
```bash
curl -s -H "X-Admin-Token: your_token" http://192.168.1.50/stats.json
```

#### Example Response:
```json
{
  "ip": "192.168.1.50",
  "blocked": 1420,
  "allowed": 18950,
  "domains": 215430,
  "rssi": -45,
  "temp": -999.0,
  "heap": 115224,
  "uptime": "14d 6h 32m",
  "upurl": "https://raw.githubusercontent.com/user/repo/main/blocklist.bin",
  "upiv": 24,
  "upstat": "ok: 215430 domains",
  "clients": [
    {
      "ip": "192.168.1.101",
      "mac": "aa:bb:cc:dd:ee:01",
      "blocked": 842,
      "allowed": 9210,
      "banned": false
    },
    {
      "ip": "192.168.1.102",
      "mac": "aa:bb:cc:dd:ee:02",
      "blocked": 12,
      "allowed": 412,
      "banned": false
    }
  ],
  "custom": [
    "ad-tracker.example.com",
    "telemetry.vendor.com"
  ]
}
```

---

### 2. Add Custom Block Domain (`POST /addblock`)
Adds a domain to the custom blacklist. Stored persistently in `/lfs/custom.txt`.

- **URL:** `/addblock?d=<domain>`
- **Method:** `POST`
- **Auth Required:** Yes

#### Parameters:
| Name | Type | Description |
| :--- | :--- | :--- |
| `d` | String | Fully-qualified domain name (letters, numbers, hyphens, dots). Subdomains automatically match. |

#### Example Request:
```bash
curl -X POST -H "X-Admin-Token: your_token" "http://192.168.1.50/addblock?d=ads.example.com"
```
#### Response:
`ok` (HTTP 200)

---

### 3. Remove Custom Block Domain (`POST /unblock`)
Removes a previously added custom domain from the blacklist.

- **URL:** `/unblock?d=<domain>`
- **Method:** `POST`
- **Auth Required:** Yes

#### Example Request:
```bash
curl -X POST -H "X-Admin-Token: your_token" "http://192.168.1.50/unblock?d=ads.example.com"
```
#### Response:
`ok` (HTTP 200)

---

### 4. Toggle Client Ban (`POST /ban`)
Toggles DNS resolution ban for a specific client IP address. Banned clients receive `0.0.0.0` for all DNS queries.

- **URL:** `/ban?ip=<client_ip>`
- **Method:** `POST`
- **Auth Required:** Yes

#### Example Request:
```bash
curl -X POST -H "X-Admin-Token: your_token" "http://192.168.1.50/ban?ip=192.168.1.102"
```
#### Response:
`ok` (HTTP 200)

---

### 5. Binary Blocklist Upload (`POST /upload`)
Uploads a pre-compiled, sorted 40-bit binary blocklist to flash memory.

- **URL:** `/upload`
- **Method:** `POST`
- **Headers:** `Content-Type: application/octet-stream`
- **Auth Required:** Yes

#### Constraints:
- Size must be $>0$ and $\le 1.20\text{ MB}$ ($1,228,800$ bytes).
- Size must be an exact multiple of 5 bytes (`size % 5 == 0`).
- Hashes must be strictly pre-sorted in ascending order.

#### Example Request:
```bash
curl -X POST -H "X-Admin-Token: your_token" \
     --data-binary @blocklist.bin \
     http://192.168.1.50/upload
```
#### Response:
`ok` (HTTP 200)

---

### 6. Configure Auto-Update (`POST /setupdate`)
Configures the remote blocklist auto-update parameters.

- **URL:** `/setupdate?u=<https_url>&h=<hours>`
- **Method:** `POST`
- **Auth Required:** Yes

#### Parameters:
| Name | Type | Constraints | Description |
| :--- | :--- | :--- | :--- |
| `u` | String | Must start with `https://` | Remote binary blocklist URL |
| `h` | Integer | Range: `1` to `720` | Check interval in hours |

#### Example Request:
```bash
curl -X POST -H "X-Admin-Token: your_token" \
     "http://192.168.1.50/setupdate?u=https://example.com/blocklist.bin&h=48"
```
#### Response:
`ok` (HTTP 200)

---

### 7. Trigger Manual Fetch (`POST /fetchnow`)
Asynchronously triggers an immediate background blocklist download using the configured update URL.

- **URL:** `/fetchnow`
- **Method:** `POST`
- **Auth Required:** Yes

#### Example Request:
```bash
curl -X POST -H "X-Admin-Token: your_token" http://192.168.1.50/fetchnow
```
#### Response:
`fetch scheduled` (HTTP 202 Accepted)

