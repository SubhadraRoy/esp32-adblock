#!/usr/bin/env python3
"""Preprocess hosts/domain blocklists into a sorted truncated-FNV-1a hash blob
for the ESP32 ad-blocker. Hashes live in flash and are binary-searched on the
device, so no PSRAM is needed. Runs on your PC, not the ESP32 -- chip-agnostic.

HASH_BYTES MUST match the firmware (src/main.cpp). 5 bytes (40-bit) keeps
~0-1 collisions up to ~500k domains while fitting half a million in <3 MB.

Usage: build_blocklist.py [out.bin] [src ...] [--preset balanced|threats|ultimate]
  Default output: blocklist.bin
  Default preset: balanced (~230k domains, ~1.15 MB, includes fake shops, scam sites,
                  malware C2, phishing, and comprehensive ad/tracker blocking).
"""
import sys, os, math, urllib.request, hashlib, argparse

HASH_BYTES = 5                          # 40-bit hashes -- must match firmware
MASK = (1 << (HASH_BYTES * 8)) - 1
FNV_OFFSET = 0xcbf29ce484222325
FNV_PRIME  = 0x100000001b3
U64 = (1 << 64) - 1

# Maximum domains to fit safely within the 1.20 MB LittleFS A/B update partition ceiling
MAX_DOMAINS_SAFE = 240000

PRESETS = {
    'balanced': [
        # 1. High-priority threat intelligence & fake/scam sites
        'https://raw.githubusercontent.com/hagezi/dns-blocklists/main/wildcard/fake-onlydomains.txt',
        'https://urlhaus.abuse.ch/downloads/hostfile/',
        # 2. Comprehensive ad/tracker/phishing baseline
        'https://raw.githubusercontent.com/hagezi/dns-blocklists/main/wildcard/pro-onlydomains.txt',
    ],
    'threats': [
        # Security threats focus: fake sites, malware C2, phishing, crypto scams
        'https://raw.githubusercontent.com/hagezi/dns-blocklists/main/wildcard/fake-onlydomains.txt',
        'https://urlhaus.abuse.ch/downloads/hostfile/',
        'https://phishing.army/download/phishing_army_blocklist_extended.txt',
        'https://raw.githubusercontent.com/hagezi/dns-blocklists/main/wildcard/tif.mini-onlydomains.txt',
    ],
    'ultimate': [
        # Full HaGeZi Ultimate (~260k+ domains) for manual upload / max coverage
        'https://raw.githubusercontent.com/hagezi/dns-blocklists/main/wildcard/fake-onlydomains.txt',
        'https://urlhaus.abuse.ch/downloads/hostfile/',
        'https://raw.githubusercontent.com/hagezi/dns-blocklists/main/wildcard/ultimate-onlydomains.txt',
    ],
}

DEFAULT_SOURCES = PRESETS['balanced']

def fnv(b: bytes) -> int:
    h = FNV_OFFSET
    for c in b:
        h = ((h ^ c) * FNV_PRIME) & U64
    return h & MASK                      # truncate to HASH_BYTES

def norm(d: str) -> str:
    d = d.strip().lower().lstrip('*').lstrip('.').rstrip('.')
    return d[4:] if d.startswith('www.') else d

def valid_hostname(d: str) -> bool:
    """Strict hostname validation matching validHostname() in firmware."""
    n = len(d)
    if n == 0 or n > 253:
        return False
    if d[0] in '.-' or d[-1] in '.-':
        return False
    saw_dot = False
    prev_dot = False
    for c in d:
        ok = ('a' <= c <= 'z') or ('0' <= c <= '9') or c in '.-'
        if not ok:
            return False
        if c == '.':
            if prev_dot:
                return False
            saw_dot = True
        prev_dot = (c == '.')
    return saw_dot

def read_source(src: str) -> str:
    if os.path.exists(src):
        return open(src, errors='ignore').read()
    print(f'  downloading {src} ...', file=sys.stderr)
    req = urllib.request.Request(src, headers={'User-Agent': 'Mozilla/5.0 (ESP32-AdBlock Preprocessor)'})
    return urllib.request.urlopen(req, timeout=180).read().decode('utf-8', 'ignore')

def main():
    parser = argparse.ArgumentParser(description="Build sorted 40-bit FNV-1a binary blocklist for ESP32 sinkhole.")
    parser.add_argument('output', nargs='?', default='blocklist.bin', help="Output binary file (default: blocklist.bin)")
    parser.add_argument('sources', nargs='*', help="Optional custom URLs or local files")
    parser.add_argument('--preset', choices=['balanced', 'threats', 'ultimate'], default='balanced',
                        help="Pre-configured source collection (default: balanced)")
    parser.add_argument('--cap', type=int, default=MAX_DOMAINS_SAFE,
                        help="Maximum domains to include to respect flash budget (default: 240,000)")
    parser.add_argument('--no-cap', action='store_true', help="Disable domain ceiling cap")

    args = parser.parse_args()
    out = args.output
    sources = args.sources if args.sources else PRESETS[args.preset]

    print(f"Building blocklist using preset: {args.preset}")
    print(f"Output destination: {out}")

    domains = set()
    for src in sources:
        try:
            data = read_source(src)
        except Exception as e:
            print(f'  !! skipped {src}: {e}', file=sys.stderr)
            continue
        count_before = len(domains)
        for line in data.splitlines():
            line = line.split('#', 1)[0].strip()
            if not line or line[0] in '!/':
                continue
            parts = line.split()
            d = parts[1] if len(parts) >= 2 and parts[0] in ('0.0.0.0','127.0.0.1','::1','::') \
                else parts[0] if len(parts) == 1 else None
            if d:
                d = norm(d)
                if valid_hostname(d):
                    domains.add(d)
        print(f"    added {len(domains) - count_before:,} unique domains from {src}")

    # Enforce safe flash cap if requested
    if not args.no_cap and len(domains) > args.cap:
        print(f"  [!] Domains exceed safe flash cap ({len(domains):,} > {args.cap:,}). Capping at {args.cap:,} to preserve LittleFS A/B update space.")
        domains = set(list(domains)[:args.cap])

    hashes = sorted(fnv(d.encode()) for d in domains)
    collisions = len(hashes) - len(set(hashes))
    uniq = sorted(set(hashes))                       # one entry per distinct hash
    blob = bytearray()
    for h in uniq:
        blob.extend(h.to_bytes(HASH_BYTES, 'little'))

    with open(out, 'wb') as f:
        f.write(blob)

    sha256_hash = hashlib.sha256(blob).hexdigest()
    with open(out + '.sha256', 'w') as sf:
        sf.write(f'{sha256_hash}  {os.path.basename(out)}\n')

    n, size = len(uniq), len(uniq) * HASH_BYTES
    print(f'\nsource domains   : {len(domains):,}')
    print(f'hash entries     : {n:,}  ({HASH_BYTES}-byte / {HASH_BYTES*8}-bit)')
    print(f'collisions       : {collisions}  (domains sharing a hash -> over-block)')
    print(f'flash blob       : {size:,} bytes  ({size/1024/1024:.2f} MB)  -> {out}')
    print(f'sha256 digest    : {sha256_hash}')
    print(f'lookup           : ~{math.ceil(math.log2(max(n,2)))} reads/query')

if __name__ == '__main__':
    main()
