#ifndef SIGMA_SIGNAL_CORE_H
#define SIGMA_SIGNAL_CORE_H

#define TRANSMISSION_MAX_BUFFER_LENGTH 5

class SigmaSignalMaster {
  int pin;
  unsigned int tBits; // bits to transmit
  unsigned int rBits; // bits to receive
  int baud;
  byte state;
  byte* printBuffer;
  byte printBufferLength;
  byte printBufferIndex;
  byte* readBuffer;
  byte readBufferLength;
  byte readBufferIndex;
  
  unsigned long transmit_timer;
  byte bitIndex;
  
  public:
  SigmaSignalMaster(int pin, unsigned int tBits, unsigned int rBits, int baud);
  void tick();
  void print(byte data);
  byte read();
  byte available(); // returns bytes available to read

  private:
  byte nextPrintBufferIndex(int step);
  byte nextReadBufferIndex(int step);
};

class SigmaSignalSlave {
  int pin;
  unsigned int tBits; // bits to transmit
  unsigned int rBits; // bits to receive
  int baud;
  byte state;
  byte* printBuffer;
  byte printBufferLength;
  byte* toPrintBuffer;
  byte toPrintBufferLength;
  byte* readBuffer;
  byte readBufferLength;
  byte readBufferIndex;

  unsigned long transmit_timer;
  byte bitIndex;

  public:
  SigmaSignalSlave(int pin, unsigned int tBits, unsigned int rBits, int baud);
  void tick();
  void toPrint(byte data); // calling this does not initiate a print, only sets what will be printed when the time comes
  void clearPrintBuffer();
  byte read();
  byte available(); // returns bytes available to read

  private:
  byte nextReadBufferIndex(int step);
};

#endif
