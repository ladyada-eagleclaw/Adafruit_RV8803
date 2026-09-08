/*!
 * @file 13_direct_registers.ino
 * @brief Hardware test 13: Direct Register Access
 *
 * Tests direct register read/write functions.
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

  Serial.println(F("=== HW Test 13: Direct Registers ==="));
  Serial.println();

  if (!rtc.begin()) {
    Serial.println(F("FAIL: RV8803 not found"));
    return;
  }

  uint8_t passed = 0;
  uint8_t total = 4;

  // Test 1: Extension register - TEST bit should be 0
  Serial.println(F("Test 1: Extension register (TEST bit) ..."));
  uint8_t ext = rtc.readExtensionRegister();
  Serial.print(F("  Extension = 0x"));
  Serial.println(ext, HEX);
  if ((ext & RV8803_EXT_TEST) == 0) {
    Serial.println(F("  PASS - TEST bit is 0"));
    passed++;
  } else {
    Serial.println(F("  FAIL - TEST bit is set!"));
  }

  // Test 2: Control register - enable/disable alarm interrupt
  Serial.println(F("Test 2: Control register (AIE bit) ..."));
  uint8_t ctrl = rtc.readControlRegister();
  Serial.print(F("  Initial Control = 0x"));
  Serial.println(ctrl, HEX);

  // Enable alarm interrupt
  ctrl |= RV8803_CTRL_AIE;
  rtc.writeControlRegister(ctrl);
  uint8_t ctrlRead = rtc.readControlRegister();
  bool enablePass = (ctrlRead & RV8803_CTRL_AIE) != 0;
  Serial.print(F("  After enable: 0x"));
  Serial.print(ctrlRead, HEX);
  Serial.println(enablePass ? F(" PASS") : F(" FAIL"));

  // Disable alarm interrupt
  ctrl &= ~RV8803_CTRL_AIE;
  rtc.writeControlRegister(ctrl);
  ctrlRead = rtc.readControlRegister();
  bool disablePass = (ctrlRead & RV8803_CTRL_AIE) == 0;
  Serial.print(F("  After disable: 0x"));
  Serial.print(ctrlRead, HEX);
  Serial.println(disablePass ? F(" PASS") : F(" FAIL"));

  if (enablePass && disablePass) {
    passed++;
  }

  // Test 3: Flag register - read all flags
  Serial.println(F("Test 3: Flag register (read flags) ..."));
  uint8_t flags = rtc.readFlagRegister();
  Serial.print(F("  Flag = 0x"));
  Serial.println(flags, HEX);
  Serial.print(F("    UF="));
  Serial.print((flags & RV8803_FLAG_UF) ? 1 : 0);
  Serial.print(F(" TF="));
  Serial.print((flags & RV8803_FLAG_TF) ? 1 : 0);
  Serial.print(F(" AF="));
  Serial.print((flags & RV8803_FLAG_AF) ? 1 : 0);
  Serial.print(F(" EVF="));
  Serial.print((flags & RV8803_FLAG_EVF) ? 1 : 0);
  Serial.print(F(" V2F="));
  Serial.print((flags & RV8803_FLAG_V2F) ? 1 : 0);
  Serial.print(F(" V1F="));
  Serial.println((flags & RV8803_FLAG_V1F) ? 1 : 0);
  Serial.println(F("  PASS - flags read successfully"));
  passed++;

  // Test 4: Event Control register - read/write
  Serial.println(F("Test 4: Event Control register ..."));
  uint8_t evctrl = rtc.readEventControl();
  Serial.print(F("  Initial EventCtrl = 0x"));
  Serial.println(evctrl, HEX);

  // Toggle EHL bit
  uint8_t newEvctrl = evctrl ^ RV8803_EVCTRL_EHL;
  rtc.writeEventControl(newEvctrl);
  uint8_t evctrlRead = rtc.readEventControl();
  bool evctrlPass = (evctrlRead == newEvctrl);
  Serial.print(F("  After toggle: 0x"));
  Serial.print(evctrlRead, HEX);
  Serial.println(evctrlPass ? F(" PASS") : F(" FAIL"));

  // Restore original
  rtc.writeEventControl(evctrl);

  if (evctrlPass) {
    passed++;
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
