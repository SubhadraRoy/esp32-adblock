import urllib.request
import urllib.error
import json
import socket
import struct
import time

HOST = '192.168.0.115'
TOKEN = 'admin123'
results = []

def record(name, passed, detail=''):
    results.append((name, passed, detail))
    status = 'PASS' if passed else 'FAIL'
    print(f'[{status}] {name}' + (f' -- {detail}' if detail else ''))

# Helper for DNS
def dns_query(domain, qtype=1, timeout=3.0):
    pkt = struct.pack('!HHHHHH', 0x2468, 0x0100, 1, 0, 0, 0)
    for part in domain.split('.'):
        pkt += bytes([len(part)]) + part.encode('ascii')
    pkt += b'\x00' + struct.pack('!HH', qtype, 1)
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    s.settimeout(timeout)
    s.sendto(pkt, (HOST, 53))
    data, _ = s.recvfrom(2048)
    txid, flags, qd, an, ns, ar = struct.unpack('!HHHHHH', data[:12])
    return flags & 0x0F, an, data

# Wait for ESP32 to be reachable
print("Connecting to ESP32...")
for i in range(10):
    try:
        with urllib.request.urlopen(f'http://{HOST}/', timeout=2) as r:
            if r.status == 200:
                print(f"ESP32 is online! (attempt {i+1})")
                break
    except Exception:
        time.sleep(1)

# 1. HTTP Root Dashboard & Security Hardening UI
try:
    with urllib.request.urlopen(f'http://{HOST}/', timeout=5) as r:
        html = r.read().decode('utf-8')
        t5 = ('tab-logs' in html and 
              'Active Clients (Online)' in html and 
              'Offline Devices' in html and 
              'nameModal' in html)
        sec_ui = ('DNS Rebinding Shield' in html and
                  'Anti-Flood Limiter' in html and
                  'Brute-Force Shield' in html and
                  'Threat Presets' in html and
                  'Fake Online Shops' in html)
        record('1. Dashboard UI & Security Hardening Center', t5 and sec_ui, f'Size: {len(html)} bytes, Hardening Center: Present')
except Exception as e:
    record('1. Dashboard UI & Security Hardening Center', False, str(e))

# 2. Stats API & Telemetry (including rebind & ratelimited)
try:
    req = urllib.request.Request(f'http://{HOST}/stats.json', headers={'X-Admin-Token': TOKEN, 'Connection': 'close'})
    with urllib.request.urlopen(req, timeout=5) as r:
        stats = json.loads(r.read().decode('utf-8'))
        has_fields = all(k in stats for k in ('ip', 'blocked', 'allowed', 'heap', 'clients', 'rebind', 'ratelimited'))
        h_val = stats.get('heap')
        c_cnt = len(stats.get('clients', []))
        rebind_val = stats.get('rebind', 0)
        rl_val = stats.get('ratelimited', 0)
        record('2. Stats API & Telemetry (/stats.json)', has_fields, f'Heap: {h_val} B, Clients: {c_cnt}, Rebind: {rebind_val}, RateLimited: {rl_val}')
except Exception as e:
    record('2. Stats API & Telemetry (/stats.json)', False, str(e))

# 3. Add Custom Domain
try:
    req = urllib.request.Request(f'http://{HOST}/addblock?d=torture-test-ad.org', data=b'', headers={'X-Admin-Token': TOKEN, 'Connection': 'close'})
    with urllib.request.urlopen(req, timeout=5) as r:
        body = r.read().decode('utf-8')
        record('3. Add Custom Block Domain', r.status == 200 and body == 'ok', f'HTTP {r.status} {body}')
except Exception as e:
    record('3. Add Custom Block Domain', False, str(e))

# 4. Upstream Allowed Query
try:
    rcode, an, data = dns_query('google.com', 1)
    record('4. DNS Allowed Query (google.com)', rcode == 0 and an > 0, f'RCODE={rcode}, ANCOUNT={an}')
except Exception as e:
    record('4. DNS Allowed Query (google.com)', False, str(e))

# 5. Blocked Query Type A -> 0.0.0.0
try:
    rcode, an, data = dns_query('torture-test-ad.org', 1)
    ans_0000 = b'\x00\x00\x00\x00' in data[-4:]
    record('5. DNS Blocked Query (Type A -> 0.0.0.0)', rcode == 0 and an == 1 and ans_0000, f'RCODE={rcode}, ANCOUNT={an}')
except Exception as e:
    record('5. DNS Blocked Query (Type A -> 0.0.0.0)', False, str(e))

# 6. Blocked Query Type AAAA -> NODATA
try:
    rcode, an, data = dns_query('torture-test-ad.org', 28)
    record('6. DNS Blocked Query (Type AAAA -> NODATA)', rcode == 0 and an == 0, f'RCODE={rcode}, ANCOUNT={an}')
except Exception as e:
    record('6. DNS Blocked Query (Type AAAA -> NODATA)', False, str(e))

# 7. DNS Rebinding Interception
# 192.168.1.1.nip.io resolves upstream (Quad9 9.9.9.9) to RFC 1918 Private IP 192.168.1.1
# ESP32 Rebind Shield intercepts upstream answer, sinkholes to 0.0.0.0, and increments rebind counter.
try:
    rcode, an, data = dns_query('192.168.1.1.nip.io', 1)
    is_sinkholed = (data[-4:] == b'\x00\x00\x00\x00')
    req = urllib.request.Request(f'http://{HOST}/stats.json', headers={'X-Admin-Token': TOKEN, 'Connection': 'close'})
    with urllib.request.urlopen(req, timeout=5) as r:
        st = json.loads(r.read().decode('utf-8'))
        rebind_cnt = st.get('rebind', 0)
    record('7. DNS Rebinding Shield (192.168.1.1.nip.io RFC1918 sinkholed to 0.0.0.0)', is_sinkholed and rebind_cnt > 0, f'Sinkholed: {is_sinkholed}, Rebind Interceptions: {rebind_cnt}')
except Exception as e:
    record('7. DNS Rebinding Shield', False, str(e))

# 8. Blocked Activity Log (/log.json with REBIND_DEFENSE)
try:
    req = urllib.request.Request(f'http://{HOST}/log.json', headers={'X-Admin-Token': TOKEN, 'Connection': 'close'})
    with urllib.request.urlopen(req, timeout=5) as r:
        logs = json.loads(r.read().decode('utf-8'))
        logged_custom = any(l.get('domain') == 'torture-test-ad.org' for l in logs)
        logged_rebind = any(l.get('domain') == '192.168.1.1.nip.io' and l.get('action') == 'REBIND_DEFENSE' for l in logs)
        l_cnt = len(logs)
        record('8. Blocked Activity Log (Tab 5 /log.json with REBIND_DEFENSE)', logged_custom and logged_rebind, f'Total: {l_cnt}, Has Custom: {logged_custom}, Has Rebind: {logged_rebind}')
except Exception as e:
    record('8. Blocked Activity Log (/log.json)', False, str(e))

# 9. Client Anti-Flood / Rate Limiting (Burst > 50 QPS threshold)
try:
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    s.settimeout(0.1)
    pkt = struct.pack('!HHHHHH', 0x9999, 0x0100, 1, 0, 0, 0) + b'\x0ftorture-test-ad\x03org\x00\x00\x01\x00\x01'
    # Send 70 rapid queries within ~250ms (exceeds 50 QPS threshold)
    for _ in range(70):
        s.sendto(pkt, (HOST, 53))
        time.sleep(0.003)
    
    # Check for REFUSED responses or ratelimited counter in stats
    refused_count = 0
    while True:
        try:
            resp, _ = s.recvfrom(512)
            flags = struct.unpack('!H', resp[2:4])[0]
            if (flags & 0x0F) == 5: # RFC 1035 REFUSED
                refused_count += 1
        except socket.timeout:
            break
    s.close()
    
    req = urllib.request.Request(f'http://{HOST}/stats.json', headers={'X-Admin-Token': TOKEN, 'Connection': 'close'})
    with urllib.request.urlopen(req, timeout=5) as r:
        st = json.loads(r.read().decode('utf-8'))
        rl_cnt = st.get('ratelimited', 0)
    record('9. Client Anti-Flood Rate Limiting (>50 QPS -> REFUSED)', (refused_count > 0 or rl_cnt > 0), f'Refused: {refused_count}, Total Rate Limited: {rl_cnt}')
except Exception as e:
    record('9. Client Anti-Flood Rate Limiting', False, str(e))

# 10. Device Friendly Renaming (/setname)
my_ip = '192.168.0.63'
try:
    req = urllib.request.Request(f'http://{HOST}/setname?ip={my_ip}&name=Primary%20QA%20Rig', data=b'', headers={'X-Admin-Token': TOKEN, 'Connection': 'close'})
    with urllib.request.urlopen(req, timeout=5) as r:
        ok_set = (r.status == 200 and r.read().decode('utf-8') == 'ok')
    req = urllib.request.Request(f'http://{HOST}/stats.json', headers={'X-Admin-Token': TOKEN, 'Connection': 'close'})
    with urllib.request.urlopen(req, timeout=5) as r:
        st = json.loads(r.read().decode('utf-8'))
        name_found = any(c.get('ip') == my_ip and c.get('name') == 'Primary QA Rig' for c in st.get('clients', []))
    record('10. Device Friendly Rename (/setname)', ok_set and name_found, f'Alias mapped to {my_ip}')
except Exception as e:
    record('10. Device Friendly Rename (/setname)', False, str(e))

# 11. Security / CSRF Foreign Origin Rejection
try:
    req = urllib.request.Request(f'http://{HOST}/ban?ip=192.168.0.99', data=b'', headers={'Origin': 'http://malicious-site.com', 'X-Admin-Token': TOKEN, 'Connection': 'close'})
    try:
        with urllib.request.urlopen(req, timeout=3) as r:
            csrf_blocked = False
    except urllib.error.HTTPError as e:
        csrf_blocked = (e.code == 401)
    record('11. Security & CSRF Strict Origin Enforcement', csrf_blocked, 'Foreign Origin strictly rejected (HTTP 401)')
except Exception as e:
    record('11. Security & CSRF Enforcement', False, str(e))

# 12. Web API Brute-force Lockout Defense (5 bad tokens -> HTTP 429 Lockout)
try:
    codes = []
    for attempt in range(6):
        try:
            req = urllib.request.Request(f'http://{HOST}/setname?ip=192.168.0.99&name=Hack', data=b'', headers={'X-Admin-Token': 'wrong_token_xyz', 'Connection': 'close'})
            with urllib.request.urlopen(req, timeout=3) as r:
                codes.append(r.status)
        except urllib.error.HTTPError as e:
            codes.append(e.code)
        time.sleep(0.05)
    
    # 5th/6th attempt should be 429 Too Many Requests
    got_429 = (429 in codes)
    record('12. API Brute-force Shield (5 Strikes -> HTTP 429 Lockout)', got_429, f'Response Codes: {codes}')
except Exception as e:
    record('12. API Brute-force Shield', False, str(e))

# 13. Zero-Bypass Lockout Check (Locked IP cannot access /stats.json even with valid token)
try:
    req = urllib.request.Request(f'http://{HOST}/stats.json', headers={'X-Admin-Token': TOKEN, 'Connection': 'close'})
    try:
        with urllib.request.urlopen(req, timeout=3) as r:
            lockout_enforced = False
    except urllib.error.HTTPError as e:
        lockout_enforced = (e.code == 429)
    record('13. Zero-Bypass Lockout Enforcement', lockout_enforced, 'Attacker locked out from all APIs during 30s cooldown')
except Exception as e:
    record('13. Zero-Bypass Lockout Enforcement', False, str(e))

# Clean up
print('\nWaiting 31s for brute-force lockout cooldown to expire...')
time.sleep(31)
try:
    req = urllib.request.Request(f'http://{HOST}/unblock?d=torture-test-ad.org', data=b'', headers={'X-Admin-Token': TOKEN, 'Connection': 'close'})
    urllib.request.urlopen(req, timeout=5)
except:
    pass

print('\n' + '='*60)
total_pass = sum(1 for _, p, _ in results if p)
print(f'Final Score: {total_pass}/{len(results)} Tests Passed ({100*total_pass/len(results):.1f}% SUCCESS)')
print('='*60)
