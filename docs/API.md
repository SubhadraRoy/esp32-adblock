# REST API Reference

The ESP32 AdBlock device provides an onboard HTTP REST API served on port 80. The API enables automation, integration with Home Assistant or Grafana, and remote administration.

---

## Authentication & Security

All mutating endpoints (`/ban`, `/addblock`, `/unblock`, `/fetchnow`, `/setupdate`, `/upload`, `/setname`, `/delclient`) require administrative authentication.

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
- **CSRF Host Protection:** The server strictly verifies `Origin` and `Referer` headers against the device IP and `esp32adblock.local`. Cross-origin browser requests from external websites are blocked.
- **Constant-Time Verification:** Token comparison runs in a fixed 64-iteration loop (`constantTimeCompare`) to resist timing side-channel attacks.
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
curl -s -H "X-Admin-Token: your_token" http://<device-ip>/stats.json
```

#### Example Response:
```json
{
  "ip": "192.168.1.50",
  "blocked": 129,
  "allowed": 662,
  "domains": 215430,
  "rssi": -36,
  "temp": -999.0,
  "heap": 105664,
  "min_heap": 82612,
  "largest_heap_block": 90112,
  "rst_reason": 1,
  "cpu_mhz": 240,
  "cores": 2,
  "core0": "DNS Engine (UDP :53)",
  "core1": "Web Server & Maintenance",
  "lan_count": 1,
  "wifi_count": 2,
  "uptime": "0d 0h 20m",
  "upurl": "https://raw.githubusercontent.com/user/repo/main/blocklist.bin",
  "upiv": 24,
  "upstat": "never",
  "rebind": 3,
  "ratelimited": 977,
  "clients": [
    {
      "ip": "192.168.1.100",
      "mac": "aa:bb:cc:dd:ee:01",
      "name": "Desktop Workstation",
      "conn": "LAN",
      "blocked": 128,
      "allowed": 630,
      "banned": false,
      "lastSeenSec": 2
    },
    {
      "ip": "192.168.1.105",
      "mac": "aa:bb:cc:dd:ee:02",
      "name": "Living Room TV",
      "conn": "WIFI",
      "blocked": 1,
      "allowed": 32,
      "banned": false,
      "lastSeenSec": 15
    }
  ],
  "custom": [
    "fake-store-checkout.shop"
  ]
}
```

---

### 2. Blocked Activity Log (`GET /log.json`)
Retrieves the onboard circular buffer (up to 64 entries) of recent blocked DNS queries with timestamps, client IP, friendly device name, and query details.

- **URL:** `/log.json`
- **Method:** `GET`
- **Auth Required:** Optional (client MAC masked as `--:--:--:--:--:--` if unauthenticated)

#### Example Response:
```json
[
  {
    "time": 1726932450,
    "ip": "192.168.1.105",
    "name": "Living Room TV",
    "mac": "aa:bb:cc:dd:ee:02",
    "domain": "graph.facebook.com",
    "type": "A",
    "action": "0.0.0.0",
    "rebind": false
  },
  {
    "time": 1726932445,
    "ip": "192.168.1.100",
    "name": "Desktop Workstation",
    "mac": "aa:bb:cc:dd:ee:01",
    "domain": "192.168.1.1.nip.io",
    "type": "A",
    "action": "REBIND_DEFENSE",
    "rebind": true
  },
  {
    "time": 1726932442,
    "ip": "192.168.1.100",
    "name": "Desktop Workstation",
    "mac": "aa:bb:cc:dd:ee:01",
    "domain": "doubleclick.net",
    "type": "AAAA",
    "action": "NODATA",
    "rebind": false
  }
]
```

---

### 3. Rename Device & Set Interface (`POST /setname`)
Assigns or clears a custom friendly name and physical interface classification for a client IP address. Persisted atomically to `/lfs/names.txt` across reboots.

- **URL:** `/setname?ip=<client_ip>&name=<friendly_name>&conn=<interface>`
- **Method:** `POST`
- **Auth Required:** Yes

#### Parameters:
| Name | Type | Description |
| :--- | :--- | :--- |
| `ip` | String | Client IPv4 address (e.g. `192.168.1.100`) |
| `name` | String | URL-encoded device alias (max 31 chars). Leave empty to clear alias. |
| `conn` | String | Connection interface: `LAN` (Wired Ethernet) or `WIFI` (Wireless). |

#### Example Request:
```bash
curl -X POST -H "X-Admin-Token: your_token" "http://<device-ip>/setname?ip=192.168.1.100&name=Desktop%20Workstation&conn=LAN"
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
curl -X POST -H "X-Admin-Token: your_token" "http://<device-ip>/delclient?ip=192.168.1.199"
```
#### Response:
`ok` (HTTP 200)

---

### 5. Add Custom Block Domain (`POST /addblock`)
Adds a domain to the custom blacklist. Stored persistently in `/lfs/custom.txt`.

- **URL:** `/addblock?d=<domain>`
- **Method:** `POST`
- **Auth Required:** Yes

#### Example Request:
```bash
curl -X POST -H "X-Admin-Token: your_token" "http://<device-ip>/addblock?d=ads.example.com"
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
curl -X POST -H "X-Admin-Token: your_token" "http://<device-ip>/unblock?d=ads.example.com"
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
curl -X POST -H "X-Admin-Token: your_token" "http://<device-ip>/ban?ip=192.168.1.105"
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
     http://<device-ip>/upload
```
#### Response:
`ok` (HTTP 200)

---

### 9. Configure Auto-Update (`POST /setupdate`)
Configures remote blocklist auto-update parameters with exponential backoff on fetch failures.

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
     "http://<device-ip>/setupdate?u=https://example.com/blocklist.bin&h=48"
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
curl -X POST -H "X-Admin-Token: your_token" http://<device-ip>/fetchnow
```
#### Response:
`fetch scheduled` (HTTP 202 Accepted)
