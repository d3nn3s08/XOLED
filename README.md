# XOLED

![XOLED Showcase](https://makerworld.bblmw.com/makerworld/model/US3a976399c751ad/design/2024-08-03_c5c91ad4dcc5d.jpeg?x-oss-process=image/resize,w_1920/format,webp)

XOLED is an ESP32-based firmware project for displaying Bambu Lab printer status and print progress on an addressable LED strip.

This repository is a fork of the original [michaelowens/XOLED](https://github.com/michaelowens/XOLED) project.

The original project was created to connect an ESP32 to Bambu Lab 3D printers via MQTT and visualize the current print progress using LEDs.

This fork is being modernized and extended, with a focus on newer Bambu Lab printer generations such as the H2 series.

> This project is currently under active development.  
> Some functionality may change while support for newer printers is added.

## Table of Contents

- [About this Fork](#about-this-fork)
- [Features](#features)
- [Current Status](#current-status)
- [Planned Improvements](#planned-improvements)
- [Installation](#installation)
- [Usage](#usage)
- [Hardware Requirements](#hardware-requirements)
- [Enclosure](#enclosure)
- [Original Project](#original-project)
- [License](#license)
- [Acknowledgements](#acknowledgements)

## About this Fork

The original XOLED firmware works with existing Bambu Lab printer generations and uses MQTT data from the printer to visualize print progress.

This fork started while testing XOLED with a Bambu Lab H2C.

The ESP32 successfully connects to the H2C via MQTT, but the original firmware does not currently interpret the H2C print state correctly.

The H2C itself provides valid status information such as:

```json
{
  "gcode_state": "RUNNING",
  "mc_percent": 21,
  "percent": 21
}
```

The goal of this fork is to improve compatibility with newer Bambu Lab printers while keeping the lightweight and simple idea of the original XOLED project.

## Features

Current and inherited functionality includes:

- Connects to Bambu Lab 3D printers using MQTT.
- Displays print progress on an addressable LED strip.
- ESP32-based standalone operation.
- WiFi configuration.
- Configurable LED count and brightness.
- Experimental I2C display support.
- 3D-printable enclosure.
- Local web configuration.

## Current Status

### Working

- ESP32 firmware
- WiFi connection
- MQTT connection to Bambu Lab printers
- Addressable LED control
- Original XOLED functionality
- X1C operation

### Under Investigation

- Bambu Lab H2C support
- H2-series MQTT payload handling
- Print-state detection on newer printers
- Large MQTT message handling
- Print progress handling on H2-series printers

## Planned Improvements

Planned changes currently include:

- Full Bambu Lab H2C support
- Improved support for newer Bambu Lab printer generations
- More robust MQTT parsing
- Support for larger MQTT payloads
- Improved printer-state handling
- Better debug logging
- Cleaner configuration handling
- Improved LED status visualization
- Improved error handling
- OTA firmware updates
- New web installer

Additional features may be added as development progresses.

## Installation

The original XOLED web installer is built for the upstream firmware and should currently not be used for this fork.

For now, this fork should be built and flashed using PlatformIO.

A dedicated web installer is planned once the firmware reaches a stable state.

### Development Installation

1. Clone this repository.
2. Open the project using Visual Studio Code with PlatformIO.
3. Connect the ESP32 using USB.
4. Build and upload the firmware.
5. Configure WiFi and printer connection settings.

## Usage

Once the firmware is installed and the ESP32 is powered on, it will:

1. Connect to the configured WiFi network.
2. Connect to the Bambu Lab printer using MQTT.
3. Subscribe to printer status updates.
4. Interpret the current printer state.
5. Display printer status and print progress using the LED strip.

## Hardware Requirements

- ESP32 microcontroller
- Addressable LED strip such as WS2812B
- Suitable 5V power supply
- 3D-printed enclosure

Depending on the final hardware design, additional components may be supported later.

## Enclosure

The original 3D-printed enclosure was designed by [ortoPilot](https://twitch.tv/ortopilot).

The enclosure can be downloaded from MakerWorld:

https://makerworld.com/en/models/570064#profileId-489970

The enclosure design belongs to its respective creator.

## Original Project

This project is based on:

**XOLED by michaelowens**

https://github.com/michaelowens/XOLED

The original XOLED project provided the foundation for:

- ESP32 firmware structure
- Bambu Lab MQTT communication
- LED progress visualization
- Web configuration
- Display support
- Hardware enclosure integration

This fork would not exist without the work done in the original project.

## License

This project remains licensed under the MIT License.

See the [LICENSE](LICENSE) file for details.

The original XOLED project is also licensed under the MIT License.

## Acknowledgements

Special thanks to:

- **michaelowens** for creating the original XOLED project.
- **ortoPilot** for designing the original 3D-printed enclosure.
- **Bambu Lab** for their 3D printers.
- **FastLED** for the FastLED library.
- **knolleary** for the PubSubClient library.
- **bblanchon** for the ArduinoJson library.
- **Aircoookie** for the ESPAsyncWebServer library.
- **jnthas** for the Improv WiFi Library.
- **Adafruit** for the GFX and SSD1306 libraries.
