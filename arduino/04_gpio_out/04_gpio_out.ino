#include <TensorFlowLite.h>

// Arduino 스타일 버전
#if defined(ARDUINO_ARDUINO_NANO33BLE)
const int gpio_pin_out = LED_BUILTIN;  // 적절한 핀 번호로 변경
#endif

#if defined(ARDUINO_RASPBERRY_PI_PICO)
const int gpio_pin_out = LED_BUILTIN;  // 또는 내장 LED용 25
#endif

void setup() {
  Serial.begin(9600);
  while (!Serial) ;

  pinMode(gpio_pin_out, OUTPUT);
  Serial.println("Initialization completed");
}

void loop() {
  digitalWrite(gpio_pin_out, HIGH);
  delay(1000);

  digitalWrite(gpio_pin_out, LOW);
  delay(1000);

  Serial.println("Executed");
}
