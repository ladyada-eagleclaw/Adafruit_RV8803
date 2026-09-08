/*!
 * @file 06_ram.ino
 * @brief Hardware test 06: RAM Read/Write + Power Cycle
 *
 * Tests the 1-byte RAM register, including loss after full power cycle.
 * Requires: VCC wired to A0, no coin battery.
 */

#include <Adafruit_RV8803.h>

#define RTC_VCC_PIN A0
#define RTC_SDA_PIN A4
#define RTC_SCL_PIN A5

Adafruit_RV8803 rtc;

/*!
 * @brief Kill all power to the RV-8803 breakout.
 *
 * Ends Wire, then drives VCC, SDA, and SCL all LOW to prevent
 * parasitic power through ESD protection diodes.
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
  delay(100);

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
  delay(100); // Let chip stabilize after power-on

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

  // Test 5: RAM lost after power cycle (no battery backup)
  Serial.print(F("Test 5: RAM lost after power cycle ... "));
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
      if (afterCycle != 0xA5) {
        Serial.println(F(" PASS (data lost)"));
        passed++;
      } else {
        Serial.println(F(" FAIL (data survived)"));
      }
    }
  }

  // Test 6: lostPower() true after power cycle
  Serial.print(F("Test 6: lostPower() after cycle ... "));
  bool lost = rtc.lostPower();
  Serial.print(lost ? F("true") : F("false"));
  if (lost) {
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
