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
- **Progressive Brute-Force Lockout:** Tracks failed administrative authentication attempts per client IP. After 5 consecutive invalid tokens, the client IP is quarantined for 30 seconds, returning `429 Too Many Requests` across all API endpoints.

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
  "rebind": 12,
  "ratelimited": 4,
  "clients": [
    {
      "ip": "192.168.1.101",
      "mac": "aa:bb:cc:dd:ee:01",
      "name": "Living Room TV",
      "blocked": 842,
      "allowed": 9210,
      "banned": false,
      "lastSeenSec": 14
    },
    {
      "ip": "192.168.1.102",
      "mac": "aa:bb:cc:dd:ee:02",
      "name": "Work Laptop",
      "blocked": 12,
      "allowed": 412,
      "banned": false,
      "lastSeenSec": 1420
    }
  ],
  "custom": [
    "ad-tracker.example.com",
    "telemetry.vendor.com"
  ]
}
```

---

### 2. Blocked Activity Log (`GET /log.json`)
Retrieves the onboard circular buffer (up to 64 entries) of recent blocked DNS queries with millisecond resolution timestamps, client IP, friendly device name, and query details.

- **URL:** `/log.json`
- **Method:** `GET`
- **Auth Required:** Optional (client MAC masked as `--:--:--:--:--:--` if unauthenticated)

#### Example Response:
```json
[
  {
    "time": 1726932450,
    "ip": "192.168.1.101",
    "name": "Living Room TV",
    "mac": "aa:bb:cc:dd:ee:01",
    "domain": "ads.samsung.com",
    "type": "A",
    "action": "0.0.0.0",
    "rebind": false
  },
  {
    "time": 1726932445,
    "ip": "192.168.1.103",
    "name": "IoT Gateway",
    "mac": "aa:bb:cc:dd:ee:03",
    "domain": "malicious-rebind.attack.com",
    "type": "A",
    "action": "REBIND_DEFENSE",
    "rebind": true
  },
  {
    "time": 1726932442,
    "ip": "192.168.1.102",
    "name": "Work Laptop",
    "mac": "aa:bb:cc:dd:ee:02",
    "domain": "telemetry.microsoft.com",
    "type": "AAAA",
    "action": "NODATA",
    "rebind": false
  }
]
```

---

### 3. Rename Device (`POST /setname`)
Assigns or clears a custom friendly name for a client IP address. Persisted atomically to `/lfs/names.txt` across reboots.

- **URL:** `/setname?ip=<client_ip>&name=<friendly_name>`
- **Method:** `POST`
- **Auth Required:** Yes

#### Parameters:
| Name | Type | Description |
| :--- | :--- | :--- |
| `ip` | String | Client IPv4 address (e.g. `192.168.1.101`) |
| `name` | String | URL-encoded device alias (e.g. `Smart%20TV`, max 31 chars). Leave empty to clear. |

#### Example Request:
```bash
curl -X POST -H "X-Admin-Token: your_token" "http://192.168.1.50/setname?ip=192.168.1.101&name=Living%20Room%20TV"
```
#### Response:
`ok` (HTTP 200)

---

### 4. Delete Client / Offline Device (`POST /delclient`)
Manually purges a device from the client tracking table and clears any stored alias from `/lfs/names.txt`.

- **URL:** `/delclient?ip=<client_ip>`
- **Method:** `POST`
- **Auth Required:** Yes

#### Example Request:
```bash
curl -X POST -H "X-Admin-Token: your_token" "http://192.168.1.50/delclient?ip=192.168.1.102"
```
#### Response:
`ok` (HTTP 200)

---

### 5. Add Custom Block Domain (`POST /addblock`)
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

### 6. Remove Custom Block Domain (`POST /unblock`)
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

### 7. Toggle Client Ban (`POST /ban`)
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

### 8. Binary Blocklist Upload (`POST /upload`)
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

### 9. Configure Auto-Update (`POST /setupdate`)
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

### 10. Trigger Manual Fetch (`POST /fetchnow`)
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

