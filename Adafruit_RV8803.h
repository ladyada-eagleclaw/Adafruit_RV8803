/*!
 * @file Adafruit_RV8803.h
 *
 * This is a library for the RV-8803-C7 Real-Time Clock
 *
 * Written by Limor 'ladyada' Fried with assistance from Claude Code
 *
 * MIT license, all text above must be included in any redistribution
 */

#ifndef ADAFRUIT_RV8803_H
#define ADAFRUIT_RV8803_H

#include <RTClib.h>

/** RV8803 I2C address (fixed) */
#define RV8803_I2C_ADDRESS 0x32

/*=========================================================================
    REGISTER DEFINITIONS
    -----------------------------------------------------------------------*/

/** @name Time/Date Registers (BCD-encoded)
 *  @{ */
#define RV8803_REG_SECONDS 0x00    /**< Seconds register (0-59 BCD) */
#define RV8803_REG_MINUTES 0x01    /**< Minutes register (0-59 BCD) */
#define RV8803_REG_HOURS 0x02      /**< Hours register (0-23 BCD, 24hr only) */
#define RV8803_REG_WEEKDAY 0x03    /**< Weekday register (one-hot encoding) */
#define RV8803_REG_DATE 0x04       /**< Date register (1-31 BCD) */
#define RV8803_REG_MONTH 0x05      /**< Month register (1-12 BCD) */
#define RV8803_REG_YEAR 0x06       /**< Year register (0-99 BCD, 2000-2099) */
#define RV8803_REG_RAM 0x07        /**< 1-byte user RAM */
#define RV8803_REG_HUNDREDTHS 0x10 /**< 100th seconds (0-99 BCD, read-only) */
/** @} */

/** @name Alarm Registers
 *  @{ */
#define RV8803_REG_MINUTES_ALARM 0x08 /**< Alarm minutes (BCD + AE_M bit) */
#define RV8803_REG_HOURS_ALARM 0x09   /**< Alarm hours (BCD + AE_H + GP0) */
#define RV8803_REG_WEEKDAY_DATE_ALARM 0x0A /**< Alarm weekday/date + AE_WD */
/** @} */

/** @name Timer Registers
 *  @{ */
#define RV8803_REG_TIMER_COUNTER0 0x0B /**< Timer lower 8 bits */
#define RV8803_REG_TIMER_COUNTER1 0x0C /**< Timer upper 4 bits + GP2-GP5 */
/** @} */

/** @name Control Registers
 *  @{ */
#define RV8803_REG_EXTENSION 0x0D /**< Extension register */
#define RV8803_REG_FLAG 0x0E      /**< Flag register */
#define RV8803_REG_CONTROL 0x0F   /**< Control register */
/** @} */

/** @name Event Capture Registers (read-only)
 *  @{ */
#define RV8803_REG_HUNDREDTHS_CP 0x20 /**< Captured 100ths on event */
#define RV8803_REG_SECONDS_CP 0x21    /**< Captured seconds on event */
/** @} */

/** @name Calibration & Event Control
 *  @{ */
#define RV8803_REG_OFFSET 0x2C        /**< Offset calibration register */
#define RV8803_REG_EVENT_CONTROL 0x2F /**< Event control register */
/** @} */

/*=========================================================================
    BIT DEFINITIONS
    -----------------------------------------------------------------------*/

/** @name Extension Register Bits (0x0D)
 *  @{ */
#define RV8803_EXT_TEST 0x80    /**< Test mode (must be 0) */
#define RV8803_EXT_WADA 0x40    /**< 0=weekday alarm, 1=date alarm */
#define RV8803_EXT_USEL 0x20    /**< 0=second update, 1=minute update */
#define RV8803_EXT_TE 0x10      /**< Timer enable */
#define RV8803_EXT_FD_MASK 0x0C /**< CLKOUT frequency mask */
#define RV8803_EXT_FD_SHIFT 2   /**< CLKOUT frequency shift */
#define RV8803_EXT_TD_MASK 0x03 /**< Timer clock source mask */
/** @} */

/** @name Flag Register Bits (0x0E)
 *  @{ */
#define RV8803_FLAG_UF 0x20  /**< Periodic update flag */
#define RV8803_FLAG_TF 0x10  /**< Timer flag */
#define RV8803_FLAG_AF 0x08  /**< Alarm flag */
#define RV8803_FLAG_EVF 0x04 /**< External event flag */
#define RV8803_FLAG_V2F 0x02 /**< Voltage low 2 - data invalid */
#define RV8803_FLAG_V1F 0x01 /**< Voltage low 1 - temp comp stopped */
/** @} */

/** @name Control Register Bits (0x0F)
 *  @{ */
#define RV8803_CTRL_UIE 0x20 /**< Update interrupt enable */
#define RV8803_CTRL_TIE 0x10 /**< Timer interrupt enable */
#define RV8803_CTRL_AIE 0x08 /**< Alarm interrupt enable */
#define RV8803_CTRL_EIE 0x04 /**< Event interrupt enable */
#define RV8803_CTRL_RESET \
  0x01 /**< Prescaler reset/stop (software must clear) */
/** @} */

/** @name Event Control Register Bits (0x2F)
 *  @{ */
#define RV8803_EVCTRL_ECP 0x80     /**< Event capture enable */
#define RV8803_EVCTRL_EHL 0x40     /**< Event polarity (0=falling, 1=rising) */
#define RV8803_EVCTRL_ET_MASK 0x30 /**< Event filter mask */
#define RV8803_EVCTRL_ET_SHIFT 4   /**< Event filter shift */
#define RV8803_EVCTRL_ERST 0x01    /**< Event reset (auto-clear on event) */
/** @} */

/** All writable status flags; writing one preserves an existing flag. */
#define RV8803_FLAG_MASK 0x3F
/** Writable control bits; reserved bits must stay zero. */
#define RV8803_CTRL_MASK 0x3D
/** Six-bit signed offset encoding. */
#define RV8803_OFFSET_MASK 0x3F
/** Invalid byte result after an I2C read failure. */
#define RV8803_READ_ERROR 0xFF

/** @name Alarm AE Bits (inverted: 0=enabled, 1=disabled)
 *  @{ */
#define RV8803_ALARM_AE_M 0x80  /**< Minutes alarm enable bit */
#define RV8803_ALARM_AE_H 0x80  /**< Hours alarm enable bit */
#define RV8803_ALARM_AE_WD 0x80 /**< Weekday/date alarm enable bit */
/** @} */

/*=========================================================================
    ENUMS
    -----------------------------------------------------------------------*/

/** CLKOUT frequency (FD bits, Extension Register) */
typedef enum {
  RV8803_SquareWave32kHz = 0x00, /**< 32.768 kHz (default) */
  RV8803_SquareWave1kHz = 0x01,  /**< 1024 Hz */
  RV8803_SquareWave1Hz = 0x02,   /**< 1 Hz */
} rv8803_sqw_mode_t;

/** Countdown timer clock source (TD bits, Extension Register) */
typedef enum {
  RV8803_Timer4096Hz = 0x00, /**< 244.14 µs/tick, max ~1s */
  RV8803_Timer64Hz = 0x01,   /**< 15.625 ms/tick, max ~64s */
  RV8803_Timer1Hz = 0x02,    /**< 1 s/tick, max ~68 min */
  RV8803_Timer1_60Hz = 0x03, /**< 60 s/tick, max ~68 hr */
} rv8803_timer_clock_t;

/** Alarm mode — which fields participate in match */
typedef enum {
  RV8803_A_MinHourDay = 0x00,  /**< Min + Hour + Weekday/Date (default) */
  RV8803_A_HourDay = 0x01,     /**< Hour + Weekday/Date */
  RV8803_A_MinDay = 0x02,      /**< Min + Weekday/Date (hourly) */
  RV8803_A_Day = 0x03,         /**< Weekday/Date only */
  RV8803_A_HourMin = 0x04,     /**< Hour + Min (daily) */
  RV8803_A_Hour = 0x05,        /**< Hour only (daily) */
  RV8803_A_Minute = 0x06,      /**< Minute only (hourly) */
  RV8803_A_EveryMinute = 0x07, /**< Every minute */
} rv8803_alarm_mode_t;

/** Event filter time (ET bits, Event Control Register) */
typedef enum {
  RV8803_EventFilterNone = 0x00,  /**< No filter, 30.5 µs min pulse */
  RV8803_EventFilter4ms = 0x01,   /**< 3.9 ms sampling */
  RV8803_EventFilter16ms = 0x02,  /**< 15.6 ms sampling */
  RV8803_EventFilter125ms = 0x03, /**< 125 ms sampling */
} rv8803_event_filter_t;

/** Periodic update mode (USEL bit) */
typedef enum {
  RV8803_UpdateSecond = 0x00, /**< Update flag every second */
  RV8803_UpdateMinute = 0x01, /**< Update flag every minute */
} rv8803_update_mode_t;

/** Interrupt sources (bits in Control Register) */
typedef enum {
  RV8803_InterruptUpdate = 0x20, /**< UIE — periodic update */
  RV8803_InterruptTimer = 0x10,  /**< TIE — countdown timer */
  RV8803_InterruptAlarm = 0x08,  /**< AIE — alarm */
  RV8803_InterruptEvent = 0x04,  /**< EIE — external event */
} rv8803_interrupt_t;

/** Captured external-event time within a minute (not a full date/time). */
typedef struct {
  uint8_t seconds;    /**< Captured seconds, 0-59. */
  uint8_t hundredths; /**< Captured hundredths, 0-99. */
} rv8803_timestamp_t;

/*=========================================================================
    CLASS
    -----------------------------------------------------------------------*/

/**
 * @brief Driver for the RV-8803-C7 Real-Time Clock
 *
 * This class inherits from RTC_I2C (RTClib base class) and provides
 * full access to the RV8803's features including time/date, alarm,
 * countdown timer, periodic update interrupt, external event capture,
 * CLKOUT, offset calibration, and 1-byte RAM.
 */
class Adafruit_RV8803 : public RTC_I2C {
 public:
  // Core — Init & Time (RTClib compatible)
  Adafruit_RV8803() = default;
  ~Adafruit_RV8803();
  Adafruit_RV8803(const Adafruit_RV8803&) = delete;
  Adafruit_RV8803& operator=(const Adafruit_RV8803&) = delete;
  bool begin(TwoWire* wire = &Wire);
  DateTime now();
  bool adjust(const DateTime& dt);
  bool lostPower();
  bool isrunning();
  uint8_t getHundredths();
  uint8_t getSeconds();
  uint8_t getMinutes();
  uint8_t getHours();
  uint8_t getWeekday();
  uint8_t getDate();
  uint8_t getMonth();
  uint16_t getYear();

  // Alarm (DS3231-style)
  bool setAlarm(const DateTime& dt, rv8803_alarm_mode_t mode);
  DateTime getAlarm();
  bool setAlarmWeekday(uint8_t weekday_mask);
  bool setAlarmDate(uint8_t date);
  rv8803_alarm_mode_t getAlarmMode();
  bool alarmFired();
  bool clearAlarm();

  // Countdown Timer (PCF8523-style)
  bool enableCountdownTimer(rv8803_timer_clock_t clock, uint16_t value);
  bool disableCountdownTimer();
  uint16_t getCountdownTimer();
  bool timerFired();
  bool clearTimer();

  // Periodic Update
  bool setUpdateMode(rv8803_update_mode_t mode);
  bool updateFired();
  bool clearUpdate();

  // External Event / Timestamp
  bool configureEvent(bool rising_edge, rv8803_event_filter_t filter);
  bool enableEventCapture(bool enable);
  bool enableEventReset(bool enable);
  bool getEventTimestamp(rv8803_timestamp_t* timestamp);
  uint8_t getEventHundredths();
  uint8_t getEventSeconds();
  bool eventFired();
  bool clearEvent();

  // Interrupts
  bool enableInterrupt(rv8803_interrupt_t source);
  bool disableInterrupt(rv8803_interrupt_t source);

  // CLKOUT (RTClib SQW-style naming)
  bool writeSqwPinMode(rv8803_sqw_mode_t mode);
  rv8803_sqw_mode_t readSqwPinMode();

  // Offset Calibration (PCF8523-style)
  bool calibrate(int8_t offset);
  int8_t getCalibration();

  // Power Management
  bool tempCompStopped();
  bool clearPowerFlags();

  // RAM & GP Bits
  bool writeRAM(uint8_t value);
  uint8_t readRAM();
  bool writeGP(uint8_t bits);
  uint8_t readGP();

  // Direct Register Access
  uint8_t readExtensionRegister();
  bool writeExtensionRegister(uint8_t value);
  uint8_t readFlagRegister();
  bool writeFlagRegister(uint8_t value);
  uint8_t readControlRegister();
  bool writeControlRegister(uint8_t value);
  uint8_t readEventControl();
  bool writeEventControl(uint8_t value);

  // Reset
  bool reset();

 private:
  static uint8_t weekday2onehot(uint8_t day);
  static uint8_t onehot2weekday(uint8_t bits);
};

#endif // ADAFRUIT_RV8803_H
