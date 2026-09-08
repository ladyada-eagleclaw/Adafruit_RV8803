# Adafruit RV8803 [![Arduino Library CI](https://github.com/adafruit/Adafruit_RV8803/actions/workflows/githubci.yml/badge.svg)](https://github.com/adafruit/Adafruit_RV8803/actions/workflows/githubci.yml) [![Documentation](https://img.shields.io/badge/documentation-doxygen-blue.svg)](https://adafruit.github.io/Adafruit_RV8803/html/index.html)

Arduino library for the Micro Crystal RV-8803-C7 real-time clock, using
RTClib's `DateTime` class. Supports alarms, countdown timers, periodic updates,
external-event timestamps, clock output, offset calibration, and RAM.

Install **Adafruit RV8803** and its **RTClib** and **Adafruit BusIO** dependencies
through Arduino Library Manager once the library is released. Before release,
download this repository as a ZIP and use **Sketch > Include Library > Add .ZIP
Library**, then install the dependencies through Library Manager.

## Examples

- `simpletest`: set and read the date and time.
- `alarm`: trigger an alarm every day at 09:30.
- `timer`: repeat a five-second countdown. The first interval takes 5–6 seconds.
- `event_capture`: capture seconds and hundredths with the EVI input.

CLOE must be HIGH for the SQW output. The clock provides 32.768 kHz, 1024 Hz,
or 1 Hz. Event timestamps contain only seconds and hundredths within a minute.

## Hardware tests

The numbered sketches in `extras/hw_tests` use a Metro Mini with VIN on A0,
SDA/SCL on A4/A5, and CLOE/INT/EVI/SQW on D2/D3/D4/D5. In `06_ram`, set
`batteryInstalled` to match the fixture; it defaults to an installed coin cell.
These tests change the RTC time and settings.

`15_no_battery` requires the coin cell removed and VIN powered only by A0.
It removes VIN for five minutes to allow the backup supply capacitor to
discharge, checks that power loss remains flagged through `begin()`, then
checks that setting the time clears the flags and restarts normal timekeeping.
The off interval is configurable with `powerOffMs`. This tests loss of backup
power; it does not detect an absent battery while VIN remains powered.

See [DESIGN.md](DESIGN.md) for register details and validation notes.

MIT licensed; see [LICENSE](LICENSE).
