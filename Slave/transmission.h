#ifndef TRANSMISSION_H
#define TRANSMISSION_H

#define TRANSMIT_PIN 2
#define BAUD 100
#define BITS 8

void transmit_loop();
void setData(byte data);
bool isNewData();

#endif
