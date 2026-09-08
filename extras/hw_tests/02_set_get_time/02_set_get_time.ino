/*!
 * @file 02_set_get_time.ino
 * @brief Hardware test 02: Set and Get Time
 *
 * Tests setting and reading time from the RV8803 RTC.
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

  Serial.println(F("=== HW Test 02: Set/Get Time ==="));
  Serial.println();

  if (!rtc.begin()) {
    Serial.println(F("FAIL: RV8803 not found"));
    return;
  }

  uint8_t passed = 0;
  uint8_t total = 2;

  // Test 1: Set time from compile timestamp and read back
  DateTime setTime(F(__DATE__), F(__TIME__));
  Serial.print(F("Test 1: Set compile time "));
  Serial.print(setTime.year());
  Serial.print(F("-"));
  Serial.print(setTime.month());
  Serial.print(F("-"));
  Serial.print(setTime.day());
  Serial.print(F(" "));
  Serial.print(setTime.hour());
  Serial.print(F(":"));
  Serial.print(setTime.minute());
  Serial.print(F(":"));
  Serial.print(setTime.second());
  Serial.println(F(" ..."));
  rtc.adjust(setTime);
  delay(100); // Small delay for registers to settle

  DateTime now = rtc.now();
  Serial.print(F("  Read back: "));
  Serial.print(now.year());
  Serial.print(F("-"));
  Serial.print(now.month());
  Serial.print(F("-"));
  Serial.print(now.day());
  Serial.print(F(" "));
  Serial.print(now.hour());
  Serial.print(F(":"));
  Serial.print(now.minute());
  Serial.print(F(":"));
  Serial.println(now.second());

  bool test1Pass =
      (now.year() == setTime.year()) && (now.month() == setTime.month()) &&
      (now.day() == setTime.day()) && (now.hour() == setTime.hour()) &&
      (now.minute() == setTime.minute()) && (now.second() >= setTime.second());
  if (test1Pass) {
    Serial.println(F("  PASS"));
    passed++;
  } else {
    Serial.println(F("  FAIL - values don't match"));
  }

  // Test 2: Wait 2 seconds, verify time advances
  Serial.println(F("Test 2: Wait 2s, verify seconds advance ..."));
  uint8_t sec1 = rtc.now().second();
  delay(2000);
  uint8_t sec2 = rtc.now().second();

  // Handle rollover at 60
  int diff = sec2 - sec1;
  if (diff < 0)
    diff += 60;

  Serial.print(F("  Before: "));
  Serial.print(sec1);
  Serial.print(F("s, After: "));
  Serial.print(sec2);
  Serial.print(F("s, Diff: "));
  Serial.println(diff);

  if (diff >= 1 && diff <= 3) {
    Serial.println(F("  PASS"));
    passed++;
  } else {
    Serial.println(F("  FAIL - seconds didn't advance correctly"));
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
