"""Source-level regressions; run with python -m unittest discover -s test."""
from pathlib import Path
import re
import unittest

ROOT = Path(__file__).resolve().parents[1]


class PrintCompletionSource(unittest.TestCase):
    def test_finished_blinks_every_led_together_in_configured_color(self):
        source = (ROOT / "src/xoled.cpp").read_text(encoding="utf-8-sig")
        expression = re.search(
            r"case PrinterState::Finished:\s*leds\[i\]\s*=\s*([^;]+);", source
        ).group(1)
        self.assertNotRegex(expression, r"\bi\b", "Completion phase must not depend on LED index")
        self.assertRegex(expression, r"\?\s*fg\s*:\s*CRGB::Black")

    def test_only_reported_state_can_indicate_printing(self):
        source = (ROOT / "src/mqtt.cpp").read_text(encoding="utf-8-sig")
        self.assertNotRegex(source, r"printer\.state\s*=\s*PrinterState::Printing")
        self.assertIn("if (next != PrinterState::Unknown) printer.state = next;", source)
        self.assertIn("printer.state = PrinterState::Unknown;", source)
        self.assertIn('!strcmp(state, "RUNNING")', source)
        self.assertIn('!strcmp(state, "FINISH")', source)
        self.assertIn('!strcmp(state, "IDLE")', source)


if __name__ == "__main__":
    unittest.main()
