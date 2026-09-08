/*!
 * @file 07_sqw_modes.ino
 * @brief Hardware test 07: SQW Pin Modes
 *
 * Tests CLKOUT frequency by bit-bang counting edges on D5.
 * CLKOE on D2 is active HIGH.
 * Also verifies read-modify-write preserves other Extension register bits.
 *
 * Wiring: SQWAVE -> D5, CLKOE -> D2
 */

#include <Adafruit_RV8803.h>

Adafruit_RV8803 rtc;

#define SQW_PIN 5
#define CLKOE_PIN 2

/*!
 * @brief Count rising edges on SQW_PIN over a given duration
 * @param ms Duration in milliseconds
 * @return Number of rising edges counted
 */
unsigned long countEdges(unsigned long ms) {
  unsigned long count = 0;
  uint8_t last = digitalRead(SQW_PIN);
  unsigned long start = millis();
  while (millis() - start < ms) {
    uint8_t cur = digitalRead(SQW_PIN);
    if (cur == HIGH && last == LOW) {
      count++;
    }
    last = cur;
  }
  return count;
}

void setup() {
  Serial.begin(115200);
  while (!Serial)
    delay(10);

  // Power the RV-8803 via GPIO (VCC wired to A0)
  pinMode(A0, OUTPUT);
  digitalWrite(A0, HIGH);
  delay(100); // Let chip stabilize after power-on

  Serial.println(F("=== HW Test 07: SQW Modes ==="));
  Serial.println();

  if (!rtc.begin()) {
    Serial.println(F("FAIL: RV8803 not found"));
    return;
  }

  uint8_t passed = 0;
  uint8_t total = 8;

  pinMode(SQW_PIN, INPUT);
  pinMode(CLKOE_PIN, OUTPUT);
  digitalWrite(CLKOE_PIN, HIGH); // Active HIGH — enable CLKOUT

  // Test 1: 1Hz — count edges over 3 seconds, expect ~3
  Serial.print(F("Test 1: 1Hz output on D5 ... "));
  rtc.writeSqwPinMode(RV8803_SquareWave1Hz);
  delay(10);
  unsigned long edges1 = countEdges(3000);
  Serial.print(F("edges in 3s = "));
  Serial.print(edges1);
  if (edges1 >= 2 && edges1 <= 4) {
    Serial.println(F(" PASS"));
    passed++;
  } else {
    Serial.println(F(" FAIL"));
  }

  // Test 2: 1kHz — count edges over 200ms, expect ~200
  Serial.print(F("Test 2: 1kHz output on D5 ... "));
  rtc.writeSqwPinMode(RV8803_SquareWave1kHz);
  delay(10);
  unsigned long edges2 = countEdges(200);
  Serial.print(F("edges in 200ms = "));
  Serial.print(edges2);
  // Allow 150-250
  if (edges2 >= 150 && edges2 <= 250) {
    Serial.println(F(" PASS"));
    passed++;
  } else {
    Serial.println(F(" FAIL"));
  }

  // Test 3: 32.768kHz — count edges over 100ms, expect ~3277
  // Note: digitalRead on AVR may not keep up at 32kHz, so we just
  // verify we see significantly more edges than 1kHz
  Serial.print(F("Test 3: 32kHz output on D5 ... "));
  rtc.writeSqwPinMode(RV8803_SquareWave32kHz);
  delay(10);
  unsigned long edges3 = countEdges(100);
  Serial.print(F("edges in 100ms = "));
  Serial.print(edges3);
  // digitalRead tops out ~150kHz on AVR, so we should see most of them
  // Expect at least 2000 edges
  if (edges3 >= 2000) {
    Serial.println(F(" PASS"));
    passed++;
  } else {
    Serial.println(F(" FAIL"));
  }

  // Test 4: Read-modify-write preserves TE bit
  Serial.println(F("Test 4: SQW change preserves TE bit ..."));
  uint8_t extReg = rtc.readExtensionRegister();
  extReg |= 0x10;
  rtc.writeExtensionRegister(extReg);

  uint8_t before = rtc.readExtensionRegister();
  bool teBefore = (before & 0x10) != 0;
  Serial.print(F("  TE before: "));
  Serial.println(teBefore ? F("1") : F("0"));

  rtc.writeSqwPinMode(RV8803_SquareWave1Hz);

  uint8_t after = rtc.readExtensionRegister();
  bool teAfter = (after & 0x10) != 0;
  Serial.print(F("  TE after SQW change: "));
  Serial.println(teAfter ? F("1") : F("0"));

  if (teBefore && teAfter) {
    Serial.println(F("  PASS - TE preserved"));
    passed++;
  } else {
    Serial.println(F("  FAIL - TE was clobbered"));
  }

  extReg = rtc.readExtensionRegister();
  extReg &= ~0x10;
  rtc.writeExtensionRegister(extReg);

  // Test 5: Read-modify-write preserves WADA and USEL bits
  Serial.println(F("Test 5: SQW change preserves WADA/USEL bits ..."));
  extReg = rtc.readExtensionRegister();
  extReg |= 0x60;
  rtc.writeExtensionRegister(extReg);

  before = rtc.readExtensionRegister();
  uint8_t wadaUselBefore = before & 0x60;
  Serial.print(F("  WADA|USEL before: 0x"));
  Serial.println(wadaUselBefore, HEX);

  rtc.writeSqwPinMode(RV8803_SquareWave32kHz);

  after = rtc.readExtensionRegister();
  uint8_t wadaUselAfter = after & 0x60;
  Serial.print(F("  WADA|USEL after: 0x"));
  Serial.println(wadaUselAfter, HEX);

  if (wadaUselBefore == wadaUselAfter && wadaUselAfter == 0x60) {
    Serial.println(F("  PASS - WADA/USEL preserved"));
    passed++;
  } else {
    Serial.println(F("  FAIL - WADA/USEL clobbered"));
  }

  // Test 6: Verify TD bits not affected by FD changes
  Serial.println(F("Test 6: SQW change preserves TD bits ..."));
  extReg = rtc.readExtensionRegister();
  extReg = (extReg & 0xFC) | 0x02;
  rtc.writeExtensionRegister(extReg);

  before = rtc.readExtensionRegister();
  uint8_t tdBefore = before & 0x03;
  Serial.print(F("  TD before: 0x"));
  Serial.println(tdBefore, HEX);

  rtc.writeSqwPinMode(RV8803_SquareWave1Hz);
  rtc.writeSqwPinMode(RV8803_SquareWave1kHz);
  rtc.writeSqwPinMode(RV8803_SquareWave32kHz);

  after = rtc.readExtensionRegister();
  uint8_t tdAfter = after & 0x03;
  Serial.print(F("  TD after: 0x"));
  Serial.println(tdAfter, HEX);

  if (tdBefore == tdAfter && tdAfter == 0x02) {
    Serial.println(F("  PASS - TD preserved"));
    passed++;
  } else {
    Serial.println(F("  FAIL - TD clobbered"));
  }

  rtc.writeExtensionRegister(0x00);

  // Test 7: CLKOE LOW disables CLKOUT (active HIGH)
  Serial.print(F("Test 7: CLKOE LOW disables CLKOUT ... "));
  rtc.writeSqwPinMode(RV8803_SquareWave1kHz);
  delay(10);
  digitalWrite(CLKOE_PIN, LOW); // Disable CLKOUT
  delay(10);
  unsigned long edgesOff = countEdges(200);
  Serial.print(F("edges in 200ms = "));
  Serial.print(edgesOff);
  if (edgesOff == 0) {
    Serial.println(F(" PASS (no pulses)"));
    passed++;
  } else {
    Serial.println(F(" FAIL (still pulsing)"));
  }

  // Test 8: CLKOE HIGH re-enables CLKOUT
  Serial.print(F("Test 8: CLKOE HIGH re-enables CLKOUT ... "));
  digitalWrite(CLKOE_PIN, HIGH); // Re-enable CLKOUT
  delay(10);
  unsigned long edgesOn = countEdges(200);
  Serial.print(F("edges in 200ms = "));
  Serial.print(edgesOn);
  if (edgesOn >= 150 && edgesOn <= 250) {
    Serial.println(F(" PASS (1kHz restored)"));
    passed++;
  } else {
    Serial.println(F(" FAIL"));
  }

  // Clean up
  rtc.writeSqwPinMode(RV8803_SquareWave32kHz);

  Serial.println();
  Serial.print(passed);
  Serial.print(F("/"));
  Serial.print(total);
  Serial.println(F(" tests passed"));
}

void loop() {
  // Nothing to do
}
