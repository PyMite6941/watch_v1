// Environment pack: a BME280 (temperature, humidity, air pressure) on the I2C
// bus, plus a small log of samples for experiments.
//
// The sensor is optional. Plug a BME280 breakout into the accessory socket
// (GND, 3V3, SDA, SCL) and the temp / humidity / pressure screens come alive;
// a BMP280 also works but has no humidity. No library needed: this is the
// compensation maths from the Bosch datasheet.
//
// The log lives in RTC memory: it survives deep sleep, not a flat battery.
// Read it out with the "log_get" command (Watch Studio saves it as CSV).
#pragma once
#include <Wire.h>
#include "types.h"

struct EnvReading {
  bool  ok;
  bool  hasHumidity;
  float tempC, humidity, hPa;
};

static uint8_t envAddr = 0;          // 0 = no sensor found
static bool    envHasHumidity = false;
static struct {
  uint16_t T1; int16_t T2, T3;
  uint16_t P1; int16_t P2, P3, P4, P5, P6, P7, P8, P9;
  uint8_t  H1; int16_t H2; uint8_t H3; int16_t H4, H5; int8_t H6;
} envCal;

static bool envReadReg(uint8_t reg, uint8_t *buf, uint8_t len) {
  Wire.beginTransmission(envAddr);
  Wire.write(reg);
  if (Wire.endTransmission() != 0) return false;
  if (Wire.requestFrom(envAddr, len) != len) return false;
  for (uint8_t i = 0; i < len; i++) buf[i] = Wire.read();
  return true;
}

static void envWriteReg(uint8_t reg, uint8_t value) {
  Wire.beginTransmission(envAddr);
  Wire.write(reg);
  Wire.write(value);
  Wire.endTransmission();
}

// Looks for the sensor at both of its possible addresses and loads its calibration.
static bool envBegin() {
  for (uint8_t addr : { (uint8_t)0x76, (uint8_t)0x77 }) {
    envAddr = addr;
    uint8_t id = 0;
    if (!envReadReg(0xD0, &id, 1) || (id != 0x60 && id != 0x58)) continue;   // 0x60 BME280, 0x58 BMP280
    envHasHumidity = id == 0x60;
    uint8_t b[26];
    if (!envReadReg(0x88, b, 26)) continue;
#define U16(i) ((uint16_t)(b[i] | (b[i + 1] << 8)))
    envCal.T1 = U16(0);  envCal.T2 = U16(2);  envCal.T3 = U16(4);
    envCal.P1 = U16(6);  envCal.P2 = U16(8);  envCal.P3 = U16(10); envCal.P4 = U16(12);
    envCal.P5 = U16(14); envCal.P6 = U16(16); envCal.P7 = U16(18); envCal.P8 = U16(20); envCal.P9 = U16(22);
#undef U16
    envCal.H1 = b[25];
    if (envHasHumidity) {
      uint8_t h[7];
      if (!envReadReg(0xE1, h, 7)) continue;
      envCal.H2 = (int16_t)(h[0] | (h[1] << 8));
      envCal.H3 = h[2];
      envCal.H4 = (int16_t)((h[3] << 4) | (h[4] & 0x0F));
      envCal.H5 = (int16_t)((h[5] << 4) | (h[4] >> 4));
      envCal.H6 = (int8_t)h[6];
    }
    return true;
  }
  envAddr = 0;
  return false;
}

// One measurement in "forced" mode: the sensor wakes, measures, sleeps again (<1 uA).
static EnvReading envRead() {
  EnvReading r = {};
  if (!envAddr) return r;
  if (envHasHumidity) envWriteReg(0xF2, 0x01);       // humidity oversampling x1
  envWriteReg(0xF4, 0x25);                           // temperature x1, pressure x1, forced mode
  delay(12);
  uint8_t b[8];
  if (!envReadReg(0xF7, b, envHasHumidity ? 8 : 6)) return r;
  double adcP = ((uint32_t)b[0] << 12) | ((uint32_t)b[1] << 4) | (b[2] >> 4);
  double adcT = ((uint32_t)b[3] << 12) | ((uint32_t)b[4] << 4) | (b[5] >> 4);

  double v1 = (adcT / 16384.0 - envCal.T1 / 1024.0) * envCal.T2;
  double v2 = (adcT / 131072.0 - envCal.T1 / 8192.0) * (adcT / 131072.0 - envCal.T1 / 8192.0) * envCal.T3;
  double tFine = v1 + v2;
  r.tempC = tFine / 5120.0;

  v1 = tFine / 2.0 - 64000.0;
  v2 = v1 * v1 * envCal.P6 / 32768.0;
  v2 = v2 + v1 * envCal.P5 * 2.0;
  v2 = v2 / 4.0 + envCal.P4 * 65536.0;
  v1 = (envCal.P3 * v1 * v1 / 524288.0 + envCal.P2 * v1) / 524288.0;
  v1 = (1.0 + v1 / 32768.0) * envCal.P1;
  if (v1 != 0.0) {
    double p = 1048576.0 - adcP;
    p = (p - v2 / 4096.0) * 6250.0 / v1;
    v1 = envCal.P9 * p * p / 2147483648.0;
    v2 = p * envCal.P8 / 32768.0;
    r.hPa = (p + (v1 + v2 + envCal.P7) / 16.0) / 100.0;
  }

  if (envHasHumidity) {
    double adcH = (b[6] << 8) | b[7];
    double h = tFine - 76800.0;
    h = (adcH - (envCal.H4 * 64.0 + envCal.H5 / 16384.0 * h)) *
        (envCal.H2 / 65536.0 * (1.0 + envCal.H6 / 67108864.0 * h * (1.0 + envCal.H3 / 67108864.0 * h)));
    h = h * (1.0 - envCal.H1 * h / 524288.0);
    r.humidity = h < 0 ? 0 : (h > 100 ? 100 : h);
    r.hasHumidity = true;
  }
  r.ok = true;
  return r;
}

// ------------------------------------------------------------------- the log
#define LOG_NONE (-32768)
struct LogRec {
  uint32_t t;          // Unix time
  int16_t  temp10;     // 0.1 C
  int16_t  hum10;      // 0.1 %
  int16_t  hpa10;      // 0.1 hPa above 500 hPa... see logAdd
  int16_t  lightMv;    // light sensor divider, mV (brighter = higher)
};
RTC_DATA_ATTR static LogRec   logBuf[LOG_CAPACITY];
RTC_DATA_ATTR static uint16_t logHead = 0;      // next slot to write
RTC_DATA_ATTR static uint16_t logCount = 0;
RTC_DATA_ATTR static time_t   logNextEpoch = 0;

static void logAdd(const EnvReading &r, int lightMv) {
  LogRec rec;
  rec.t = (uint32_t)time(nullptr);
  rec.temp10 = r.ok ? (int16_t)lround(r.tempC * 10) : LOG_NONE;
  rec.hum10 = r.ok && r.hasHumidity ? (int16_t)lround(r.humidity * 10) : LOG_NONE;
  rec.hpa10 = r.ok ? (int16_t)lround((r.hPa - 500.0) * 10) : LOG_NONE;   // 500-3700 hPa fits in 16 bits
  rec.lightMv = (int16_t)lightMv;
  logBuf[logHead] = rec;
  logHead = (logHead + 1) % LOG_CAPACITY;
  if (logCount < LOG_CAPACITY) logCount++;
}

// i = 0 is the oldest sample still held.
static const LogRec &logAt(uint16_t i) {
  return logBuf[(logHead + LOG_CAPACITY - logCount + i) % LOG_CAPACITY];
}

static void logClear() {
  logHead = logCount = 0;
}
