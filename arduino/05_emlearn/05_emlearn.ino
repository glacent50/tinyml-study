
#include "model.h"

#define INPUT_SZ 3

float X_TEST[3][INPUT_SZ] = {{1, 2, 3},
                             {1, 18, 4},
                             {14, 12, 13}};

void setup() {
  Serial.begin(115200);
  while (!Serial);
    Serial.print("Initialization completed\n");
}

void loop() {
  for(int32_t i = 0; i < 3; ++i) {
    int32_t out = model_predict(&X_TEST[i][0], INPUT_SZ);
    Serial.println(out);
  }

  //while(1);

  delay(2000);
  Serial.print("Executed\n");
}