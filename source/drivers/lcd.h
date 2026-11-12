#ifndef LCD_H
#define LCD_H
#include <stdint.h>
uint8_t LCD_Init(void);
void LCD_Clear(void);
uint8_t LCD_Flush(void);
void LCD_DrawPixel(uint8_t x, uint8_t y, uint8_t color);
void LCD_DrawRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t color);
void LCD_DrawText(uint8_t x, uint8_t y, const char *text);
void LCD_DrawNumber(uint8_t x, uint8_t y, uint16_t value);
const uint8_t *LCD_GetBuffer(void);
#endif
