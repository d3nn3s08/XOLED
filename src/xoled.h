#pragma once
#include <Arduino.h>
#include <FastLED.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include "ImprovWiFiLibrary.h"
#include "const.h"
#include "config.h"
enum class PrinterState : uint8_t { Unknown, Disconnected, Idle, Preparing, Printing, Paused, Finished, Error };
struct PrinterStatus { PrinterState state = PrinterState::Unknown; uint8_t progress = 0; uint16_t layer = 0, total_layers = 0; uint32_t remaining_seconds = 0; String job_name; };
extern PrinterStatus printer;
extern CRGB* leds;
void leds_begin(); void leds_apply_config(); void leds_render(); void leds_test_start();
void wifi_begin(); void wifi_loop(); bool wifi_is_configured();
const char* printer_state_name(PrinterState state);
