/*!
 * @file 11_periodic_update.ino
 * @brief Hardware test 11: Periodic Update
 *
 * Tests periodic update interrupt functionality.
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

  Serial.println(F("=== HW Test 11: Periodic Update ==="));
  Serial.println();

  if (!rtc.begin()) {
    Serial.println(F("FAIL: RV8803 not found"));
    return;
  }

  uint8_t passed = 0;
  uint8_t total = 2;

  // Test 1: Set up second-update mode and wait for flag
  Serial.println(F("Test 1: Second update mode (wait up to 2s) ..."));
  rtc.setUpdateMode(RV8803_UpdateSecond);
  rtc.enableInterrupt(RV8803_InterruptUpdate);
  rtc.clearUpdate();

  unsigned long start = millis();
  bool fired = false;

  while (millis() - start < 2000) {
    if (rtc.updateFired()) {
      fired = true;
      Serial.print(F("  Update fired after "));
      Serial.print((millis() - start));
      Serial.println(F(" ms"));
      break;
    }
    delay(100);
  }

  if (fired) {
    Serial.println(F("  PASS - update flag set"));
    passed++;
  } else {
    Serial.println(F("  FAIL - update flag not set within 2s"));
  }

  // Test 2: Clear update flag
  Serial.println(F("Test 2: Clear update flag ..."));
  rtc.clearUpdate();
  if (!rtc.updateFired()) {
    Serial.println(F("  PASS - update flag cleared"));
    passed++;
  } else {
    Serial.println(F("  FAIL - update flag still set"));
  }

  // Disable update interrupt
  rtc.disableInterrupt(RV8803_InterruptUpdate);

  Serial.println();
  Serial.print(passed);
  Serial.print(F("/"));
  Serial.print(total);
  Serial.println(F(" tests passed"));
}

void loop() {
  // Nothing to do
}
