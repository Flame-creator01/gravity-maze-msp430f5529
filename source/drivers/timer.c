#include <msp430.h>
#include "board_config.h"
#include "app_config.h"
#include "key.h"
#include "timer.h"
static volatile uint32_t milliseconds;
static uint8_t key_divider;
void Timer_Init(void)
{
    milliseconds = 0UL;
    key_divider = 0u;
    TA0CCR0 = (uint16_t)((BOARD_SMCLK_HZ + 500UL) / 1000UL - 1UL);
    TA0CCTL0 = CCIE;
    TA0CTL = TASSEL__SMCLK | MC__UP | TACLR;
}
uint32_t Timer_GetMilliseconds(void)
{
    uint16_t gie;
    uint32_t value;
    gie = __get_SR_register() & GIE;
    __disable_interrupt();
    value = milliseconds;
    if (gie != 0u) __enable_interrupt();
    return value;
}
uint8_t Timer_Elapsed(uint32_t now, uint32_t *last, uint16_t period)
{
    if ((uint32_t)(now - *last) < period) return 0u;
    *last = now;
    return 1u;
}
#pragma vector=TIMER0_A0_VECTOR
__interrupt void Timer_A0_ISR(void)
{
    milliseconds++;
    key_divider++;
    if (key_divider >= KEY_SCAN_PERIOD_MS)
    {
        key_divider = 0u;
        Key_Scan(milliseconds);
    }
}
