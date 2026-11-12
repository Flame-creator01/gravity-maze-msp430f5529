#include <msp430.h>
#include "board.h"
#include "key.h"
#include "led.h"
void Board_DelayMs(uint16_t ms)
{
    while (ms != 0u) { __delay_cycles(1049); ms--; }
}
void Board_Init(void)
{
    /* 32 * REFO: DCOCLKDIV/SMCLK/MCLK approximately 1.048576 MHz. */
    UCSCTL6 |= XT1OFF | XT2OFF;
    UCSCTL3 = SELREF__REFOCLK;
    __bis_SR_register(SCG0);
    UCSCTL0 = 0;
    UCSCTL1 = DCORSEL_2;
    UCSCTL2 = FLLD_1 | 31;
    UCSCTL4 = SELA__REFOCLK | SELS__DCOCLKDIV | SELM__DCOCLKDIV;
    __bic_SR_register(SCG0);
    __delay_cycles(32768);
    UCSCTL7 &= ~(XT2OFFG | XT1LFOFFG | DCOFFG);
    SFRIFG1 &= ~OFIFG;
    /* Keep unused SD card deselected on the shared LCD SPI bus. */
    P3OUT |= BIT7; P3DIR |= BIT7; P3SEL &= ~BIT7;
    P1SEL &= ~(BIT0 | BIT7); P2SEL &= ~BIT2;
    Key_Init();
    Led_Init();
}
