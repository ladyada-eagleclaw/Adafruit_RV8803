/*!
 * @file 03_hundredths.ino
 * @brief Hardware test 03: Hundredths of a Second
 *
 * Tests reading hundredths of a second from the RV8803 RTC.
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

  Serial.println(F("=== HW Test 03: Hundredths ==="));
  Serial.println();

  if (!rtc.begin()) {
    Serial.println(F("FAIL: RV8803 not found"));
    return;
  }

  uint8_t passed = 0;
  uint8_t total = 1;

  // Test: Read hundredths 10 times with 50ms delay
  Serial.println(F("Test 1: Read hundredths 10 times (50ms apart) ..."));
  uint8_t values[10];
  uint8_t uniqueCount = 0;

  for (int i = 0; i < 10; i++) {
    values[i] = rtc.getHundredths();
    Serial.print(F("  ["));
    Serial.print(i);
    Serial.print(F("] = "));
    Serial.println(values[i]);
    delay(50);
  }

  // Count unique values
  for (int i = 1; i < 10; i++) {
    if (values[i] != values[i - 1]) {
      uniqueCount++;
    }
  }

  Serial.print(F("  Changes detected: "));
  Serial.println(uniqueCount);

  // Should see at least 3-4 changes in 450ms with 50ms intervals
  if (uniqueCount >= 3) {
    Serial.println(F("  PASS"));
    passed++;
  } else {
    Serial.println(F("  FAIL - values not changing enough"));
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
