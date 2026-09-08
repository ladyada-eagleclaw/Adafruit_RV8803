/*!
 * @file 08_calibration.ino
 * @brief Hardware test 08: Offset Calibration
 *
 * Tests calibrate() and getCalibration() round-trip, plus raw register
 * verification and boundary clamping.
 */

#include <Adafruit_RV8803.h>
#include <Wire.h>

Adafruit_RV8803 rtc;

// Read raw OFFSET register (0x2C) directly
uint8_t readRawOffset() {
  Wire.beginTransmission(0x32);
  Wire.write(0x2C);
  Wire.endTransmission(false);
  Wire.requestFrom(0x32, 1);
  return Wire.read();
}

void setup() {
  Serial.begin(115200);
  while (!Serial)
    delay(10);

  // Power the RV-8803 via GPIO (VCC wired to A0)
  pinMode(A0, OUTPUT);
  digitalWrite(A0, HIGH);
  delay(100); // Let chip stabilize after power-on

  Serial.println(F("=== HW Test 08: Calibration ==="));
  Serial.println();

  if (!rtc.begin()) {
    Serial.println(F("FAIL: RV8803 not found"));
    return;
  }

  uint8_t passed = 0;
  uint8_t total = 8;

  // Test 1-5: Basic round-trip tests
  int8_t testValues[] = {0, 1, 31, -1, -32};

  for (int i = 0; i < 5; i++) {
    Serial.print(F("Test "));
    Serial.print(i + 1);
    Serial.print(F(": calibrate("));
    Serial.print(testValues[i]);
    Serial.print(F(") ... "));

    rtc.calibrate(testValues[i]);
    int8_t readBack = rtc.getCalibration();

    Serial.print(F("read "));
    Serial.print(readBack);

    // Also verify raw register encoding
    uint8_t raw = readRawOffset();
    Serial.print(F(", raw=0x"));
    Serial.print(raw, HEX);

    if (readBack == testValues[i]) {
      // Verify the raw encoding is correct (6-bit two's complement)
      // For positive: raw should equal value
      // For negative: raw should be 64 + value (two's complement in 6 bits)
      uint8_t expectedRaw;
      if (testValues[i] >= 0) {
        expectedRaw = testValues[i];
      } else {
        expectedRaw = 64 + testValues[i]; // Two's complement in 6 bits
      }

      if ((raw & 0x3F) == expectedRaw) {
        Serial.println(F(" PASS"));
        passed++;
      } else {
        Serial.print(F(" FAIL (expected raw 0x"));
        Serial.print(expectedRaw, HEX);
        Serial.println(F(")"));
      }
    } else {
      Serial.println(F(" FAIL"));
    }
  }

  // Test 6: Verify reset works (calibrate(0) after calibrate(-32))
  Serial.println(F("Test 6: Reset after -32 ..."));
  rtc.calibrate(-32);
  int8_t val1 = rtc.getCalibration();
  Serial.print(F("  After calibrate(-32): "));
  Serial.println(val1);

  rtc.calibrate(0);
  int8_t val2 = rtc.getCalibration();
  Serial.print(F("  After calibrate(0): "));
  Serial.println(val2);

  uint8_t rawAfterReset = readRawOffset();
  Serial.print(F("  Raw register: 0x"));
  Serial.println(rawAfterReset, HEX);

  if (val1 == -32 && val2 == 0 && (rawAfterReset & 0x3F) == 0) {
    Serial.println(F("  PASS"));
    passed++;
  } else {
    Serial.println(F("  FAIL"));
  }

  // Test 7: Boundary clamping - positive overflow
  Serial.println(F("Test 7: Boundary clamp (32 -> 31) ..."));
  rtc.calibrate(32); // Should clamp to 31
  int8_t clamped = rtc.getCalibration();
  Serial.print(F("  calibrate(32) -> getCalibration() = "));
  Serial.println(clamped);

  if (clamped == 31) {
    Serial.println(F("  PASS"));
    passed++;
  } else {
    Serial.println(F("  FAIL - expected 31"));
  }

  // Test 8: Boundary clamping - negative overflow
  Serial.println(F("Test 8: Boundary clamp (-33 -> -32) ..."));
  rtc.calibrate(-33); // Should clamp to -32
  clamped = rtc.getCalibration();
  Serial.print(F("  calibrate(-33) -> getCalibration() = "));
  Serial.println(clamped);

  if (clamped == -32) {
    Serial.println(F("  PASS"));
    passed++;
  } else {
    Serial.println(F("  FAIL - expected -32"));
  }

  // Reset to 0 when done
  rtc.calibrate(0);

  Serial.println();
  Serial.print(passed);
  Serial.print(F("/"));
  Serial.print(total);
  Serial.println(F(" tests passed"));
}

void loop() {
  // Nothing to do
}
