// One-Wire data transmission
#include "Arduino.h"
#include "transmission.h"

byte dataOut;
byte dataIn;
bool newData = false;
bool newRData = false;

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
  UI_T_COOLDOWN_1,
  UI_T_COOLDOWN_2
};
byte tstate = UI_T_INIT;
unsigned long transmit_timer;

void transmit_loop() {
  switch (tstate) {
    case UI_T_INIT:
      pinMode(TRANSMIT_PIN, OUTPUT);
      tstate = UI_T_IDLE;
      break;
    case UI_T_IDLE:
      if (newData) {
        newData = false;
        tstate = UI_T_SEND_1;
      }
      break;
    case UI_T_SEND_1:
      Serial.print("> ");
      Serial.print(dataOut);
      digitalWrite(TRANSMIT_PIN, HIGH);
      transmit_timer = millis() + 1000 / BAUD;
      bitIndex = 0;
      tstate = UI_T_SEND_2;
      break;
    case UI_T_SEND_2:
      if (millis() > transmit_timer) {
        tstate = (bitIndex == BITS) ? UI_T_RECEIVE_1 : UI_T_SEND_3;
      }
      break;
    case UI_T_SEND_3:
      digitalWrite(TRANSMIT_PIN, (dataOut >> (BITS - 1 - bitIndex)) & 1);
      
      bitIndex++;
      transmit_timer += 1000 / BAUD;
      tstate = UI_T_SEND_2;
      break;
    case UI_T_RECEIVE_1:
        digitalWrite(TRANSMIT_PIN, LOW);
        pinMode(TRANSMIT_PIN, INPUT);
        bitIndex = 0;
        dataIn = 0;
        tstate = UI_T_RECEIVE_2;
        transmit_timer += 1000 / BAUD;
      break;
    case UI_T_RECEIVE_2:
      if (millis() > transmit_timer) {
        tstate = (bitIndex == BITS) ? UI_T_COOLDOWN_1 : UI_T_RECEIVE_3;
      }
      break;
    case UI_T_RECEIVE_3:
      dataIn = (dataIn << 1) | digitalRead(TRANSMIT_PIN);
      bitIndex++;
      transmit_timer += 1000 / BAUD;
      tstate = UI_T_RECEIVE_2;
      break;
    case UI_T_COOLDOWN_1:
      Serial.print(" > ");
      Serial.println(dataIn);
      newRData = true;
      transmit_timer = millis() + 1000 / BAUD;
      tstate = UI_T_COOLDOWN_2;
      break;
    case UI_T_COOLDOWN_2:
      if (millis() > transmit_timer) {
        tstate = UI_T_INIT;
      }
      break;
  }
}

void sendData(byte data) {
  newData = true;
  dataOut = data;
}

bool isNewData() {
  if (newRData) {
    newRData = false;
    return true;
  }
  return false;
}
