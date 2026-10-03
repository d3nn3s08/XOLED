#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>

struct Config {
  char device_name[33] = "XOLED";
  char language[3] = "de";
  char wifi_ssid[33]{};
  char wifi_pass[65]{};
  char printer_ip[16]{};
  char access_code[33]{};
  char printer_serial[33]{};
  uint16_t led_count = 10;
  uint8_t brightness = 64;
  uint32_t max_milliwatts = 850;
  uint8_t background_color[3] = {20, 20, 20};
  uint8_t progress_color[3] = {0, 180, 80};
};

extern Config config;
void config_load();
bool config_save();
bool config_update(JsonObjectConst values, String& error);
bool config_reset();
bool config_has_wifi();
bool config_has_printer();
void config_to_json(JsonObject out, bool include_secrets = false);
