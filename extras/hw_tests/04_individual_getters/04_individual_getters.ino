/*!
 * @file 04_individual_getters.ino
 * @brief Hardware test 04: Individual Getters
 *
 * Tests that individual getter methods work by verifying time actually
 * advances through rollovers (seconds->minutes->hours).
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

  Serial.println(F("=== HW Test 04: Individual Getters ==="));
  Serial.println();

  if (!rtc.begin()) {
    Serial.println(F("FAIL: RV8803 not found"));
    return;
  }

  uint8_t passed = 0;
  uint8_t total = 6;

  // Test 1: Verify static date/time values after set
  Serial.println(F("Test 1: Set time and verify date components..."));
  // Set to 2026-03-04 22:59:55 (Wednesday = day 3)
  DateTime setTime(2026, 3, 4, 22, 59, 55);
  rtc.adjust(setTime);
  delay(100);

  bool datePass = true;
  uint16_t yr = rtc.getYear();
  uint8_t mon = rtc.getMonth();
  uint8_t day = rtc.getDate();
  uint8_t wd = rtc.getWeekday();

  Serial.print(F("  Year: "));
  Serial.print(yr);
  if (yr != 2026) {
    Serial.println(F(" FAIL"));
    datePass = false;
  } else {
    Serial.println(F(" OK"));
  }

  Serial.print(F("  Month: "));
  Serial.print(mon);
  if (mon != 3) {
    Serial.println(F(" FAIL"));
    datePass = false;
  } else {
    Serial.println(F(" OK"));
  }

  Serial.print(F("  Date: "));
  Serial.print(day);
  if (day != 4) {
    Serial.println(F(" FAIL"));
    datePass = false;
  } else {
    Serial.println(F(" OK"));
  }

  Serial.print(F("  Weekday: "));
  Serial.print(wd);
  Serial.print(F(" (expect 3=Wed)"));
  if (wd != 3) {
    Serial.println(F(" FAIL"));
    datePass = false;
  } else {
    Serial.println(F(" OK"));
  }

  if (datePass) {
    Serial.println(F("  PASS"));
    passed++;
  } else {
    Serial.println(F("  FAIL"));
  }

  // Test 2: Verify seconds advance
  Serial.println(F("Test 2: Verify seconds advance (55->56->57)..."));
  // Re-set to ensure we start at 55
  rtc.adjust(DateTime(2026, 3, 4, 22, 59, 55));
  delay(50);

  uint8_t lastSec = rtc.getSeconds();
  Serial.print(F("  Starting at second: "));
  Serial.println(lastSec);

  bool secAdvanced = false;
  unsigned long start = millis();

  // Wait up to 3 seconds to see seconds advance
  while (millis() - start < 3000) {
    uint8_t sec = rtc.getSeconds();
    if (sec != lastSec) {
      Serial.print(F("  Second changed: "));
      Serial.print(lastSec);
      Serial.print(F(" -> "));
      Serial.println(sec);
      secAdvanced = true;
      break;
    }
    delay(100);
  }

  if (secAdvanced) {
    Serial.println(F("  PASS"));
    passed++;
  } else {
    Serial.println(F("  FAIL - seconds did not advance"));
  }

  // Test 3: Verify minute rollover (59->0)
  Serial.println(F("Test 3: Wait for minute rollover (59->0)..."));
  // Set to 22:59:57 to catch minute rollover soon
  rtc.adjust(DateTime(2026, 3, 4, 22, 59, 57));
  delay(50);

  uint8_t startMin = rtc.getMinutes();
  Serial.print(F("  Starting minute: "));
  Serial.println(startMin);

  bool minRolled = false;
  start = millis();

  // Wait up to 6 seconds for minute to roll
  while (millis() - start < 6000) {
    uint8_t min = rtc.getMinutes();
    uint8_t sec = rtc.getSeconds();
    if (min != startMin) {
      Serial.print(F("  Minute rolled: "));
      Serial.print(startMin);
      Serial.print(F(" -> "));
      Serial.print(min);
      Serial.print(F(" at second "));
      Serial.println(sec);
      if (min == 0) {
        minRolled = true;
      }
      break;
    }
    delay(200);
  }

  if (minRolled) {
    Serial.println(F("  PASS"));
    passed++;
  } else {
    Serial.println(F("  FAIL - minute did not roll to 0"));
  }

  // Test 4: Verify hour rollover (22->23)
  Serial.println(F("Test 4: Wait for hour rollover (22->23)..."));
  // Set to 22:59:57 again
  rtc.adjust(DateTime(2026, 3, 4, 22, 59, 57));
  delay(50);

  uint8_t startHr = rtc.getHours();
  Serial.print(F("  Starting hour: "));
  Serial.println(startHr);

  bool hrRolled = false;
  start = millis();

  // Wait up to 6 seconds for hour to roll
  while (millis() - start < 6000) {
    uint8_t hr = rtc.getHours();
    uint8_t min = rtc.getMinutes();
    if (hr != startHr) {
      Serial.print(F("  Hour rolled: "));
      Serial.print(startHr);
      Serial.print(F(" -> "));
      Serial.print(hr);
      Serial.print(F(" at minute "));
      Serial.println(min);
      if (hr == 23) {
        hrRolled = true;
      }
      break;
    }
    delay(200);
  }

  if (hrRolled) {
    Serial.println(F("  PASS"));
    passed++;
  } else {
    Serial.println(F("  FAIL - hour did not roll to 23"));
  }

  // Test 5: Verify getHours/getMinutes/getSeconds are consistent
  Serial.println(F("Test 5: Consistency check (multiple reads)..."));
  rtc.adjust(DateTime(2026, 3, 4, 12, 30, 15));
  delay(100);

  bool consistent = true;
  for (int i = 0; i < 5; i++) {
    uint8_t h1 = rtc.getHours();
    uint8_t m1 = rtc.getMinutes();
    uint8_t s1 = rtc.getSeconds();
    delay(10);
    uint8_t h2 = rtc.getHours();
    uint8_t m2 = rtc.getMinutes();
    uint8_t s2 = rtc.getSeconds();

    // Hours and minutes should match (seconds might advance by 1)
    if (h1 != h2 || m1 != m2) {
      Serial.print(F("  Inconsistent read at iteration "));
      Serial.println(i);
      consistent = false;
      break;
    }
  }

  if (consistent) {
    Serial.println(F("  PASS"));
    passed++;
  } else {
    Serial.println(F("  FAIL"));
  }

  // Test 6: Verify getHundredths works
  Serial.println(F("Test 6: getHundredths() returns valid values..."));
  bool hundredthsOk = true;
  uint8_t prev = rtc.getHundredths();
  bool changed = false;

  for (int i = 0; i < 50; i++) {
    uint8_t h = rtc.getHundredths();
    if (h > 99) {
      Serial.print(F("  Invalid hundredths value: "));
      Serial.println(h);
      hundredthsOk = false;
      break;
    }
    if (h != prev) {
      changed = true;
    }
    delay(20);
  }

  if (hundredthsOk && changed) {
    Serial.println(F("  PASS"));
    passed++;
  } else if (!hundredthsOk) {
    Serial.println(F("  FAIL - invalid value"));
  } else {
    Serial.println(F("  FAIL - hundredths never changed"));
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
