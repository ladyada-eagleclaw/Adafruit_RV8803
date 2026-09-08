/*!
 * @file 01_i2c_detect.ino
 * @brief Hardware test 01: I2C Detection
 *
 * Tests basic I2C connectivity to the RV8803 RTC.
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
  delay(100); // Let chip stabilize after power-on

  Serial.println(F("=== HW Test 01: I2C Detect ==="));
  Serial.println();

  uint8_t passed = 0;
  uint8_t total = 1;

  // Test: begin() returns true
  Serial.print(F("Test 1: begin() ... "));
  if (rtc.begin()) {
    Serial.println(F("PASS"));
    Serial.print(F("  I2C address confirmed: 0x"));
    Serial.println(RV8803_I2C_ADDRESS, HEX);
    passed++;
  } else {
    Serial.println(F("FAIL"));
    Serial.println(F("  RV8803 not found at 0x32"));
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
