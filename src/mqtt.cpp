#include "xoled.h"
#include "mqtt.h"
#include <PubSubClient.h>
#include <esp_heap_caps.h>
#include <atomic>
#include <limits.h>

namespace {
WiFiClientSecure tls;
PubSubClient client(tls);
constexpr uint32_t kInitialRetry = 5000, kMaxRetry = 60000;
uint32_t retry_at = 0, retry_delay = kInitialRetry, heap_at = 0;
bool buffer_ready = false, socket_active = false, attempted = false;
std::atomic<bool> connected{false};
struct Settings {
  char ip[sizeof(config.printer_ip)]{};
  char serial[sizeof(config.printer_serial)]{};
  char access_code[sizeof(config.access_code)]{};
};
Settings active, pending;
portMUX_TYPE settings_mux = portMUX_INITIALIZER_UNLOCKED;
bool settings_pending = false;

// Temporary diagnostics; retain the H2C buffer until measurements justify a change.
void log_heap(const char* event) {
  Serial.printf("[HEAP] %s free=%lu min=%lu largest=%lu\n", event,
                (unsigned long)ESP.getFreeHeap(), (unsigned long)ESP.getMinFreeHeap(),
                (unsigned long)heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));
}

// Only setup()/loop() may touch either client, including connected() (which can stop TLS).
void close_connection() {
  if (buffer_ready) client.disconnect();
  tls.stop();
  socket_active = false;
  connected.store(false);
  printer.state = PrinterState::Disconnected;
}

void failed_connection(const char* event) {
  close_connection();
  retry_at = millis() + retry_delay;
  Serial.printf("MQTT getrennt; neuer Versuch in %lu ms\n", (unsigned long)retry_delay);
  retry_delay = retry_delay < kMaxRetry / 2 ? retry_delay * 2 : kMaxRetry;
  log_heap(event);
}

PrinterState decode_state(const char* state) { if (!state) return PrinterState::Unknown; if (!strcmp(state, "RUNNING")) return PrinterState::Printing; if (!strcmp(state, "PAUSE") || !strcmp(state, "PAUSED")) return PrinterState::Paused; if (!strcmp(state, "FINISH") || !strcmp(state, "FINISHED")) return PrinterState::Finished; if (!strcmp(state, "FAILED") || !strcmp(state, "ERROR")) return PrinterState::Error; if (!strcmp(state, "PREPARE") || !strcmp(state, "PREPARING")) return PrinterState::Preparing; if (!strcmp(state, "IDLE")) return PrinterState::Idle; return PrinterState::Unknown; }

void on_message(char*, byte* payload, unsigned int length) {
  JsonDocument filter;
  for (const char* key : {"gcode_state", "mc_percent", "percent", "layer_num", "total_layer_num", "remaining_time", "subtask_name"}) filter["print"][key] = true;
  JsonDocument doc;
  // Parse directly from PubSubClient's buffer; copy retained strings before returning.
  if (deserializeJson(doc, payload, length, DeserializationOption::Filter(filter))) {
    Serial.println("MQTT-Nutzdaten nicht lesbar");
    return;
  }
  JsonObjectConst print = doc["print"];
  if (print.isNull()) return;
  static bool received = false;
  static uint32_t report_at = 0;
  if (!received || (uint32_t)(millis() - report_at) >= 60000) {
    received = true;
    report_at = millis();
    Serial.printf("[MQTT] Druckerbericht empfangen, bytes=%u\n", length);
  }
  const PrinterState next = decode_state(print["gcode_state"] | nullptr);
  if (next != PrinterState::Unknown) printer.state = next;
  int value = print["mc_percent"] | (print["percent"] | -1);
  if (value >= 0 && value <= 100) printer.progress = value;
  printer.layer = print["layer_num"] | printer.layer;
  printer.total_layers = print["total_layer_num"] | printer.total_layers;
  if (print["remaining_time"].is<unsigned long>()) {
    const unsigned long minutes = print["remaining_time"].as<unsigned long>();
    if (minutes <= UINT32_MAX / 60UL) printer.remaining_seconds = minutes * 60UL;
  }
  if (print["subtask_name"].is<const char*>()) printer.job_name = (const char*)print["subtask_name"];
  if (printer.state == PrinterState::Disconnected && (printer.progress > 0 || printer.job_name.length())) printer.state = PrinterState::Printing;
}
}

void mqtt_begin() {
  log_heap("boot");
  tls.setInsecure();
  buffer_ready = client.setBufferSize(49152);
  if (!buffer_ready) Serial.println("MQTT: 49152-Byte-Puffer nicht verfuegbar; Verbindung deaktiviert");
  client.setCallback(on_message);
  mqtt_reconfigure();
}

void mqtt_reconfigure() {
  // Async web callbacks only queue a snapshot; TLS teardown runs on the owner task.
  portENTER_CRITICAL(&settings_mux);
  strlcpy(pending.ip, config.printer_ip, sizeof(pending.ip));
  strlcpy(pending.serial, config.printer_serial, sizeof(pending.serial));
  strlcpy(pending.access_code, config.access_code, sizeof(pending.access_code));
  settings_pending = true;
  portEXIT_CRITICAL(&settings_mux);
}

bool mqtt_is_connected() { return connected.load(); }

void mqtt_loop() {
  const uint32_t now = millis();
  if ((uint32_t)(now - heap_at) >= 60000) {
    heap_at = now;
    log_heap("periodic");
  }
  Settings requested;
  bool apply;
  portENTER_CRITICAL(&settings_mux);
  apply = settings_pending;
  if (apply) { requested = pending; settings_pending = false; }
  portEXIT_CRITICAL(&settings_mux);
  if (apply && (strcmp(active.ip, requested.ip) || strcmp(active.serial, requested.serial) || strcmp(active.access_code, requested.access_code))) {
    close_connection();
    active = requested;
    client.setServer(active.ip, 8883);
    retry_delay = kInitialRetry;
    retry_at = attempted ? millis() + kInitialRetry : millis();
    log_heap("reconfigure");
  }
  if (!buffer_ready) return;
  if (WiFi.status() != WL_CONNECTED || !active.ip[0] || !active.serial[0] || !active.access_code[0]) {
    if (socket_active) failed_connection("disconnected");
    printer.state = PrinterState::Disconnected;
    return;
  }
  if (socket_active) {
    if (!client.connected()) { failed_connection("disconnected"); return; }
    if (!client.loop()) { failed_connection("loop failed"); return; }
    return;
  }
  // Signed subtraction keeps deadlines correct across millis() wraparound.
  if ((int32_t)(now - retry_at) < 0) return;
  close_connection();
  const bool reconnecting = attempted;
  if (reconnecting) log_heap("reconnect");
  attempted = true;
  log_heap("before connect");
  char id[40];
  snprintf(id, sizeof(id), "XOLED-%06lx", (unsigned long)(ESP.getEfuseMac() & 0xffffff));
  if (!client.connect(id, "bblp", active.access_code)) {
    Serial.printf("MQTT-Verbindung fehlgeschlagen (Code %d)\n", client.state());
    failed_connection("connect failed");
    return;
  }
  char topic[80];
  snprintf(topic, sizeof(topic), "device/%s/report", active.serial);
  if (!client.subscribe(topic)) { failed_connection("subscribe failed"); return; }
  socket_active = true;
  connected.store(true);
  retry_delay = kInitialRetry;
  Serial.println("MQTT verbunden");
  log_heap("connected");
  if (reconnecting) log_heap("reconnected");
}
