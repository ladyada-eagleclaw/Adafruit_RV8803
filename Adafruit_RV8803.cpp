/*!
 * @file Adafruit_RV8803.cpp
 *
 * @mainpage Adafruit RV-8803-C7 Real-Time Clock Library
 *
 * @section intro_sec Introduction
 *
 * This is a library for the RV-8803-C7 Real-Time Clock from Micro Crystal.
 * The RV8803 is a high-accuracy (±3ppm) RTC with I2C interface, alarm,
 * countdown timer, external event timestamp, CLKOUT, and offset calibration.
 *
 * @section dependencies Dependencies
 *
 * This library depends on:
 * - Adafruit RTClib
 * - Adafruit BusIO
 *
 * @section author Author
 *
 * Written by Limor 'ladyada' Fried with assistance from Claude Code
 *
 * @section license License
 *
 * MIT license, all text above must be included in any redistribution
 */

#include "Adafruit_RV8803.h"

#include <Adafruit_BusIO_Register.h>

/** @brief Release the owned I2C interface. */
Adafruit_RV8803::~Adafruit_RV8803() {
  delete i2c_dev;
}

/**
 * @brief Initialize the RV8803 RTC
 * @param wire Pointer to TwoWire instance (default &Wire)
 * @return true if device found, false on NACK
 */
bool Adafruit_RV8803::begin(TwoWire* wire) {
  delete i2c_dev;
  i2c_dev = new Adafruit_I2CDevice(RV8803_I2C_ADDRESS, wire);
  if (!i2c_dev->begin()) {
    return false;
  }
  // There is no device ID. Confirm that status can be read without disturbing
  // a battery-backed clock or clearing evidence of power loss.
  return readFlagRegister() != RV8803_READ_ERROR;
}

/**
 * @brief Read current date/time from RTC
 * @return DateTime object with current time
 * @note Repeats the calendar read at second 59 (manual section 4.12).
 * Returns an invalid DateTime on I2C failure.
 */
DateTime Adafruit_RV8803::now() {
  Adafruit_BusIO_Register time_reg(i2c_dev, RV8803_REG_SECONDS, 7);
  uint8_t buffer[7];
  if (!time_reg.read(buffer, sizeof(buffer))) {
    return DateTime(2100, 1, 1); // Invalid DateTime; caller can use isValid().
  }
  // The calendar is NOT latched by a burst read. Manual section 4.12 requires
  // another complete read at second 59 to avoid mixed minute/day/year data.
  if (bcd2bin(buffer[0]) == 59) {
    uint8_t confirm[7];
    if (!time_reg.read(confirm, sizeof(confirm))) {
      return DateTime(2100, 1, 1);
    }
    if (bcd2bin(confirm[0]) != 59) {
      memcpy(buffer, confirm, sizeof(buffer));
    }
  }
  for (uint8_t i = 0; i < sizeof(buffer); i++) {
    if (i != 3 && ((buffer[i] & 0x0F) > 9 || (buffer[i] >> 4) > 9)) {
      return DateTime(2100, 1, 1);
    }
  }
  return DateTime(2000 + bcd2bin(buffer[6]), bcd2bin(buffer[5]),
                  bcd2bin(buffer[4]), bcd2bin(buffer[2]), bcd2bin(buffer[1]),
                  bcd2bin(buffer[0]));
}

/**
 * @brief Set the RTC date/time
 * @param dt DateTime object with desired time
 * @return true after writing, restarting, and clearing V1F/V2F; false on error.
 */
bool Adafruit_RV8803::adjust(const DateTime& dt) {
  if (!dt.isValid()) {
    return false;
  }
  // Manual section 4.13: hold the prescaler while writing the calendar, then
  // explicitly release RESET. RESET is not a self-clearing register reset.
  Adafruit_BusIO_Register ctrl_reg(i2c_dev, RV8803_REG_CONTROL, 1);
  Adafruit_BusIO_RegisterBits stop(&ctrl_reg, 1, 0);
  if (!stop.write(1)) {
    return false;
  }
  uint8_t buffer[] = {
      bin2bcd(dt.second()),     bin2bcd(dt.minute()),
      bin2bcd(dt.hour()),       weekday2onehot(dt.dayOfTheWeek()),
      bin2bcd(dt.day()),        bin2bcd(dt.month()),
      bin2bcd(dt.year() - 2000)};
  // Seconds through Year are written together while timekeeping is stopped.
  Adafruit_BusIO_Register time_reg(i2c_dev, RV8803_REG_SECONDS, 7);
  bool written = time_reg.write(buffer, sizeof(buffer));
  bool started = stop.write(0); // Attempt to restart even if the write failed.
  return written && started && clearPowerFlags();
}

/**
 * @brief Check if RTC lost power and time is invalid
 * @return true if V2F flag is set (time data invalid, must call adjust())
 */
bool Adafruit_RV8803::lostPower() {
  uint8_t flags = readFlagRegister();
  // Check the full-byte failure sentinel before decoding a field: RegisterBits
  // would turn a failed read into an asserted flag.
  if (flags == RV8803_READ_ERROR) {
    return true;
  }
  return (flags & RV8803_FLAG_V2F) != 0;
}

/**
 * @brief Check that timekeeping is enabled and no time data loss is flagged
 * @return true if RESET and V2F are clear; false on I2C error
 */
bool Adafruit_RV8803::isrunning() {
  uint8_t control = readControlRegister();
  uint8_t flags = readFlagRegister();
  return control != RV8803_READ_ERROR && flags != RV8803_READ_ERROR &&
         !(control & RV8803_CTRL_RESET) && !(flags & RV8803_FLAG_V2F);
}

/**
 * @brief Read hundredths of a second (0-99)
 * @return Hundredths value from register 0x10
 * @note This register is read-only and cleared when writing seconds
 */
uint8_t Adafruit_RV8803::getHundredths() {
  Adafruit_BusIO_Register reg(i2c_dev, RV8803_REG_HUNDREDTHS, 1);
  uint8_t value;
  if (!reg.read(&value) || (value & 0x0F) > 9 || bcd2bin(value) > 99) {
    return RV8803_READ_ERROR;
  }
  return bcd2bin(value);
}

/**
 * @brief Read seconds (0-59)
 * @return Seconds value
 */
uint8_t Adafruit_RV8803::getSeconds() {
  Adafruit_BusIO_Register reg(i2c_dev, RV8803_REG_SECONDS, 1);
  uint8_t value;
  if (!reg.read(&value) || (value & 0x0F) > 9 || bcd2bin(value) > 59) {
    return RV8803_READ_ERROR;
  }
  return bcd2bin(value);
}

/**
 * @brief Read minutes (0-59)
 * @return Minutes value
 */
uint8_t Adafruit_RV8803::getMinutes() {
  Adafruit_BusIO_Register reg(i2c_dev, RV8803_REG_MINUTES, 1);
  uint8_t value;
  if (!reg.read(&value) || (value & 0x0F) > 9 || bcd2bin(value) > 59) {
    return RV8803_READ_ERROR;
  }
  return bcd2bin(value);
}

/**
 * @brief Read hours (0-23)
 * @return Hours value (24-hour format)
 */
uint8_t Adafruit_RV8803::getHours() {
  Adafruit_BusIO_Register reg(i2c_dev, RV8803_REG_HOURS, 1);
  uint8_t value;
  if (!reg.read(&value) || (value & 0x0F) > 9 || bcd2bin(value) > 23) {
    return RV8803_READ_ERROR;
  }
  return bcd2bin(value);
}

/**
 * @brief Read weekday (0-6, 0=Sunday)
 * @return Weekday value converted from one-hot encoding
 */
uint8_t Adafruit_RV8803::getWeekday() {
  Adafruit_BusIO_Register weekday_reg(i2c_dev, RV8803_REG_WEEKDAY, 1);
  uint8_t value;
  if (!weekday_reg.read(&value) || value == 0 || value > 0x7F ||
      (value & (value - 1))) {
    return RV8803_READ_ERROR;
  }
  return onehot2weekday(value);
}

/**
 * @brief Read day of month (1-31)
 * @return Day value
 */
uint8_t Adafruit_RV8803::getDate() {
  Adafruit_BusIO_Register reg(i2c_dev, RV8803_REG_DATE, 1);
  uint8_t value;
  if (!reg.read(&value) || (value & 0x0F) > 9 || bcd2bin(value) > 31) {
    return RV8803_READ_ERROR;
  }
  return bcd2bin(value);
}

/**
 * @brief Read month (1-12)
 * @return Month value
 */
uint8_t Adafruit_RV8803::getMonth() {
  Adafruit_BusIO_Register reg(i2c_dev, RV8803_REG_MONTH, 1);
  uint8_t value;
  if (!reg.read(&value) || (value & 0x0F) > 9 || bcd2bin(value) > 12) {
    return RV8803_READ_ERROR;
  }
  return bcd2bin(value);
}

/**
 * @brief Read year (2000-2099)
 * @return Full year value
 */
uint16_t Adafruit_RV8803::getYear() {
  Adafruit_BusIO_Register year_reg(i2c_dev, RV8803_REG_YEAR, 1);
  uint8_t value;
  if (!year_reg.read(&value) || (value & 0x0F) > 9 || (value >> 4) > 9) {
    return 0;
  }
  return 2000 + bcd2bin(value);
}

/**
 * @brief Set the alarm time and mode
 * @param dt DateTime with alarm time (minute, hour, day fields used)
 * @param mode Alarm mode determining which fields participate
 * @return true on success, false on I2C error
 * @note AE bits are inverted: 0=enabled, 1=disabled
 */
bool Adafruit_RV8803::setAlarm(const DateTime& dt, rv8803_alarm_mode_t mode) {
  // Determine AE bit values based on mode
  // Mode bits: [AE_WD][AE_H][AE_M] where 0=enabled, 1=disabled
  uint8_t ae_m = (mode & 0x01) ? 1 : 0;
  uint8_t ae_h = (mode & 0x02) ? 1 : 0;
  uint8_t ae_wd = (mode & 0x04) ? 1 : 0;

  // Minutes alarm register
  Adafruit_BusIO_Register min_alarm_reg(i2c_dev, RV8803_REG_MINUTES_ALARM, 1);
  Adafruit_BusIO_RegisterBits ae_m_bit(&min_alarm_reg, 1, 7);
  uint8_t minutes_alarm = bin2bcd(dt.minute());
  if (!min_alarm_reg.write(minutes_alarm)) {
    return false;
  }
  if (!ae_m_bit.write(ae_m)) {
    return false;
  }

  // Hours alarm register (preserve GP0 bit)
  Adafruit_BusIO_Register hours_alarm_reg(i2c_dev, RV8803_REG_HOURS_ALARM, 1);
  Adafruit_BusIO_RegisterBits ae_h_bit(&hours_alarm_reg, 1, 7);
  Adafruit_BusIO_RegisterBits gp0_bit(&hours_alarm_reg, 1, 6);
  uint8_t gp0_val = gp0_bit.read();
  if (!hours_alarm_reg.write(bin2bcd(dt.hour()) | (gp0_val << 6))) {
    return false;
  }
  if (!ae_h_bit.write(ae_h)) {
    return false;
  }

  // Weekday/Date alarm register
  Adafruit_BusIO_Register ext_reg(i2c_dev, RV8803_REG_EXTENSION, 1);
  Adafruit_BusIO_RegisterBits wada(&ext_reg, 1, 6);

  Adafruit_BusIO_Register wd_alarm_reg(i2c_dev, RV8803_REG_WEEKDAY_DATE_ALARM,
                                       1);
  Adafruit_BusIO_RegisterBits ae_wd_bit(&wd_alarm_reg, 1, 7);
  Adafruit_BusIO_RegisterBits gp1_bit(&wd_alarm_reg, 1, 6);

  if (wada.read()) {
    // Date mode (preserve GP1 bit)
    uint8_t gp1_val = gp1_bit.read();
    if (!wd_alarm_reg.write(bin2bcd(dt.day()) | (gp1_val << 6))) {
      return false;
    }
  } else {
    // Weekday mode (one-hot)
    if (!wd_alarm_reg.write(weekday2onehot(dt.dayOfTheWeek()))) {
      return false;
    }
  }
  if (!ae_wd_bit.write(ae_wd)) {
    return false;
  }

  return true;
}

/**
 * @brief Read the current alarm settings
 * @return DateTime with alarm minute, hour, and day fields
 */
DateTime Adafruit_RV8803::getAlarm() {
  Adafruit_BusIO_Register min_alarm_reg(i2c_dev, RV8803_REG_MINUTES_ALARM, 1);
  Adafruit_BusIO_Register hours_alarm_reg(i2c_dev, RV8803_REG_HOURS_ALARM, 1);
  Adafruit_BusIO_Register wd_alarm_reg(i2c_dev, RV8803_REG_WEEKDAY_DATE_ALARM,
                                       1);
  Adafruit_BusIO_Register ext_reg(i2c_dev, RV8803_REG_EXTENSION, 1);
  Adafruit_BusIO_RegisterBits wada(&ext_reg, 1, 6);

  uint8_t minutes = bcd2bin(min_alarm_reg.read() & 0x7F);
  uint8_t hours = bcd2bin(hours_alarm_reg.read() & 0x3F);

  uint8_t wd_val = wd_alarm_reg.read();
  uint8_t day;

  if (wada.read()) {
    // Date mode
    day = bcd2bin(wd_val & 0x3F);
  } else {
    // Weekday mode - convert one-hot to day number
    day = onehot2weekday(wd_val & 0x7F);
  }

  // Return DateTime with alarm fields (year/month set to minimum)
  return DateTime(2000, 1, day, hours, minutes, 0);
}

/**
 * @brief Set weekday alarm with multi-day mask (WADA=0)
 * @param weekday_mask One-hot mask for days (bit 0=day 1, bit 6=day 7)
 * @return true on success
 * @note Sets WADA=0 for weekday mode
 */
bool Adafruit_RV8803::setAlarmWeekday(uint8_t weekday_mask) {
  if (weekday_mask == 0 || weekday_mask > 0x7F) {
    return false;
  }
  // Set WADA=0 for weekday mode
  Adafruit_BusIO_Register ext_reg(i2c_dev, RV8803_REG_EXTENSION, 1);
  Adafruit_BusIO_RegisterBits wada(&ext_reg, 1, 6);
  if (!wada.write(0)) {
    return false;
  }

  // Read current AE bit and write weekday mask
  Adafruit_BusIO_Register wd_alarm_reg(i2c_dev, RV8803_REG_WEEKDAY_DATE_ALARM,
                                       1);
  Adafruit_BusIO_RegisterBits ae_wd_bit(&wd_alarm_reg, 1, 7);
  uint8_t ae_val = ae_wd_bit.read();
  if (!wd_alarm_reg.write((ae_val << 7) | (weekday_mask & 0x7F))) {
    return false;
  }
  return true;
}

/**
 * @brief Set date alarm (WADA=1)
 * @param date Day of month (1-31)
 * @return true on success
 * @note Sets WADA=1 for date mode
 */
bool Adafruit_RV8803::setAlarmDate(uint8_t date) {
  if (date < 1 || date > 31) {
    return false;
  }
  // Set WADA=1 for date mode
  Adafruit_BusIO_Register ext_reg(i2c_dev, RV8803_REG_EXTENSION, 1);
  Adafruit_BusIO_RegisterBits wada(&ext_reg, 1, 6);
  if (!wada.write(1)) {
    return false;
  }

  // Read current AE bit and GP1 bit, write date
  Adafruit_BusIO_Register wd_alarm_reg(i2c_dev, RV8803_REG_WEEKDAY_DATE_ALARM,
                                       1);
  Adafruit_BusIO_RegisterBits ae_wd_bit(&wd_alarm_reg, 1, 7);
  Adafruit_BusIO_RegisterBits gp1_bit(&wd_alarm_reg, 1, 6);
  uint8_t ae_val = ae_wd_bit.read();
  uint8_t gp1_val = gp1_bit.read();
  if (!wd_alarm_reg.write((ae_val << 7) | (gp1_val << 6) | bin2bcd(date))) {
    return false;
  }
  return true;
}

/**
 * @brief Get the current alarm mode
 * @return rv8803_alarm_mode_t enum value
 */
rv8803_alarm_mode_t Adafruit_RV8803::getAlarmMode() {
  // AE bits are configuration in hardware, including after another begin().
  Adafruit_BusIO_Register alarm_reg(i2c_dev, RV8803_REG_MINUTES_ALARM, 3);
  uint8_t values[3];
  if (!alarm_reg.read(values, sizeof(values))) {
    return (rv8803_alarm_mode_t)RV8803_READ_ERROR;
  }
  return (rv8803_alarm_mode_t)(bitRead(values[0], 7) |
                               (bitRead(values[1], 7) << 1) |
                               (bitRead(values[2], 7) << 2));
}

/**
 * @brief Check if alarm has fired
 * @return true if AF flag is set
 */
bool Adafruit_RV8803::alarmFired() {
  uint8_t flags = readFlagRegister();
  // Check the full-byte failure sentinel before decoding a field: RegisterBits
  // would turn a failed read into an asserted flag.
  if (flags == RV8803_READ_ERROR) {
    return false;
  }
  return (flags & RV8803_FLAG_AF) != 0;
}

/**
 * @brief Clear the alarm flag
 * @return true on success
 */
bool Adafruit_RV8803::clearAlarm() {
  // Write zero only to the requested flag; preserve flags set during I2C
  // access.
  return writeFlagRegister(RV8803_FLAG_MASK & ~RV8803_FLAG_AF);
}

/**
 * @brief Enable the countdown timer
 * @param clock Timer clock source (frequency)
 * @param value Countdown ticks (1-4095); zero is rejected
 * @return true on success
 * @note Preserves GP2-GP5. Each call restarts the timer. Manual section 4.5.3
 * allows an extra source-clock tick in the first interval (plus 61 us at
 * 4096 Hz). Automatic reloads take value / clock seconds. At 1 Hz, value=5
 * initially takes 5-6 seconds, then repeats every 5 seconds. The hardware
 * treats a zero preset as stopped.
 */
bool Adafruit_RV8803::enableCountdownTimer(rv8803_timer_clock_t clock,
                                           uint16_t value) {
  if (value == 0 || value > 4095) {
    return false;
  }
  // Manual section 4.5.2: stop before changing the preset or source clock.
  if (!disableCountdownTimer()) {
    return false;
  }

  // Write lower 8 bits
  Adafruit_BusIO_Register tc0_reg(i2c_dev, RV8803_REG_TIMER_COUNTER0, 1);
  if (!tc0_reg.write(value & 0xFF)) {
    return false;
  }

  // Write upper 4 bits, preserving GP2-GP5
  Adafruit_BusIO_Register tc1_reg(i2c_dev, RV8803_REG_TIMER_COUNTER1, 1);
  Adafruit_BusIO_RegisterBits timer_upper(&tc1_reg, 4, 0);
  if (!timer_upper.write((value >> 8) & 0x0F)) {
    return false;
  }

  // Set TD bits and enable timer (TE=1)
  Adafruit_BusIO_Register ext_reg(i2c_dev, RV8803_REG_EXTENSION, 1);
  Adafruit_BusIO_RegisterBits td(&ext_reg, 2, 0); // TD is bits 0-1
  Adafruit_BusIO_RegisterBits te(&ext_reg, 1, 4); // TE is bit 4
  if (!td.write(clock & 0x03)) {
    return false;
  }
  return te.write(1);
}

/**
 * @brief Disable the countdown timer
 * @return true on success
 */
bool Adafruit_RV8803::disableCountdownTimer() {
  Adafruit_BusIO_Register ext_reg(i2c_dev, RV8803_REG_EXTENSION, 1);
  Adafruit_BusIO_RegisterBits te(&ext_reg, 1, 4); // TE is bit 4
  return te.write(0);
}

/**
 * @brief Read the countdown timer preset value
 * @return 12-bit timer value (not live countdown)
 */
uint16_t Adafruit_RV8803::getCountdownTimer() {
  // Read both preset bytes together; upper four bits are GP storage.
  Adafruit_BusIO_Register timer_reg(i2c_dev, RV8803_REG_TIMER_COUNTER0, 2);
  uint16_t value;
  if (!timer_reg.read(&value)) {
    return UINT16_MAX;
  }
  return value & 4095;
}

/**
 * @brief Check if timer has fired
 * @return true if TF flag is set
 */
bool Adafruit_RV8803::timerFired() {
  uint8_t flags = readFlagRegister();
  // Check the full-byte failure sentinel before decoding a field: RegisterBits
  // would turn a failed read into an asserted flag.
  if (flags == RV8803_READ_ERROR) {
    return false;
  }
  return (flags & RV8803_FLAG_TF) != 0;
}

/**
 * @brief Clear the timer flag
 * @return true on success
 */
bool Adafruit_RV8803::clearTimer() {
  // Write zero only to the requested flag; preserve flags set during I2C
  // access.
  return writeFlagRegister(RV8803_FLAG_MASK & ~RV8803_FLAG_TF);
}

/**
 * @brief Set periodic update mode (second or minute)
 * @param mode RV8803_UpdateSecond or RV8803_UpdateMinute
 * @return true on success
 */
bool Adafruit_RV8803::setUpdateMode(rv8803_update_mode_t mode) {
  Adafruit_BusIO_Register ext_reg(i2c_dev, RV8803_REG_EXTENSION, 1);
  Adafruit_BusIO_RegisterBits usel(&ext_reg, 1, 5); // USEL is bit 5
  return usel.write(mode == RV8803_UpdateMinute ? 1 : 0);
}

/**
 * @brief Check if periodic update flag is set
 * @return true if UF flag is set
 */
bool Adafruit_RV8803::updateFired() {
  uint8_t flags = readFlagRegister();
  // Check the full-byte failure sentinel before decoding a field: RegisterBits
  // would turn a failed read into an asserted flag.
  if (flags == RV8803_READ_ERROR) {
    return false;
  }
  return (flags & RV8803_FLAG_UF) != 0;
}

/**
 * @brief Clear the periodic update flag
 * @return true on success
 */
bool Adafruit_RV8803::clearUpdate() {
  // Write zero only to the requested flag; preserve flags set during I2C
  // access.
  return writeFlagRegister(RV8803_FLAG_MASK & ~RV8803_FLAG_UF);
}

/**
 * @brief Configure external event detection
 * @param rising_edge true for rising edge, false for falling edge
 * @param filter Event filter time
 * @return true on success
 */
bool Adafruit_RV8803::configureEvent(bool rising_edge,
                                     rv8803_event_filter_t filter) {
  Adafruit_BusIO_Register evctrl_reg(i2c_dev, RV8803_REG_EVENT_CONTROL, 1);
  Adafruit_BusIO_RegisterBits ehl(&evctrl_reg, 1, 6); // EHL is bit 6
  Adafruit_BusIO_RegisterBits et(&evctrl_reg, 2, 4);  // ET is bits 4-5
  if (!ehl.write(rising_edge ? 1 : 0)) {
    return false;
  }
  return et.write(filter & 0x03);
}

/**
 * @brief Enable or disable event timestamp capture
 * @param enable true to enable capture
 * @return true on success
 */
bool Adafruit_RV8803::enableEventCapture(bool enable) {
  Adafruit_BusIO_Register evctrl_reg(i2c_dev, RV8803_REG_EVENT_CONTROL, 1);
  Adafruit_BusIO_RegisterBits ecp(&evctrl_reg, 1, 7); // ECP is bit 7
  return ecp.write(enable ? 1 : 0);
}

/**
 * @brief Enable or disable auto-reset of hundredths on event
 * @param enable true to enable auto-reset
 * @return true on success
 */
bool Adafruit_RV8803::enableEventReset(bool enable) {
  Adafruit_BusIO_Register evctrl_reg(i2c_dev, RV8803_REG_EVENT_CONTROL, 1);
  Adafruit_BusIO_RegisterBits erst(&evctrl_reg, 1, 0); // ERST is bit 0
  return erst.write(enable ? 1 : 0);
}

/**
 * @brief Read both external-event capture registers in one transaction
 * @param timestamp Destination for seconds and hundredths; unchanged on error
 * @return true on success, false on I2C error or invalid capture data
 * @note A later input event can replace the capture. Disable capture before
 * reading if events can recur during I2C access.
 */
bool Adafruit_RV8803::getEventTimestamp(rv8803_timestamp_t* timestamp) {
  if (!timestamp) {
    return false;
  }
  Adafruit_BusIO_Register reg(i2c_dev, RV8803_REG_HUNDREDTHS_CP, 2);
  uint8_t values[2];
  if (!reg.read(values, sizeof(values)) || (values[0] & 0x0F) > 9 ||
      bcd2bin(values[0]) > 99 || (values[1] & 0x0F) > 9 ||
      bcd2bin(values[1]) > 59) {
    return false;
  }
  timestamp->hundredths = bcd2bin(values[0]);
  timestamp->seconds = bcd2bin(values[1]);
  return true;
}

/**
 * @brief Read captured hundredths from external event
 * @return Hundredths value (0-99)
 */
uint8_t Adafruit_RV8803::getEventHundredths() {
  Adafruit_BusIO_Register reg(i2c_dev, RV8803_REG_HUNDREDTHS_CP, 1);
  uint8_t value;
  if (!reg.read(&value) || (value & 0x0F) > 9 || bcd2bin(value) > 99) {
    return RV8803_READ_ERROR;
  }
  return bcd2bin(value);
}

/**
 * @brief Read captured seconds from external event
 * @return Seconds value (0-59)
 */
uint8_t Adafruit_RV8803::getEventSeconds() {
  Adafruit_BusIO_Register reg(i2c_dev, RV8803_REG_SECONDS_CP, 1);
  uint8_t value;
  if (!reg.read(&value) || (value & 0x0F) > 9 || bcd2bin(value) > 59) {
    return RV8803_READ_ERROR;
  }
  return bcd2bin(value);
}

/**
 * @brief Check if external event flag is set
 * @return true if EVF flag is set
 */
bool Adafruit_RV8803::eventFired() {
  uint8_t flags = readFlagRegister();
  // Check the full-byte failure sentinel before decoding a field: RegisterBits
  // would turn a failed read into an asserted flag.
  if (flags == RV8803_READ_ERROR) {
    return false;
  }
  return (flags & RV8803_FLAG_EVF) != 0;
}

/**
 * @brief Clear the external event flag
 * @return true on success
 */
bool Adafruit_RV8803::clearEvent() {
  // Write zero only to the requested flag; preserve flags set during I2C
  // access.
  return writeFlagRegister(RV8803_FLAG_MASK & ~RV8803_FLAG_EVF);
}

/**
 * @brief Enable an interrupt source
 * @param source Interrupt source to enable
 * @return true on success
 */
bool Adafruit_RV8803::enableInterrupt(rv8803_interrupt_t source) {
  Adafruit_BusIO_Register ctrl_reg(i2c_dev, RV8803_REG_CONTROL, 1);
  uint8_t ctrl;
  if (!ctrl_reg.read(&ctrl)) {
    return false;
  }
  ctrl |= source;
  return ctrl_reg.write(ctrl);
}

/**
 * @brief Disable an interrupt source
 * @param source Interrupt source to disable
 * @return true on success
 */
bool Adafruit_RV8803::disableInterrupt(rv8803_interrupt_t source) {
  Adafruit_BusIO_Register ctrl_reg(i2c_dev, RV8803_REG_CONTROL, 1);
  uint8_t ctrl;
  if (!ctrl_reg.read(&ctrl)) {
    return false;
  }
  ctrl &= ~source;
  return ctrl_reg.write(ctrl);
}

/**
 * @brief Set the CLKOUT frequency
 * @param mode Square wave frequency mode
 * @return true on success
 * @note CLKOE pin must be tied HIGH for CLKOUT to work
 */
bool Adafruit_RV8803::writeSqwPinMode(rv8803_sqw_mode_t mode) {
  Adafruit_BusIO_Register ext_reg(i2c_dev, RV8803_REG_EXTENSION, 1);
  Adafruit_BusIO_RegisterBits fd(&ext_reg, 2, 2); // FD is bits 2-3
  return fd.write(mode & 0x03);
}

/**
 * @brief Read the current CLKOUT frequency setting
 * @return Square wave frequency mode
 */
rv8803_sqw_mode_t Adafruit_RV8803::readSqwPinMode() {
  Adafruit_BusIO_Register ext_reg(i2c_dev, RV8803_REG_EXTENSION, 1);
  Adafruit_BusIO_RegisterBits fd(&ext_reg, 2, 2); // FD is bits 2-3
  return (rv8803_sqw_mode_t)fd.read();
}

/**
 * @brief Set the offset calibration value
 * @param offset 6-bit two's complement (-32 to +31), 0.2384 ppm/step
 * @return true on success
 */
bool Adafruit_RV8803::calibrate(int8_t offset) {
  // Clamp to 6-bit signed range
  if (offset > 31)
    offset = 31;
  if (offset < -32)
    offset = -32;

  Adafruit_BusIO_Register offset_reg(i2c_dev, RV8803_REG_OFFSET, 1);
  Adafruit_BusIO_RegisterBits offset_bits(&offset_reg, 6, 0);
  return offset_bits.write(offset & 0x3F);
}

/**
 * @brief Read the current calibration offset
 * @return Signed offset value (-32 to +31)
 */
int8_t Adafruit_RV8803::getCalibration() {
  Adafruit_BusIO_Register offset_reg(i2c_dev, RV8803_REG_OFFSET, 1);
  Adafruit_BusIO_RegisterBits offset_bits(&offset_reg, 6, 0);
  uint8_t regval = offset_bits.read();
  // Sign-extend from 6 bits
  if (regval & 0x20) {
    return (int8_t)(regval | 0xC0);
  }
  return (int8_t)regval;
}

/**
 * @brief Check if temperature compensation was interrupted
 * @return true if V1F flag is set
 */
bool Adafruit_RV8803::tempCompStopped() {
  uint8_t flags = readFlagRegister();
  // Check the full-byte failure sentinel before decoding a field: RegisterBits
  // would turn a failed read into an asserted flag.
  if (flags == RV8803_READ_ERROR) {
    return true;
  }
  return (flags & RV8803_FLAG_V1F) != 0;
}

/**
 * @brief Clear both power flags (V1F and V2F)
 * @return true on success
 */
bool Adafruit_RV8803::clearPowerFlags() {
  // V1F/V2F clear together. Preserve all interrupt flags, including new events.
  return writeFlagRegister(RV8803_FLAG_MASK &
                           ~(RV8803_FLAG_V1F | RV8803_FLAG_V2F));
}

/**
 * @brief Write to the 1-byte RAM register
 * @param value Byte to store
 * @return true on success
 */
bool Adafruit_RV8803::writeRAM(uint8_t value) {
  Adafruit_BusIO_Register ram_reg(i2c_dev, RV8803_REG_RAM, 1);
  return ram_reg.write(value);
}

/**
 * @brief Read from the 1-byte RAM register
 * @return Stored byte value
 */
uint8_t Adafruit_RV8803::readRAM() {
  Adafruit_BusIO_Register ram_reg(i2c_dev, RV8803_REG_RAM, 1);
  return ram_reg.read();
}

/**
 * @brief Write to general purpose bits (GP0-GP5)
 * @param bits 6-bit value (bit 0=GP0, bit 5=GP5)
 * @return true on success
 * @note Requires date alarm mode (WADA=1); GP1 is Saturday in weekday mode.
 */
bool Adafruit_RV8803::writeGP(uint8_t bits) {
  // GP1 shares Saturday's alarm bit in weekday mode. Refuse a write there
  // rather than silently modifying the alarm's selected weekdays.
  uint8_t extension = readExtensionRegister();
  if (extension == RV8803_READ_ERROR || !(extension & RV8803_EXT_WADA) ||
      bits > 63) {
    return false;
  }
  // GP0 is in Hours Alarm [6]
  Adafruit_BusIO_Register hours_alarm_reg(i2c_dev, RV8803_REG_HOURS_ALARM, 1);
  Adafruit_BusIO_RegisterBits gp0(&hours_alarm_reg, 1, 6);
  if (!gp0.write(bits & 0x01)) {
    return false;
  }

  // GP1 is in Weekday/Date Alarm [6]
  Adafruit_BusIO_Register wd_alarm_reg(i2c_dev, RV8803_REG_WEEKDAY_DATE_ALARM,
                                       1);
  Adafruit_BusIO_RegisterBits gp1(&wd_alarm_reg, 1, 6);
  if (!gp1.write((bits >> 1) & 0x01)) {
    return false;
  }

  // GP2-GP5 are in Timer Counter 1 [7:4]
  Adafruit_BusIO_Register tc1_reg(i2c_dev, RV8803_REG_TIMER_COUNTER1, 1);
  Adafruit_BusIO_RegisterBits gp2_5(&tc1_reg, 4, 4);
  return gp2_5.write((bits >> 2) & 0x0F);
}

/**
 * @brief Read general purpose bits (GP0-GP5)
 * @return 6-bit value (bit 0=GP0, bit 5=GP5)
 */
uint8_t Adafruit_RV8803::readGP() {
  uint8_t result = 0;

  // GP0 is in Hours Alarm [6]
  Adafruit_BusIO_Register hours_alarm_reg(i2c_dev, RV8803_REG_HOURS_ALARM, 1);
  Adafruit_BusIO_RegisterBits gp0(&hours_alarm_reg, 1, 6);
  result |= gp0.read();

  // GP1 is in Weekday/Date Alarm [6]
  Adafruit_BusIO_Register wd_alarm_reg(i2c_dev, RV8803_REG_WEEKDAY_DATE_ALARM,
                                       1);
  Adafruit_BusIO_RegisterBits gp1(&wd_alarm_reg, 1, 6);
  result |= (gp1.read() << 1);

  // GP2-GP5 are in Timer Counter 1 [7:4]
  Adafruit_BusIO_Register tc1_reg(i2c_dev, RV8803_REG_TIMER_COUNTER1, 1);
  Adafruit_BusIO_RegisterBits gp2_5(&tc1_reg, 4, 4);
  result |= (gp2_5.read() << 2);

  return result;
}

/**
 * @brief Read the Extension Register (0x0D)
 * @return Register value
 */
uint8_t Adafruit_RV8803::readExtensionRegister() {
  Adafruit_BusIO_Register ext_reg(i2c_dev, RV8803_REG_EXTENSION, 1);
  return ext_reg.read();
}

/**
 * @brief Write to the Extension Register (0x0D)
 * @param value Value to write (TEST bit should be 0)
 * @return true on success
 */
bool Adafruit_RV8803::writeExtensionRegister(uint8_t value) {
  value &= ~RV8803_EXT_TEST; // Ensure TEST bit is always 0
  Adafruit_BusIO_Register ext_reg(i2c_dev, RV8803_REG_EXTENSION, 1);
  return ext_reg.write(value);
}

/**
 * @brief Read the Flag Register (0x0E)
 * @return Register value
 */
uint8_t Adafruit_RV8803::readFlagRegister() {
  Adafruit_BusIO_Register flag_reg(i2c_dev, RV8803_REG_FLAG, 1);
  return flag_reg.read();
}

/**
 * @brief Write to the Flag Register (0x0E)
 * @param value Value to write (write 0 to clear flags)
 * @return true on success
 */
bool Adafruit_RV8803::writeFlagRegister(uint8_t value) {
  Adafruit_BusIO_Register flag_reg(i2c_dev, RV8803_REG_FLAG, 1);
  return flag_reg.write(value & RV8803_FLAG_MASK);
}

/**
 * @brief Read the Control Register (0x0F)
 * @return Register value
 */
uint8_t Adafruit_RV8803::readControlRegister() {
  Adafruit_BusIO_Register ctrl_reg(i2c_dev, RV8803_REG_CONTROL, 1);
  return ctrl_reg.read();
}

/**
 * @brief Write to the Control Register (0x0F)
 * @param value Value to write
 * @return true on success
 */
bool Adafruit_RV8803::writeControlRegister(uint8_t value) {
  Adafruit_BusIO_Register ctrl_reg(i2c_dev, RV8803_REG_CONTROL, 1);
  return ctrl_reg.write(value & RV8803_CTRL_MASK);
}

/**
 * @brief Read the Event Control Register (0x2F)
 * @return Register value
 */
uint8_t Adafruit_RV8803::readEventControl() {
  Adafruit_BusIO_Register evctrl_reg(i2c_dev, RV8803_REG_EVENT_CONTROL, 1);
  return evctrl_reg.read();
}

/**
 * @brief Write to the Event Control Register (0x2F)
 * @param value Value to write
 * @return true on success
 */
bool Adafruit_RV8803::writeEventControl(uint8_t value) {
  Adafruit_BusIO_Register evctrl_reg(i2c_dev, RV8803_REG_EVENT_CONTROL, 1);
  return evctrl_reg.write(value);
}

/**
 * @brief Perform software reset
 * @return true on success
 * @note Resets and releases the prescaler; preserves time/calendar and
 * settings.
 */
bool Adafruit_RV8803::reset() {
  // Manual section 4.13: RESET holds the prescaler; it does not erase registers
  // or clear itself. Pulse it to restart a full second without losing the date.
  Adafruit_BusIO_Register ctrl_reg(i2c_dev, RV8803_REG_CONTROL, 1);
  Adafruit_BusIO_RegisterBits reset_bit(&ctrl_reg, 1, 0);
  if (!reset_bit.write(1)) {
    return false;
  }
  return reset_bit.write(0);
}

/**
 * @brief Convert weekday (0-6) to one-hot encoding
 * @param day Weekday number (0=Sunday, 6=Saturday)
 * @return One-hot encoded byte (bit 0 = Sunday)
 */
uint8_t Adafruit_RV8803::weekday2onehot(uint8_t day) {
  if (day > 6)
    day = 0;
  return 1 << day;
}

/**
 * @brief Convert one-hot encoding to weekday (0-6)
 * @param bits One-hot encoded byte
 * @return Weekday number (0=Sunday, 6=Saturday)
 */
uint8_t Adafruit_RV8803::onehot2weekday(uint8_t bits) {
  for (uint8_t i = 0; i < 7; i++) {
    if (bits & (1 << i)) {
      return i;
    }
  }
  return 0; // Default to Sunday if no bit set
}
