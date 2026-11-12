#ifndef KEY_H
#define KEY_H
#include "app_types.h"
void Key_Init(void);
/* Called only by the timer ISR, every KEY_SCAN_PERIOD_MS. Never draws the LCD. */
void Key_Scan(uint32_t now);
KeyEvent Key_GetEvent(void);
#endif
