/*!
 * @file 06_ram.ino
 * @brief Hardware test 06: RAM Read/Write + Power Cycle
 *
 * Tests the 1-byte RAM register while cycling VIN from A0.
 * Set batteryInstalled to match the fixture before running.
 */

#include <Adafruit_RV8803.h>

#define RTC_VCC_PIN A0
#define RTC_SDA_PIN A4
#define RTC_SCL_PIN A5

Adafruit_RV8803 rtc;
const bool batteryInstalled = true;

/*!
 * @brief Remove VIN from the RV-8803 breakout.
 *
 * Ends Wire, then drives VCC, SDA, and SCL all LOW to prevent
 * parasitic power through ESD protection diodes. An installed coin cell
 * continues supplying the RTC.
 */
void rtcPowerOff() {
  Wire.end();
  pinMode(RTC_VCC_PIN, OUTPUT);
  pinMode(RTC_SDA_PIN, OUTPUT);
  pinMode(RTC_SCL_PIN, OUTPUT);
  digitalWrite(RTC_VCC_PIN, LOW);
  digitalWrite(RTC_SDA_PIN, LOW);
  digitalWrite(RTC_SCL_PIN, LOW);
  delay(2000); // Long delay to fully discharge internal caps below VLOW2
}

/*!
 * @brief Restore power to the RV-8803 and re-init I2C.
 *
 * Releases SDA/SCL back to Wire, powers VCC, and calls Wire.begin().
 * @return true if rtc.begin() succeeds within retries.
 */
bool rtcPowerOn() {
  // Release SDA/SCL before Wire.begin() reclaims them
  pinMode(RTC_SDA_PIN, INPUT);
  pinMode(RTC_SCL_PIN, INPUT);

  // Power on
  pinMode(RTC_VCC_PIN, OUTPUT);
  digitalWrite(RTC_VCC_PIN, HIGH);
  delay(600); // Power-on reset can take 500 ms (manual section 7.4).

  Wire.begin();
  delay(50);

  // Retry begin() — chip may need a moment after power-on
  for (int i = 0; i < 10; i++) {
    if (rtc.begin()) {
      return true;
    }
    delay(50);
  }
  return false;
}

void setup() {
  Serial.begin(115200);
  while (!Serial)
    delay(10);

  // Power the RV-8803 via GPIO (VCC wired to A0)
  pinMode(RTC_VCC_PIN, OUTPUT);
  digitalWrite(RTC_VCC_PIN, HIGH);
  delay(600); // Power-on reset can take 500 ms (manual section 7.4).

  Serial.println(F("=== HW Test 06: RAM ==="));
  Serial.println();

  if (!rtc.begin()) {
    Serial.println(F("FAIL: RV8803 not found"));
    return;
  }

  uint8_t passed = 0;
  uint8_t total = 6;

  // Tests 1-4: Basic write/readback
  uint8_t testValues[] = {0xA5, 0x5A, 0x00, 0xFF};

  for (int i = 0; i < 4; i++) {
    Serial.print(F("Test "));
    Serial.print(i + 1);
    Serial.print(F(": Write 0x"));
    Serial.print(testValues[i], HEX);
    Serial.print(F(" ... "));

    rtc.writeRAM(testValues[i]);
    uint8_t readBack = rtc.readRAM();

    Serial.print(F("read 0x"));
    Serial.print(readBack, HEX);

    if (readBack == testValues[i]) {
      Serial.println(F(" PASS"));
      passed++;
    } else {
      Serial.println(F(" FAIL"));
    }
  }

  // Establish valid time and cleared power flags before removing VIN.
  if (!rtc.adjust(DateTime(2026, 9, 8, 12, 0, 0))) {
    Serial.println(F("FAIL: Could not initialize time before power cycle"));
    return;
  }
  // Test 5: Compare retention against the installed battery configuration.
  Serial.print(F("Test 5: RAM after VIN power cycle ... "));
  Serial.flush();

  rtc.writeRAM(0xA5);
  if (rtc.readRAM() != 0xA5) {
    Serial.println(F("FAIL (sanity write failed)"));
  } else {
    rtcPowerOff();

    if (!rtcPowerOn()) {
      Serial.println(F("FAIL (RTC not found after power cycle)"));
    } else {
      uint8_t afterCycle = rtc.readRAM();
      Serial.print(F("before=0xA5 after=0x"));
      Serial.print(afterCycle, HEX);
      bool retained = afterCycle == 0xA5;
      if (retained == batteryInstalled) {
        Serial.println(F(" PASS (matches battery configuration)"));
        passed++;
      } else {
        Serial.println(F(" FAIL (unexpected retention state)"));
      }
    }
  }

  // Test 6: Backup should prevent V2F; full power loss should set it.
  Serial.print(F("Test 6: lostPower() after cycle ... "));
  bool lost = rtc.lostPower();
  Serial.print(lost ? F("true") : F("false"));
  if (rtc.readFlagRegister() != RV8803_READ_ERROR && lost == !batteryInstalled) {
    Serial.println(F(" PASS"));
    passed++;
  } else {
    Serial.println(F(" FAIL"));
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
