#include <ArduinoJson.h>
#include <SD.h>
#include "SettingsStore.h"
#include "utils.h"

static AppSettings g_settings;
AppSettings& settings() { return g_settings; }

static const AccentOption kAccentPresets[] = {
  {"Orange", 0xFBE4},
  {"Green",  0x07E0},
  {"Red",    0xF800},
  {"Cyan",   0x07FF},
  {"Purple", 0xF81F},
  {"Yellow", 0xFFE0},
  {"White",  0xFFFF},
};

uint8_t accentPresetClamp(uint8_t preset) {
  if (preset >= ACCENT_PRESET_COUNT) return 0;
  return preset;
}

uint16_t accentColor565(uint8_t preset) {
  return kAccentPresets[accentPresetClamp(preset)].color565;
}

const char* accentPresetName(uint8_t preset) {
  return kAccentPresets[accentPresetClamp(preset)].name;
}

static bool ensureDir(const char* dirPath) {
  if (!isSDCardAvailable()) return false;
  if (SD.exists(dirPath)) return true;
  if (SD.mkdir(dirPath)) return true;
  if (dirPath && dirPath[0] == '/') return SD.mkdir(dirPath + 1);
  return false;
}

bool settingsLoad() {
  if (!isSDCardAvailable()) return false;
  if (!SD.exists(SETTINGS_PATH)) return true;

  File f = SD.open(SETTINGS_PATH, FILE_READ);
  if (!f) return false;

  StaticJsonDocument<512> doc;
  DeserializationError err = deserializeJson(doc, f);
  f.close();
  if (err) return false;

  auto& s = g_settings;
  s.brightness      = doc["brightness"]      | s.brightness;
  s.theme           = (Theme)(uint8_t)(doc["theme"] | (uint8_t)s.theme);
  s.accentColor     = accentPresetClamp(doc["accentColor"] | s.accentColor);
  s.neopixelEnabled = doc["neopixelEnabled"] | s.neopixelEnabled;

  s.autoWifiScan    = doc["autoWifiScan"]    | s.autoWifiScan;
  s.autoBleScan     = doc["autoBleScan"]     | s.autoBleScan;

  if (s.autoWifiScan != s.autoBleScan) {
    const bool en = (s.autoWifiScan || s.autoBleScan);
    s.autoWifiScan = en;
    s.autoBleScan  = en;
  }
  return true;
}

bool settingsSave() {
  if (!ensureDir("/config")) {
    return false;
  }

  File f = SD.open(SETTINGS_PATH, FILE_WRITE);
  if (!f) return false;

  auto& s = g_settings;
  StaticJsonDocument<512> doc;
  doc["brightness"]      = s.brightness;
  doc["theme"]           = (uint8_t)s.theme;
  doc["accentColor"]     = s.accentColor;
  doc["neopixelEnabled"] = s.neopixelEnabled;
  doc["autoWifiScan"]    = s.autoWifiScan;
  doc["autoBleScan"]     = s.autoBleScan;

  const bool ok = serializeJson(doc, f) > 0;
  f.close();
  return ok;
}