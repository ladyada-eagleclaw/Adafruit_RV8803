// Metro Mini: VIN=A0, INT=D3, EVI=D4, SDA=A4, SCL=A5.
// EVI is pulled up on the breakout. D4 only pulls low or releases it.
#include <Adafruit_RV8803.h>

Adafruit_RV8803 rtc;
const uint16_t eventPin = 4;
const uint16_t interruptPin = 3;

void setup() {
  Serial.begin(115200);
  // Wait for Serial Monitor on native USB boards; remove for standalone use.
  while (!Serial) delay(10);
  delay(250);
  Serial.println(F("Adafruit RV8803 external event test"));
  digitalWrite(A0, HIGH);
  pinMode(A0, OUTPUT);
  pinMode(eventPin, INPUT);
  pinMode(interruptPin, INPUT);
  delay(100);
  check(rtc.begin(), F("Begin succeeded"));
  check(rtc.writeControlRegister(0), F("Interrupts disabled"));
  check(rtc.enableEventReset(false), F("Event reset disabled"));
  check(rtc.enableEventCapture(true), F("Timestamp capture enabled"));
  const rv8803_event_filter_t filters[] = {RV8803_EventFilterNone,
      RV8803_EventFilter4ms, RV8803_EventFilter16ms, RV8803_EventFilter125ms};

  for (uint8_t polarity = 0; polarity < 2; polarity++) {
    bool rising = polarity == 1;
    for (uint8_t i = 0; i < 4; i++) {
      Serial.println();
      Serial.print(F("Rising edge: "));
      Serial.print(rising);
      Serial.print(F(", filter selection: "));
      Serial.println(i);
      check(rtc.disableInterrupt(RV8803_InterruptEvent), F("Event interrupt disabled"));
      setInputLevel(!rising);
      check(rtc.configureEvent(rising, filters[i]), F("Polarity and filter configured"));
      delay(300);
      check(rtc.clearEvent(), F("Event flag cleared"));
      check(rtc.adjust(DateTime(2026, 9, 7, 12, 0, 17)), F("Reference time set"));
      check(rtc.enableInterrupt(RV8803_InterruptEvent), F("Event interrupt enabled"));
      check(digitalRead(interruptPin) == HIGH, F("INT initially released"));
      delay(300);
      setInputLevel(rising);
      delay(300); // At least two sampling periods for the longest filter.
      check(rtc.eventFired(), F("External event detected"));
      check(digitalRead(interruptPin) == LOW, F("INT asserted"));
      rv8803_timestamp_t captured;
      check(rtc.getEventTimestamp(&captured), F("Capture registers read together"));
      uint8_t seconds = captured.seconds;
      uint8_t hundredths = captured.hundredths;
      Serial.print(F("Captured time: "));
      Serial.print(seconds);
      Serial.print('.');
      Serial.println(hundredths);
      check(seconds == 17 && hundredths >= 25 && hundredths < 90,
            F("Timestamp corresponds to the input event"));
      delay(100);
      check(rtc.getEventSeconds() == seconds && rtc.getEventHundredths() == hundredths,
            F("Timestamp remained captured while the clock advanced"));
      check(rtc.clearEvent(), F("Event flag cleared after capture"));
      check(digitalRead(interruptPin) == HIGH, F("INT released after clear"));
    }
  }

  Serial.println();
  check(rtc.configureEvent(true, RV8803_EventFilter125ms), F("Long filter selected"));
  setInputLevel(false);
  delay(300);
  check(rtc.clearEvent(), F("Filter test armed"));
  setInputLevel(true);
  delay(2);
  setInputLevel(false);
  delay(300);
  check(!rtc.eventFired(), F("Long filter rejected a 2 ms pulse"));

  Serial.println();
  check(rtc.configureEvent(true, RV8803_EventFilterNone), F("Unfiltered rising edge selected"));
  check(rtc.enableEventReset(true), F("One-shot event reset armed"));
  setInputLevel(true);
  delay(20);
  check(rtc.eventFired(), F("Reset event detected"));
  check(!(rtc.readEventControl() & RV8803_EVCTRL_ERST), F("ERST cleared automatically"));
  check(rtc.getEventSeconds() == 0 && rtc.getEventHundredths() == 0,
        F("Event reset cleared both capture registers"));
  check(rtc.getHundredths() < 10, F("Event reset restarted fractional seconds"));
  check(rtc.enableEventCapture(false), F("Capture disabled"));
  setInputLevel(false);
  delay(100);
  setInputLevel(true);
  delay(20);
  check(rtc.getEventSeconds() == 0 && rtc.getEventHundredths() == 0,
        F("Disabled capture left the timestamp unchanged"));
  cleanup();
  Serial.println(F("ALL PASSED"));
}

void loop() {}

void setInputLevel(bool high) {
  if (high) {
    pinMode(eventPin, INPUT); // Release to the breakout pull-up.
  } else {
    digitalWrite(eventPin, LOW);
    pinMode(eventPin, OUTPUT);
  }
}

void cleanup() {
  rtc.disableInterrupt(RV8803_InterruptEvent);
  rtc.enableEventCapture(false);
  rtc.enableEventReset(false);
  rtc.clearEvent();
  pinMode(eventPin, INPUT);
}

void clearOutputsAndHalt(const __FlashStringHelper* message) {
  Serial.print(F("FAIL: "));
  Serial.println(message);
  cleanup();
  while (true) delay(10);
}

void check(bool ok, const __FlashStringHelper* message) {
  if (!ok) clearOutputsAndHalt(message);
  Serial.println(message);
}
