// Metro Mini: VIN=A0, SDA=A4, SCL=A5. No other fixture pins are used.
#include <Adafruit_RV8803.h>

Adafruit_RV8803 rtc;

void setup() {
  Serial.begin(115200);
  // Wait for Serial Monitor on native USB boards; remove for standalone use.
  while (!Serial) delay(10);
  delay(250);
  Serial.println(F("Adafruit RV8803 basic and reset test"));
  digitalWrite(A0, HIGH);
  pinMode(A0, OUTPUT);
  delay(100);
  check(rtc.begin(), F("Begin succeeded"));
  check(rtc.adjust(DateTime(2026, 9, 7, 12, 34, 20)), F("Time set"));
  check(rtc.now() == DateTime(2026, 9, 7, 12, 34, 20), F("Time read back"));
  check(rtc.isrunning(), F("Timekeeping enabled"));
  check(rtc.writeRAM(0xA5), F("RAM written"));
  check(rtc.setAlarmDate(7), F("Date alarm selected"));
  check(rtc.setAlarm(DateTime(2026, 9, 7, 15, 30), RV8803_A_HourMin), F("Alarm configured"));

  Serial.println();
  check(rtc.writeControlRegister(RV8803_CTRL_RESET), F("Prescaler held in reset"));
  check(!rtc.isrunning(), F("Stopped clock reported correctly"));
  DateTime stopped = rtc.now();
  delay(1100);
  check(rtc.now() == stopped, F("Clock stayed stopped"));
  check(rtc.reset(), F("Reset completed"));
  check(rtc.isrunning(), F("Reset released the prescaler"));
  check(rtc.getHundredths() < 10, F("Reset cleared fractional seconds"));
  check(rtc.readRAM() == 0xA5, F("Reset preserved RAM"));
  delay(1100);
  check((rtc.now() - stopped).totalseconds() == 1, F("Clock advanced after reset"));
  check(rtc.begin(), F("Second begin succeeded"));
  check(rtc.getAlarmMode() == RV8803_A_HourMin, F("Second begin preserved alarm mode"));

  Serial.println();
  checkRollover(DateTime(2024, 2, 28, 23, 59, 59), DateTime(2024, 2, 29));
  checkRollover(DateTime(2026, 12, 31, 23, 59, 59), DateTime(2027, 1, 1));
  Serial.println(F("ALL PASSED"));
}

void loop() {}

void clearOutputsAndHalt(const __FlashStringHelper* message) {
  Serial.print(F("FAIL: "));
  Serial.println(message);
  rtc.writeControlRegister(0);
  while (true) delay(10);
}

void check(bool ok, const __FlashStringHelper* message) {
  if (!ok) clearOutputsAndHalt(message);
  Serial.println(message);
}

void checkRollover(DateTime before, DateTime after) {
  check(rtc.adjust(before), F("Rollover start time set"));
  unsigned long start = millis();
  bool crossed = false;
  while (millis() - start < 1500) {
    DateTime value = rtc.now();
    if (value != before && value != after) {
      clearOutputsAndHalt(F("Mixed calendar data during rollover"));
    }
    if (value == after) crossed = true;
  }
  check(crossed, F("Calendar rollover stayed coherent"));
}
