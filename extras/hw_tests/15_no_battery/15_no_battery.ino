// Metro Mini: VIN=A0, CLOE=D2, INT=D3, EVI=D4, SQW=D5, SDA=A4, SCL=A5.
// Remove the coin cell and any other power source before running this test.
// This tests loss of backup power; the RV8803 cannot identify a missing cell
// while VIN is present. This test changes the RTC time.
#include <Adafruit_RV8803.h>

Adafruit_RV8803 rtc;
const uint16_t powerPin = A0;
const uint16_t sdaPin = A4;
const uint16_t sclPin = A5;
// The diode-isolated VDD capacitor can keep this low-power RTC alive after VIN
// is removed. Allow five minutes for discharge; adjust for the actual board.
const unsigned long powerOffMs = 300000;

void setup() {
  Serial.begin(115200);
  // Wait for Serial Monitor on native USB boards; remove for standalone use.
  while (!Serial) delay(10);
  delay(250);
  Serial.println(F("Adafruit RV8803 no-battery power-loss test"));
  Serial.println(F("Requires: coin cell removed, VIN powered only by A0"));

  // Release every auxiliary connection so no MCU output or pull-up can supply
  // the RTC while VIN is off. The breakout provides its own input biasing.
  for (uint16_t pin = 2; pin <= 5; pin++) pinMode(pin, INPUT);
  digitalWrite(powerPin, HIGH);
  pinMode(powerPin, OUTPUT);
  delay(100);
  check(rtc.begin(), F("Begin succeeded"));
  check(rtc.adjust(DateTime(2026, 1, 1, 12, 0, 0)), F("Reference time set"));
  uint8_t flags = rtc.readFlagRegister();
  check(flags != RV8803_READ_ERROR, F("Initial flags read successfully"));
  check(!(flags & (RV8803_FLAG_V1F | RV8803_FLAG_V2F)),
        F("Power flags initially clear"));
  check(!rtc.lostPower(), F("lostPower is false before removing VIN"));

  Serial.println();
  Serial.print(F("Removing VIN for "));
  Serial.print(powerOffMs / 1000);
  Serial.println(F(" seconds to discharge the backup supply capacitor"));
  Serial.flush();
  Wire.end();
  // Disable Wire's pull-ups and hold the bus low to prevent back-powering.
  digitalWrite(sdaPin, LOW);
  digitalWrite(sclPin, LOW);
  pinMode(sdaPin, OUTPUT);
  pinMode(sclPin, OUTPUT);
  digitalWrite(powerPin, LOW);
  delay(powerOffMs);

  restorePower();
  check(rtc.begin(), F("RTC communicates after power is restored"));
  // Read the full register first: lostPower() also returns true on I2C error.
  // Do not adjust the time or clear flags until power loss has been verified.
  flags = rtc.readFlagRegister();
  check(flags != RV8803_READ_ERROR, F("Post-cycle flags read successfully"));
  Serial.print(F("Flags after power cycle: 0x"));
  Serial.println(flags, HEX);
  check(flags & RV8803_FLAG_V2F,
        F("V2F asserts after power loss; if absent, check battery, other power, and discharge time"));
  check(rtc.lostPower(), F("lostPower detects invalid time after power loss"));

  check(rtc.begin(), F("Second begin succeeded"));
  flags = rtc.readFlagRegister();
  check(flags != RV8803_READ_ERROR && (flags & RV8803_FLAG_V2F),
        F("begin preserves the power-loss evidence"));

  Serial.println();
  check(rtc.adjust(DateTime(2026, 1, 1, 12, 0, 0)), F("Time reinitialized"));
  flags = rtc.readFlagRegister();
  check(flags != RV8803_READ_ERROR &&
        !(flags & (RV8803_FLAG_V1F | RV8803_FLAG_V2F)),
        F("adjust cleared both power flags"));
  check(!rtc.lostPower(), F("lostPower is false after recovery"));
  DateTime before = rtc.now();
  check(before.isValid(), F("Recovered time is valid"));
  delay(1100);
  DateTime after = rtc.now();
  check(after.isValid() && (after - before).totalseconds() >= 1 &&
        (after - before).totalseconds() <= 2,
        F("Clock advances after recovery"));
  Serial.println(F("ALL PASSED"));
}

void loop() {}

void restorePower() {
  pinMode(sdaPin, INPUT);
  pinMode(sclPin, INPUT);
  digitalWrite(powerPin, HIGH);
  pinMode(powerPin, OUTPUT);
  delay(250);
  Wire.begin();
}

void clearOutputsAndHalt(const __FlashStringHelper* message) {
  Serial.print(F("FAIL: "));
  Serial.println(message);
  restorePower(); // Leave power available and preserve status flags for diagnosis.
  while (true) delay(10);
}

void check(bool ok, const __FlashStringHelper* message) {
  if (!ok) clearOutputsAndHalt(message);
  Serial.println(message);
}
