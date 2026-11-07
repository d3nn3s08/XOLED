# XOLED v3.0.5

## Changes

- Read MQTT `gcode_state` as a string instead of passing an always-null value
  to the decoder. Running, finished, paused and idle reports are recognized.
- Read remaining minutes from `remaining_time`, `mc_remaining_time` or
  `remain_time`, with validation and overflow protection.
- Blink the entire LED strip together when a print finishes, using the
  configured progress color. Return to idle when the printer reports `IDLE`.
- Keep MQTT/TLS access on the main task, clean up failed connections and retry
  with backoff. Retained progress no longer implies a running print after reconnect.
- Publish freshly built ESP32 and ESP32-C3 installer binaries and a compatible
  LittleFS WebUI image. API, WebUI, Improv and installer identify version 3.0.5.

## Installation

Use the [web installer](https://d3nn3s08.github.io/XOLED/) for a full USB installation.
It can reset stored settings. For firmware-only OTA updates, upload the matching
standalone application image from the release assets:

- `XOLED-3.0.5-esp32dev.bin`: classic ESP32.
- `XOLED-3.0.5-esp32c3dev.bin`: ESP32-C3.
- `XOLED-3.0.5-littlefs.bin`: WebUI filesystem image, not an OTA application.
- `SHA256SUMS.txt`: SHA-256 checksums for the three images.

Do not upload a bootloader or filesystem image through the application's OTA form.

## Verification and limits

Both firmware targets and filesystem builds passed. All eight automated checks
passed, including a native test of the actual ArduinoJson MQTT parser with
partial reports, invalid values and a filtered payload over 30 KB. Installer
offsets, partition compatibility, image checksums and the extracted WebUI were
checked. No partition-layout change was made.

The release package has not undergone a physical installation or a 20-30 minute
hardware stability test. Intermittent WebUI transfer failures remain under
investigation; this release does not claim to resolve them or all outstanding
configuration/concurrency issues.
