# Router & Network Configuration Guide

This guide explains how to integrate ESP32 AdBlock into your home or office network for network-wide ad blocking, DNS rebinding protection, and client traffic observability.

---

## Step 1: Assign a Static DHCP Reservation

To ensure your ESP32 maintains a stable IP address across router reboots and firmware updates, assign it a fixed DHCP lease:

1. Log into your router's admin portal (typically `http://192.168.1.1` or `http://192.168.0.1`).
2. Navigate to **DHCP Server** -> **Address Reservation** / **Static Lease**.
3. Locate the ESP32 device by its MAC address (visible on the serial monitor or hostname `esp32adblock`).
4. Bind it to an IP address outside your dynamic pool (e.g., `192.168.1.50`).
5. Save settings.

---

## Step 2: Choose Your DNS Deployment Topology

There are two primary ways to configure your router to use ESP32 AdBlock:

```
Topology A: DHCP Option 6 Distribution (STRONGLY RECOMMENDED)
┌──────────────┐          DHCP Lease with DNS=<Your-ESP32-IP>
│    Router    │ ──────────────────────────────────────────┐
└──────────────┘                                           │
                                                           ▼
┌──────────────┐         Direct Port 53 UDP        ┌──────────────┐
│  LAN Client  │ ─────────────────────────────────►│ ESP32 AdBlock│
└──────────────┘                                   └──────────────┘
• Per-client IP visibility in dashboard
• Per-client 100 QPS anti-flood limits
• Accurate LAN vs Wi-Fi classification

─────────────────────────────────────────────────────────────────

Topology B: Router DNS Relay / Proxy Mode
┌──────────────┐      Direct Port 53       ┌──────────────┐
│  LAN Client  │ ────────────────────────► │    Router    │
└──────────────┘                           └──────┬───────┘
                                                  │ Proxied Port 53 (Router IP)
                                                  ▼
                                           ┌──────────────┐
                                           │ ESP32 AdBlock│
                                           └──────────────┘
• All queries appear from the router's IP
• Entire network shares ONE 100 QPS rate limit bucket
```

### Topology A: DHCP Option 6 Distribution (Recommended)
In this mode, the router's DHCP server instructs all LAN clients to query the ESP32 directly.
- **Advantages:**
  - Individual client IPs appear in the dashboard (e.g., `192.168.1.100`, `192.168.1.105`).
  - Independent 100 QPS rate-limiting buckets per device (a single streaming stick won't throttle your workstation).
  - Accurate device classification (Wired LAN vs Wi-Fi).

### Topology B: Router DNS Relay / Proxy Mode
In this mode, clients send queries to the router (`192.168.1.1`), and the router forwards them to the ESP32.
- > [!WARNING]
  > **Rate Limiting Caveat:** Because all queries originate from the router's single IP address, the entire network will share a single 100 QPS anti-flood bucket. If your network has 10+ active devices, aggregate DNS bursts may trigger RFC 5625 `REFUSED` deflections. Use **Topology A (DHCP Option 6)** whenever possible!

---

## Step 3: Router Configuration by Manufacturer

### TP-Link
1. Navigate to **Advanced** -> **Network** -> **DHCP Server**.
2. Set **Primary DNS:** `<Your-ESP32-IP>` (e.g. `192.168.1.50`).
3. Set **Secondary DNS:** Leave **blank** (or set to `0.0.0.0`).
   > [!IMPORTANT]
   > Do NOT enter a public DNS like `8.8.8.8` as secondary DNS! Modern operating systems query all configured DNS servers concurrently. If secondary DNS is configured, ad queries will bypass the ESP32.
4. Click **Save** and reboot the router.

### ASUS (Asuswrt / Asuswrt-Merlin)
1. Navigate to **LAN** -> **DHCP Server**.
2. Under **DNS and WINS Server Setting**, set **DNS Server 1:** `<Your-ESP32-IP>`.
3. Set **Advertise router's IP in addition to user-specified DNS:** **No**.
4. Click **Apply**.

### OpenWrt
1. Navigate to **Network** -> **Interfaces** -> **LAN** -> **DHCP Server** -> **Advanced Settings**.
2. Under **DHCP-Options**, enter option 6:
   ```text
   6,<Your-ESP32-IP>
   ```
3. Click **Save & Apply**.

### pfSense / OPNsense
1. Navigate to **Services** -> **DHCP Server** -> **LAN**.
2. Under **DNS Servers**, enter your ESP32 IP in the **DNS Server 1** field. Leave DNS 2–4 empty.
3. Save and apply changes.

### AVM FRITZ!Box
1. Go to **Home Network** -> **Network** -> **Network Settings**.
2. Under **IP Addresses**, click **IPv4 Configuration**.
3. Under **Local DNS Server**, enter `<Your-ESP32-IP>`.
4. Click **OK**.

---

## Step 4: Mitigating IPv6 DNS Bypasses

If your ISP assigns native IPv6 (`/64` prefix), devices may automatically query the router's IPv6 DNS server via RDNSS (Router Advertisement), bypassing IPv4 ad blocking entirely.

### Solution Options:
1. **Option 1 (Recommended):** In your router's IPv6 LAN settings, disable **IPv6 DNS auto-configuration (RDNSS)** or set IPv6 DNS distribution to **None / Disabled**. LAN devices will continue routing IPv6 payload traffic normally while sending all DNS resolution queries over IPv4 to the ESP32.
2. **Option 2:** If your router mandates an IPv6 DNS entry, configure an unroutable Unique Local Address (ULA) or disable IPv6 on the WAN interface if not required.

---

## Step 5: Configuring Individual Devices (Bypass Router)

If you cannot modify router settings (e.g. university dorm, corporate guest Wi-Fi, or ISP-locked gateway), configure devices individually:

### Windows 11 / 10
1. Open **Settings** -> **Network & internet** -> **Wi-Fi** (or **Ethernet**).
2. Click **Hardware properties**.
3. Next to **DNS server assignment**, click **Edit**.
4. Select **Manual**, toggle **IPv4** to **ON**, and enter:
   - **Preferred DNS:** `<Your-ESP32-IP>` (e.g. `192.168.1.50`).
   - **Alternate DNS:** Leave blank.
5. Click **Save**.

### macOS
1. Open **System Settings** -> **Network**.
2. Select your active Wi-Fi or Ethernet adapter -> click **Details...** -> **DNS**.
3. Under **DNS Servers**, click **+** and add your ESP32 IP.
4. Remove any other DNS servers listed.
5. Click **OK** -> **Apply**.

### iOS / iPadOS
1. Open **Settings** -> **Wi-Fi**.
2. Tap the **(i)** info icon next to your network.
3. Scroll down to **Configure DNS** -> select **Manual**.
4. Remove existing servers, tap **Add Server**, and enter `<Your-ESP32-IP>`.
5. Tap **Save**.

### Android 10+
1. Android uses **Private DNS (DNS-over-TLS)** which bypasses local port 53 DNS servers!
2. Open **Settings** -> **Network & internet** -> **Private DNS**.
3. Select **Off** (or configure your router to block outbound port 853).
4. Go to **Wi-Fi** -> Select your network -> **Edit / Advanced** -> Change IP settings from DHCP to **Static**, and set **DNS 1** to `<Your-ESP32-IP>`.

---

## Step 6: Client Management & Topology Customization

In the ESP32 AdBlock dashboard at `http://<device-ip>/` (or `http://esp32adblock.local`):

### 1. Client Table Observability:
- All active devices communicating with the ESP32 appear in the **Clients** tab with real-time query counts and blocked percentages.

### 2. Custom Device Renaming & Connection Type:
- Click the **Pencil (Rename)** icon next to any client IP in the **Clients** tab.
- Set a friendly name (e.g. `Desktop Workstation` or `Living Room TV`).
- Select the physical **Connection Interface**:
  - **Wired Ethernet (LAN):** For desktop PCs, servers, or consoles connected to switches/cables.
  - **Wireless Wi-Fi:** For smartphones, laptops, and IoT devices connected over 802.11 Wi-Fi.
- Click **Save**. The configuration is persisted to `/lfs/names.txt` across reboots.

---

## Step 7: Verification & Leak Testing

1. **Verify Local DNS Sinkhole:**
   ```powershell
   nslookup doubleclick.net <Your-ESP32-IP>
   ```
   **Expected Output:** Address: `0.0.0.0`.

2. **Verify Normal Internet Forwarding:**
   ```powershell
   nslookup google.com <Your-ESP32-IP>
   ```
   **Expected Output:** Canonical Google IPv4 addresses.

3. **Verify Anti-Flood Throttling:**
   ```powershell
   python tools/verify_security.py --ip <Your-ESP32-IP> --token <Your-Admin-Token>
   ```
   **Expected Output:** All 13 security and performance tests pass.

4. **Verify Live Web Telemetry:**
   Open `http://<Your-ESP32-IP>/` in your browser. Verify the HTML5 Canvas 2D throughput graph is live and counters increment in real-time.
