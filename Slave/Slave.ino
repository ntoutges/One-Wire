#include "transmission.h"

void setup() {
  Serial.begin(9600);
}

void loop() {
  transmit_loop();
}
