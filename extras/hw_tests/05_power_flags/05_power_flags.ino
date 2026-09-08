/*!
 * @file 05_power_flags.ino
 * @brief Hardware test 05: Power Flags
 *
 * Tests lostPower() and tempCompStopped() flags.
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

  Serial.println(F("=== HW Test 05: Power Flags ==="));
  Serial.println();

  if (!rtc.begin()) {
    Serial.println(F("FAIL: RV8803 not found"));
    return;
  }

  uint8_t passed = 0;
  uint8_t total = 2;

  // Test 1: Check initial flag state, then clear with adjust()
  Serial.println(F("Test 1: Flag state before/after adjust() ..."));
  bool lostBefore = rtc.lostPower();
  bool tempBefore = rtc.tempCompStopped();

  Serial.print(F("  Before adjust: lostPower="));
  Serial.print(lostBefore ? F("true") : F("false"));
  Serial.print(F(", tempCompStopped="));
  Serial.println(tempBefore ? F("true") : F("false"));

  // Set time (this clears flags)
  DateTime setTime(2026, 3, 4, 22, 30, 45);
  rtc.adjust(setTime);
  delay(100);

  bool lostAfter = rtc.lostPower();
  bool tempAfter = rtc.tempCompStopped();

  Serial.print(F("  After adjust:  lostPower="));
  Serial.print(lostAfter ? F("true") : F("false"));
  Serial.print(F(", tempCompStopped="));
  Serial.println(tempAfter ? F("true") : F("false"));

  if (!lostAfter) {
    Serial.println(F("  PASS - lostPower cleared after adjust"));
    passed++;
  } else {
    Serial.println(F("  FAIL - lostPower still set after adjust"));
  }

  // Test 2: tempCompStopped should be false after clearing
  Serial.println(F("Test 2: tempCompStopped after clear ..."));
  if (!tempAfter) {
    Serial.println(F("  PASS - tempCompStopped is false"));
    passed++;
  } else {
    Serial.println(F("  FAIL - tempCompStopped still set"));
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
