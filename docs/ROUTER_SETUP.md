# Router & Network Configuration Guide

This guide explains how to point your home router or individual client devices to the ESP32 AdBlock DNS sinkhole.

---

## Step 1: Assign a Static DHCP Reservation
To prevent your ESP32's IP address from changing when the router reboots, assign it a fixed DHCP lease.

1. Log into your router's admin portal (e.g., `http://192.168.1.1` or `http://192.168.0.1`).
2. Navigate to **DHCP Server** -> **Address Reservation** / **Static Lease**.
3. Locate the ESP32 device by its MAC address (e.g. found on serial monitor or hostname `esp32adblock`).
4. Bind it to an IP address outside your dynamic pool (e.g., `192.168.1.50`).
5. Save settings.

---

## Step 2: Configure Router-Wide DNS
Setting the DNS server on your router automatically protects every connected device (smart TVs, consoles, smartphones, PCs, smart speakers) without manual setup per device.

### Popular Router Interfaces:

#### TP-Link:
1. Go to **Advanced** -> **Network** -> **DHCP Server**.
2. Set **Primary DNS:** `<Your-ESP32-IP>` (e.g. `192.168.1.50`).
3. Set **Secondary DNS:** Leave **blank** (or `0.0.0.0`).
   > [!IMPORTANT]
   > If you set an external secondary DNS like `8.8.8.8`, devices will query both servers concurrently and ads will leak through!
4. Click **Save** and reboot the router.

#### ASUS (Asuswrt / Merlin):
1. Go to **LAN** -> **DHCP Server**.
2. Under **DNS and WINS Server Setting**, set **DNS Server 1:** `<Your-ESP32-IP>`.
3. Ensure **Advertise router's IP in addition to user-specified DNS** is set to **No**.
4. Click **Apply**.

#### OpenWrt:
1. Go to **Network** -> **Interfaces** -> **LAN** -> **DHCP Server** -> **Advanced Settings**.
2. Under **DHCP-Options**, enter option 6:
   ```text
   6,<Your-ESP32-IP>
   ```
3. Click **Save & Apply**.

#### pfSense / OPNsense:
1. Go to **Services** -> **DHCP Server** -> **LAN**.
2. Under **DNS Servers**, enter your ESP32 IP in the **DNS 1** field. Leave DNS 2–4 empty.
3. Save and apply changes.

---

## Step 3: Handling IPv6 DNS Bypasses
If your ISP provides native IPv6, devices will automatically query the router's IPv6 DNS server, bypassing your IPv4 ESP32 ad-blocker!

To prevent IPv6 ad leaks, choose one of the following:
1. **Option A (Recommended):** In your router's IPv6 LAN settings, disable **IPv6 DNS auto-configuration (RDNSS)** or set IPv6 DNS distribution to **Off/None**. Devices will continue routing IPv6 internet traffic normally while sending all DNS queries over IPv4 to the ESP32.
2. **Option B:** If your router requires an IPv6 DNS server address, enter an unroutable ULA address or disable IPv6 entirely if your network does not require it.

---

## Step 4: Configuring Individual Devices (Optional)

### Windows 11 / 10:
1. Open **Settings** -> **Network & internet** -> **Wi-Fi** (or **Ethernet**).
2. Click **Hardware properties**.
3. Next to **DNS server assignment**, click **Edit**.
4. Select **Manual**, toggle **IPv4** on, and enter:
   - **Preferred DNS:** `<Your-ESP32-IP>` (e.g. `192.168.1.50`).
   - **Alternate DNS:** Leave blank.
5. Click **Save**.

### macOS:
1. Open **System Settings** -> **Network**.
2. Select your active Wi-Fi or Ethernet connection, then click **Details...** -> **DNS**.
3. Under **DNS Servers**, click **+** and add your ESP32 IP.
4. Remove any existing DNS servers listed.
5. Click **OK** and **Apply**.

### iOS (iPhone / iPad):
1. Open **Settings** -> **Wi-Fi**.
2. Tap the **(i)** icon next to your connected Wi-Fi network.
3. Scroll down and tap **Configure DNS**.
4. Select **Manual**, delete any existing servers, and tap **Add Server**.
5. Enter `<Your-ESP32-IP>` and tap **Save**.

### Android (10+):
1. Note: Android's **Private DNS (DoT)** feature bypasses local port 53 DNS!
2. Open **Settings** -> **Network & internet** -> **Private DNS**.
3. Set **Private DNS** to **Off** (or **Automatic**, provided your router does not broadcast an external DoT server).
4. Go to **Wi-Fi** -> Your connected network -> **Advanced** -> Change **IP settings** to **Static**, and set **DNS 1** to your ESP32 IP.

---

## Step 5: Verification & Leak Testing

1. **Verify Local DNS Sinkhole:**
   ```powershell
   nslookup doubleclick.net
   ```
   **Expected Response:** `0.0.0.0` from your ESP32 IP.

2. **Verify Normal Internet Forwarding:**
   ```powershell
   nslookup google.com
   ```
   **Expected Response:** Real Google IP addresses.

3. **Check Live Dashboard:**
   Open `http://esp32adblock.local` (or `http://<Your-ESP32-IP>`). You will see your computer's IP address appear in the **Clients** table, with the blocked/allowed counters incrementing in real-time.

