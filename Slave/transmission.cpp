// One-Wire data transmission
#include "Arduino.h"
#include "transmission.h"

byte dataOut = 0b01101100; // test pattern
byte dataIn;
bool newData = false;

byte bitIndex;

enum states {
  UI_T_INIT,
  UI_T_IDLE,
  UI_T_SEND_1,
  UI_T_SEND_2,
  UI_T_SEND_3,
  UI_T_RECEIVE_1,
  UI_T_RECEIVE_2,
  UI_T_RECEIVE_3,
  UI_T_COOLDOWN
};
byte tstate = UI_T_INIT;
unsigned long transmit_timer;

void transmit_loop() {
  switch (tstate) {
    case UI_T_INIT:
      pinMode(TRANSMIT_PIN, INPUT);
      tstate = UI_T_IDLE;
      break;
    case UI_T_IDLE:
      if (digitalRead(TRANSMIT_PIN)) {
        tstate = UI_T_RECEIVE_1;
      }
      break;
    case UI_T_RECEIVE_1:
        bitIndex = 0;
        dataIn = 0;
        tstate = UI_T_RECEIVE_2;
        transmit_timer = millis() + 1500 / BAUD;
      break;
    case UI_T_RECEIVE_2:
      if (millis() > transmit_timer) {
        tstate = (bitIndex == BITS) ? UI_T_SEND_1 : UI_T_RECEIVE_3;
      }
      break;
    case UI_T_RECEIVE_3:
      dataIn = (dataIn << 1) | digitalRead(TRANSMIT_PIN);
      bitIndex++;
      transmit_timer += 1000 / BAUD;
      tstate = UI_T_RECEIVE_2;
      break;
    case UI_T_SEND_1:
      Serial.print("< ");
      Serial.print(dataIn);
      pinMode(TRANSMIT_PIN, OUTPUT);
      digitalWrite(TRANSMIT_PIN, HIGH);
      bitIndex = 0;
      tstate = UI_T_SEND_2;
      break;
    case UI_T_SEND_2:
      if (millis() > transmit_timer) {
        tstate = (bitIndex == BITS) ? UI_T_COOLDOWN : UI_T_SEND_3;
      }
      break;
    case UI_T_SEND_3:
      digitalWrite(TRANSMIT_PIN, (dataOut >> (BITS - 1 - bitIndex)) & 1);
      
      bitIndex++;
      transmit_timer += ((bitIndex == BITS) ? 2500 : 1000) / BAUD;
      tstate = UI_T_SEND_2;
      break;
    case UI_T_COOLDOWN:
      Serial.print(" < ");
      Serial.println(dataOut);
      newData = true;
      digitalWrite(TRANSMIT_PIN, LOW);
      tstate = UI_T_INIT;
      break;
  }
}

bool isNewData() {
  if (newData) {
    newData = false;
    return true;
  }
  return false;
}

void setNewData(byte data) {
  dataOut = data;
}
