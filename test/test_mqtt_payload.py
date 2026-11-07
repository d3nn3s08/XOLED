"""Exercise the actual MQTT parser with ArduinoJson on a Windows host (MSVC)."""
import os
from pathlib import Path
import subprocess
import unittest

ROOT = Path(__file__).resolve().parents[1]


def extract_function(source, signature):
    start = source.index(signature)
    opening = source.index("{", start)
    depth = 1
    end = opening + 1
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[start:end]


class MqttPayload(unittest.TestCase):
    def test_real_parser_status_and_remaining_time(self):
        vswhere = Path(os.environ.get("ProgramFiles(x86)", "")) / "Microsoft Visual Studio/Installer/vswhere.exe"
        if not vswhere.is_file():
            self.skipTest("Requires Windows MSVC and the project's ArduinoJson headers")
        result = subprocess.run([
            str(vswhere), "-latest", "-products", "*", "-requires",
            "Microsoft.VisualStudio.Component.VC.Tools.x86.x64", "-find",
            r"VC\Auxiliary\Build\vcvars64.bat",
        ], capture_output=True, text=True, check=True)
        if not result.stdout.strip():
            self.skipTest("MSVC C++ compiler is unavailable")
        vcvars = result.stdout.strip().splitlines()[0]
        source = (ROOT / "src/mqtt.cpp").read_text(encoding="utf-8-sig")
        types = (ROOT / "src/xoled.h").read_text(encoding="utf-8-sig")
        enum = types[types.index("enum class PrinterState"):types.index(";", types.index("enum class PrinterState")) + 1]
        functions = "\n".join(extract_function(source, signature) for signature in (
            "PrinterState decode_state(", "void on_message(",
        ))
        program = r'''
#include <ArduinoJson.h>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
#include <iostream>
using byte = unsigned char;
struct SerialStub {
  void println(const char*) {}
  template<class... Args> void printf(const char*, Args...) {}
} Serial;
uint32_t millis() { return 60000; }
''' + enum + r'''
struct PrinterStatus {
  PrinterState state = PrinterState::Unknown;
  uint8_t progress = 0;
  uint16_t layer = 0, total_layers = 0;
  uint32_t remaining_seconds = 0;
  std::string job_name;
} printer;
''' + functions + r'''
void report(const std::string& payload) {
  std::vector<byte> buffer(payload.begin(), payload.end());
  on_message(nullptr, buffer.data(), static_cast<unsigned int>(buffer.size()));
}
int main() {
  report(R"({"print":{"gcode_state":"RUNNING","mc_percent":46,"layer_num":51,"total_layer_num":150,"mc_remaining_time":73,"remain_time":73,"subtask_name":"Synthetic job"}})");
  assert(printer.state == PrinterState::Printing);
  assert(printer.progress == 46 && printer.layer == 51 && printer.total_layers == 150);
  assert(printer.remaining_seconds == 4380 && printer.job_name == "Synthetic job");
  for (int i = 0; i < 10; ++i) report(R"({"print":{"mc_percent":47}})");
  assert(printer.state == PrinterState::Printing && printer.remaining_seconds == 4380);
  for (const char* status : {"FINISH", "FINISHED", "PAUSE", "PAUSED", "IDLE", "PREPARE", "ERROR"}) {
    report(std::string("{\"print\":{\"gcode_state\":\"") + status + "\"}}");
    assert(printer.state == decode_state(status));
  }
  report(R"({"print":{"gcode_state":null}})");
  assert(printer.state == PrinterState::Error);
  for (const char* key : {"remaining_time", "mc_remaining_time", "remain_time"}) {
    report(std::string("{\"print\":{\"") + key + "\":2}}");
    assert(printer.remaining_seconds == 120);
    for (const char* invalid : {"null", "-1", "2.5", "\"2\"", "true", "71582789"}) {
      report(std::string("{\"print\":{\"") + key + "\":" + invalid + "}}");
      assert(printer.remaining_seconds == 120);
    }
  }
  report(R"({"print":{"remaining_time":3,"mc_remaining_time":4,"remain_time":5}})");
  assert(printer.remaining_seconds == 180);
  report(R"({"print":{"remaining_time":-1,"mc_remaining_time":0,"remain_time":5}})");
  assert(printer.remaining_seconds == 0);
  report(std::string("{\"print\":{\"unused\":\"") + std::string(30000, 'x') + "\",\"gcode_state\":\"RUNNING\",\"mc_remaining_time\":73}}");
  assert(printer.state == PrinterState::Printing && printer.remaining_seconds == 4380);
  std::cout << "Actual parser: states, partial reports, time aliases, invalid values and 30 KB filtered payload passed\n";
}
'''
        output = ROOT / "build_output/mqtt_payload_test"
        output.mkdir(parents=True, exist_ok=True)
        cpp = output / "parser.cpp"
        cpp.write_text(program, encoding="utf-8")
        headers = ROOT / ".pio/libdeps/esp32dev/ArduinoJson/src"
        command = (
            f'call "{vcvars}" >nul && cl /nologo /EHsc /std:c++17 '
            f'/I"{headers}" /Fo"{output / "parser.obj"}" '
            f'/Fe"{output / "parser.exe"}" "{cpp}" && "{output / "parser.exe"}"'
        )
        batch = output / "run.cmd"
        batch.write_text("@echo off\n" + command + "\n", encoding="utf-8")
        result = subprocess.run(["cmd", "/d", "/c", str(batch)], cwd=output,
                                capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        print(result.stdout.strip())


if __name__ == "__main__":
    unittest.main()
