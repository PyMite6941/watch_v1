// The configuration link: one JSON command per line in, one JSON reply per
// line out. The same commands work over
//   - USB:       the SuperMini's USB-C port shows up as a serial port (115200)
//   - Bluetooth: Nordic UART service, while the watch is in link mode
// The full command list is in docs/CONFIGURATION.md; tools/configurator is a
// ready-made web page that speaks it.
#pragma once
#include <ArduinoJson.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include "types.h"
#include "settings.h"
#include "timesource.h"
#include "mesh.h"
#include "env.h"

#define NUS_SERVICE "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define NUS_RX      "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"   // computer -> watch
#define NUS_TX      "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"   // watch -> computer

static bool rebootWanted = false;

// ------------------------------------------------------------------ commands
// `trusted` is false for Bluetooth until the PIN (if one is set) has been given.
static String linkHandle(const String &line, bool &trusted) {
  JsonDocument in, out;
  out["ok"] = false;
  String reply;
  if (deserializeJson(in, line)) {
    out["err"] = "bad json";
    serializeJson(out, reply);
    return reply;
  }
  const char *cmd = in["cmd"] | "";
  if (in["id"].is<JsonVariantConst>()) out["id"] = in["id"];
  keepAwake();

  if (!strcmp(cmd, "auth")) {
    trusted = trusted || !strcmp(in["pin"] | "", settings.pin);
    out["ok"] = trusted;
    if (!trusted) out["err"] = "wrong pin";
  } else if (!strcmp(cmd, "info")) {
    uint16_t mv = batteryMilliVolts();
    out["ok"] = true;
    out["fw"] = FW_VERSION;
    out["name"] = settings.name;
    out["mac"] = WiFi.macAddress();
    out["battery_mv"] = mv;
    out["battery_pct"] = batteryPercent(mv);
    out["charging"] = isCharging();
    out["rtc"] = rtcPresent;
    out["time"] = (uint32_t)time(nullptr);
    out["time_valid"] = timeValid();
    out["last_sync"] = (uint32_t)lastSyncEpoch;
    out["usb"] = usbConnected();
    out["peers"] = meshPeerCount();
    out["locked"] = !trusted;
  } else if (!trusted) {
    out["err"] = "locked: send {\"cmd\":\"auth\",\"pin\":\"...\"} first";
  } else if (!strcmp(cmd, "get")) {
    out["ok"] = true;
    settingsToJson(out["cfg"].to<JsonObject>(), false);
  } else if (!strcmp(cmd, "set")) {
    if (settingsFromJson(in["cfg"].as<JsonObjectConst>(), false)) {
      settingsSave();
      setenv("TZ", settings.tz, 1);
      tzset();
      logNextEpoch = settings.log_min ? time(nullptr) + (time_t)settings.log_min * 60 : 0;
      out["ok"] = true;
    } else {
      out["err"] = "no known settings in cfg";
    }
  } else if (!strcmp(cmd, "wifi_add")) {
    if (wifiAdd(in["ssid"] | "", in["pass"] | "")) {
      settingsSave();
      out["ok"] = true;
    } else {
      out["err"] = "need ssid, and at most " + String(MAX_WIFI) + " networks";
    }
  } else if (!strcmp(cmd, "wifi_del")) {
    out["ok"] = wifiDel(in["ssid"] | "");
    if (out["ok"]) settingsSave(); else out["err"] = "no such network";
  } else if (!strcmp(cmd, "wifi_scan")) {
    meshStop();
    WiFi.mode(WIFI_STA);
    int n = WiFi.scanNetworks();
    JsonArray nets = out["networks"].to<JsonArray>();
    for (int i = 0; i < n && i < 20; i++) {
      JsonObject o = nets.add<JsonObject>();
      o["ssid"] = WiFi.SSID(i);
      o["rssi"] = WiFi.RSSI(i);
      o["open"] = WiFi.encryptionType(i) == WIFI_AUTH_OPEN;
    }
    WiFi.scanDelete();
    wifiOff();
    out["ok"] = true;
  } else if (!strcmp(cmd, "sync")) {
    meshStop();
    showText("SYnC", 20000);
    out["ok"] = timeSync();
    showText(out["ok"] ? "  On" : " Err", 800);
    if (!out["ok"]) out["err"] = "no saved network in range, or the time server did not answer";
  } else if (!strcmp(cmd, "time")) {
    uint32_t epoch = in["epoch"] | 0u;
    if (epoch > 1735689600u) {
      setClock(epoch);
      out["ok"] = true;
    } else {
      out["err"] = "need epoch (Unix seconds, UTC)";
    }
  } else if (!strcmp(cmd, "buzz")) {
    buzz(constrain((int)(in["ms"] | 200), 10, 2000));
    out["ok"] = true;
  } else if (!strcmp(cmd, "show")) {
    showText(in["text"] | "", constrain((int)(in["ms"] | 2000), 100, 30000));
    out["ok"] = true;
  } else if (!strcmp(cmd, "mesh_send")) {
    const char *text = in["text"] | "";
    out["ok"] = meshSend(text[0] ? MESH_TEXT : MESH_BUZZ, text);
    if (!out["ok"]) out["err"] = "mesh is off (set \"mesh\": true)";
  } else if (!strcmp(cmd, "env")) {
    EnvReading r = envRead();
    out["ok"] = r.ok;
    if (r.ok) {
      out["temp_c"] = serialized(String(r.tempC, 2));
      if (r.hasHumidity) out["humidity"] = serialized(String(r.humidity, 1));
      out["hpa"] = serialized(String(r.hPa, 1));
    } else {
      out["err"] = "no BME280 / BMP280 on the accessory socket";
    }
    out["light_mv"] = analogReadMilliVolts(PIN_LDR_SENSE);
  } else if (!strcmp(cmd, "log_get")) {
    // Paged, oldest first: ask again with "from" = from + rows returned until you have "total".
    uint16_t from = in["from"] | 0;
    out["ok"] = true;
    out["total"] = logCount;
    out["from"] = from;
    JsonArray rows = out["rows"].to<JsonArray>();      // [unix time, C, %RH, hPa, light mV]; null = no reading
    for (uint16_t i = from; i < logCount && i < from + 40; i++) {
      const LogRec &r = logAt(i);
      JsonArray row = rows.add<JsonArray>();
      row.add(r.t);
      if (r.temp10 == LOG_NONE) row.add(nullptr); else row.add(serialized(String(r.temp10 / 10.0, 1)));
      if (r.hum10 == LOG_NONE) row.add(nullptr); else row.add(serialized(String(r.hum10 / 10.0, 1)));
      if (r.hpa10 == LOG_NONE) row.add(nullptr); else row.add(serialized(String(500.0 + r.hpa10 / 10.0, 1)));
      row.add(r.lightMv);
    }
  } else if (!strcmp(cmd, "log_clear")) {
    logClear();
    out["ok"] = true;
  } else if (!strcmp(cmd, "reboot")) {
    rebootWanted = true;
    out["ok"] = true;
  } else if (!strcmp(cmd, "factory_reset")) {
    settingsErase();
    rebootWanted = true;
    out["ok"] = true;
  } else {
    out["err"] = "unknown cmd";
  }
  serializeJson(out, reply);
  return reply;
}

// ----------------------------------------------------------------------- USB
static String usbLine;
static bool   usbTrusted = true;        // whoever holds the cable holds the watch

static void usbLoop() {
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') {
      if (usbLine.length() && usbLine[0] == '{') Serial.println(linkHandle(usbLine, usbTrusted));
      usbLine = "";
    } else if (usbLine.length() < 1024) {
      usbLine += c;
    }
  }
}

// ----------------------------------------------------------------- Bluetooth
static BLECharacteristic *bleTx = nullptr;
static bool          bleRunning = false, bleConnected = false, bleTrusted = false;
static String        bleBuf, blePending;
static volatile bool bleHasLine = false;
static portMUX_TYPE  bleLock = portMUX_INITIALIZER_UNLOCKED;

class LinkServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer *) override {
    bleConnected = true;
    bleTrusted = settings.pin[0] == 0;
  }
  void onDisconnect(BLEServer *server) override {
    bleConnected = false;
    bleTrusted = false;
    server->startAdvertising();
  }
};

class LinkRxCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *c) override {
    String chunk = c->getValue();
    for (size_t i = 0; i < chunk.length(); i++) {
      char ch = chunk[i];
      if (ch == '\n' || ch == '\r') {
        if (bleBuf.length()) {
          portENTER_CRITICAL(&bleLock);
          blePending = bleBuf;
          bleHasLine = true;
          portEXIT_CRITICAL(&bleLock);
        }
        bleBuf = "";
      } else if (bleBuf.length() < 1024) {
        bleBuf += ch;
      }
    }
  }
};

static void bleStart() {
  if (bleRunning) return;
  String name = String("Watch-") + settings.name;
  BLEDevice::init(name.c_str());
  BLEServer *server = BLEDevice::createServer();
  server->setCallbacks(new LinkServerCallbacks());
  BLEService *service = server->createService(NUS_SERVICE);
  bleTx = service->createCharacteristic(NUS_TX, BLECharacteristic::PROPERTY_NOTIFY);
  bleTx->addDescriptor(new BLE2902());
  BLECharacteristic *rx = service->createCharacteristic(
      NUS_RX, BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_WRITE_NR);
  rx->setCallbacks(new LinkRxCallbacks());
  service->start();
  BLEAdvertising *adv = BLEDevice::getAdvertising();
  adv->addServiceUUID(NUS_SERVICE);
  adv->setScanResponse(true);
  BLEDevice::startAdvertising();
  bleRunning = true;
}

static void bleLoop() {
  if (!bleRunning || !bleHasLine) return;
  portENTER_CRITICAL(&bleLock);
  String line = blePending;
  bleHasLine = false;
  portEXIT_CRITICAL(&bleLock);
  String reply = linkHandle(line, bleTrusted) + "\n";
  // Notifications carry 20 bytes unless a bigger MTU was negotiated; stay safe.
  for (size_t i = 0; i < reply.length() && bleConnected; i += 20) {
    String part = reply.substring(i, i + 20);
    bleTx->setValue((uint8_t *)part.c_str(), part.length());
    bleTx->notify();
    delay(8);
  }
}
