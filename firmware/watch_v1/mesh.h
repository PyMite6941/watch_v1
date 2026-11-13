// Watch-to-watch messaging with ESP-NOW (no router, no pairing).
//
// Every watch with the same "mesh_group" and "mesh_ch" settings hears the
// others' broadcasts, up to roughly a classroom or two away. It is used for:
//   - presence:  the mesh screen shows how many group members are in range
//   - buzz:      SET on the mesh screen vibrates everyone else's watch
//   - time:      a watch with no Wi-Fi picks the time up from one that has it
//
// The radio is only on while the mesh screen is showing, in link mode, or
// while plugged into a computer - a watch asleep in deep sleep hears nothing.
// The group name keeps strangers' packets out; it is not encryption.
#pragma once
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include "types.h"

enum MeshType : uint8_t { MESH_HELLO = 0, MESH_BUZZ = 1, MESH_TEXT = 2 };

struct __attribute__((packed)) MeshPacket {
  char     magic[2];      // "W1"
  uint16_t group;         // hash of the group name
  uint8_t  type;
  char     name[12];      // sender's watch name
  uint32_t epoch;         // sender's time (0 = it does not know)
  char     text[5];       // MESH_TEXT: four characters to show
};

#define MESH_MAX_PEERS 12
#define MESH_PEER_TTL_MS 30000
#define MESH_HELLO_MS 5000

struct MeshPeer { uint8_t mac[6]; uint32_t seen; };
static MeshPeer meshPeers[MESH_MAX_PEERS];
static bool     meshOn = false;
static uint32_t meshLastHello = 0;
static const uint8_t MESH_BROADCAST[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

// Filled in by the radio task, consumed by meshLoop().
static volatile bool meshGot = false;
static MeshPacket    meshIn;
static portMUX_TYPE  meshLock = portMUX_INITIALIZER_UNLOCKED;

static uint16_t meshGroupHash() {
  uint32_t h = 2166136261u;                        // FNV-1a
  for (const char *p = settings.mesh_group; *p; p++) h = (h ^ (uint8_t)*p) * 16777619u;
  return (uint16_t)(h ^ (h >> 16));
}

static void meshOnReceive(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
  if (len != sizeof(MeshPacket)) return;
  const MeshPacket *p = (const MeshPacket *)data;
  if (p->magic[0] != 'W' || p->magic[1] != '1' || p->group != meshGroupHash()) return;
  uint32_t now = millis();
  int slot = -1;
  for (int i = 0; i < MESH_MAX_PEERS; i++) {
    if (memcmp(meshPeers[i].mac, info->src_addr, 6) == 0) { slot = i; break; }
    if (slot < 0 && (meshPeers[i].seen == 0 || now - meshPeers[i].seen > MESH_PEER_TTL_MS)) slot = i;
  }
  if (slot >= 0) {
    memcpy(meshPeers[slot].mac, info->src_addr, 6);
    meshPeers[slot].seen = now ? now : 1;
  }
  portENTER_CRITICAL(&meshLock);
  if (!meshGot || p->type != MESH_HELLO) {         // a hello never overwrites a waiting buzz
    meshIn = *p;
    meshGot = true;
  }
  portEXIT_CRITICAL(&meshLock);
}

static uint8_t meshPeerCount() {
  uint8_t n = 0;
  uint32_t now = millis();
  for (int i = 0; i < MESH_MAX_PEERS; i++)
    if (meshPeers[i].seen && now - meshPeers[i].seen <= MESH_PEER_TTL_MS) n++;
  return n;
}

static bool meshSend(MeshType type, const char *text = "") {
  if (!meshOn) return false;
  MeshPacket p = {};
  p.magic[0] = 'W';
  p.magic[1] = '1';
  p.group = meshGroupHash();
  p.type = type;
  strlcpy(p.name, settings.name, sizeof(p.name));
  p.epoch = timeValid() ? (uint32_t)time(nullptr) : 0;
  strlcpy(p.text, text, sizeof(p.text));
  return esp_now_send(MESH_BROADCAST, (const uint8_t *)&p, sizeof(p)) == ESP_OK;
}

static void meshStart() {
  if (meshOn) return;
  WiFi.mode(WIFI_STA);
  esp_wifi_set_channel(settings.mesh_ch, WIFI_SECOND_CHAN_NONE);
  if (esp_now_init() != ESP_OK) return;
  esp_now_register_recv_cb(meshOnReceive);
  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, MESH_BROADCAST, 6);
  peer.channel = 0;
  peer.ifidx = WIFI_IF_STA;
  esp_now_add_peer(&peer);
  meshOn = true;
  meshLastHello = 0;
}

static void meshStop() {
  if (!meshOn) return;
  esp_now_deinit();
  WiFi.mode(WIFI_OFF);
  meshOn = false;
}

// Call often. `wanted` = should the radio be on right now.
static void meshLoop(bool wanted) {
  if (wanted && settings.mesh) meshStart(); else meshStop();
  if (!meshOn) return;
  uint32_t now = millis();
  if (meshLastHello == 0 || now - meshLastHello > MESH_HELLO_MS) {
    meshLastHello = now ? now : 1;
    meshSend(MESH_HELLO);
  }
  if (!meshGot) return;
  portENTER_CRITICAL(&meshLock);
  MeshPacket p = meshIn;
  meshGot = false;
  portEXIT_CRITICAL(&meshLock);
  p.name[sizeof(p.name) - 1] = 0;
  p.text[sizeof(p.text) - 1] = 0;
  if (!timeValid() && p.epoch > 1735689600u) setClock(p.epoch);   // borrow the time
  if (p.type == MESH_BUZZ) {
    keepAwake();
    buzz(350);
    showText(p.name, 1500);
  } else if (p.type == MESH_TEXT) {
    keepAwake();
    buzz(120);
    showText(p.text, 2500);
  }
}
