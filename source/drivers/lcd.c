#include <string.h>
#include "app_config.h"
#include "lcd.h"
#include "lcd_port.h"
#include "font6x8.h"
static uint8_t frame_buffer[816];
uint8_t LCD_Init(void) { LCD_Clear(); return LCD_Port_Init(); }
void LCD_Clear(void) { memset(frame_buffer, 0, sizeof(frame_buffer)); }
void LCD_DrawPixel(uint8_t x, uint8_t y, uint8_t color)
{
    uint16_t index;
    uint8_t mask;
    if (x >= LCD_WIDTH || y >= LCD_HEIGHT) return;
    index = (uint16_t)(y / 8u) * LCD_WIDTH + x;
    mask = (uint8_t)(0x80u >> (y % 8u));
    if (color) frame_buffer[index] |= mask;
    else frame_buffer[index] &= (uint8_t)~mask;
}
void LCD_DrawRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t color)
{
    uint16_t ix, iy;
    for (iy = y; iy < (uint16_t)y + h && iy < LCD_HEIGHT; iy++)
        for (ix = x; ix < (uint16_t)x + w && ix < LCD_WIDTH; ix++)
            LCD_DrawPixel((uint8_t)ix, (uint8_t)iy, color);
}
void LCD_DrawText(uint8_t x, uint8_t y, const char *text)
{
    uint16_t pos;
    uint8_t c, col, row;
    if (text == 0) return;
    pos = x;
    while (*text != '\0' && pos + 5u < LCD_WIDTH)
    {
        c = (uint8_t)*text++;
        if (c < 32u || c > 126u) c = '?';
        for (col = 0u; col < 6u; col++)
            for (row = 0u; row < 8u && (uint16_t)y + row < LCD_HEIGHT; row++)
                LCD_DrawPixel((uint8_t)(pos + col), (uint8_t)(y + row),
                    (uint8_t)((FONT6x8[(uint16_t)(c - 32u) * 6u + col] >> (7u - row)) & 1u));
        pos += 6u;
    }
}
void LCD_DrawNumber(uint8_t x, uint8_t y, uint16_t value)
{
    char text[6];
    uint8_t i;
    text[5] = '\0';
    for (i = 0u; i < 5u; i++) { text[4u-i] = (char)('0' + value % 10u); value /= 10u; }
    LCD_DrawText(x, y, text);
}
uint8_t LCD_Flush(void) { return LCD_Port_Flush(frame_buffer, sizeof(frame_buffer)); }
const uint8_t *LCD_GetBuffer(void) { return frame_buffer; }
