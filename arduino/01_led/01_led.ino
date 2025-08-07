#include <Arduino.h>
#define LED_PIN LED_BUILTIN  // 또는 25

void setup() {
  pinMode(LED_PIN, OUTPUT);

  Serial.begin(9600);
  while (!Serial);
  Serial.print("Initialization completed\n");
}

void loop() {
  digitalWrite(LED_PIN, HIGH);
  delay(500);
  digitalWrite(LED_PIN, LOW);
  delay(500);

  Serial.print("Executed HHHH Good ..\n");
}

