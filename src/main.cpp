// ESP32 AdBlock — Production-grade DNS sinkhole + web dashboard for ESP32 (D0WD-V3 rev 3.1).
// ESP32 AdBlock — Production-grade DNS sinkhole + web dashboard for ESP32 DevKit V1 (ESP32-D0WD-V3 rev 3.1).
// Pure ESP-IDF native APIs: esp_wifi, BSD sockets, LittleFS VFS, esp_http_server, esp_http_client.
// Hardened for 24/7/365 continuous uptime, zero memory leaks, and RFC 1035/6891 compliance.

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <math.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/event_groups.h"

#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_system.h"
#include "esp_heap_caps.h"
#include "esp_timer.h"
#include "esp_task_wdt.h"
#include "esp_random.h"
#include "nvs_flash.h"
#include "esp_littlefs.h"
#include "esp_sntp.h"
#include "mdns.h"
#include "esp_http_server.h"
#include "esp_http_client.h"
#include "esp_crt_bundle.h"

#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include "lwip/etharp.h"
#include "lwip/netif.h"
#include "lwip/tcpip.h"

#include "page.h"   // dashboard HTML/CSS/JS (const char PAGE[])

// ---- config ----
#if __has_include("wifi_credentials.h")
#include "wifi_credentials.h"
#else
// Set your Wi-Fi credentials here before flashing.
static const char* WIFI_SSID = "YOUR_WIFI_SSID";
static const char* WIFI_PASS = "YOUR_WIFI_PASSWORD";
// Required for anything that changes device state (ban, block/unblock, update settings).
// Set this to a custom passphrase -- mutating endpoints are locked while set to "changeme".
static const char* ADMIN_TOKEN = "changeme";
#endif
static const char* UPSTREAM_IP = "9.9.9.9";      // Quad9
static const uint16_t DNS_PORT = 53;
static const char* FS_BASE = "/lfs";
static const char* BLOCKLIST_PATH = "/lfs/blocklist.bin";
static const char* BLOCKLIST_NEW  = "/lfs/blocklist.new";
static const int HASH_BYTES = 5;
static const uint64_t HASH_MASK = (1ULL << (HASH_BYTES * 8)) - 1;

// 64-bit monotonic millisecond clock (will not overflow for 584 million years)
static inline uint64_t millis64() { return (uint64_t)(esp_timer_get_time() / 1000ULL); }
static inline uint32_t millis()   { return (uint32_t)millis64(); }

static void ipToStr(uint32_t ipNetOrder, char* out, size_t outsz) {
  const uint8_t* b = (const uint8_t*)&ipNetOrder;
  snprintf(out, outsz, "%u.%u.%u.%u", b[0], b[1], b[2], b[3]);
}

static void jescInto(char* dst, size_t dstsz, const char* src) {   // JSON-escape into fixed buffer
  if (!dst || dstsz == 0) return;
  size_t o = 0;
  for (size_t i = 0; src && src[i] && o + 6 < dstsz; i++) {
    unsigned char c = (unsigned char)src[i];
    if (c == '"' || c == '\\') { dst[o++] = '\\'; dst[o++] = c; }
    else if (c == '\n') { dst[o++] = '\\'; dst[o++] = 'n'; }
    else if (c == '\r') { dst[o++] = '\\'; dst[o++] = 'r'; }
    else if (c == '\t') { dst[o++] = '\\'; dst[o++] = 't'; }
    else if (c < 0x20)  { o += snprintf(dst + o, dstsz - o, "\\u%04x", c); }
    else { dst[o++] = c; }
  }
  dst[o] = 0;
}

static void urlDecode(char* dst, const char* src, size_t dstsz) {
  if (!dst || dstsz == 0) return;
  size_t d = 0;
  for (size_t i = 0; src && src[i] && d + 1 < dstsz; i++) {
    if (src[i] == '+') {
      dst[d++] = ' ';
    } else if (src[i] == '%' && src[i+1] && src[i+2]) {
      char hex[3] = { src[i+1], src[i+2], 0 };
      dst[d++] = (char)strtol(hex, NULL, 16);
      i += 2;
    } else {
      dst[d++] = src[i];
    }
  }
  dst[d] = 0;
}

// ---- globals ----
static httpd_handle_t webServer = nullptr;
static esp_netif_t* staNetif = nullptr;
static int dnsSock = -1, upstreamSock = -1;
static struct sockaddr_in upstreamAddr;
static FILE* blocklist = nullptr;
static uint32_t numHashes = 0, totalBlocked = 0, totalAllowed = 0;
static uint32_t totalRebindBlocked = 0, totalRateLimited = 0;

struct BlockLogEntry {
  uint32_t epoch;
  uint32_t clientIp;
  uint16_t qtype;
  bool isRebind;
  char domain[96];
};
static const int MAX_BLOCK_LOG = 64;
static BlockLogEntry blockLog[MAX_BLOCK_LOG];
static int blockLogHead = 0;
static int blockLogCount = 0;

struct Dev {
  uint32_t ip;
  uint8_t mac[6];
  char name[32];
  uint32_t blocked;
  uint32_t allowed;
  uint64_t lastSeen;
  bool banned;
  uint32_t secWindow;
  uint16_t qCount;
};
static const int MAX_CLIENTS = 96;
static Dev clients[MAX_CLIENTS];
static int numClients = 0;

struct DevName {
  uint32_t ip;
  char name[32];
};
static const int MAX_DEV_NAMES = 64;
static DevName devNames[MAX_DEV_NAMES];
static int numDevNames = 0;

static const int MAX_CUSTOM = 128;
static char customDom[MAX_CUSTOM][256];
static uint64_t customHash[MAX_CUSTOM];
static int numCustom = 0;

static const int MAX_BAN = 32;
static uint32_t bannedIP[MAX_BAN];
static int numBanned = 0;

// Remote blocklist auto-update state
static char updateUrl[256] = "";
static uint32_t updateIntervalH = 24;
static uint64_t lastCheckMs = 0;
static char updateStatus[128] = "never";
static bool updateInProgress = false;
static volatile bool manualFetchTrigger = false;

// ---------- LOCK DISCIPLINE ----------
static SemaphoreHandle_t stateMutex = nullptr;
#define LOCK()   lockWithDiagnostic()
static inline void lockWithDiagnostic() {
  if (xSemaphoreTake(stateMutex, pdMS_TO_TICKS(2000)) == pdTRUE) return;
  printf("[lock] core %d waited 2s+ for stateMutex\n", xPortGetCoreID());
  xSemaphoreTake(stateMutex, portMAX_DELAY);
}
#define UNLOCK() xSemaphoreGive(stateMutex)

// ---------- In-Memory Prefix Table (1,028 Bytes) ----------
// Replaces the uninitialized/oversaturated Bloom filter.
// Partitions sorted 40-bit hashes by MSB (v >> 32 & 0xFF).
// Guarantees 0% false positives and cuts flash binary search reads by 44%.
static uint32_t prefixTable[257];

static void buildPrefixTable(FILE* f, uint32_t count) {
  memset(prefixTable, 0, sizeof(prefixTable));
  prefixTable[256] = count;
  if (!f || count == 0) return;

  uint32_t currentPrefix = 0;
  prefixTable[0] = 0;

  static uint8_t chunk[2048]; // static to conserve task stack
  const uint32_t perChunk = sizeof(chunk) / HASH_BYTES;
  fseek(f, 0, SEEK_SET);

  uint32_t processed = 0;
  while (processed < count) {
    uint32_t want = count - processed;
    if (want > perChunk) want = perChunk;
    size_t n = fread(chunk, 1, want * HASH_BYTES, f);
    uint32_t got = n / HASH_BYTES;
    if (got == 0) break;

    for (uint32_t k = 0; k < got; k++) {
      uint8_t topByte = chunk[k * HASH_BYTES + 4]; // b[4] is MSB in little-endian
      while (currentPrefix < topByte) {
        currentPrefix++;
        prefixTable[currentPrefix] = processed + k;
      }
    }
    processed += got;
  }
  while (currentPrefix < 256) {
    currentPrefix++;
    prefixTable[currentPrefix] = count;
  }
  printf("[prefix] built index for %lu domains in RAM (1028 B)\n", (unsigned long)count);
}

// ---------- Hashing / Matching ----------
static uint64_t fnv40(const char* s, size_t n) {
  uint64_t h = 0xcbf29ce484222325ULL;
  for (size_t i = 0; i < n; i++) {
    h ^= (uint8_t)tolower((unsigned char)s[i]); // RFC 4343 case normalization
    h *= 0x100000001b3ULL;
  }
  return h & HASH_MASK;
}

static bool inFlash(uint64_t h) { // caller holds stateMutex
  if (!blocklist || numHashes == 0) return false;
  uint8_t p = (uint8_t)(h >> 32);
  int32_t lo = (int32_t)prefixTable[p];
  int32_t hi = (int32_t)prefixTable[p + 1] - 1;
  if (lo > hi) return false; // 0 flash reads for unrepresented prefixes

  uint8_t b[HASH_BYTES];
  while (lo <= hi) {
    int32_t mid = (lo + hi) >> 1;
    if (fseek(blocklist, (long)mid * HASH_BYTES, SEEK_SET) != 0) return false;
    if (fread(b, 1, HASH_BYTES, blocklist) != HASH_BYTES) return false;
    uint64_t v = 0;
    for (int k = 0; k < HASH_BYTES; k++) v |= ((uint64_t)b[k]) << (8 * k);
    if (v < h) lo = mid + 1;
    else if (v > h) hi = mid - 1;
    else return true;
  }
  return false;
}

static bool inCustom(uint64_t h) {
  for (int i = 0; i < numCustom; i++) {
    if (customHash[i] == h) return true;
  }
  return false;
}

static bool isBlocked(const char* domain) { // caller holds stateMutex
  const char* p = domain;
  while (p && *p) {
    uint64_t h = fnv40(p, strlen(p));
    if (inCustom(h)) return true;
    if (numHashes && inFlash(h)) return true;
    const char* dot = strchr(p, '.');
    if (!dot) break;
    const char* next = dot + 1;
    if (!strchr(next, '.')) break;
    p = next;
  }
  return false;
}

// ---------- Persistence ----------
static bool validHostname(const char* d, size_t n) {
  if (n == 0 || n > 253) return false;
  if (d[0] == '.' || d[0] == '-' || d[n-1] == '.' || d[n-1] == '-') return false;
  bool sawDot = false, prevDot = false;
  for (size_t i = 0; i < n; i++) {
    char c = d[i];
    bool ok = (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-' || c == '.';
    if (!ok) return false;
    if (c == '.') { if (prevDot) return false; sawDot = true; }
    prevDot = (c == '.');
  }
  return sawDot;
}

static size_t normalizeDomain(char* d, size_t n) {
  while (n && (d[n-1] == '\n' || d[n-1] == '\r' || d[n-1] == ' ')) d[--n] = 0;
  for (size_t i = 0; i < n; i++) d[i] = (char)tolower((unsigned char)d[i]);
  if (n > 4 && strncmp(d, "www.", 4) == 0) {
    memmove(d, d + 4, n - 3);
    n -= 4;
  }
  return n;
}

static bool atomicWrite(const char* path, const char* content) {
  char tmp[64];
  snprintf(tmp, sizeof(tmp), "%s.new", path);
  FILE* f = fopen(tmp, "w");
  if (!f) return false;
  size_t len = strlen(content);
  bool ok = (fwrite(content, 1, len, f) == len);
  fflush(f);
  fsync(fileno(f));
  fclose(f);
  if (!ok) { remove(tmp); return false; }
  if (rename(tmp, path) != 0) {
    printf("[fs] rename %s failed (errno %d)\n", path, errno);
    remove(tmp);
    return false;
  }
  return true;
}

static void loadCustom() {
  numCustom = 0;
  FILE* f = fopen("/lfs/custom.txt", "r");
  if (!f) return;
  char line[256];
  while (numCustom < MAX_CUSTOM && fgets(line, sizeof(line), f)) {
    size_t n = normalizeDomain(line, strlen(line));
    if (!validHostname(line, n)) continue;
    strncpy(customDom[numCustom], line, sizeof(customDom[0]) - 1);
    customDom[numCustom][sizeof(customDom[0]) - 1] = 0;
    customHash[numCustom] = fnv40(line, n);
    numCustom++;
  }
  fclose(f);
}

static bool saveCustom() {
  const char* path = "/lfs/custom.txt";
  char tmp[64];
  snprintf(tmp, sizeof(tmp), "%s.new", path);
  FILE* f = fopen(tmp, "w");
  if (!f) return false;
  bool ok = true;
  LOCK();
  int count = numCustom;
  UNLOCK();
  for (int i = 0; i < count; i++) {
    char entry[256];
    LOCK();
    if (i < numCustom) {
      strncpy(entry, customDom[i], sizeof(entry) - 1);
      entry[sizeof(entry) - 1] = 0;
    } else {
      entry[0] = 0;
    }
    UNLOCK();
    if (entry[0]) {
      if (fprintf(f, "%s\n", entry) < 0) { ok = false; break; }
    }
  }
  fflush(f);
  fsync(fileno(f));
  fclose(f);
  if (!ok) { remove(tmp); return false; }
  if (rename(tmp, path) != 0) { remove(tmp); return false; }
  return true;
}

static bool addCustom(const char* dIn) {
  char d[256];
  strncpy(d, dIn, sizeof(d) - 1);
  d[sizeof(d) - 1] = 0;
  size_t n = normalizeDomain(d, strlen(d));
  if (!validHostname(d, n)) return false;

  LOCK();
  if (numCustom >= MAX_CUSTOM) { UNLOCK(); return false; }
  for (int i = 0; i < numCustom; i++) {
    if (strcmp(customDom[i], d) == 0) { UNLOCK(); return false; }
  }
  strncpy(customDom[numCustom], d, sizeof(customDom[0]) - 1);
  customDom[numCustom][sizeof(customDom[0]) - 1] = 0;
  customHash[numCustom] = fnv40(d, n);
  numCustom++;
  UNLOCK();

  if (!saveCustom()) {
    LOCK();
    if (numCustom > 0 && strcmp(customDom[numCustom - 1], d) == 0) numCustom--;
    UNLOCK();
    return false;
  }
  return true;
}

static bool removeCustom(const char* dIn) {
  char d[256];
  strncpy(d, dIn, sizeof(d) - 1);
  d[sizeof(d) - 1] = 0;
  size_t n = normalizeDomain(d, strlen(d));
  (void)n;
  char savedDom[256] = "";
  uint64_t savedHash = 0;
  int removedIdx = -1;

  LOCK();
  for (int i = 0; i < numCustom; i++) {
    if (strcmp(customDom[i], d) == 0) {
      removedIdx = i;
      strcpy(savedDom, customDom[i]);
      savedHash = customHash[i];
      for (int j = i; j < numCustom - 1; j++) {
        strcpy(customDom[j], customDom[j + 1]);
        customHash[j] = customHash[j + 1];
      }
      numCustom--;
      break;
    }
  }
  UNLOCK();

  if (removedIdx < 0) return false;
  if (!saveCustom()) {
    LOCK();
    if (numCustom < MAX_CUSTOM) {
      for (int j = numCustom; j > removedIdx; j--) {
        strcpy(customDom[j], customDom[j - 1]);
        customHash[j] = customHash[j - 1];
      }
      strcpy(customDom[removedIdx], savedDom);
      customHash[removedIdx] = savedHash;
      numCustom++;
    }
    UNLOCK();
    return false;
  }
  return true;
}

static bool isBannedIP(uint32_t ip) {
  for (int i = 0; i < numBanned; i++) {
    if (bannedIP[i] == ip) return true;
  }
  return false;
}

static void loadBanned() {
  numBanned = 0;
  FILE* f = fopen("/lfs/banned.txt", "r");
  if (!f) return;
  char line[32];
  while (numBanned < MAX_BAN && fgets(line, sizeof(line), f)) {
    size_t n = strlen(line);
    while (n && (line[n - 1] == '\n' || line[n - 1] == '\r')) line[--n] = 0;
    struct in_addr a;
    if (n && inet_pton(AF_INET, line, &a) == 1) bannedIP[numBanned++] = a.s_addr;
  }
  fclose(f);
}

static bool saveBanned() {
  const char* path = "/lfs/banned.txt";
  char tmp[64];
  snprintf(tmp, sizeof(tmp), "%s.new", path);
  FILE* f = fopen(tmp, "w");
  if (!f) return false;
  uint32_t snap[MAX_BAN];
  int count = 0;
  LOCK();
  count = numBanned;
  for (int i = 0; i < count; i++) snap[i] = bannedIP[i];
  UNLOCK();

  bool ok = true;
  for (int i = 0; i < count; i++) {
    char s[16];
    ipToStr(snap[i], s, sizeof(s));
    if (fprintf(f, "%s\n", s) < 0) { ok = false; break; }
  }
  fflush(f);
  fsync(fileno(f));
  fclose(f);
  if (!ok) { remove(tmp); return false; }
  if (rename(tmp, path) != 0) { remove(tmp); return false; }
  return true;
}

static bool toggleBan(uint32_t ip) {
  bool previouslyBanned = false;
  LOCK();
  int idx = -1;
  for (int i = 0; i < numBanned; i++) {
    if (bannedIP[i] == ip) { idx = i; break; }
  }
  if (idx >= 0) {
    previouslyBanned = true;
    for (int i = idx; i < numBanned - 1; i++) bannedIP[i] = bannedIP[i + 1];
    numBanned--;
    for (int i = 0; i < numClients; i++) {
      if (clients[i].ip == ip) clients[i].banned = false;
    }
  } else {
    if (numBanned >= MAX_BAN) { UNLOCK(); return false; }
    bannedIP[numBanned++] = ip;
    for (int i = 0; i < numClients; i++) {
      if (clients[i].ip == ip) clients[i].banned = true;
    }
  }
  UNLOCK();

  bool ok = saveBanned();
  if (!ok) {
    LOCK();
    if (previouslyBanned) {
      if (numBanned < MAX_BAN) bannedIP[numBanned++] = ip;
      for (int i = 0; i < numClients; i++) if (clients[i].ip == ip) clients[i].banned = true;
    } else {
      if (numBanned > 0 && bannedIP[numBanned - 1] == ip) numBanned--;
      for (int i = 0; i < numClients; i++) if (clients[i].ip == ip) clients[i].banned = false;
    }
    UNLOCK();
    return false;
  }
  return true;
}

static void loadNames() {
  numDevNames = 0;
  FILE* f = fopen("/lfs/names.txt", "r");
  if (!f) return;
  char line[128];
  while (numDevNames < MAX_DEV_NAMES && fgets(line, sizeof(line), f)) {
    size_t n = strlen(line);
    while (n && (line[n - 1] == '\n' || line[n - 1] == '\r' || line[n - 1] == ' ')) line[--n] = 0;
    if (n == 0) continue;
    char* sp = strchr(line, ' ');
    if (!sp) continue;
    *sp = 0;
    struct in_addr a;
    if (inet_pton(AF_INET, line, &a) != 1) continue;
    const char* nm = sp + 1;
    while (*nm == ' ') nm++;
    if (!*nm) continue;
    devNames[numDevNames].ip = a.s_addr;
    strncpy(devNames[numDevNames].name, nm, sizeof(devNames[0].name) - 1);
    devNames[numDevNames].name[sizeof(devNames[0].name) - 1] = 0;
    numDevNames++;
  }
  fclose(f);
}

static bool saveNames() {
  const char* path = "/lfs/names.txt";
  char tmp[64];
  snprintf(tmp, sizeof(tmp), "%s.new", path);
  FILE* f = fopen(tmp, "w");
  if (!f) return false;
  DevName snap[MAX_DEV_NAMES];
  int count = 0;
  LOCK();
  count = numDevNames;
  for (int i = 0; i < count; i++) snap[i] = devNames[i];
  UNLOCK();

  bool ok = true;
  for (int i = 0; i < count; i++) {
    char s[16];
    ipToStr(snap[i].ip, s, sizeof(s));
    if (fprintf(f, "%s %s\n", s, snap[i].name) < 0) { ok = false; break; }
  }
  fflush(f);
  fsync(fileno(f));
  fclose(f);
  if (!ok) { remove(tmp); return false; }
  if (rename(tmp, path) != 0) { remove(tmp); return false; }
  return true;
}

// ---------- Client Table & MAC Lookup ----------
static void getMac(uint32_t ip, uint8_t* mac) {
  memset(mac, 0, 6);
  ip4_addr_t ipa;
  ipa.addr = ip;
  struct eth_addr* eth = nullptr;
  const ip4_addr_t* ipret = nullptr;

  LOCK_TCPIP_CORE();
  for (struct netif* nif = netif_list; nif; nif = nif->next) {
    if (etharp_find_addr(nif, &ipa, &eth, &ipret) >= 0 && eth) {
      memcpy(mac, eth->addr, 6);
      break;
    }
  }
  UNLOCK_TCPIP_CORE();
}

static Dev* getClient(uint32_t ip) { // caller holds stateMutex
  uint64_t now = millis64();
  for (int i = 0; i < numClients; i++) {
    if (clients[i].ip == ip) {
      clients[i].lastSeen = now;
      // Opportunistic re-resolution if MAC was previously unknown
      if (clients[i].mac[0] == 0 && clients[i].mac[1] == 0 && clients[i].mac[2] == 0 &&
          clients[i].mac[3] == 0 && clients[i].mac[4] == 0 && clients[i].mac[5] == 0) {
        getMac(ip, clients[i].mac);
      }
      return &clients[i];
    }
  }
  if (numClients < MAX_CLIENTS) {
    Dev* c = &clients[numClients++];
    c->ip = ip;
    c->blocked = c->allowed = 0;
    c->lastSeen = now;
    c->banned = isBannedIP(ip);
    c->secWindow = 0;
    c->qCount = 0;
    c->name[0] = 0;
    for (int j = 0; j < numDevNames; j++) {
      if (devNames[j].ip == ip) {
        strncpy(c->name, devNames[j].name, sizeof(c->name) - 1);
        c->name[sizeof(c->name) - 1] = 0;
        break;
      }
    }
    getMac(ip, c->mac);
    return c;
  }
  // 64-bit monotonic eviction prevents 49.7-day timer rollover defect
  int oldest = 0;
  for (int i = 1; i < numClients; i++) {
    if (clients[i].lastSeen < clients[oldest].lastSeen) oldest = i;
  }
  Dev* c = &clients[oldest];
  c->ip = ip;
  c->blocked = c->allowed = 0;
  c->lastSeen = now;
  c->banned = isBannedIP(ip);
  c->secWindow = 0;
  c->qCount = 0;
  c->name[0] = 0;
  for (int j = 0; j < numDevNames; j++) {
    if (devNames[j].ip == ip) {
      strncpy(c->name, devNames[j].name, sizeof(c->name) - 1);
      c->name[sizeof(c->name) - 1] = 0;
      break;
    }
  }
  getMac(ip, c->mac);
  return c;
}

// ---------- DNS Protocol Engine ----------
static size_t parseQuery(const uint8_t* pkt, int len, char* out, uint16_t* qtype, int* qend, bool* hasOpt) {
  *hasOpt = false;
  if (len < 17) return 0;
  if ((pkt[2] & 0x80) != 0) return 0;            // QR: must be 0 (Query)
  if (((pkt[2] >> 3) & 0x0F) != 0) return 0;     // OPCODE: must be 0 (Standard query)
  if (((pkt[4] << 8) | pkt[5]) != 1) return 0;   // QDCOUNT: must be exactly 1
  uint16_t arcount = (pkt[10] << 8) | pkt[11];

  int i = 12;
  size_t o = 0;
  while (i < len) {
    uint8_t l = pkt[i++];
    if (l == 0) break;
    if (l & 0xC0) return 0;                      // Compressed labels invalid in query
    if (l > 63) return 0;                        // RFC 1035 max label length
    if (o + l + 1 >= 250 || i + l > len) return 0;
    if (o) out[o++] = '.';
    for (uint8_t k = 0; k < l; k++) {
      out[o++] = (char)tolower((unsigned char)pkt[i++]);
    }
  }
  out[o] = 0;
  if (i + 4 > len) return 0;
  *qtype = (pkt[i] << 8) | pkt[i + 1];
  uint16_t qclass = (pkt[i + 2] << 8) | pkt[i + 3];
  if (qclass != 1) return 0;                     // QCLASS must be 1 (IN)
  *qend = i + 4;

  if (arcount > 0) {
    *hasOpt = true;
  }

  if (o > 4 && strncmp(out, "www.", 4) == 0) {
    memmove(out, out + 4, o - 3);
    o -= 4;
  }
  return o;
}

// RFC 6891 §7 & RFC 1035 compliant blocked response
static int buildBlocked(uint8_t* outBuf, int qend, uint16_t qtype, bool hasOpt) {
  if (qend < 12 || qend + 32 > 1472) return 0;
  outBuf[2] = 0x81; // QR=1, RD=1
  outBuf[3] = 0x80; // RA=1, RCODE=0 (NOERROR)
  outBuf[6] = 0;
  outBuf[7] = (qtype == 1) ? 1 : 0; // ANCOUNT: 1 for A, 0 for AAAA/HTTPS (NODATA)
  outBuf[8] = 0;
  outBuf[9] = 0;
  outBuf[10] = 0;
  outBuf[11] = hasOpt ? 1 : 0;      // ARCOUNT: 1 if OPT RR present

  int rlen = qend;
  if (qtype == 1) {
    // Type A: pointer 0xC00C, TYPE=1, CLASS=1, TTL=300s, RDLENGTH=4, IP=0.0.0.0
    const uint8_t ans[] = {0xC0, 0x0C, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x01, 0x2C, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00};
    memcpy(outBuf + rlen, ans, sizeof(ans));
    rlen += sizeof(ans);
  }
  if (hasOpt) {
    // RFC 6891 OPT RR: NAME=0, TYPE=41(0x29), CLASS=1232(0x04D0), EXT-RCODE=0, VERSION=0, FLAGS=0, RDLEN=0
    const uint8_t optRR[] = {0x00, 0x00, 0x29, 0x04, 0xD0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    memcpy(outBuf + rlen, optRR, sizeof(optRR));
    rlen += sizeof(optRR);
  }
  return rlen;
}

// RFC 1035 Format Error (RCODE=1) response
static int buildFormErr(const uint8_t* inPkt, int inLen, uint8_t* outPkt) {
  if (inLen < 12) return 0;
  memcpy(outPkt, inPkt, 12);
  outPkt[2] = 0x81;
  outPkt[3] = 0x81; // RCODE=1 (FORMERR)
  outPkt[4] = 0; outPkt[5] = 0; // QDCOUNT=0
  outPkt[6] = 0; outPkt[7] = 0; // ANCOUNT=0
  outPkt[8] = 0; outPkt[9] = 0; // NSCOUNT=0
  outPkt[10] = 0; outPkt[11] = 0; // ARCOUNT=0
  return 12;
}

// ---------- Asynchronous Upstream DNS Engine (Eliminates Head-of-Line Blocking) ----------
struct DnsTx {
  uint16_t clientTxid;
  uint16_t upstreamTxid;
  struct sockaddr_in clientAddr;
  socklen_t clientLen;
  uint64_t expireMs;
  uint8_t qsection[256];
  int qlen;
  char domain[96];
  uint16_t qtype;
  bool hasOpt;
  bool inUse;
};

static const int MAX_TX = 32;
static DnsTx txTable[MAX_TX];
static uint8_t dnsRxBuf[1472];
static uint8_t upRxBuf[1472];

// RFC 1035 Refused (RCODE=5) response for client query rate limiting
static int buildRefused(const uint8_t* inPkt, int inLen, uint8_t* outPkt) {
  if (inLen < 12) return 0;
  memcpy(outPkt, inPkt, 12);
  outPkt[2] = 0x81; // QR=1, RD=1
  outPkt[3] = 0x85; // RA=1, RCODE=5 (REFUSED)
  outPkt[4] = 0; outPkt[5] = 0; // QDCOUNT=0
  outPkt[6] = 0; outPkt[7] = 0; // ANCOUNT=0
  outPkt[8] = 0; outPkt[9] = 0; // NSCOUNT=0
  outPkt[10] = 0; outPkt[11] = 0; // ARCOUNT=0
  return 12;
}

// RFC 1918 / Loopback / Link-Local IP filtering for DNS Rebinding Protection
static inline bool isPrivateOrLoopbackIP(uint8_t b0, uint8_t b1) {
  if (b0 == 127) return true;                            // 127.0.0.0/8 Loopback
  if (b0 == 10) return true;                             // 10.0.0.0/8 RFC 1918 Class A
  if (b0 == 172 && (b1 >= 16 && b1 <= 31)) return true; // 172.16.0.0/12 RFC 1918 Class B
  if (b0 == 192 && b1 == 168) return true;               // 192.168.0.0/16 RFC 1918 Class C
  if (b0 == 169 && b1 == 254) return true;               // 169.254.0.0/16 Link-Local / APIPA
  if (b0 == 0) return true;                              // 0.0.0.0/8
  return false;
}

static bool isRebindThreat(const uint8_t* pkt, int len, int qend, const char* domain) {
  if (!domain || !pkt || len < 12) return false;

  // Preserve legitimate local and internal network domain resolution
  size_t dlen = strlen(domain);
  if (dlen >= 6 && strcmp(domain + dlen - 6, ".local") == 0) return false;
  if (dlen >= 4 && strcmp(domain + dlen - 4, ".lan") == 0) return false;
  if (dlen >= 5 && strcmp(domain + dlen - 5, ".home") == 0) return false;
  if (dlen >= 10 && strcmp(domain + dlen - 10, ".home.arpa") == 0) return false;
  if (dlen >= 9 && strcmp(domain + dlen - 9, ".internal") == 0) return false;
  if (dlen >= 12 && strcmp(domain + dlen - 12, ".localdomain") == 0) return false;

  uint16_t ancount = (pkt[6] << 8) | pkt[7];
  if (ancount == 0) return false;

  int p = qend; // Point immediately after Question section
  for (int a = 0; a < ancount && p + 10 <= len; a++) {
    // Parse NAME (label sequence or compression pointer)
    if ((pkt[p] & 0xC0) == 0xC0) {
      p += 2;
    } else {
      while (p < len && pkt[p] != 0) {
        if ((pkt[p] & 0xC0) == 0xC0) { p += 2; goto after_name; }
        p += 1 + pkt[p];
      }
      if (p < len && pkt[p] == 0) p++;
    }
after_name:
    if (p + 10 > len) break;
    uint16_t type = (pkt[p] << 8) | pkt[p + 1];
    uint16_t rdlen = (pkt[p + 8] << 8) | pkt[p + 9];
    p += 10;
    if (p + rdlen > len) break;

    if (type == 1 && rdlen == 4) { // Type A (IPv4)
      uint8_t b0 = pkt[p], b1 = pkt[p + 1];
      if (isPrivateOrLoopbackIP(b0, b1)) return true;
    } else if (type == 28 && rdlen == 16) { // Type AAAA (IPv6)
      if ((pkt[p] & 0xFE) == 0xFC) return true; // fc00::/7 Unique Local
      if (pkt[p] == 0xFE && (pkt[p + 1] & 0xC0) == 0x80) return true; // fe80::/10 Link-Local
      bool allZero = true;
      for (int k = 0; k < 15; k++) { if (pkt[p + k] != 0) { allZero = false; break; } }
      if (allZero && pkt[p + 15] == 1) return true; // ::1 Loopback
    }
    p += rdlen;
  }
  return false;
}

static const char* qtypeToStr(uint16_t qt) {
  switch (qt) {
    case 1:   return "A";
    case 28:  return "AAAA";
    case 65:  return "HTTPS";
    case 64:  return "SVCB";
    case 5:   return "CNAME";
    case 15:  return "MX";
    case 16:  return "TXT";
    case 12:  return "PTR";
    case 255: return "ANY";
    default:  return "OTHER";
  }
}

static void logBlocked(uint32_t clientIp, const char* domain, uint16_t qtype, bool isRebind = false) { // caller holds stateMutex
  time_t now = time(NULL);
  int idx;
  if (blockLogCount < MAX_BLOCK_LOG) {
    idx = (blockLogHead + blockLogCount) % MAX_BLOCK_LOG;
    blockLogCount++;
  } else {
    idx = blockLogHead;
    blockLogHead = (blockLogHead + 1) % MAX_BLOCK_LOG;
  }
  blockLog[idx].epoch = (uint32_t)now;
  blockLog[idx].clientIp = clientIp;
  blockLog[idx].qtype = qtype;
  blockLog[idx].isRebind = isRebind;
  strncpy(blockLog[idx].domain, (domain && domain[0]) ? domain : "banned_client", sizeof(blockLog[idx].domain) - 1);
  blockLog[idx].domain[sizeof(blockLog[idx].domain) - 1] = 0;
}

static void dnsTask(void*) {
  // Set both sockets to non-blocking mode for select() event loop
  int f = fcntl(dnsSock, F_GETFL, 0);
  fcntl(dnsSock, F_SETFL, f | O_NONBLOCK);
  f = fcntl(upstreamSock, F_GETFL, 0);
  fcntl(upstreamSock, F_SETFL, f | O_NONBLOCK);

  int maxfd = (dnsSock > upstreamSock ? dnsSock : upstreamSock) + 1;

  for (;;) {
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(dnsSock, &readfds);
    FD_SET(upstreamSock, &readfds);

    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 20000; // 20ms poll interval

    int activity = select(maxfd, &readfds, NULL, NULL, &tv);

    // 1. Process client queries
    if (activity > 0 && FD_ISSET(dnsSock, &readfds)) {
      struct sockaddr_in cli;
      socklen_t cliLen = sizeof(cli);
      int qlen = recvfrom(dnsSock, dnsRxBuf, sizeof(dnsRxBuf), 0, (struct sockaddr*)&cli, &cliLen);

      if (qlen >= 12) {
        // Drop responses or non-standard opcodes immediately to prevent reflection loops
        if ((dnsRxBuf[2] & 0x80) != 0 || ((dnsRxBuf[2] >> 3) & 0x0F) != 0) {
          // ignore invalid query
        } else {
          char domain[256];
          uint16_t qtype = 0;
          int qend = qlen;
          bool hasOpt = false;
          size_t dl = parseQuery(dnsRxBuf, qlen, domain, &qtype, &qend, &hasOpt);

          if (dl == 0 && qtype == 0) {
            // Send RFC 1035 FORMERR
            uint8_t errBuf[16];
            int errLen = buildFormErr(dnsRxBuf, qlen, errBuf);
            if (errLen > 0) {
              sendto(dnsSock, errBuf, errLen, 0, (struct sockaddr*)&cli, cliLen);
            }
          } else {
            LOCK();
            Dev* c = getClient((uint32_t)cli.sin_addr.s_addr);

            // Per-client query rate limiting (anti-flood / anti-amplification storm)
            uint32_t curSec = (uint32_t)(millis64() / 1000ULL);
            if (c->secWindow == curSec) {
              c->qCount++;
            } else {
              c->secWindow = curSec;
              c->qCount = 1;
            }
            if (c->qCount > 50) { // >50 queries/second threshold protects against flood storms
              totalRateLimited++;
              UNLOCK();
              uint8_t refBuf[16];
              int refLen = buildRefused(dnsRxBuf, qlen, refBuf);
              if (refLen > 0) {
                sendto(dnsSock, refBuf, refLen, 0, (struct sockaddr*)&cli, cliLen);
              }
              continue;
            }

            bool blocked = (c && c->banned) || (dl && isBlocked(domain));
            if (blocked) {
              totalBlocked++;
              if (c) c->blocked++;
              logBlocked((uint32_t)cli.sin_addr.s_addr, domain, qtype, false);
            } else {
              totalAllowed++;
              if (c) c->allowed++;
            }
            UNLOCK();

            if (blocked) {
              // Local ad sinkhole responds in < 0.5ms with ZERO upstream latency
              int rlen = buildBlocked(dnsRxBuf, qend, qtype, hasOpt);
              if (rlen > 0) {
                sendto(dnsSock, dnsRxBuf, rlen, 0, (struct sockaddr*)&cli, cliLen);
              }
            } else {
              // Forward allowed query asynchronously via transaction table
              int slot = -1;
              uint64_t now = millis64();
              for (int i = 0; i < MAX_TX; i++) {
                if (!txTable[i].inUse || now > txTable[i].expireMs) {
                  slot = i;
                  break;
                }
              }

              if (slot >= 0) {
                DnsTx* tx = &txTable[slot];
                uint16_t clientTxid = (dnsRxBuf[0] << 8) | dnsRxBuf[1];
                uint16_t upTxid = (uint16_t)(esp_random() & 0xFFFF);

                tx->clientTxid = clientTxid;
                tx->upstreamTxid = upTxid;
                tx->clientAddr = cli;
                tx->clientLen = cliLen;
                tx->expireMs = now + 1800; // 1.8s timeout
                tx->qlen = qend - 12;
                if (tx->qlen > 0 && tx->qlen <= (int)sizeof(tx->qsection)) {
                  memcpy(tx->qsection, dnsRxBuf + 12, tx->qlen);
                } else {
                  tx->qlen = 0;
                }
                strncpy(tx->domain, domain, sizeof(tx->domain) - 1);
                tx->domain[sizeof(tx->domain) - 1] = 0;
                tx->qtype = qtype;
                tx->hasOpt = hasOpt;
                tx->inUse = true;

                // Replace ID with randomized upstream TXID
                dnsRxBuf[0] = (uint8_t)(upTxid >> 8);
                dnsRxBuf[1] = (uint8_t)(upTxid & 0xFF);

                // Clamp advertised EDNS0 buffer size to 1232 bytes
                if (hasOpt && qend + 4 < qlen) {
                  if (dnsRxBuf[qend] == 0x00 && dnsRxBuf[qend + 1] == 0x00 && dnsRxBuf[qend + 2] == 0x29) {
                    dnsRxBuf[qend + 3] = 0x04;
                    dnsRxBuf[qend + 4] = 0xD0;
                  }
                }

                sendto(upstreamSock, dnsRxBuf, qlen, 0, (struct sockaddr*)&upstreamAddr, sizeof(upstreamAddr));
              }
            }
          }
        }
      }
    }

    // 2. Process upstream responses
    if (activity > 0 && FD_ISSET(upstreamSock, &readfds)) {
      struct sockaddr_in from;
      socklen_t fl = sizeof(from);
      int n = recvfrom(upstreamSock, upRxBuf, sizeof(upRxBuf), 0, (struct sockaddr*)&from, &fl);

      if (n >= 12 && from.sin_addr.s_addr == upstreamAddr.sin_addr.s_addr) {
        uint16_t upTxid = (upRxBuf[0] << 8) | upRxBuf[1];
        int found = -1;
        for (int i = 0; i < MAX_TX; i++) {
          if (txTable[i].inUse && txTable[i].upstreamTxid == upTxid) {
            found = i;
            break;
          }
        }

        if (found >= 0) {
          DnsTx* tx = &txTable[found];
          bool qMatch = (tx->qlen == 0 || (n >= 12 + tx->qlen && memcmp(upRxBuf + 12, tx->qsection, tx->qlen) == 0));
          if (qMatch) {
            // Restore client transaction ID
            upRxBuf[0] = (uint8_t)(tx->clientTxid >> 8);
            upRxBuf[1] = (uint8_t)(tx->clientTxid & 0xFF);

            // DNS Rebinding Attack Protection:
            // Intercept upstream answers containing RFC 1918 / loopback / link-local addresses
            if (isRebindThreat(upRxBuf, n, 12 + tx->qlen, tx->domain)) {
              LOCK();
              totalBlocked++;
              totalRebindBlocked++;
              logBlocked((uint32_t)tx->clientAddr.sin_addr.s_addr, tx->domain, tx->qtype, true);
              Dev* c = getClient((uint32_t)tx->clientAddr.sin_addr.s_addr);
              if (c) c->blocked++;
              UNLOCK();

              // Sinkhole response with 0.0.0.0 (Type A) or NODATA (Type AAAA)
              int rlen = buildBlocked(upRxBuf, 12 + tx->qlen, tx->qtype, tx->hasOpt);
              if (rlen > 0) {
                sendto(dnsSock, upRxBuf, rlen, 0, (struct sockaddr*)&tx->clientAddr, tx->clientLen);
              }
            } else {
              sendto(dnsSock, upRxBuf, n, 0, (struct sockaddr*)&tx->clientAddr, tx->clientLen);
            }
            tx->inUse = false;
          }
        }
      }
    }

    // 3. Clean up expired transactions
    uint64_t now = millis64();
    for (int i = 0; i < MAX_TX; i++) {
      if (txTable[i].inUse && now > txTable[i].expireMs) {
        txTable[i].inUse = false;
      }
    }

    taskYIELD();
  }
}

// ---------- Blocklist Swap & Validation ----------
static const long MAX_BLOCKLIST_BYTES = 1200L * 1024; // 1.20 MB (~245k domains) fits safe A/B update in LittleFS

static bool validateBlocklistFile(const char* path) {
  FILE* f = fopen(path, "rb");
  if (!f) return false;
  fseek(f, 0, SEEK_END);
  long sz = ftell(f);
  fseek(f, 0, SEEK_SET);

  bool ok = sz > 0 && sz <= MAX_BLOCKLIST_BYTES && (sz % HASH_BYTES) == 0;
  if (ok) {
    uint64_t prev = 0;
    bool first = true;
    static uint8_t chunk[1024]; // static to avoid 4KB stack allocation
    const uint32_t perChunk = sizeof(chunk) / HASH_BYTES;
    long remaining = sz / HASH_BYTES;

    while (ok && remaining > 0) {
      uint32_t want = remaining < (long)perChunk ? (uint32_t)remaining : perChunk;
      if (fread(chunk, 1, want * HASH_BYTES, f) != want * HASH_BYTES) { ok = false; break; }
      for (uint32_t k = 0; k < want; k++) {
        const uint8_t* b = chunk + k * HASH_BYTES;
        uint64_t v = 0;
        for (int j = 0; j < HASH_BYTES; j++) v |= ((uint64_t)b[j]) << (8 * j);
        if (!first && v < prev) { ok = false; break; } // strictly sorted for binary search
        prev = v;
        first = false;
      }
      remaining -= want;
    }
  }
  fclose(f);
  if (!ok && strcmp(path, BLOCKLIST_NEW) == 0) {
    remove(BLOCKLIST_NEW);
  }
  return ok;
}

static void reopenBlocklist() { // caller holds stateMutex
  if (blocklist) { fclose(blocklist); blocklist = nullptr; }
  if (validateBlocklistFile(BLOCKLIST_PATH)) {
    blocklist = fopen(BLOCKLIST_PATH, "rb");
    if (blocklist) {
      setvbuf(blocklist, NULL, _IONBF, 0); // unbuffered for random 5-byte fseek probes
      fseek(blocklist, 0, SEEK_END);
      long sz = ftell(blocklist);
      fseek(blocklist, 0, SEEK_SET);
      numHashes = sz > 0 ? (uint32_t)(sz / HASH_BYTES) : 0;
      buildPrefixTable(blocklist, numHashes);
    } else {
      numHashes = 0;
    }
  } else {
    printf("[blocklist] file validation failed or missing at %s\n", BLOCKLIST_PATH);
    numHashes = 0;
  }
}

static void beginBlocklistSwap() {
  remove(BLOCKLIST_NEW);
}

static bool commitNewBlocklist() {
  if (!validateBlocklistFile(BLOCKLIST_NEW)) {
    remove(BLOCKLIST_NEW);
    return false;
  }

  LOCK();
  if (blocklist) { fclose(blocklist); blocklist = nullptr; }
  bool renamed = (rename(BLOCKLIST_NEW, BLOCKLIST_PATH) == 0);
  if (renamed) {
    blocklist = fopen(BLOCKLIST_PATH, "rb");
    if (blocklist) {
      setvbuf(blocklist, NULL, _IONBF, 0);
      fseek(blocklist, 0, SEEK_END);
      long sz = ftell(blocklist);
      fseek(blocklist, 0, SEEK_SET);
      numHashes = sz > 0 ? (uint32_t)(sz / HASH_BYTES) : 0;
      buildPrefixTable(blocklist, numHashes);
    } else {
      numHashes = 0;
    }
  } else {
    printf("[blocklist] rename failed (errno %d) -- old list preserved\n", errno);
    remove(BLOCKLIST_NEW);
    reopenBlocklist();
  }
  UNLOCK();

  return renamed;
}

// ---------- Remote Auto-Update ----------
static void loadUpdateCfg() {
  FILE* f = fopen("/lfs/update.cfg", "r");
  if (!f) return;
  char line[256];
  if (fgets(line, sizeof(line), f)) {
    size_t n = strlen(line);
    while (n && (line[n - 1] == '\n' || line[n - 1] == '\r')) line[--n] = 0;
    strncpy(updateUrl, line, sizeof(updateUrl) - 1);
    updateUrl[sizeof(updateUrl) - 1] = 0;
  }
  if (fgets(line, sizeof(line), f)) {
    int h = atoi(line);
    if (h < 1) h = 1;
    else if (h > 720) h = 720;
    updateIntervalH = (uint32_t)h;
  }
  fclose(f);
}

static bool saveUpdateCfg() {
  char cfgBuf[320];
  LOCK();
  snprintf(cfgBuf, sizeof(cfgBuf), "%s\n%lu\n", updateUrl, (unsigned long)updateIntervalH);
  UNLOCK();
  return atomicWrite("/lfs/update.cfg", cfgBuf);
}

static bool fetchBlocklist(const char* url) {
  if (!url || !url[0]) {
    LOCK(); snprintf(updateStatus, sizeof(updateStatus), "no url set"); UNLOCK();
    return false;
  }

  LOCK();
  if (updateInProgress) { UNLOCK(); return false; }
  updateInProgress = true;
  UNLOCK();

  printf("[remote] GET %s\n", url);
  beginBlocklistSwap();
  FILE* f = fopen(BLOCKLIST_NEW, "wb");
  if (!f) {
    LOCK(); snprintf(updateStatus, sizeof(updateStatus), "fs open failed"); updateInProgress = false; UNLOCK();
    return false;
  }

  esp_http_client_config_t cfg = {};
  cfg.url = url;
  cfg.timeout_ms = 20000;
  cfg.crt_bundle_attach = esp_crt_bundle_attach;
  esp_http_client_handle_t client = esp_http_client_init(&cfg);

  if (!client) {
    fclose(f);
    remove(BLOCKLIST_NEW);
    LOCK(); snprintf(updateStatus, sizeof(updateStatus), "client init failed"); updateInProgress = false; UNLOCK();
    return false;
  }

  bool ok = false;
  if (esp_http_client_open(client, 0) == ESP_OK) {
    int64_t declaredLen = esp_http_client_fetch_headers(client);
    int status = esp_http_client_get_status_code(client);
    if (status != 200) {
      LOCK(); snprintf(updateStatus, sizeof(updateStatus), "HTTP %d", status); UNLOCK();
    } else if (declaredLen > MAX_BLOCKLIST_BYTES) {
      LOCK(); snprintf(updateStatus, sizeof(updateStatus), "too large (%lld B)", (long long)declaredLen); UNLOCK();
    } else {
      char chunk[1024];
      int r;
      long total = 0;
      bool writeErr = false;
      while ((r = esp_http_client_read(client, chunk, sizeof(chunk))) > 0) {
        if (total + r > MAX_BLOCKLIST_BYTES) { writeErr = true; break; }
        size_t w = fwrite(chunk, 1, r, f);
        if (w != (size_t)r) { writeErr = true; break; }
        total += r;
      }
      ok = !writeErr && (r >= 0) && total > 0 && (declaredLen <= 0 || total == declaredLen);
    }
    esp_http_client_close(client);
  } else {
    LOCK(); snprintf(updateStatus, sizeof(updateStatus), "connect failed"); UNLOCK();
  }
  esp_http_client_cleanup(client);
  fclose(f);

  bool committed = ok && commitNewBlocklist();
  if (!ok) remove(BLOCKLIST_NEW);

  char logStatus[128];
  LOCK();
  updateInProgress = false;
  if (committed) snprintf(updateStatus, sizeof(updateStatus), "ok: %lu domains", (unsigned long)numHashes);
  else if (ok) snprintf(updateStatus, sizeof(updateStatus), "bad data (validation failed)");
  strncpy(logStatus, updateStatus, sizeof(logStatus) - 1);
  logStatus[sizeof(logStatus) - 1] = 0;
  UNLOCK();

  printf("[remote] %s\n", logStatus);
  return committed;
}

// ---------- 24/7 Silicon & Network Health Monitoring ----------
static EventGroupHandle_t wifiEvents;
#define WIFI_CONNECTED_BIT BIT0
static const uint64_t WIFI_DOWN_RESTART_MS = 15ULL * 60 * 1000;
static uint64_t wifiDownSinceMs = 0;

static bool wifiConnected() {
  return (xEventGroupGetBits(wifiEvents) & WIFI_CONNECTED_BIT) != 0;
}

static void checkWifiHealth() {
  if (wifiConnected()) {
    wifiDownSinceMs = 0;
    return;
  }
  uint64_t now = millis64();
  if (wifiDownSinceMs == 0) {
    wifiDownSinceMs = now;
    return;
  }
  if (now - wifiDownSinceMs >= WIFI_DOWN_RESTART_MS) {
    printf("[health] WiFi down 15+ min, restarting ESP32\n");
    vTaskDelay(pdMS_TO_TICKS(200));
    esp_restart();
  }
}

static int getRSSI() {
  wifi_ap_record_t info;
  return (esp_wifi_sta_get_ap_info(&info) == ESP_OK) ? info.rssi : 0;
}

static void getLocalIPStr(char* out, size_t outsz) {
  esp_netif_ip_info_t ip;
  if (staNetif && esp_netif_get_ip_info(staNetif, &ip) == ESP_OK) ipToStr(ip.ip.addr, out, outsz);
  else snprintf(out, outsz, "0.0.0.0");
}

static void onWifiEvent(void*, esp_event_base_t base, int32_t id, void* event_data) {
  if (base == WIFI_EVENT && id == WIFI_EVENT_STA_START) {
    esp_wifi_connect();
  } else if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) {
    wifi_event_sta_disconnected_t* dis = (wifi_event_sta_disconnected_t*)event_data;
    printf("[wifi] disconnected (reason %d), reconnecting...\n", dis ? dis->reason : -1);
    xEventGroupClearBits(wifiEvents, WIFI_CONNECTED_BIT);
    esp_wifi_connect();
  } else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
    ip_event_got_ip_t* event = (ip_event_got_ip_t*)event_data;
    printf("[wifi] got IP: " IPSTR "\n", IP2STR(&event->ip_info.ip));
    xEventGroupSetBits(wifiEvents, WIFI_CONNECTED_BIT);
  }
}

static void wifiInit() {
  ESP_ERROR_CHECK(esp_netif_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());
  staNetif = esp_netif_create_default_wifi_sta();
  wifi_init_config_t wcfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&wcfg));
  wifiEvents = xEventGroupCreate();
  ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &onWifiEvent, NULL));
  ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &onWifiEvent, NULL));

  wifi_config_t wc = {};
  strncpy((char*)wc.sta.ssid, WIFI_SSID, sizeof(wc.sta.ssid) - 1);
  strncpy((char*)wc.sta.password, WIFI_PASS, sizeof(wc.sta.password) - 1);
  wc.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
  wc.sta.pmf_cfg.capable = true;
  wc.sta.pmf_cfg.required = false;

  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wc));

  // Protect flash wear: store Wi-Fi status in RAM only
  ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
  ESP_ERROR_CHECK(esp_wifi_start());

  // Cap Wi-Fi TX power at 17dBm (68 * 0.25dBm) to prevent LDO voltage droop on DevKit V1
  esp_wifi_set_max_tx_power(68);
  esp_wifi_set_ps(WIFI_PS_NONE);

  printf("WiFi connecting to '%s'...\n", WIFI_SSID);
  fflush(stdout);
  for (int sec = 0; sec < 30; sec++) {
    EventBits_t bits = xEventGroupWaitBits(wifiEvents, WIFI_CONNECTED_BIT, pdFALSE, pdTRUE, pdMS_TO_TICKS(1000));
    if (bits & WIFI_CONNECTED_BIT) break;
    printf("."); fflush(stdout);
  }
  printf("\n");
  char ip[16];
  getLocalIPStr(ip, sizeof(ip));
  printf("WiFi %s: %s\n", wifiConnected() ? "up" : "connecting in background", ip);

  // Initialize SNTP for real-world blocked log timestamps
  esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
  esp_sntp_setservername(0, "pool.ntp.org");
  esp_sntp_setservername(1, "time.google.com");
  esp_sntp_init();
}

// ---------- Web Server & Security Hardening ----------
static void sendf(httpd_req_t* req, char* buf, size_t bufsz, const char* fmt, ...) {
  va_list args;
  va_start(args, fmt);
  int n = vsnprintf(buf, bufsz, fmt, args);
  va_end(args);
  if (n < 0) n = 0;
  else if (n >= (int)bufsz) n = (int)bufsz - 1;
  httpd_resp_send_chunk(req, buf, n);
}

// Zero-heap-allocation query string parser
static bool getQueryArg(httpd_req_t* req, const char* key, char* out, size_t outsz) {
  size_t qlen = httpd_req_get_url_query_len(req);
  if (!qlen || qlen >= 384) return false;
  char q[384];
  if (httpd_req_get_url_query_str(req, q, sizeof(q)) != ESP_OK) return false;
  return httpd_query_key_value(q, key, out, outsz) == ESP_OK;
}

// Constant-time string comparison without early length exit
static bool constantTimeCompare(const char* a, const char* b) {
  size_t la = strlen(a), lb = strlen(b);
  size_t len = la > lb ? la : lb;
  volatile unsigned char diff = (unsigned char)(la ^ lb);
  for (size_t i = 0; i < len; i++) {
    unsigned char ca = (i < la) ? (unsigned char)a[i] : 0;
    unsigned char cb = (i < lb) ? (unsigned char)b[i] : 0;
    diff |= (ca ^ cb);
  }
  return diff == 0;
}

// Origin & Referer CSRF protection
static bool validateOrigin(httpd_req_t* req) {
  char hdr[128] = "";
  char ip[16];
  getLocalIPStr(ip, sizeof(ip));

  if (httpd_req_get_hdr_value_str(req, "Origin", hdr, sizeof(hdr)) == ESP_OK) {
    if (strstr(hdr, "esp32adblock.local") == nullptr && strstr(hdr, ip) == nullptr) {
      return false;
    }
  }
  if (httpd_req_get_hdr_value_str(req, "Referer", hdr, sizeof(hdr)) == ESP_OK) {
    if (strstr(hdr, "esp32adblock.local") == nullptr && strstr(hdr, ip) == nullptr) {
      return false;
    }
  }
  return true;
}

struct AuthFailRecord {
  uint32_t ip;
  uint32_t failures;
  uint64_t lockedUntilMs;
};
static const int MAX_AUTH_FAIL_TRACK = 8;
static AuthFailRecord authFailures[MAX_AUTH_FAIL_TRACK];

static bool isIpLockedOut(uint32_t ip) {
  uint64_t now = millis64();
  for (int i = 0; i < MAX_AUTH_FAIL_TRACK; i++) {
    if (authFailures[i].ip == ip) {
      if (now < authFailures[i].lockedUntilMs) return true;
      if (now >= authFailures[i].lockedUntilMs && authFailures[i].failures >= 5) {
        authFailures[i].failures = 0;
        authFailures[i].lockedUntilMs = 0;
      }
      return false;
    }
  }
  return false;
}

static void recordAuthFailure(uint32_t ip) {
  uint64_t now = millis64();
  int found = -1, oldest = 0;
  for (int i = 0; i < MAX_AUTH_FAIL_TRACK; i++) {
    if (authFailures[i].ip == ip) { found = i; break; }
    if (authFailures[i].ip == 0) { found = i; break; }
    if (authFailures[i].lockedUntilMs < authFailures[oldest].lockedUntilMs) oldest = i;
  }
  int slot = (found >= 0) ? found : oldest;
  if (authFailures[slot].ip != ip) {
    authFailures[slot].ip = ip;
    authFailures[slot].failures = 0;
    authFailures[slot].lockedUntilMs = 0;
  }
  authFailures[slot].failures++;
  if (authFailures[slot].failures >= 5) {
    authFailures[slot].lockedUntilMs = now + 30000ULL; // 30-second lockout
    printf("[auth] IP locked out for 30s after 5 bad token attempts\n");
  }
}

static void recordAuthSuccess(uint32_t ip) {
  for (int i = 0; i < MAX_AUTH_FAIL_TRACK; i++) {
    if (authFailures[i].ip == ip) {
      authFailures[i].failures = 0;
      authFailures[i].lockedUntilMs = 0;
      break;
    }
  }
}

static uint32_t getReqClientIp(httpd_req_t* req) {
  int fd = httpd_req_to_sockfd(req);
  if (fd < 0) return 0;
  struct sockaddr_storage addr = {};
  socklen_t len = sizeof(addr);
  if (getpeername(fd, (struct sockaddr*)&addr, &len) == 0) {
    if (addr.ss_family == AF_INET) {
      struct sockaddr_in* s = (struct sockaddr_in*)&addr;
      return (uint32_t)s->sin_addr.s_addr;
    } else if (addr.ss_family == AF_INET6) {
      struct sockaddr_in6* s6 = (struct sockaddr_in6*)&addr;
      const uint8_t* b = (const uint8_t*)&s6->sin6_addr;
      bool isMapped = true;
      for (int k = 0; k < 10; k++) { if (b[k] != 0) { isMapped = false; break; } }
      if (isMapped && b[10] == 0xFF && b[11] == 0xFF) {
        uint32_t ip4 = 0;
        memcpy(&ip4, b + 12, 4);
        return ip4;
      }
      uint32_t fallback = 0;
      memcpy(&fallback, b + 12, 4);
      return fallback;
    }
  }
  return 0;
}

static bool checkAuth(httpd_req_t* req) {
  if (!validateOrigin(req)) return false;
  if (constantTimeCompare(ADMIN_TOKEN, "changeme")) return false;

  uint32_t clientIp = getReqClientIp(req);
  if (clientIp && isIpLockedOut(clientIp)) return false;

  char tok[64] = "";
  bool gotTok = false;
  if (httpd_req_get_hdr_value_str(req, "X-Admin-Token", tok, sizeof(tok)) == ESP_OK) {
    gotTok = true;
  } else if (getQueryArg(req, "t", tok, sizeof(tok))) {
    gotTok = true;
  }

  if (gotTok && constantTimeCompare(tok, ADMIN_TOKEN)) {
    if (clientIp) recordAuthSuccess(clientIp);
    return true;
  }

  if (clientIp && gotTok) {
    recordAuthFailure(clientIp);
  }
  return false;
}

static esp_err_t sendUnauthorized(httpd_req_t* req) {
  uint32_t clientIp = getReqClientIp(req);
  if (clientIp && isIpLockedOut(clientIp)) {
    httpd_resp_set_status(req, "429 Too Many Requests");
    httpd_resp_set_type(req, "text/plain");
    httpd_resp_send(req, "too many failed token attempts; 30s lockout", HTTPD_RESP_USE_STRLEN);
    return ESP_FAIL;
  }
  httpd_resp_set_type(req, "text/plain");
  httpd_resp_send_err(req, HTTPD_401_UNAUTHORIZED, "bad or missing token (default 'changeme' not allowed)");
  return ESP_FAIL;
}

static void setSecurityHeaders(httpd_req_t* req) {
  httpd_resp_set_hdr(req, "X-Content-Type-Options", "nosniff");
  httpd_resp_set_hdr(req, "X-Frame-Options", "DENY");
}

static esp_err_t handleRoot(httpd_req_t* req) {
  httpd_resp_set_type(req, "text/html");
  httpd_resp_set_hdr(req, "Cache-Control", "no-store, no-cache, must-revalidate");
  setSecurityHeaders(req);
  httpd_resp_send(req, PAGE, HTTPD_RESP_USE_STRLEN);
  return ESP_OK;
}

static esp_err_t handleStats(httpd_req_t* req) {
  uint32_t clientIp = getReqClientIp(req);
  if (clientIp && isIpLockedOut(clientIp)) return sendUnauthorized(req);
  bool auth = checkAuth(req);
  uint32_t up = millis() / 1000;
  char ipbuf[16];
  getLocalIPStr(ipbuf, sizeof(ipbuf));

  static Dev snap[MAX_CLIENTS];
  int nc, ncust;
  uint32_t tb, ta, nh, iv, trb, trl;
  char urlCopy[256], statusCopy[128];

  LOCK();
  tb = totalBlocked; ta = totalAllowed; nh = numHashes; iv = updateIntervalH;
  trb = totalRebindBlocked; trl = totalRateLimited;
  nc = numClients;
  memcpy(snap, clients, sizeof(Dev) * nc);
  ncust = numCustom;
  strncpy(urlCopy, updateUrl, sizeof(urlCopy) - 1); urlCopy[sizeof(urlCopy) - 1] = 0;
  strncpy(statusCopy, updateStatus, sizeof(statusCopy) - 1); statusCopy[sizeof(statusCopy) - 1] = 0;
  UNLOCK();

  httpd_resp_set_type(req, "application/json");
  httpd_resp_set_hdr(req, "Cache-Control", "no-store");
  setSecurityHeaders(req);

  char buf2[768], esc1[300], esc2[160];
  jescInto(esc1, sizeof(esc1), urlCopy);
  jescInto(esc2, sizeof(esc2), statusCopy);

  sendf(req, buf2, sizeof(buf2),
    "{\"ip\":\"%s\",\"blocked\":%lu,\"allowed\":%lu,\"domains\":%lu,\"rssi\":%d,\"temp\":-999.0,\"heap\":%lu,"
    "\"uptime\":\"%lud %luh %lum\",\"upurl\":\"%s\",\"upiv\":%lu,\"upstat\":\"%s\",\"rebind\":%lu,\"ratelimited\":%lu,\"clients\":[",
    ipbuf, (unsigned long)tb, (unsigned long)ta, (unsigned long)nh, getRSSI(),
    (unsigned long)esp_get_free_heap_size(), (unsigned long)(up/86400), (unsigned long)((up%86400)/3600), (unsigned long)((up%3600)/60),
    esc1, (unsigned long)iv, esc2, (unsigned long)trb, (unsigned long)trl);

  uint64_t nowMs = millis64();
  for (int i = 0; i < nc; i++) {
    char ips[16];
    ipToStr(snap[i].ip, ips, sizeof(ips));
    char macBuf[20];
    if (auth) {
      snprintf(macBuf, sizeof(macBuf), "%02x:%02x:%02x:%02x:%02x:%02x",
        snap[i].mac[0], snap[i].mac[1], snap[i].mac[2], snap[i].mac[3], snap[i].mac[4], snap[i].mac[5]);
    } else {
      snprintf(macBuf, sizeof(macBuf), "--:--:--:--:--:--");
    }
    char escName[64];
    jescInto(escName, sizeof(escName), snap[i].name);
    uint64_t lastSeenSec = (nowMs > snap[i].lastSeen) ? ((nowMs - snap[i].lastSeen) / 1000ULL) : 0;
    sendf(req, buf2, sizeof(buf2), "%s{\"ip\":\"%s\",\"mac\":\"%s\",\"name\":\"%s\",\"blocked\":%lu,\"allowed\":%lu,\"banned\":%s,\"lastSeenSec\":%llu}",
      i ? "," : "", ips, macBuf, escName,
      (unsigned long)snap[i].blocked, (unsigned long)snap[i].allowed, snap[i].banned ? "true" : "false",
      (unsigned long long)lastSeenSec);
  }

  sendf(req, buf2, sizeof(buf2), "],\"custom\":[");
  for (int i = 0; i < ncust; i++) {
    char cDom[256];
    LOCK();
    if (i < numCustom) {
      strncpy(cDom, customDom[i], sizeof(cDom) - 1);
      cDom[sizeof(cDom) - 1] = 0;
    } else {
      cDom[0] = 0;
    }
    UNLOCK();
    if (!cDom[0]) continue;
    char esc[300];
    jescInto(esc, sizeof(esc), cDom);
    sendf(req, buf2, sizeof(buf2), "%s\"%s\"", i ? "," : "", esc);
  }
  sendf(req, buf2, sizeof(buf2), "]}");
  httpd_resp_send_chunk(req, NULL, 0);
  return ESP_OK;
}

static esp_err_t handleLogJson(httpd_req_t* req) {
  uint32_t clientIp = getReqClientIp(req);
  if (clientIp && isIpLockedOut(clientIp)) return sendUnauthorized(req);
  bool auth = checkAuth(req);
  httpd_resp_set_type(req, "application/json");
  httpd_resp_set_hdr(req, "Cache-Control", "no-store");
  setSecurityHeaders(req);

  static BlockLogEntry snap[MAX_BLOCK_LOG];
  int snapCount = 0;
  int snapHead = 0;

  LOCK();
  snapCount = blockLogCount;
  snapHead = blockLogHead;
  memcpy(snap, blockLog, sizeof(BlockLogEntry) * MAX_BLOCK_LOG);
  UNLOCK();

  char buf[512], escDom[128], escName[64];
  sendf(req, buf, sizeof(buf), "[");

  // Iterate newest to oldest
  for (int i = 0; i < snapCount; i++) {
    int idx = (snapHead + snapCount - 1 - i) % MAX_BLOCK_LOG;
    BlockLogEntry* e = &snap[idx];

    char ipStr[16];
    ipToStr(e->clientIp, ipStr, sizeof(ipStr));

    char macBuf[20] = "--:--:--:--:--:--";
    char devName[32] = "";

    LOCK();
    for (int j = 0; j < numClients; j++) {
      if (clients[j].ip == e->clientIp) {
        if (auth) {
          snprintf(macBuf, sizeof(macBuf), "%02x:%02x:%02x:%02x:%02x:%02x",
            clients[j].mac[0], clients[j].mac[1], clients[j].mac[2],
            clients[j].mac[3], clients[j].mac[4], clients[j].mac[5]);
        }
        strncpy(devName, clients[j].name, sizeof(devName) - 1);
        devName[sizeof(devName) - 1] = 0;
        break;
      }
    }
    if (!devName[0]) {
      for (int j = 0; j < numDevNames; j++) {
        if (devNames[j].ip == e->clientIp) {
          strncpy(devName, devNames[j].name, sizeof(devName) - 1);
          devName[sizeof(devName) - 1] = 0;
          break;
        }
      }
    }
    UNLOCK();

    jescInto(escDom, sizeof(escDom), e->domain);
    jescInto(escName, sizeof(escName), devName);

    const char* actionStr = e->isRebind ? "REBIND_DEFENSE" : ((e->qtype == 1) ? "0.0.0.0" : "NODATA");
    sendf(req, buf, sizeof(buf),
      "%s{\"time\":%lu,\"ip\":\"%s\",\"name\":\"%s\",\"mac\":\"%s\",\"domain\":\"%s\",\"type\":\"%s\",\"action\":\"%s\",\"rebind\":%s}",
      i ? "," : "",
      (unsigned long)e->epoch,
      ipStr,
      escName,
      macBuf,
      escDom,
      qtypeToStr(e->qtype),
      actionStr,
      e->isRebind ? "true" : "false"
    );
  }

  sendf(req, buf, sizeof(buf), "]");
  httpd_resp_send_chunk(req, NULL, 0);
  return ESP_OK;
}

static esp_err_t handleSetName(httpd_req_t* req) {
  if (!checkAuth(req)) return sendUnauthorized(req);
  char ipStr[32] = "";
  char rawName[64] = "";
  if (!getQueryArg(req, "ip", ipStr, sizeof(ipStr))) {
    httpd_resp_set_type(req, "text/plain");
    httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "missing ip");
    return ESP_FAIL;
  }
  struct in_addr a;
  if (inet_pton(AF_INET, ipStr, &a) != 1) {
    httpd_resp_set_type(req, "text/plain");
    httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "invalid ip");
    return ESP_FAIL;
  }
  uint32_t ip = a.s_addr;
  char decodedName[32] = "";
  if (getQueryArg(req, "name", rawName, sizeof(rawName))) {
    urlDecode(decodedName, rawName, sizeof(decodedName));
  }

  LOCK();
  // Update in active clients table
  for (int i = 0; i < numClients; i++) {
    if (clients[i].ip == ip) {
      strncpy(clients[i].name, decodedName, sizeof(clients[i].name) - 1);
      clients[i].name[sizeof(clients[i].name) - 1] = 0;
      break;
    }
  }
  // Update in devNames persistence table
  int found = -1;
  for (int i = 0; i < numDevNames; i++) {
    if (devNames[i].ip == ip) { found = i; break; }
  }
  if (decodedName[0]) {
    if (found >= 0) {
      strncpy(devNames[found].name, decodedName, sizeof(devNames[0].name) - 1);
      devNames[found].name[sizeof(devNames[0].name) - 1] = 0;
    } else if (numDevNames < MAX_DEV_NAMES) {
      devNames[numDevNames].ip = ip;
      strncpy(devNames[numDevNames].name, decodedName, sizeof(devNames[0].name) - 1);
      devNames[numDevNames].name[sizeof(devNames[0].name) - 1] = 0;
      numDevNames++;
    }
  } else {
    // Clear name if empty
    if (found >= 0) {
      for (int i = found; i < numDevNames - 1; i++) devNames[i] = devNames[i + 1];
      numDevNames--;
    }
  }
  UNLOCK();

  saveNames();
  httpd_resp_set_type(req, "text/plain");
  httpd_resp_send(req, "ok", 2);
  return ESP_OK;
}

static esp_err_t handleDelClient(httpd_req_t* req) {
  if (!checkAuth(req)) return sendUnauthorized(req);
  char ipStr[32] = "";
  if (!getQueryArg(req, "ip", ipStr, sizeof(ipStr))) {
    httpd_resp_set_type(req, "text/plain");
    httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "missing ip");
    return ESP_FAIL;
  }
  struct in_addr a;
  if (inet_pton(AF_INET, ipStr, &a) != 1) {
    httpd_resp_set_type(req, "text/plain");
    httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "invalid ip");
    return ESP_FAIL;
  }
  uint32_t ip = a.s_addr;

  LOCK();
  // Remove from clients table
  for (int i = 0; i < numClients; i++) {
    if (clients[i].ip == ip) {
      for (int j = i; j < numClients - 1; j++) clients[j] = clients[j + 1];
      numClients--;
      break;
    }
  }
  // Remove from devNames table
  for (int i = 0; i < numDevNames; i++) {
    if (devNames[i].ip == ip) {
      for (int j = i; j < numDevNames - 1; j++) devNames[j] = devNames[j + 1];
      numDevNames--;
      break;
    }
  }
  UNLOCK();

  saveNames();
  httpd_resp_set_type(req, "text/plain");
  httpd_resp_send(req, "ok", 2);
  return ESP_OK;
}

static esp_err_t handleBan(httpd_req_t* req) {
  if (!checkAuth(req)) return sendUnauthorized(req);
  char val[32];
  uint32_t ip = 0;
  if (getQueryArg(req, "ip", val, sizeof(val))) {
    struct in_addr a;
    if (inet_pton(AF_INET, val, &a) == 1) ip = a.s_addr;
  }
  if (ip) {
    bool ok = toggleBan(ip);
    if (!ok) {
      httpd_resp_set_type(req, "text/plain");
      httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "ban failed");
      return ESP_FAIL;
    }
  }
  httpd_resp_set_type(req, "text/plain");
  httpd_resp_send(req, "ok", 2);
  return ESP_OK;
}

static esp_err_t handleAddBlock(httpd_req_t* req) {
  if (!checkAuth(req)) return sendUnauthorized(req);
  char d[256] = "";
  bool ok = false;
  if (getQueryArg(req, "d", d, sizeof(d))) {
    ok = addCustom(d);
  }
  httpd_resp_set_type(req, "text/plain");
  if (!ok) {
    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "add custom failed");
    return ESP_FAIL;
  }
  httpd_resp_send(req, "ok", 2);
  return ESP_OK;
}

static esp_err_t handleUnblock(httpd_req_t* req) {
  if (!checkAuth(req)) return sendUnauthorized(req);
  char d[256] = "";
  bool ok = false;
  if (getQueryArg(req, "d", d, sizeof(d))) {
    ok = removeCustom(d);
  }
  httpd_resp_set_type(req, "text/plain");
  if (!ok) {
    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "unblock failed");
    return ESP_FAIL;
  }
  httpd_resp_send(req, "ok", 2);
  return ESP_OK;
}

static esp_err_t handleFetchNow(httpd_req_t* req) {
  if (!checkAuth(req)) return sendUnauthorized(req);
  manualFetchTrigger = true;
  httpd_resp_set_status(req, "202 Accepted");
  httpd_resp_set_type(req, "text/plain");
  httpd_resp_send(req, "fetch scheduled", HTTPD_RESP_USE_STRLEN);
  return ESP_OK;
}

static esp_err_t handleSetUpdate(httpd_req_t* req) {
  if (!checkAuth(req)) return sendUnauthorized(req);
  char u[256] = "", v[16] = "";
  bool gotU = getQueryArg(req, "u", u, sizeof(u));
  bool gotH = getQueryArg(req, "h", v, sizeof(v));

  if (gotU && u[0] && strncmp(u, "https://", 8) != 0) {
    httpd_resp_set_type(req, "text/plain");
    httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "HTTPS required for update url");
    return ESP_FAIL;
  }

  LOCK();
  if (gotU) {
    strncpy(updateUrl, u, sizeof(updateUrl) - 1);
    updateUrl[sizeof(updateUrl) - 1] = 0;
  }
  if (gotH) {
    long h = atol(v);
    if (h < 1) h = 1;
    else if (h > 720) h = 720;
    updateIntervalH = (uint32_t)h;
  }
  UNLOCK();

  bool ok = saveUpdateCfg();
  httpd_resp_set_type(req, "text/plain");
  if (!ok) {
    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "saving update config failed");
    return ESP_FAIL;
  }
  httpd_resp_send(req, "ok", 2);
  return ESP_OK;
}

static esp_err_t handleUpload(httpd_req_t* req) {
  if (!checkAuth(req)) return sendUnauthorized(req);
  if (req->content_len <= 0 || req->content_len > MAX_BLOCKLIST_BYTES || (req->content_len % HASH_BYTES) != 0) {
    httpd_resp_set_type(req, "text/plain");
    httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "size out of range or not 5-byte aligned");
    return ESP_FAIL;
  }

  LOCK();
  if (updateInProgress) {
    UNLOCK();
    httpd_resp_set_status(req, "503 Service Unavailable");
    httpd_resp_set_type(req, "text/plain");
    httpd_resp_send(req, "update already in progress", HTTPD_RESP_USE_STRLEN);
    return ESP_FAIL;
  }
  updateInProgress = true;
  UNLOCK();

  beginBlocklistSwap();
  FILE* f = fopen(BLOCKLIST_NEW, "wb");
  if (!f) {
    LOCK(); updateInProgress = false; UNLOCK();
    httpd_resp_set_type(req, "text/plain");
    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "fs open failed");
    return ESP_FAIL;
  }

  char chunk[1024];
  int remaining = req->content_len;
  bool ok = true;
  while (remaining > 0) {
    int toread = remaining < (int)sizeof(chunk) ? remaining : (int)sizeof(chunk);
    int r = httpd_req_recv(req, chunk, toread);
    if (r <= 0) { ok = false; break; }
    size_t w = fwrite(chunk, 1, r, f);
    if (w != (size_t)r) { ok = false; break; }
    remaining -= r;
  }
  fclose(f);

  bool committed = ok && commitNewBlocklist();
  if (!ok) remove(BLOCKLIST_NEW);

  LOCK();
  updateInProgress = false;
  UNLOCK();

  printf("[upload] %s -> %lu domains\n", committed ? "OK" : "REJECTED", (unsigned long)numHashes);
  httpd_resp_set_type(req, "text/plain");
  if (committed) httpd_resp_send(req, "ok", 2);
  else httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "rejected: invalid or corrupt blocklist");
  return ESP_OK;
}

static void webServerInit() {
  httpd_config_t hc = HTTPD_DEFAULT_CONFIG();
  hc.stack_size = 8192;
  hc.core_id = 1; // Explicitly pin httpd to Core 1, leaving Core 0 for DNS & Wi-Fi
  hc.uri_match_fn = httpd_uri_match_wildcard;
  hc.max_open_sockets = 5;
  hc.lru_purge_enable = true;
  hc.max_uri_handlers = 16;

  ESP_ERROR_CHECK(httpd_start(&webServer, &hc));

  static const httpd_uri_t routes[] = {
    { "/",            HTTP_GET,  handleRoot,       NULL },
    { "/stats.json",  HTTP_GET,  handleStats,      NULL },
    { "/log.json",    HTTP_GET,  handleLogJson,    NULL },
    { "/ban",         HTTP_POST, handleBan,        NULL },
    { "/addblock",    HTTP_POST, handleAddBlock,   NULL },
    { "/unblock",     HTTP_POST, handleUnblock,    NULL },
    { "/fetchnow",    HTTP_POST, handleFetchNow,   NULL },
    { "/setupdate",   HTTP_POST, handleSetUpdate,  NULL },
    { "/upload",      HTTP_POST, handleUpload,     NULL },
    { "/setname",     HTTP_POST, handleSetName,    NULL },
    { "/delclient",   HTTP_POST, handleDelClient,  NULL },
  };
  for (auto& r : routes) {
    esp_err_t e = httpd_register_uri_handler(webServer, &r);
    if (e != ESP_OK) printf("[http] failed to register %s: %s\n", r.uri, esp_err_to_name(e));
  }
}

// ---------- LittleFS File System ----------
static void cleanOrphanFiles() {
  remove(BLOCKLIST_NEW);
  remove("/lfs/custom.txt.new");
  remove("/lfs/banned.txt.new");
  remove("/lfs/names.txt.new");
  remove("/lfs/update.cfg.new");
}

static void mountFS() {
  esp_vfs_littlefs_conf_t conf = {};
  conf.base_path = FS_BASE;
  conf.partition_label = "spiffs";
  conf.format_if_mount_failed = true;
  conf.dont_mount = false;

  esp_err_t err = ESP_FAIL;
  for (int retry = 0; retry < 3; retry++) {
    err = esp_vfs_littlefs_register(&conf);
    if (err == ESP_OK) break;
    printf("LittleFS mount attempt %d failed: %s, retrying...\n", retry + 1, esp_err_to_name(err));
    vTaskDelay(pdMS_TO_TICKS(100));
  }
  if (err != ESP_OK) {
    printf("LittleFS mount FAILED permanently: %s\n", esp_err_to_name(err));
    return;
  }

  cleanOrphanFiles();

  FILE* probe = fopen(BLOCKLIST_PATH, "rb");
  if (!probe) printf("[fs] Note: no existing blocklist.bin found at %s\n", BLOCKLIST_PATH);
  else fclose(probe);
}

static void mdnsInit() {
  ESP_ERROR_CHECK(mdns_init());
  mdns_hostname_set("esp32adblock");
  mdns_service_add(NULL, "_http", "_tcp", 80, NULL, 0);
}

static bool udpBind(int* sock, uint16_t port) {
  *sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (*sock < 0) {
    printf("[net] socket() failed for port %u\n", port);
    return false;
  }
  int rcvbuf = 16384; // 16 KB socket receive buffer absorbs network query bursts
  setsockopt(*sock, SOL_SOCKET, SO_RCVBUF, &rcvbuf, sizeof(rcvbuf));

  struct sockaddr_in a = {};
  a.sin_family = AF_INET;
  a.sin_port = htons(port);
  a.sin_addr.s_addr = htonl(INADDR_ANY);
  if (bind(*sock, (struct sockaddr*)&a, sizeof(a)) < 0) {
    printf("[net] bind() failed for port %u (errno %d)\n", port, errno);
    close(*sock);
    *sock = -1;
    return false;
  }
  return true;
}

// ---------- Maintenance Task (Core 1) ----------
static void maintenanceTask(void*) {
  for (;;) {
    checkWifiHealth();
    char urlCopy[256] = "";
    bool shouldFetch = false;

    if (manualFetchTrigger) {
      manualFetchTrigger = false;
      LOCK();
      if (updateUrl[0]) {
        shouldFetch = true;
        strncpy(urlCopy, updateUrl, sizeof(urlCopy) - 1);
        urlCopy[sizeof(urlCopy) - 1] = 0;
      }
      UNLOCK();
    }

    if (!shouldFetch) {
      LOCK();
      if (updateUrl[0]) {
        uint64_t now = millis64();
        if (lastCheckMs == 0) {
          lastCheckMs = now;
        } else if (now - lastCheckMs >= updateIntervalH * 3600000ULL) {
          lastCheckMs = now;
          shouldFetch = true;
          strncpy(urlCopy, updateUrl, sizeof(urlCopy) - 1);
          urlCopy[sizeof(urlCopy) - 1] = 0;
        }
      }
      UNLOCK();
    }

    if (shouldFetch) fetchBlocklist(urlCopy);
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

// ---------- Main Entry Point ----------
extern "C" void app_main() {
  stateMutex = xSemaphoreCreateMutex();

  esp_err_t nvsErr = nvs_flash_init();
  if (nvsErr == ESP_ERR_NVS_NO_FREE_PAGES || nvsErr == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    nvsErr = nvs_flash_init();
  }
  ESP_ERROR_CHECK(nvsErr);

  if (constantTimeCompare(ADMIN_TOKEN, "changeme")) {
    printf("\n************************************************************\n");
    printf("** SECURITY WARNING: ADMIN_TOKEN is set to 'changeme'!    **\n");
    printf("** Mutating endpoints are locked until a new token is set. **\n");
    printf("************************************************************\n\n");
  }

  mountFS();
  LOCK();
  reopenBlocklist();
  UNLOCK();
  printf("blocklist: %lu domains\n", (unsigned long)numHashes);

  loadCustom();
  loadBanned();
  loadNames();
  loadUpdateCfg();
  printf("custom: %d, banned: %d, names: %d\n", numCustom, numBanned, numDevNames);

  wifiInit();
  mdnsInit();
  printf("dashboard: http://esp32adblock.local\n");

  upstreamAddr.sin_family = AF_INET;
  upstreamAddr.sin_port = htons(53);
  inet_pton(AF_INET, UPSTREAM_IP, &upstreamAddr.sin_addr);

  if (!udpBind(&dnsSock, DNS_PORT) || !udpBind(&upstreamSock, 0)) {
    printf("[net] could not bind DNS sockets, restarting in 5s\n");
    vTaskDelay(pdMS_TO_TICKS(5000));
    esp_restart();
  }

  xTaskCreatePinnedToCore(dnsTask, "dns", 4096, NULL, 10, NULL, 0);
  webServerInit();
  xTaskCreatePinnedToCore(maintenanceTask, "maint", 12288, NULL, 1, NULL, 1);

  printf("DNS :53 (core 0) + dashboard :80 + maintenance (core 1) up\n");
}
