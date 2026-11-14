// Optional. Copy this file to "secrets.h" (same folder) to give freshly
// flashed watches a first Wi-Fi network without using Watch Studio.
// It is only used when a watch has no saved settings yet; after that, networks
// are managed from Watch Studio (app/) (see docs/CONFIGURATION.md).
// secrets.h is git-ignored so the password never reaches GitHub.
#pragma once

#define WIFI_SSID     "your-wifi-name"
#define WIFI_PASSWORD "your-wifi-password"
