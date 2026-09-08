/*!
 * @file 12_gp_bits.ino
 * @brief Hardware test 12: General Purpose Bits
 *
 * Tests GP0-GP5 bit read/write.
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

  Serial.println(F("=== HW Test 12: GP Bits ==="));
  Serial.println();

  if (!rtc.begin()) {
    Serial.println(F("FAIL: RV8803 not found"));
    return;
  }

  rtc.setAlarmDate(1); // GP1 is available only in date alarm mode.

  uint8_t passed = 0;
  uint8_t total = 4;

  uint8_t testValues[] = {0x00, 0x3F, 0x15, 0x2A};
  const char* names[] = {"0x00", "0x3F (all)", "0x15 (alt)", "0x2A (alt)"};

  for (int i = 0; i < 4; i++) {
    Serial.print(F("Test "));
    Serial.print(i + 1);
    Serial.print(F(": Write "));
    Serial.print(names[i]);
    Serial.print(F(" ... "));

    rtc.writeGP(testValues[i]);
    uint8_t readBack = rtc.readGP();

    Serial.print(F("read 0x"));
    Serial.print(readBack, HEX);

    if (readBack == testValues[i]) {
      Serial.println(F(" PASS"));
      passed++;
    } else {
      Serial.println(F(" FAIL"));
    }
  }

  // Clean up: set GP bits to 0
  rtc.writeGP(0x00);

  Serial.println();
  Serial.print(passed);
  Serial.print(F("/"));
  Serial.print(total);
  Serial.println(F(" tests passed"));
}

void loop() {
  // Nothing to do
}
