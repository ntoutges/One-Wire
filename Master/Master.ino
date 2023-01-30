#include "transmission.h"

extern byte dataIn;

void setup() {
  Serial.begin(9600);
}

void loop() {
  transmit_loop();

  if (Serial.available()) {
    sendData(Serial.read());
  }
//  if (isNewData()) {
//    Serial.println(dataIn);
//  }
}
