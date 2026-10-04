"""Source-level safety checks; hardware/TLS integration still requires an ESP32.

Run: python test/test_mqtt_source.py
"""
from pathlib import Path
import re
import unittest

SOURCE = (Path(__file__).resolve().parents[1] / "src/mqtt.cpp").read_text(encoding="utf-8-sig")


def body(name):
    start = SOURCE.index("{", SOURCE.index(name + "("))
    depth = 1
    end = start + 1
    while depth:
        depth += (SOURCE[end] == "{") - (SOURCE[end] == "}")
        end += 1
    return SOURCE[start + 1:end - 1]


class MqttSourceSafety(unittest.TestCase):
    def test_web_callbacks_do_not_touch_transport(self):
        for name in ("mqtt_reconfigure", "mqtt_is_connected"):
            self.assertNotRegex(body(name), r"\b(?:client|tls)\.")
        self.assertIn("connected.load()", body("mqtt_is_connected"))

    def test_persistent_clients_and_h2c_buffer(self):
        self.assertEqual(len(re.findall(r"WiFiClientSecure\s+\w+\s*;", SOURCE)), 1)
        self.assertEqual(len(re.findall(r"PubSubClient\s+\w+\s*\(", SOURCE)), 1)
        self.assertIn("buffer_ready = client.setBufferSize(49152)", SOURCE)

    def test_teardown_before_retry_and_reconfigure(self):
        close = body("close_connection")
        self.assertLess(close.index("client.disconnect()"), close.index("tls.stop()"))
        self.assertLess(close.index("tls.stop()"), close.index("PrinterState::Disconnected"))
        failure = body("failed_connection")
        self.assertLess(failure.index("close_connection()"), failure.index("retry_at ="))
        loop = body("mqtt_loop")
        self.assertLess(loop.index("close_connection()"), loop.index("client.setServer("))
        self.assertLess(loop.rindex("close_connection()"), loop.index("client.connect("))

    def test_loop_requires_live_connection_and_returns_on_failure(self):
        self.assertRegex(SOURCE, r"if \(!client.connected\(\)\) \{ failed_connection\([^;]+; return; \}")
        self.assertRegex(SOURCE, r"if \(!client.loop\(\)\) \{ failed_connection\([^;]+; return; \}")
        self.assertEqual(SOURCE.count("client.loop()"), 1)
        self.assertIn("(int32_t)(now - retry_at) < 0", SOURCE)

    def test_remaining_time_and_filtered_payload(self):
        self.assertIn('if (print["remaining_time"].is<unsigned long>())', SOURCE)
        self.assertIn("minutes <= UINT32_MAX / 60UL", SOURCE)
        self.assertNotIn('(print["remaining_time"] | printer.remaining_seconds)', SOURCE)
        self.assertIn("deserializeJson(doc, payload, length, DeserializationOption::Filter(filter))", SOURCE)
        keys = re.search(r"for \(const char\* key : \{([^}]+)\}", SOURCE).group(1)
        self.assertEqual(set(re.findall(r'"([^"]+)"', keys)), {
            "gcode_state", "mc_percent", "percent", "layer_num",
            "total_layer_num", "remaining_time", "subtask_name",
        })


if __name__ == "__main__":
    unittest.main()
