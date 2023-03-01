#include "sigmaSignalCore.h"

SigmaSignalSlave mySlave(2,8,8,500);

void setup() {
  Serial.begin(115200);
}

void loop() {
  mySlave.tick();
  while (mySlave.available()) {
    byte echo1 = mySlave.read();
    Serial.print("GOT: ");
    Serial.print(echo1);
    Serial.print("; SENT:");
    Serial.println(echo1);
    mySlave.toPrint(echo1);
  }
}
