#include "Arduino.h"
#include "sigmaSignalCore.h"

enum MasterStates {
  M_S_INIT,
  M_S_RESET,
  M_S_IDLE,
  M_S_SEND_1,
  M_S_SEND_2,
  M_S_SEND_3,
  M_S_SHIFT_BUFFER,
  M_S_RECEIVE_1,
  M_S_RECEIVE_2,
  M_S_COOLDOWN_1,
  M_S_COOLDOWN_2
};

enum SlaveStates {
  S_S_INIT,
  S_S_RESET,
  S_S_IDLE,
  S_S_RECEIVE_1,
  S_S_RECEIVE_2,
  S_S_UPDATE_PRINT_BUFFER,
  S_S_SEND_1,
  S_S_SEND_2,
  S_S_SEND_3,
  S_S_COOLDOWN
};

// master code
SigmaSignalMaster::SigmaSignalMaster(int pin = 2, unsigned int tBits = 8, unsigned int rBits = 8, int baud = 100) {
  this->pin = pin;
  this->tBits = tBits;
  this->rBits = rBits;
  this->baud = baud;
  this->state = M_S_INIT;
}

void SigmaSignalMaster::tick() {
  switch (this->state) {
    case M_S_INIT:
      this->printBuffer = new byte[TRANSMISSION_MAX_BUFFER_LENGTH]; // circular buffer
      this->printBufferLength = 0;
      this->printBufferIndex = 0;
      this->readBuffer = new byte[TRANSMISSION_MAX_BUFFER_LENGTH]; // circular buffer
      this->readBufferLength = 0;
      this->readBufferIndex = 0;
      this->state = M_S_RESET;
      break;
    case M_S_RESET:
      this->bitIndex = 0;
      pinMode(this->pin, OUTPUT);
      digitalWrite(this->pin, LOW);
      this->state = M_S_IDLE;
      break;
    case M_S_IDLE:
      if ((this->tBits % 8 == 0 && this->printBufferLength >= this->tBits / 8) || this->printBufferLength > this->tBits / 8) {
        this->state = M_S_SEND_1;
      }
      break;
    case M_S_SEND_1:
      digitalWrite(this->pin, HIGH); // Send initial pulse to notify slave of incoming transmission
      this->transmit_timer = millis() + 1000 / baud;
      this->state = M_S_SEND_2;
      break;
    case M_S_SEND_2:
      if (millis() > this->transmit_timer) {
        byte workingBitIndex = this->bitIndex % 8;
        if (workingBitIndex == 0) {
          this->printBufferLength--;
        }
        byte localWriteBufferIndex = this->nextPrintBufferIndex(-this->printBufferLength);

        digitalWrite(this->pin, (this->printBuffer[localWriteBufferIndex] >> (7 - workingBitIndex)) & 1); // send each individual bit as either a 1 or 0
        this->bitIndex++;
        this->transmit_timer += 1000 / this->baud;

        if (this->bitIndex == this->tBits) {
          this->state = M_S_SEND_3;
        }
      }
      break;
    case M_S_SEND_3:
      if (millis() > this->transmit_timer) {
        digitalWrite(this->pin, LOW);
        pinMode(this->pin, INPUT);
        this->state = M_S_RECEIVE_1;
      }
      break;
    case M_S_RECEIVE_1:
      digitalWrite(this->pin, LOW);
      pinMode(this->pin, INPUT);
      this->bitIndex = 0;
      this->transmit_timer += 1000 / this->baud;
      if (this->readBufferLength == TRANSMISSION_MAX_BUFFER_LENGTH) {
        this->readBufferLength--; // sacrafice last item in buffer to keep transmissions up-to-date
      }
      this->state = M_S_RECEIVE_2;
      break;
    case M_S_RECEIVE_2:
      if (millis() > this->transmit_timer) {
        byte workingBitIndex = this->bitIndex % 8;
        if (workingBitIndex == 0) {
          this->readBufferIndex = this->nextReadBufferIndex(1);
//          this->readBufferLength++; // counteract increase in index
        }
        this->readBuffer[this->readBufferIndex] = this->readBuffer[this->readBufferIndex] << 1 | digitalRead(this->pin);
        this->bitIndex++;
        this->transmit_timer += 1000 / this->baud;

        if (this->bitIndex == this->rBits) {
          this->readBufferLength += this->bitIndex / 8;
          if (workingBitIndex != 7) { // calculated before wrap around of (mod 7)
            this->readBufferLength++;
          }
          this->state = M_S_COOLDOWN_1;
        }
      }
      break;
    case M_S_COOLDOWN_1:
      pinMode(this->pin, OUTPUT);
      this->bitIndex++;
      this->transmit_timer += 1500 / this->baud;
      this->state = M_S_COOLDOWN_2;
      break;
    case M_S_COOLDOWN_2:
      if (millis() > this->transmit_timer) {
        this->state = M_S_RESET;
      }
      break;
  }
}

void SigmaSignalMaster::print(byte data) {
  if (this->printBufferLength == TRANSMISSION_MAX_BUFFER_LENGTH-1) {
    return; // purge data that cannot be sent
  }
  this->printBufferIndex = this->nextPrintBufferIndex(1);
  this->printBuffer[this->printBufferIndex] = data;
  this->printBufferLength++;
}

byte SigmaSignalMaster::read() {
  if (this->readBufferLength == 0) {
    return 0; // default value -- cannot try to find a value that does not exist
  }
//  this->readBufferIndex = this->nextReadBufferIndex(-1);
  byte toReturn = this->readBuffer[this->nextReadBufferIndex(-readBufferLength+1)];
  this->readBufferLength--;
  return toReturn;
}

byte SigmaSignalMaster::available() {
  return this->readBufferLength;
}

byte SigmaSignalMaster::nextPrintBufferIndex(int step) {
  if (step < 0) {
    step += TRANSMISSION_MAX_BUFFER_LENGTH;
  }
  return (this->printBufferIndex + step) % TRANSMISSION_MAX_BUFFER_LENGTH;
}

byte SigmaSignalMaster::nextReadBufferIndex(int step) {
  if (step < 0) {
    step += TRANSMISSION_MAX_BUFFER_LENGTH;
  }
  return (this->readBufferIndex + step) % TRANSMISSION_MAX_BUFFER_LENGTH;
}


// slave code
SigmaSignalSlave::SigmaSignalSlave(int pin = 2, unsigned int tBits = 8, unsigned int rBits = 8, int baud = 100) {
  this->pin = pin;
  this->tBits = tBits;
  this->rBits = rBits;
  this->baud = baud;
  this->state = S_S_INIT;
}

void SigmaSignalSlave::tick() {
  switch (this->state) {
    case S_S_INIT:
      this->printBuffer = new byte[TRANSMISSION_MAX_BUFFER_LENGTH];
      this->printBuffer[0] = 0b01101100; // test pattern
      this->printBufferLength = 1;
      this->readBufferIndex = 1;
      this->readBuffer = new byte[TRANSMISSION_MAX_BUFFER_LENGTH];
      this->readBufferLength = 0;
      this->readBufferIndex = 0;
      this->state = S_S_RESET;
      this->toPrintBuffer = new byte[TRANSMISSION_MAX_BUFFER_LENGTH];
      this->toPrintBufferLength = 0;
      break;
    case S_S_RESET:
      this->bitIndex = 0;
      pinMode(this->pin, INPUT);
      digitalWrite(this->pin, LOW);
      this->state = S_S_IDLE;
      break;
    case S_S_IDLE:
      if (digitalRead(this->pin)) { // message starts as soon as this->pin pulled HIGH
        this->state = S_S_RECEIVE_1;
      }
      break;
    case S_S_RECEIVE_1:
      this->transmit_timer = millis() + 1500 / this->baud;
//      if (this->readBufferLength == TRANSMISSION_MAX_BUFFER_LENGTH) {
//        this->readBufferLength--; // sacrafice last item in buffer to keep transmissions up-to-date
//      }
      this->state = S_S_RECEIVE_2;
      break;
    case S_S_RECEIVE_2:
      if (millis() > this->transmit_timer) {
        byte workingBitIndex = this->bitIndex % 8;
        if (workingBitIndex == 0) {
          this->readBufferIndex = this->nextReadBufferIndex(1);
          this->readBuffer[this->readBufferIndex] = 0; // reset garbage in circular buffer
        }
        this->readBuffer[this->readBufferIndex] = this->readBuffer[this->readBufferIndex] << 1 | digitalRead(this->pin);
        this->bitIndex++;
        this->transmit_timer += 1000 / this->baud;

        if (this->bitIndex == this->rBits) {
          this->readBufferLength += this->bitIndex / 8;
          if (workingBitIndex != 7) { // calculated before wrap around of (mod 7)
            this->readBufferLength++;
          }
          this->state = S_S_UPDATE_PRINT_BUFFER;
        }
      }
      break;
    case S_S_UPDATE_PRINT_BUFFER:
      this->printBufferLength = 0;
      for (byte i = 0; i < this->toPrintBufferLength; i++) {
        this->printBuffer[this->printBufferLength] = this->toPrintBuffer[i];
        this->printBufferLength++;
      }
      this->toPrintBufferLength  = 0;
      this->state = S_S_SEND_1;
      break;
    case S_S_SEND_1:
      pinMode(this->pin, OUTPUT);
      this->bitIndex = 0;
      this->state = S_S_SEND_2;
      break;
    case S_S_SEND_2:
      if (millis() > this->transmit_timer) {
        digitalWrite(this->pin, (this->printBuffer[this->bitIndex / 8] >> (7 - this->bitIndex % 8)) & 1); // send each individual bit as either a 1 or 0
        this->bitIndex++;
        this->transmit_timer += 1000 / this->baud;

        if (this->bitIndex == this->tBits) {
          this->state = S_S_SEND_3;
        }
      }
      break;
    case S_S_SEND_3:
      if (millis() > this->transmit_timer) {
        digitalWrite(this->pin, LOW);
        pinMode(this->pin, INPUT);
        this->transmit_timer += 1000 / this->baud;
        this->state = S_S_COOLDOWN;
      }
      break;
    case S_S_COOLDOWN:
      if (millis() > this->transmit_timer) {
        this->state = S_S_RESET;
      }
      break;
  }
}

void SigmaSignalSlave::toPrint(byte data) {
  if (this->toPrintBufferLength == TRANSMISSION_MAX_BUFFER_LENGTH) {
    return; // throw out extra data
  }
  this->toPrintBuffer[this->toPrintBufferLength] = data;
  this->toPrintBufferLength++;
}

void SigmaSignalSlave::clearPrintBuffer() {
  this->printBufferLength = 0;
}

byte SigmaSignalSlave::read() {
  if (this->readBufferLength == 0) {
    return 0; // default value -- cannot try to find a value that does not exist
  }
//  this->readBufferIndex = this->nextReadBufferIndex(-1);
  byte toReturn = this->readBuffer[this->nextReadBufferIndex(-readBufferLength+1)];
  this->readBufferLength--;
  return toReturn;
}

byte SigmaSignalSlave::available() {
  return this->readBufferLength;
}

byte SigmaSignalSlave::nextReadBufferIndex(int step) {
  if (step < 0) {
    step += TRANSMISSION_MAX_BUFFER_LENGTH;
  }
  return (this->readBufferIndex + step) % TRANSMISSION_MAX_BUFFER_LENGTH;
}
