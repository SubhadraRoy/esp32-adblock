#!/usr/bin/env python3
"""Preprocess hosts/domain blocklists into a sorted truncated-FNV-1a hash blob
for the ESP32 ad-blocker. Hashes live in flash and are binary-searched on the
device, so no PSRAM is needed. Runs on your PC, not the ESP32 -- chip-agnostic.

HASH_BYTES MUST match the firmware (src/main.cpp). 5 bytes (40-bit) keeps
~0-1 collisions up to ~500k domains while fitting half a million in <3 MB.

Usage: build_blocklist.py [out.bin] [src ...]
  src = local file or URL. With none given, downloads Hagezi Ultimate (max
  tier, ~260k+ domains): blocks ads/trackers/malware/scam as broadly as this
  scale of blocklist can -- also blocks some social/messaging embeds, so
  something occasionally needs manually unblocking (see the dashboard's
  custom-domain list to allowlist by removing the offending entry from a
  locally-edited source instead, since Ultimate itself can't be un-included
  per-domain at build time).

  Prefer fewer false blocks over max coverage? Swap the URL below for a
  lighter Hagezi tier, smallest to largest (verified against the repo's
  current layout -- it moved from /domains/ to /wildcard/*-onlydomains/ at
  some point; if these ever 404 again the repo's likely restructured again,
  check https://github.com/hagezi/dns-blocklists#readme for the current path):
    .../wildcard/light-onlydomains.txt    (~42k,  zero-hassle)
    .../wildcard/multi-onlydomains.txt    (~180k, this is the "Normal" tier --
                                            yes, the file's called "multi", not
                                            "normal"; that's Hagezi's naming,
                                            not a typo here)
    .../wildcard/pro-onlydomains.txt      (~215k, rare hiccups)
    .../wildcard/ultimate-onlydomains.txt (~260k+, current default, most complete)
  Whatever you pick, this script prints the resulting domain count and flash
  size when it finishes -- check that against partitions.csv's littlefs
  partition size before flashing.
"""
import sys, os, math, urllib.request, hashlib

HASH_BYTES = 5                          # 40-bit hashes -- must match firmware
MASK = (1 << (HASH_BYTES * 8)) - 1
FNV_OFFSET = 0xcbf29ce484222325
FNV_PRIME  = 0x100000001b3
U64 = (1 << 64) - 1

# Recommended tier: Hagezi Pro (~215k domains) or Multi (~180k domains).
# The firmware caps blocklist size at 1.20 MB (~245,760 domains) so that
# blocklist.bin and incoming blocklist.new can safely coexist during atomic A/B
# updates without exhausting the 2.625 MB LittleFS partition.
DEFAULT_SOURCES = [
    'https://raw.githubusercontent.com/hagezi/dns-blocklists/main/wildcard/pro-onlydomains.txt',  # Hagezi Pro (~215k domains)
]

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
    return urllib.request.urlopen(src, timeout=180).read().decode('utf-8', 'ignore')

def main():
    if '-h' in sys.argv or '--help' in sys.argv:
        print(f"Usage: {sys.argv[0]} [output_file.bin] [url_or_file1 url_or_file2 ...]")
        print("Builds a sorted 40-bit FNV-1a binary blocklist for the ESP32 sinkhole.")
        print("Default output: blocklist.bin")
        print("Default source: HaGeZi Pro (~215k domains)")
        return

    args = sys.argv[1:]
    out = args[0] if args else 'blocklist.bin'
    sources = args[1:] if len(args) > 1 else DEFAULT_SOURCES

    domains = set()
    for src in sources:
        try:
            data = read_source(src)
        except Exception as e:
            print(f'  !! skipped {src}: {e}', file=sys.stderr); continue
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
    print(f'source domains   : {len(domains):,}')
    print(f'hash entries     : {n:,}  ({HASH_BYTES}-byte / {HASH_BYTES*8}-bit)')
    print(f'collisions       : {collisions}  (domains sharing a hash -> over-block)')
    print(f'flash blob       : {size:,} bytes  ({size/1024/1024:.2f} MB)  -> {out}')
    print(f'sha256 digest    : {sha256_hash}')
    print(f'lookup           : ~{math.ceil(math.log2(max(n,2)))} reads/query')

if __name__ == '__main__':
    main()
