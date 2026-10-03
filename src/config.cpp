#include "config.h"
#include <Preferences.h>
#include <IPAddress.h>

Config config;
namespace {
constexpr uint16_t kMinLeds = 1, kMaxLeds = 255;
constexpr uint32_t kMinPower = 100, kMaxPower = 30000;
bool copy_value(JsonObjectConst values, const char* key, char* dest, size_t size) {
  if (!values[key].is<const char*>()) return true;
  const char* value = values[key];
  if (strlen(value) >= size) return false;
  strlcpy(dest, value, size); return true;
}
bool parse_color(const char* value, uint8_t color[3]) {
  if (!value || strlen(value) != 7 || value[0] != '#') return false;
  unsigned int r, g, b;
  if (sscanf(value + 1, "%02x%02x%02x", &r, &g, &b) != 3) return false;
  color[0] = r; color[1] = g; color[2] = b; return true;
}
void color_to_string(const uint8_t color[3], char output[8]) { snprintf(output, 8, "#%02X%02X%02X", color[0], color[1], color[2]); }
}
bool config_has_wifi() { return config.wifi_ssid[0] != '\0'; }
bool config_has_printer() { return config.printer_ip[0] && config.printer_serial[0] && config.access_code[0]; }
void config_load() {
  Preferences p;
  if (!p.begin("xoled", false)) { Serial.println("NVS nicht verfuegbar; Defaults aktiv"); return; }
  if (p.isKey("device_name")) p.getString("device_name", config.device_name, sizeof(config.device_name)); if (p.isKey("language")) p.getString("language", config.language, sizeof(config.language));
  if (p.isKey("wifi_ssid")) p.getString("wifi_ssid", config.wifi_ssid, sizeof(config.wifi_ssid)); if (p.isKey("wifi_pass")) p.getString("wifi_pass", config.wifi_pass, sizeof(config.wifi_pass));
  if (p.isKey("printer_ip")) p.getString("printer_ip", config.printer_ip, sizeof(config.printer_ip)); if (p.isKey("access_code")) p.getString("access_code", config.access_code, sizeof(config.access_code)); if (p.isKey("printer_serial")) p.getString("printer_serial", config.printer_serial, sizeof(config.printer_serial));
  config.led_count = constrain(p.getUShort("led_count", 10), kMinLeds, kMaxLeds); config.brightness = p.getUChar("brightness", 64); config.max_milliwatts = constrain(p.getUInt("max_mw", 850), kMinPower, kMaxPower);
  for (uint8_t i = 0; i < 3; ++i) { config.background_color[i] = p.getUChar((String("bg") + i).c_str(), config.background_color[i]); config.progress_color[i] = p.getUChar((String("fg") + i).c_str(), config.progress_color[i]); }
  p.end();
}
bool config_save() {
  Preferences p; if (!p.begin("xoled", false)) return false;
  p.putString("device_name", config.device_name); p.putString("language", config.language);
  p.putString("wifi_ssid", config.wifi_ssid); p.putString("wifi_pass", config.wifi_pass); p.putString("printer_ip", config.printer_ip); p.putString("access_code", config.access_code); p.putString("printer_serial", config.printer_serial);
  p.putUShort("led_count", config.led_count); p.putUChar("brightness", config.brightness); p.putUInt("max_mw", config.max_milliwatts);
  for (uint8_t i = 0; i < 3; ++i) { p.putUChar((String("bg") + i).c_str(), config.background_color[i]); p.putUChar((String("fg") + i).c_str(), config.progress_color[i]); }
  p.end(); Serial.println("NVS-Konfiguration gespeichert"); return true;
}
bool config_update(JsonObjectConst values, String& error) {
  Config candidate = config;
  if (!copy_value(values, "device_name", candidate.device_name, sizeof(candidate.device_name)) || !copy_value(values, "language", candidate.language, sizeof(candidate.language)) || !copy_value(values, "wifi_ssid", candidate.wifi_ssid, sizeof(candidate.wifi_ssid)) || !copy_value(values, "wifi_pass", candidate.wifi_pass, sizeof(candidate.wifi_pass)) || !copy_value(values, "printer_ip", candidate.printer_ip, sizeof(candidate.printer_ip)) || !copy_value(values, "access_code", candidate.access_code, sizeof(candidate.access_code)) || !copy_value(values, "printer_serial", candidate.printer_serial, sizeof(candidate.printer_serial))) { error = "Textfeld zu lang"; return false; }
  if (!candidate.device_name[0] || (strcmp(candidate.language, "de") && strcmp(candidate.language, "en"))) { error = "Geraetename oder Sprache ungueltig"; return false; }
  if (values["led_count"].is<int>()) candidate.led_count = values["led_count"]; if (values["brightness"].is<int>()) { const int percent = values["brightness"]; if (percent < 0 || percent > 100) { error = "Helligkeit ungueltig"; return false; } candidate.brightness = (percent * 255 + 50) / 100; } if (values["max_milliwatts"].is<uint32_t>()) candidate.max_milliwatts = values["max_milliwatts"];
  if (candidate.led_count < kMinLeds || candidate.led_count > kMaxLeds || candidate.max_milliwatts < kMinPower || candidate.max_milliwatts > kMaxPower) { error = "LED- oder Leistungswert ungueltig"; return false; }
  if (candidate.printer_ip[0] && !IPAddress().fromString(candidate.printer_ip)) { error = "Drucker-IP ungueltig"; return false; }
  if (values["background_color"].is<const char*>() && !parse_color(values["background_color"], candidate.background_color)) { error = "Hintergrundfarbe ungueltig"; return false; }
  if (values["progress_color"].is<const char*>() && !parse_color(values["progress_color"], candidate.progress_color)) { error = "Fortschrittsfarbe ungueltig"; return false; }
  config = candidate; return config_save();
}
bool config_reset() { Config defaults; config = defaults; Preferences p; if (!p.begin("xoled", false)) return false; const bool cleared = p.clear(); p.end(); return cleared; }
void config_to_json(JsonObject out, bool include_secrets) {
  char bg[8], fg[8]; color_to_string(config.background_color, bg); color_to_string(config.progress_color, fg);
  out["device_name"] = config.device_name; out["language"] = config.language; out["wifi_ssid"] = config.wifi_ssid; out["printer_ip"] = config.printer_ip; out["printer_serial"] = config.printer_serial;
  if (include_secrets) { out["wifi_pass"] = config.wifi_pass; out["access_code"] = config.access_code; }
  out["led_count"] = config.led_count; out["brightness"] = (config.brightness * 100 + 127) / 255; out["max_milliwatts"] = config.max_milliwatts; out["background_color"] = bg; out["progress_color"] = fg;
}
