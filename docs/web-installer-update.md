# Update the web installer

GitHub Pages serves `docs/`. The installer reads `docs/bins/manifest.json`;
pushing source changes alone does not update the downloadable firmware.

Run the following from the project root in PowerShell. Continue to the next
step only when each command reports success. These commands prepare local
files; they do not flash a device or publish anything.

## 1. Build both targets and the WebUI image

For a versioned release, first update `XOLED_VERSION` in `src/const.h`, the
version labels/fallbacks in `data/index.html`, the heading in `docs/index.html`
and `version` in `docs/bins/manifest.json`. API and Improv use `XOLED_VERSION`.

```powershell
$pio = "$env:USERPROFILE\.platformio\penv\Scripts\platformio.exe"
$pioPython = "$env:USERPROFILE\.platformio\penv\Scripts\python.exe"
$esptool = "$env:USERPROFILE\.platformio\packages\tool-esptoolpy\esptool.py"
$bootApp = "$env:USERPROFILE\.platformio\packages\framework-arduinoespressif32\tools\partitions\boot_app0.bin"
& $pio run -e esp32dev -e esp32c3dev
& $pio run -e esp32dev -e esp32c3dev -t buildfs
```

## 2. Prepare the ESP32 boot image

The existing manifest flashes one combined ESP32 boot image at address zero.
It contains the bootloader at `0x1000`, partition table at `0x8000` and OTA
initialization data at `0xe000`. Keep these offsets and the partition layout.

```powershell
& $pioPython $esptool --chip esp32 merge_bin --target-offset 0 -o docs/bins/esp32_bootloader_v4.bin 0x1000 .pio/build/esp32dev/bootloader.bin 0x8000 .pio/build/esp32dev/partitions.bin 0xe000 $bootApp
```

## 3. Replace the files referenced by the manifest

```powershell
Copy-Item .pio/build/esp32dev/firmware.bin docs/bins/firmware_esp32dev.bin
Copy-Item .pio/build/esp32dev/firmware.bin docs/bins/firmware.bin
Copy-Item .pio/build/esp32c3dev/firmware.bin docs/bins/firmware_esp32c3dev.bin
Copy-Item .pio/build/esp32c3dev/bootloader.bin 'docs/bins/bootloader copy.bin'
Copy-Item .pio/build/esp32dev/partitions.bin docs/bins/partitions.bin
Copy-Item .pio/build/esp32dev/littlefs.bin docs/bins/spiffs.bin
```

`spiffs.bin` is a legacy filename retained for the manifest. Its contents must
be **LittleFS**, matching `LittleFS.begin(false)` in the firmware. Do not reuse
an old SPIFFS image. Both targets currently use the same filesystem geometry.
Other old binary files in `docs/bins/` are not referenced by this manifest.
`firmware.bin` is kept as a compatibility copy of the current ESP32 application.

## 4. Verify and publish

- Run `python -B -m unittest discover -s test` and `git diff --check`.
- Verify both firmware images with `esptool image_info`; they must fit the
  `0x180000`-byte application partitions.
- Compare the installer firmware files with their build outputs using SHA-256.
- List/unpack `spiffs.bin` using `mklittlefs` with block size 4096, page size 256
  and image size 983040; `/index.html` must match `data/index.html`.
- Confirm all manifest paths exist, offsets do not overlap and the partition
  tables for both targets match. Reassess this workflow if the layout changes.
- Review and explicitly commit/push the changed installer files and documentation.
  After GitHub Pages finishes publishing, check the deployed files against the
  local hashes. The URL remains `https://d3nn3s08.github.io/XOLED/`.
- For an explicitly requested versioned release, tag the checked commit as
  `v<version>` and push that tag without changing existing tags. Create a GitHub
  Release for it, upload the two standalone application images with versioned
  filenames and their SHA-256 checksums, then publish it as the latest release.

The full ESP32 installer boot image covers the NVS address range with padding.
Installing it can reset saved settings. For a firmware-only update of an
already configured device, use its OTA upload with the target's standalone
`firmware.bin`; do not upload the combined boot image through OTA.

## Release v3.0.5: 2026-11-07

Based on source commit `dbe762a`, with the displayed version updated to 3.0.5,
including the MQTT status/remaining-time fixes and synchronized completion
blinking. Both firmware builds and filesystem
builds passed. Firmware image checksums/hashes, manifest ranges and the ESP32
boot-image contents were verified. The rebuilt LittleFS image was unpacked and
its `/index.html` matched `data/index.html` byte for byte. The previous installer
filesystem was confirmed to be SPIFFS and was replaced with LittleFS; no
partition-layout changes were required.

No physical ESP32 or ESP32-C3 installation test has been performed for this
installer update. Publication is separate from local preparation.
