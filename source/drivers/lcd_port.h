#ifndef LCD_PORT_H
#define LCD_PORT_H
#include <stdint.h>
uint8_t LCD_Port_Init(void);
uint8_t LCD_Port_Flush(const uint8_t *buffer, uint16_t size);
#endif
