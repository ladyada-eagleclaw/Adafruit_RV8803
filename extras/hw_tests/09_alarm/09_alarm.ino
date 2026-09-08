/*!
 * @file 09_alarm.ino
 * @brief Hardware test 09: Alarm
 *
 * Tests alarm functionality including flag operations and
 * specific time matching with HourMin mode.
 */

#include <Adafruit_RV8803.h>

Adafruit_RV8803 rtc;

void setup() {
  Serial.begin(115200);
  while (!Serial)
    delay(10);

  // Power the RV-8803 via GPIO (VCC wired to A0)
  pinMode(A0, OUTPUT);
  digitalWrite(A0, HIGH);
  delay(600); // Power-on reset can take 500 ms (manual section 7.4).

  Serial.println(F("=== HW Test 09: Alarm ==="));
  Serial.println();

  if (!rtc.begin()) {
    Serial.println(F("FAIL: RV8803 not found"));
    return;
  }

  uint8_t passed = 0;
  uint8_t total = 4;

  // Test 1: Clear alarm flag
  Serial.println(F("Test 1: Clear existing alarm flag ..."));
  rtc.clearAlarm();
  if (!rtc.alarmFired()) {
    Serial.println(F("  PASS - alarm flag cleared"));
    passed++;
  } else {
    Serial.println(F("  FAIL - alarm flag still set"));
  }

  // Test 2: Alarm mode and register round-trip
  Serial.println(F("Test 2: Alarm register round-trip ..."));

  // Test date mode
  rtc.setAlarmDate(15);
  DateTime alarmDt = rtc.getAlarm();
  bool datePass = (alarmDt.day() == 15);
  Serial.print(F("  setAlarmDate(15) -> getAlarm().day() = "));
  Serial.print(alarmDt.day());
  Serial.println(datePass ? F(" OK") : F(" FAIL"));

  // Test weekday mode (weekday mask 0x08 = bit 3 = Wednesday)
  rtc.setAlarmWeekday(0x08);
  alarmDt = rtc.getAlarm();
  bool wdPass = (alarmDt.day() == 3);
  Serial.print(F("  setAlarmWeekday(0x08) -> getAlarm().day() = "));
  Serial.print(alarmDt.day());
  Serial.println(wdPass ? F(" OK") : F(" FAIL"));

  if (datePass && wdPass) {
    Serial.println(F("  PASS"));
    passed++;
  } else {
    Serial.println(F("  FAIL"));
  }

  // Test 3: Specific time alarm (HourMin mode) - quick version
  Serial.println(F("Test 3: Specific time alarm (HourMin daily mode) ..."));
  Serial.println(F("  Setting time to 22:30:55, alarm for 22:31"));

  // Set current time to 22:30:55 (5 seconds before alarm)
  rtc.adjust(DateTime(2026, 3, 4, 22, 30, 55));
  delay(100);

  // Clear any existing alarm
  rtc.clearAlarm();

  // Set alarm for 22:31 in HourMin mode
  DateTime alarmTime(2026, 3, 4, 22, 31, 0);
  rtc.setAlarm(alarmTime, RV8803_A_HourMin);
  rtc.enableInterrupt(RV8803_InterruptAlarm);

  // Verify alarm is set correctly
  DateTime alarmCheck = rtc.getAlarm();
  Serial.print(F("  Alarm set for: "));
  Serial.print(alarmCheck.hour());
  Serial.print(F(":"));
  Serial.println(alarmCheck.minute());

  // Wait up to 10 seconds for alarm (should fire in ~5s)
  unsigned long start = millis();
  bool fired = false;
  unsigned long fireTime = 0;

  Serial.println(F("  Waiting for alarm (up to 10s)..."));

  while (millis() - start < 10000) {
    if (rtc.alarmFired()) {
      fired = true;
      fireTime = millis() - start;
      DateTime nowTime = rtc.now();
      Serial.print(F("  Alarm fired at "));
      Serial.print(nowTime.hour());
      Serial.print(F(":"));
      Serial.print(nowTime.minute());
      Serial.print(F(":"));
      Serial.print(nowTime.second());
      Serial.print(F(" (after "));
      Serial.print(fireTime / 1000);
      Serial.println(F("s)"));
      break;
    }
    delay(100);
  }

  if (fired) {
    // Verify it fired around 5 seconds (allow 3-8s window)
    if (fireTime >= 3000 && fireTime <= 8000) {
      Serial.println(F("  PASS - alarm fired at expected time"));
      passed++;
    } else {
      Serial.print(F("  FAIL - fired at wrong time (expected ~5s, got "));
      Serial.print(fireTime / 1000);
      Serial.println(F("s)"));
    }
  } else {
    Serial.println(F("  FAIL - alarm did not fire within 10s"));
  }

  // Test 4: Clear alarm and verify after firing
  Serial.println(F("Test 4: Clear alarm flag after firing ..."));
  rtc.clearAlarm();
  rtc.disableInterrupt(RV8803_InterruptAlarm);
  if (!rtc.alarmFired()) {
    Serial.println(F("  PASS - alarm flag cleared"));
    passed++;
  } else {
    Serial.println(F("  FAIL - alarm flag still set"));
  }

  Serial.println();
  Serial.print(passed);
  Serial.print(F("/"));
  Serial.print(total);
  Serial.println(F(" tests passed"));
}

void loop() {
  // Nothing to do
}
