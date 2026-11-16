#pragma once

#include <Arduino.h>
#include "shared.h"

#ifndef SETTINGS_PATH
#define SETTINGS_PATH "/config/settings.json"
#endif

struct AppSettings {
  uint8_t  brightness = BKL_LEVEL_MED;
  Theme    theme      = Theme::Dark;
  uint8_t  accentColor = 0;
  bool     neopixelEnabled = false;
  bool     autoWifiScan    = false;
  bool     autoBleScan     = false;
};

struct AccentOption {
  const char* name;
  uint16_t color565;
};

constexpr uint8_t ACCENT_PRESET_COUNT = 7;

AppSettings& settings();
uint8_t accentPresetClamp(uint8_t preset);
uint16_t accentColor565(uint8_t preset);
const char* accentPresetName(uint8_t preset);

bool settingsLoad();
bool settingsSave();