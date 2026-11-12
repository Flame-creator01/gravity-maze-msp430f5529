#ifndef SAVE_PORT_H
#define SAVE_PORT_H
#include <stdint.h>
/* Slot 0 = INFO D (0x1800): progress copy A
   Slot 1 = INFO C (0x1880): progress copy B
   Slot 2 = INFO B (0x1900): sensitivity tuning record
   The host test substitutes this port with an in-memory model. */
#define SAVE_SLOT_COUNT 3u
#define SAVE_SLOT_SIZE 64u
uint8_t SavePort_Read(uint8_t slot, uint8_t offset, uint8_t *buffer, uint8_t length);
uint8_t SavePort_Erase(uint8_t slot);
uint8_t SavePort_Write(uint8_t slot, uint8_t offset, const uint8_t *buffer, uint8_t length);
#endif
