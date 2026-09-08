# Adafruit_RV8803 — Design Document

## Chip Overview
- **Part:** RV-8803-C7 (Micro Crystal)
- **Function:** Real-Time Clock with I2C interface
- **I2C Address:** 0x32 (fixed)
- **I2C Modes:** Standard (100 kHz, VDD ≥ 1.5V), Fast (400 kHz, VDD ≥ 2.0V)
- **VDD Range:** 1.5V–5.5V for specified operation
- **Time Format:** 24-hour only, BCD-encoded
- **Year Range:** 2000–2099 (with leap year correction)
- **Features:** Alarm, countdown timer, periodic update interrupt, external event with timestamp, CLKOUT, offset calibration, 1-byte RAM, voltage low detection

## Register Map

### Time/Date Registers (BCD-encoded)

| Addr | Name | Bits | Range | Notes |
|------|------|------|-------|-------|
| 0x10 | 100th Seconds | [7:0] | 00–99 | Read-only. Cleared when writing Seconds. |
| 0x00 | Seconds | [6:0] | 00–59 | Bit 7 always 0. Writing clears 100ths. |
| 0x01 | Minutes | [6:0] | 00–59 | Bit 7 always 0 |
| 0x02 | Hours | [5:0] | 00–23 | Bits 7:6 always 0. 24-hour only. |
| 0x03 | Weekday | [6:0] | 1–7 | **One-hot encoding** (not BCD). One bit per day. |
| 0x04 | Date | [5:0] | 01–31 | Bits 7:6 always 0 |
| 0x05 | Month | [4:0] | 01–12 | Bits 7:5 always 0 |
| 0x06 | Year | [7:0] | 00–99 | Represents 2000–2099 |

Mirror registers at 0x11–0x17 (writing either range updates both).

### RAM Register

| Addr | Name | Bits | Range | Notes |
|------|------|------|-------|-------|
| 0x07 | RAM | [7:0] | 0x00–0xFF | 1 byte user storage. Reset: 0x00. |

### Alarm Registers

| Addr | Name | Bits | Notes |
|------|------|------|-------|
| 0x08 | Minutes Alarm | [7] AE_M, [6:0] BCD 00–59 | AE_M: 0=enabled, 1=disabled |
| 0x09 | Hours Alarm | [7] AE_H, [6] GP0, [5:0] BCD 00–23 | AE_H: 0=enabled, 1=disabled |
| 0x0A | Weekday/Date Alarm | [7] AE_WD, [6:0] varies | WADA=0: one-hot weekday (multi-select). WADA=1: [6] GP1, [5:0] BCD date 01–31 |

**Alarm Enable Truth Table:**

| AE_WD | AE_H | AE_M | Fires when... |
|-------|------|------|---------------|
| 0 | 0 | 0 | minutes + hours + weekday/date match |
| 0 | 0 | 1 | hours + weekday/date match |
| 0 | 1 | 0 | minutes + weekday/date match (hourly) |
| 0 | 1 | 1 | weekday/date matches |
| 1 | 0 | 0 | hours + minutes match (daily) |
| 1 | 0 | 1 | hours match (daily) |
| 1 | 1 | 0 | minutes match (hourly) |
| 1 | 1 | 1 | every minute |

Note: AE bits are **inverted** — 0 = enabled, 1 = disabled.

### Timer Registers

| Addr | Name | Bits | Notes |
|------|------|------|-------|
| 0x0B | Timer Counter 0 | [7:0] | Lower 8 bits of 12-bit preset value |
| 0x0C | Timer Counter 1 | [7:4] GP5–GP2, [3:0] upper 4 bits | Total: 12-bit timer (0–4095) |

Repeating countdown period = timer_value / timer_clock_frequency.

The first interval after enabling or restarting the timer includes an asynchronous
startup delay. It must be checked separately from automatically reloaded periods.
For a preset `n`, the manufacturer's first-period limits are:

| Timer clock | First interval | Later intervals |
|-------------|----------------|-----------------|
| 4096 Hz | n × 244 µs + 61 µs through (n + 1) × 244 µs + 61 µs | n × 244 µs |
| 64 Hz | n / 64 through (n + 1) / 64 seconds | n / 64 seconds |
| 1 Hz | n through n + 1 seconds | n seconds |
| 1/60 Hz | n × 60 through (n + 1) × 60 seconds | n × 60 seconds |

The 4096 Hz entries use the manual's rounded times. For example, a five-count
timer at 1 Hz initially takes **5–6 seconds**, then repeats every 5 seconds.
At 64 Hz, a preset of 320 gives a first interval of 5–5.015625 seconds and
subsequent intervals of 5 seconds. Do not subtract one from the preset to
compensate: that would shorten every repeated period.

The hardware test measures INT edges on D3. Its 50 ms fixture tolerance covers
the Metro's uncalibrated clock and setup overhead; it is not a measurement of
the RTC's ppm accuracy. Flag polling adds up to one polling interval of detection
latency. The old 4.5–5.5-second startup test incorrectly rejected normal behavior.

Source: [Micro Crystal application manual, sections 4.5.2–4.5.3](https://www.microcrystal.com/fileadmin/Media/Products/RTC/App.Manual/RV-8803-C7_App-Manual.pdf#page=29).

### Extension Register (0x0D)

| Bit | Name | Values | Description |
|-----|------|--------|-------------|
| 7 | TEST | 0 | Must always be 0 |
| 6 | WADA | 0/1 | 0=weekday alarm, 1=date alarm |
| 5 | USEL | 0/1 | 0=second update, 1=minute update |
| 4 | TE | 0/1 | Timer enable |
| 3:2 | FD | 00–11 | CLKOUT freq (see below) |
| 1:0 | TD | 00–11 | Timer clock source (see below) |

**FD — CLKOUT Frequency:**

| FD | Frequency |
|----|-----------|
| 00 | 32.768 kHz (default) |
| 01 | 1024 Hz |
| 10 | 1 Hz |
| 11 | 32.768 kHz |

**TD — Timer Clock Source:**

| TD | Clock | Period per tick | Max countdown |
|----|-------|-----------------|---------------|
| 00 | 4096 Hz | 244.14 µs | ~1 second |
| 01 | 64 Hz | 15.625 ms | ~64 seconds |
| 10 | 1 Hz | 1 s | ~68 minutes |
| 11 | 1/60 Hz | 60 s | ~68 hours |

### Flag Register (0x0E)

| Bit | Name | Description |
|-----|------|-------------|
| 5 | UF | Periodic time update flag (only fires when UIE=1) |
| 4 | TF | Countdown timer flag |
| 3 | AF | Alarm flag |
| 2 | EVF | External event flag |
| 1 | V2F | Voltage low 2 — **all data invalid**, must reinitialize. Set at POR. |
| 0 | V1F | Voltage low 1 — temp compensation stopped. Set at POR. |

Bits 7:6 always 0. Clear flags by writing 0. Writing 1 has no effect on V1F/V2F.
Clearing V1F clears both V1F and V2F. Clearing V2F clears both.

### Control Register (0x0F)

| Bit | Name | Description |
|-----|------|-------------|
| 5 | UIE | Periodic update interrupt enable |
| 4 | TIE | Timer interrupt enable |
| 3 | AIE | Alarm interrupt enable |
| 2 | EIE | External event interrupt enable |
| 0 | RESET | Holds the prescaler stopped while 1. Software must clear it to resume timekeeping. It does not erase the registers. |

Bits 7:6 reserved. Bit 1 always 0.

### Event Capture Registers (read-only)

| Addr | Name | Bits | Notes |
|------|------|------|-------|
| 0x20 | 100th Seconds CP | [7:0] BCD 00–99 | Captured on external event |
| 0x21 | Seconds CP | [6:0] BCD 00–59 | Captured on external event |

### Offset Register (0x2C)

| Bit | Name | Notes |
|-----|------|-------|
| 5:0 | OFFSET | 6-bit two's complement. Range: -32 to +31. Resolution: 0.2384 ppm/step. Max correction: ±7.6 ppm. |

Bits 7:6 always 0.

### Event Control Register (0x2F)

| Bit | Name | Description |
|-----|------|-------------|
| 7 | ECP | Event capture enable (timestamp on event) |
| 6 | EHL | Event polarity: 0=falling edge, 1=rising edge |
| 5:4 | ET | Event filter: 00=none (30.5µs min), 01=3.9ms, 10=15.6ms, 11=125ms |
| 0 | ERST | Event reset: auto-clear 100ths + capture regs on event |

Bits 3:1 always 0.

### General Purpose Bits

| Bit | Location | Notes |
|-----|----------|-------|
| GP0 | 0x09[6] | In Hours Alarm register |
| GP1 | 0x0A[6] | In Weekday/Date Alarm register (only when WADA=1) |
| GP2 | 0x0C[4] | In Timer Counter 1 |
| GP3 | 0x0C[5] | In Timer Counter 1 |
| GP4 | 0x0C[6] | In Timer Counter 1 |
| GP5 | 0x0C[7] | In Timer Counter 1 |

6 bits total of general-purpose storage (in addition to the 8-bit RAM register).

## API Design

### RTClib Compatibility

This library follows Adafruit RTClib conventions so users familiar with
DS3231/PCF8523/DS1307 drivers feel at home. Key patterns:

- Uses RTClib's `DateTime` class for get/set (via `now()` / `adjust()`)
- `lostPower()` for power-fail detection
- `readSqwPinMode()` / `writeSqwPinMode()` for CLKOUT
- Alarm API modeled on DS3231 (`setAlarm()` with mode enum + `DateTime`)
- Timer API modeled on PCF8523 (`enableCountdownTimer()`)
- Calibration modeled on PCF8523 (`calibrate()`)
- Inherits `RTC_I2C` base (bcd2bin/bin2bcd, read_register/write_register)

**Requires:** `#include <RTClib.h>` (Adafruit RTClib as dependency)

### Class: Adafruit_RV8803 : public RTC_I2C

### Enums

```cpp
/** CLKOUT frequency (FD bits, Extension Register) */
typedef enum {
  RV8803_SquareWave32kHz = 0x00,  /**< 32.768 kHz (default) */
  RV8803_SquareWave1kHz  = 0x01,  /**< 1024 Hz */
  RV8803_SquareWave1Hz   = 0x02,  /**< 1 Hz */
} rv8803_sqw_mode_t;

/** Countdown timer clock source (TD bits, Extension Register) */
typedef enum {
  RV8803_Timer4096Hz = 0x00,  /**< 244.14 µs/tick, max ~1s */
  RV8803_Timer64Hz   = 0x01,  /**< 15.625 ms/tick, max ~64s */
  RV8803_Timer1Hz    = 0x02,  /**< 1 s/tick, max ~68 min */
  RV8803_Timer1_60Hz = 0x03,  /**< 60 s/tick, max ~68 hr */
} rv8803_timer_clock_t;

/** Alarm mode — which fields participate in match */
typedef enum {
  RV8803_A_MinHourDay  = 0x00, /**< Min + Hour + Weekday/Date (default) */
  RV8803_A_HourDay     = 0x01, /**< Hour + Weekday/Date */
  RV8803_A_MinDay      = 0x02, /**< Min + Weekday/Date (hourly) */
  RV8803_A_Day         = 0x03, /**< Weekday/Date only */
  RV8803_A_HourMin     = 0x04, /**< Hour + Min (daily) */
  RV8803_A_Hour        = 0x05, /**< Hour only (daily) */
  RV8803_A_Minute      = 0x06, /**< Minute only (hourly) */
  RV8803_A_EveryMinute = 0x07, /**< Every minute */
} rv8803_alarm_mode_t;

/** Event filter time (ET bits, Event Control Register) */
typedef enum {
  RV8803_EventFilterNone  = 0x00,  /**< No filter, 30.5 µs min pulse */
  RV8803_EventFilter4ms   = 0x01,  /**< 3.9 ms sampling */
  RV8803_EventFilter16ms  = 0x02,  /**< 15.6 ms sampling */
  RV8803_EventFilter125ms = 0x03,  /**< 125 ms sampling */
} rv8803_event_filter_t;

/** Periodic update mode (USEL bit) */
typedef enum {
  RV8803_UpdateSecond = 0x00,  /**< Update flag every second */
  RV8803_UpdateMinute = 0x01,  /**< Update flag every minute */
} rv8803_update_mode_t;

/** Interrupt sources (bits in Control Register) */
typedef enum {
  RV8803_InterruptUpdate = 0x20,  /**< UIE — periodic update */
  RV8803_InterruptTimer  = 0x10,  /**< TIE — countdown timer */
  RV8803_InterruptAlarm  = 0x08,  /**< AIE — alarm */
  RV8803_InterruptEvent  = 0x04,  /**< EIE — external event */
} rv8803_interrupt_t;
```

### Core — Init & Time (RTClib compatible)

```cpp
bool begin(TwoWire *wire = &Wire);
  // Init I2C at 0x32. Returns false on NACK.

DateTime now();
  // Reads time/date together and repeats at second 59 to handle rollover.
  // Returns an invalid DateTime on I2C failure; check isValid().
  // DateTime.dayOfTheWeek() is computed by RTClib, not from the
  // one-hot weekday register.

bool adjust(const DateTime &dt);
  // Sets all time/date registers from DateTime.
  // Also writes the one-hot weekday register from dt.dayOfTheWeek().
  // Holds and releases RESET around the write, then clears V1F and V2F flags.
  // Returns false for invalid dates or an I2C error.

bool lostPower();
  // Returns true if V2F set — time data is invalid, must call adjust().
  // Equivalent to DS3231's lostPower().

bool isrunning();
  // Returns true if RESET and V2F are clear; false on I2C error.

// Hundredths — not in DateTime, so a dedicated getter
uint8_t getHundredths();  // 0–99, read-only register at 0x10

// Individual time field getters (convenience, each reads one register)
uint8_t getSeconds();     // 0–59
uint8_t getMinutes();     // 0–59
uint8_t getHours();       // 0–23
uint8_t getWeekday();     // 0–6 (converted from one-hot)
uint8_t getDate();        // 1–31
uint8_t getMonth();       // 1–12
uint16_t getYear();       // 2000–2099
```

### Alarm (DS3231-style)

```cpp
bool setAlarm(const DateTime &dt, rv8803_alarm_mode_t mode);
  // Sets alarm minutes, hours, weekday/date from DateTime fields.
  // Mode controls which AE bits are set (which fields participate).
  // Weekday/date selection controlled by setAlarmWeekday()/setAlarmDate().
  // Returns false on I2C error.

DateTime getAlarm();
  // Reads alarm registers, returns DateTime with matched fields.

bool setAlarmWeekday(uint8_t weekday_mask);
  // One-hot weekday alarm (WADA=0). Multiple days selectable.
  // Bit 0 = day 1, bit 6 = day 7.

bool setAlarmDate(uint8_t date);
  // Date alarm (WADA=1). BCD day of month 1–31.

rv8803_alarm_mode_t getAlarmMode();

bool alarmFired();
  // Returns AF flag state.

bool clearAlarm();
  // Clears AF flag. Returns false on I2C error.
```

### Countdown Timer (PCF8523-style)

```cpp
bool enableCountdownTimer(rv8803_timer_clock_t clock, uint16_t value);
  // Sets the countdown preset (1–4095) and clock source.
  // This API rejects zero; the hardware itself treats a zero preset as stopped.
  // Enables timer (TE=1). Preserves GP2–GP5 bits.
  // Every call restarts the timer, so the first-period limits above apply again.

bool disableCountdownTimer();
  // Stops timer (TE=0).

uint16_t getCountdownTimer();
  // Reads preset value (not live countdown).

bool timerFired();
  // Returns TF flag state.

bool clearTimer();
  // Clears TF flag.
```

### Periodic Update

```cpp
bool setUpdateMode(rv8803_update_mode_t mode);
  // USEL: second or minute update.

bool updateFired();
  // Returns UF flag state.

bool clearUpdate();
  // Clears UF flag.
```

### External Event / Timestamp

```cpp
bool configureEvent(bool rising_edge, rv8803_event_filter_t filter);
  // Sets EHL polarity and ET filter time.

bool enableEventCapture(bool enable);
  // Sets ECP bit — capture seconds + hundredths on event.

bool enableEventReset(bool enable);
  // Sets ERST bit — auto-reset hundredths on event.

uint8_t getEventHundredths();  // 0–99, from 0x20
uint8_t getEventSeconds();     // 0–59, from 0x21

bool getEventTimestamp(rv8803_timestamp_t *timestamp);
  // Reads seconds and hundredths in one transaction; false on error.
  // Disable capture first if input events can recur during the read.

bool eventFired();
  // Returns EVF flag state.

bool clearEvent();
  // Clears EVF flag.
```

### Interrupts (INT pin, active-low open-drain)

```cpp
bool enableInterrupt(rv8803_interrupt_t source);
bool disableInterrupt(rv8803_interrupt_t source);
  // Sets/clears UIE, TIE, AIE, EIE in control register.
```

### CLKOUT (RTClib SQW-style naming)

```cpp
bool writeSqwPinMode(rv8803_sqw_mode_t mode);
  // Sets FD bits. CLKOE pin must be tied HIGH for output.

rv8803_sqw_mode_t readSqwPinMode();
```

### Offset Calibration (PCF8523-style)

```cpp
bool calibrate(int8_t offset);
  // 6-bit two's complement, -32 to +31.
  // Each step = ±0.2384 ppm. Max correction ±7.6 ppm.

int8_t getCalibration();
```

### Power Management

```cpp
bool lostPower();
  // (listed above) V2F — all data invalid.

bool tempCompStopped();
  // V1F — temp compensation was interrupted (less severe than V2F).

bool clearPowerFlags();
  // Clears both V1F and V2F.
```

### RAM & GP Bits

```cpp
bool writeRAM(uint8_t value);
uint8_t readRAM();
  // Single byte at 0x07.

bool writeGP(uint8_t bits);
uint8_t readGP();
  // 6-bit value mapped from GP0–GP5 scattered across registers.
  // bits[0]=GP0, bits[1]=GP1, ..., bits[5]=GP5.
```

### Direct Register Access

```cpp
// Extension Register (0x0D) — read/write full byte
uint8_t readExtensionRegister();
bool writeExtensionRegister(uint8_t value);

// Flag Register (0x0E) — read full byte, clear specific flags
uint8_t readFlagRegister();
bool writeFlagRegister(uint8_t value);
  // Use to clear multiple flags at once. Writing 0 clears a flag.
  // Writing 1 to V1F/V2F has no effect.

// Control Register (0x0F) — read/write full byte
uint8_t readControlRegister();
bool writeControlRegister(uint8_t value);

// Event Control Register (0x2F) — read/write full byte
uint8_t readEventControl();
bool writeEventControl(uint8_t value);
```

### Reset

```cpp
bool reset();
  // Sets and then clears RESET to restart the prescaler.
  // Preserves time/calendar and configuration registers.
  // The hardware RESET bit does not auto-clear.
```

## BCD Helpers

```cpp
// Inherited from RTC_I2C base class:
static uint8_t bcd2bin(uint8_t val);
static uint8_t bin2bcd(uint8_t val);

// RV8803-specific (private):
static uint8_t weekday2onehot(uint8_t day);  // 0–6 → one-hot bit
static uint8_t onehot2weekday(uint8_t bits); // one-hot bit → 0–6
```

## Dependencies

- **Adafruit RTClib** — for DateTime, TimeSpan, RTC_I2C base class
- **Adafruit BusIO** — for Adafruit_I2CDevice (pulled in by RTClib)

## File Structure

```
Adafruit_RV8803.h          — class declaration, register defines, enums
Adafruit_RV8803.cpp        — implementation
DESIGN.md                  — this file
library.properties         — metadata
LICENSE                    — MIT
examples/
  simpletest/simpletest.ino
  alarm/alarm.ino
  timer/timer.ino
hw_tests/                  — hardware validation tests
```

## Hardware Notes

- **INT pin:** Active-low, open-drain. Requires external pull-up.
- **CLKOE pin:** Must be tied HIGH for CLKOUT, LOW to disable. Not software-controlled.
- **EVI pin:** Must not float. Tie to VDD or GND if unused.
- **Weekday encoding:** One-hot (bit 0 = Sunday or user-defined). NOT sequential 0–6 in register — driver converts.
- **Power-on:** V1F and V2F are set. EVF may be set depending on EVI pin state. Clear all flags after init.

### Metro Mini fixture and battery backup

VIN is driven by A0, SDA/SCL use A4/A5, and CLOE/INT/EVI/SQW connect to
D2/D3/D4/D5 respectively. This order matches the saved Rev A schematic,
silkscreen, and board routing. The chip pin numbers are 4/6/7/2 respectively.

The fitted CR1220 keeps the RTC supplied through the BAT54C when VIN is turned
off. RAM retention and `lostPower() == false` are therefore expected. The legacy
`06_ram` full-loss assertions require the coin cell to be removed; those
assertions are not valid battery-backup tests.

### CLKOUT requirements and current bench finding

- CLKOUT is push-pull while enabled and high impedance when CLKOE is LOW.
  It does not require an open-drain output pull-up.
- The Rev A board pulls CLKOE down with 100 kΩ. CLKOE must reach at least
  0.8 × the RTC's VDD at chip pin 4 to guarantee enable. Reading D2 HIGH on
  the Metro does not establish the voltage at the chip.
- FD selects 32.768 kHz, 1024 Hz, or 1 Hz; there is no separate software
  CLKOUT-enable bit. The 32.768 kHz enable delay is at most 30.5 µs.
- RESET can stop the 1024 Hz and 1 Hz outputs. It does not stop 32.768 kHz;
  ERST does not stop any CLKOUT frequency.
- The bench has working timekeeping and timed INT pulses, but the earlier SQW
  probe saw no D5 edges, including with FD=00 and RESET=0. The saved board
  routes both CLKOE and SQW to the correct chip pads. This does not establish
  continuity or voltage on the assembled prototype.
- Next physical checks are VDD at chip pin 3, CLKOE at chip pin 4, and CLKOUT
  at chip pin 2 versus the SQW header. These separate supply/enable problems
  from an output connection or assembly problem.

Sources: [application manual §§2.2, 4.9, 7.2](https://www.microcrystal.com/fileadmin/Media/Products/RTC/App.Manual/RV-8803-C7_App-Manual.pdf#page=38)
and [published errata](https://www.microcrystal.com/fileadmin/Media/Products/RTC/App.Manual/RV-8803-C7_Errata_Sheet.pdf).
The errata cover older-silicon I2C communication and STOP detection, with no
listed CLKOUT-specific defect.
