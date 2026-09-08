// Press the breakout's EVI button to capture seconds and hundredths.
// EVI must have a defined idle level; the breakout pulls it up to VIN.
#include <Adafruit_RV8803.h>

Adafruit_RV8803 rtc;

void setup() {
  Serial.begin(115200);
  // Wait for Serial Monitor on native USB boards; remove for standalone use.
  while (!Serial) delay(10);
  delay(250);
  Serial.println(F("Adafruit RV8803 event capture"));
  if (!rtc.begin()) {
    Serial.println(F("RTC not found"));
    while (true) delay(10);
  }
  if (rtc.lostPower() && !rtc.adjust(DateTime(F(__DATE__), F(__TIME__)))) {
    Serial.println(F("Could not set the time"));
    while (true) delay(10);
  }
  // Falling edge with a 15.6 ms sampling interval filters short button glitches.
  // ERST must be off for timestamps: an event reset clears the capture registers.
  if (!rtc.disableInterrupt(RV8803_InterruptEvent) ||
      !rtc.configureEvent(false, RV8803_EventFilter16ms) ||
      !rtc.enableEventReset(false) || !rtc.enableEventCapture(true) ||
      !rtc.clearEvent() || !rtc.enableInterrupt(RV8803_InterruptEvent)) {
    Serial.println(F("Could not configure event capture"));
    while (true) delay(10);
  }
  Serial.println(F("Press EVI to capture a timestamp within the current minute"));
}

void loop() {
  if (rtc.eventFired()) {
    rv8803_timestamp_t captured;
    if (rtc.getEventTimestamp(&captured)) {
      Serial.print(F("Captured second: "));
      Serial.print(captured.seconds);
      Serial.print('.');
      if (captured.hundredths < 10) Serial.print('0');
      Serial.println(captured.hundredths);
      if (!rtc.clearEvent()) Serial.println(F("Could not clear the event flag"));
    }
  }
  delay(10);
}
