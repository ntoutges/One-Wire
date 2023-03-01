#include "sigmaSignalCore.h"

// behavior with non-multiples of 8 for bits to send not yet solidified
SigmaSignalMaster myMaster(2,8,8,500);

void setup() {
  Serial.begin(115200);
}

void loop() {
  myMaster.tick();
  if (Serial.available()) {
    byte toSend = Serial.read();
//    Serial.print("SENT: ");
//    Serial.print(toSend);
    myMaster.print(toSend);
  }
  if (myMaster.available()) {
//    Serial.print("; GOT: ");
      Serial.print((char) myMaster.read());
//    Serial.print(myMaster.read());
//    Serial.print(",");
//    Serial.println(myMaster.read());
  }
}
