// Sound pack: instrument tuner.
//
// Needs a microphone amplifier board (MAX4466 or MAX9814) on the accessory
// socket: board VCC -> AUX_PWR, GND -> GND, OUT -> AIN. Capacitor C4 lays the
// audio on top of the battery-sense voltage, so the tuner reads the same ADC
// pin as the battery gauge and simply ignores the steady part.
//
// Method: record about 150 ms of sound, then find the period with the YIN
// difference function (how unlike itself the wave is when shifted by n
// samples; the first deep dip is one period). Good for roughly 60-1000 Hz,
// which covers voice, guitar, violin, most wind instruments.
#pragma once
#include <Arduino.h>
#include "types.h"

#define TUNER_RATE_HZ   10000
#define TUNER_SAMPLES   1536
#define TUNER_MIN_HZ    60
#define TUNER_MAX_HZ    1000
#define TUNER_MIN_PP_MV 40        // quieter than this (peak to peak) = silence

struct Pitch {
  bool    ok;
  float   hz;
  uint8_t note;     // 0 = C ... 11 = B
  int8_t  octave;   // scientific: A4 = 440 Hz
  int8_t  cents;    // -50 .. +50 from the nearest note
};

static int16_t tunerBuf[TUNER_SAMPLES];

static Pitch tunerListen() {
  Pitch out = {};
  const uint32_t period = 1000000UL / TUNER_RATE_HZ;
  uint32_t next = micros();
  long sum = 0;
  int lo = 32767, hi = -32768;
  for (int i = 0; i < TUNER_SAMPLES; i++) {
    while ((int32_t)(micros() - next) < 0) {}
    next += period;
    int v = analogReadMilliVolts(PIN_AIN);
    tunerBuf[i] = v;
    sum += v;
    if (v < lo) lo = v;
    if (v > hi) hi = v;
  }
  if (hi - lo < TUNER_MIN_PP_MV) return out;
  int mean = sum / TUNER_SAMPLES;
  for (int i = 0; i < TUNER_SAMPLES; i++) tunerBuf[i] -= mean;

  const int tauMin = TUNER_RATE_HZ / TUNER_MAX_HZ;
  const int tauMax = TUNER_RATE_HZ / TUNER_MIN_HZ;
  const int window = TUNER_SAMPLES - tauMax - 1;
  static float d[TUNER_RATE_HZ / TUNER_MIN_HZ + 2];
  double running = 0;
  d[0] = 1;
  for (int tau = 1; tau <= tauMax; tau++) {
    int64_t acc = 0;
    for (int i = 0; i < window; i++) {
      int32_t diff = tunerBuf[i] - tunerBuf[i + tau];
      acc += diff * diff;
    }
    running += (double)acc;
    d[tau] = running > 0 ? (float)((double)acc * tau / running) : 1;   // cumulative-mean normalised
  }
  int best = -1;
  for (int tau = tauMin; tau < tauMax; tau++) {
    if (d[tau] < 0.15f) {
      while (tau + 1 < tauMax && d[tau + 1] < d[tau]) tau++;           // walk to the bottom of the dip
      best = tau;
      break;
    }
  }
  if (best < 0) return out;
  // Parabola through the dip and its neighbours: sub-sample accuracy.
  float a = d[best - 1], b = d[best], c = d[best + 1];
  float denom = a - 2 * b + c;
  float tau = best + (denom != 0 ? 0.5f * (a - c) / denom : 0);
  out.hz = TUNER_RATE_HZ / tau;

  float midi = 69.0f + 12.0f * log2f(out.hz / settings.tune_a4);
  int nearest = (int)lroundf(midi);
  out.cents = (int8_t)lroundf((midi - nearest) * 100.0f);
  out.note = ((nearest % 12) + 12) % 12;
  out.octave = nearest / 12 - 1;
  out.ok = true;
  return out;
}
