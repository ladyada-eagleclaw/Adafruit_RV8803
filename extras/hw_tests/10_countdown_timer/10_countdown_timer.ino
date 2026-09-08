// Metro Mini: VIN=A0, INT=D3, SDA=A4, SCL=A5.
// Manual sections 4.5.2-4.5.3 distinguish startup from repeating periods.
#include <Adafruit_RV8803.h>

Adafruit_RV8803 rtc;
const uint16_t interruptPin = 3;
// Covers the uncalibrated Metro clock and I2C setup, not RTC ppm accuracy.
const uint32_t timingToleranceUs = 50000;
volatile uint32_t edgeTime = 0;
volatile uint8_t edgeCount = 0;

void setup() {
  Serial.begin(115200);
  // Wait for Serial Monitor on native USB boards; remove for standalone use.
  while (!Serial) delay(10);
  delay(250);
  Serial.println(F("Adafruit RV8803 countdown timing test"));
  digitalWrite(A0, HIGH);
  pinMode(A0, OUTPUT);
  pinMode(interruptPin, INPUT);
  delay(600); // Power-on reset can take 500 ms (manual section 7.4).
  check(rtc.begin(), F("Begin succeeded"));
  // Isolate the timer on the shared INT output and release prescaler RESET.
  check(rtc.writeControlRegister(0), F("Interrupt sources disabled"));
  attachInterrupt(digitalPinToInterrupt(interruptPin), timerEdge, FALLING);

  const uint16_t presets[] = {3, 5};
  for (uint8_t i = 0; i < 2; i++) {
    uint16_t preset = presets[i];
    Serial.println();
    Serial.print(F("Countdown preset: "));
    Serial.println(preset);
    // Manual 4.5.2: clear TE, TIE, TF in that order before configuring.
    check(rtc.disableCountdownTimer(), F("Timer stopped for configuration"));
    check(rtc.disableInterrupt(RV8803_InterruptTimer), F("Timer interrupt disabled"));
    check(rtc.clearTimer(), F("Timer flag cleared"));
    delay(10);
    check(digitalRead(interruptPin) == HIGH, F("INT initially released"));
    noInterrupts();
    edgeCount = 0;
    edgeTime = 0;
    interrupts();
    check(rtc.enableInterrupt(RV8803_InterruptTimer), F("Timer interrupt enabled"));
    uint32_t previous = micros();
    check(rtc.enableCountdownTimer(RV8803_Timer1Hz, preset), F("Countdown started"));
    check(rtc.getCountdownTimer() == preset, F("Preset readback matched"));

    for (uint8_t event = 1; event <= 3; event++) {
      unsigned long waitStart = millis();
      while (edgeCount < event && millis() - waitStart < (preset + 2UL) * 1000) {
        delay(1);
      }
      noInterrupts();
      uint32_t captured = edgeTime;
      uint8_t count = edgeCount;
      interrupts();
      check(count == event, F("Expected INT falling edge received"));
      uint32_t elapsed = captured - previous;
      Serial.print(F("INT interval (seconds): "));
      Serial.println(elapsed / 1000000.0, 6);
      uint32_t nominal = preset * 1000000UL;
      if (event == 1) {
        // First interval: n through n+1 seconds, plus fixture tolerance.
        check(elapsed >= nominal - timingToleranceUs &&
              elapsed <= nominal + 1000000UL + timingToleranceUs,
              F("Startup interval matched the datasheet"));
      } else {
        check(abs((long)elapsed - (long)nominal) <= (long)timingToleranceUs,
              F("Repeated interval matched the preset"));
      }
      check(rtc.timerFired(), F("Timer flag asserted"));
      check(rtc.clearTimer(), F("Timer flag cleared after event"));
      check(!rtc.timerFired(), F("Timer flag read back clear"));
      previous = captured;
      // Leave TE enabled so the next event uses automatic reload.
    }
  }
  check(rtc.disableCountdownTimer(), F("Timer stopped"));
  check(rtc.disableInterrupt(RV8803_InterruptTimer), F("Timer interrupt disabled"));
  check(rtc.clearTimer(), F("Final timer flag cleared"));
  uint8_t stoppedCount = edgeCount;
  delay(5100);
  check(edgeCount == stoppedCount && !rtc.timerFired(), F("No events after stopping"));
  detachInterrupt(digitalPinToInterrupt(interruptPin));
  Serial.println(F("ALL PASSED"));
}

void loop() {}

void timerEdge() {
  edgeTime = micros();
  edgeCount++;
}

void clearOutputsAndHalt(const __FlashStringHelper* message) {
  Serial.print(F("FAIL: "));
  Serial.println(message);
  rtc.disableCountdownTimer();
  rtc.disableInterrupt(RV8803_InterruptTimer);
  detachInterrupt(digitalPinToInterrupt(interruptPin));
  while (true) delay(10);
}

void check(bool success, const __FlashStringHelper* message) {
  if (!success) clearOutputsAndHalt(message);
  Serial.println(message);
}
