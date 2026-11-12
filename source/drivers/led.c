#include "board_config.h"
#include "led.h"
void Led_Init(void) { LED_PORT_DIR |= LED_BIT; LED_PORT_OUT &= ~LED_BIT; }
void Led_Set(uint8_t on) { if (on != 0u) LED_PORT_OUT |= LED_BIT; else LED_PORT_OUT &= ~LED_BIT; }
void Led_Toggle(void) { LED_PORT_OUT ^= LED_BIT; }
