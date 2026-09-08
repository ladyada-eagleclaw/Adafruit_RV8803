// A daily alarm. Wire INT to an input if you also want a physical wake signal.
#include <Adafruit_RV8803.h>

Adafruit_RV8803 rtc;

void setup() {
  Serial.begin(115200);
  // Wait for Serial Monitor on native USB boards; remove for standalone use.
  while (!Serial) delay(10);
  delay(250);
  Serial.println(F("Adafruit RV8803 daily alarm"));
  if (!rtc.begin()) {
    Serial.println(F("RTC not found"));
    while (true) delay(10);
  }
  if (rtc.lostPower() && !rtc.adjust(DateTime(F(__DATE__), F(__TIME__)))) {
    Serial.println(F("Could not set the time"));
    while (true) delay(10);
  }
  // Disable alarm output while changing the match fields (manual section 4.7.2).
  // HourMin matches every day at 09:30. Date/weekday is ignored in this mode.
  if (!rtc.disableInterrupt(RV8803_InterruptAlarm) ||
      !rtc.setAlarm(DateTime(2026, 1, 1, 9, 30), RV8803_A_HourMin) ||
      !rtc.clearAlarm() || !rtc.enableInterrupt(RV8803_InterruptAlarm)) {
    Serial.println(F("Could not configure the alarm"));
    while (true) delay(10);
  }
  Serial.println(F("Alarm set for 09:30 every day"));
}

void loop() {
  if (rtc.alarmFired()) {
    Serial.println(F("Daily alarm fired"));
    if (!rtc.clearAlarm()) Serial.println(F("Could not clear the alarm"));
  }
  delay(100);
}
