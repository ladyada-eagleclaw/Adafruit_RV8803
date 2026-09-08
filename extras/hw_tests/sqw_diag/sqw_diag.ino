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

  Serial.println(F("=== SQW Simple Diagnostic ==="));

  if (!rtc.begin()) {
    Serial.println(F("RV8803 not found"));
    return;
  }

  // Set 32kHz mode
  rtc.writeSqwPinMode(RV8803_SquareWave32kHz);

  // Read back extension register
  uint8_t ext = rtc.readExtensionRegister();
  Serial.print(F("Extension reg: 0x"));
  Serial.println(ext, HEX);
  Serial.print(F("FD bits: "));
  Serial.println((ext >> 2) & 0x03);

  // Try all CLKOE states, hold each for 5 seconds so scope can see
  Serial.println(F("D2 as OUTPUT LOW for 5s... (scope SQWAVE now)"));
  pinMode(2, OUTPUT);
  digitalWrite(2, LOW);
  delay(5000);

  Serial.println(F("D2 as OUTPUT HIGH for 5s... (scope SQWAVE now)"));
  digitalWrite(2, HIGH);
  delay(5000);

  Serial.println(F("D2 as INPUT (floating) for 5s... (scope SQWAVE now)"));
  pinMode(2, INPUT);
  delay(5000);

  Serial.println(F("D2 as INPUT_PULLUP for 5s... (scope SQWAVE now)"));
  pinMode(2, INPUT_PULLUP);
  delay(5000);

  Serial.println(F("Done. D2 left as INPUT_PULLUP."));
}

void loop() {}
