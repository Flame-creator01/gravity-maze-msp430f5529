/*******************************************************************************
 *
 *  HAL_Dogs102x6.c - Driver for the DOGS 102x6 display
 *
 *  Copyright (C) 2010 Texas Instruments Incorporated - http://www.ti.com/
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *    Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 *    Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the
 *    distribution.
 *
 *    Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 ******************************************************************************/
/* Adapted interface and bounded polling for GravityMaze. */

#include <msp430.h>
#include "board_config.h"
#include "app_config.h"
#include "board.h"
#include "lcd_port.h"
static uint8_t bus_ok;
static void Send(uint8_t value)
{
    uint16_t n = SPI_WAIT_LIMIT;
    while (!(UCB1IFG & UCTXIFG) && n != 0u) n--;
    if (n == 0u) { bus_ok = 0u; return; }
    UCB1TXBUF = value;
    n = SPI_WAIT_LIMIT;
    while (!(UCB1IFG & UCRXIFG) && n != 0u) n--;
    if (n == 0u) { bus_ok = 0u; return; }
    (void)UCB1RXBUF;
}
uint8_t LCD_Port_Init(void)
{
    uint8_t i;
    static const uint8_t init[] = {
        0x40,0xA1,0xC8,0xA4,0xA6,0xA2,0x2F,0x27,0x81,
        LCD_CONTRAST,0xFA,0x90,0xAF
    };
    bus_ok = 1u;
    P5SEL &= ~(LCD_RESET | LCD_DC);
    P7SEL &= ~(LCD_CS | LCD_BACKLIGHT);
    P5OUT &= ~(LCD_RESET | LCD_DC); P5DIR |= LCD_RESET | LCD_DC;
    P7OUT |= LCD_CS | LCD_BACKLIGHT; P7DIR |= LCD_CS | LCD_BACKLIGHT;
    Board_DelayMs(5u); P5OUT |= LCD_RESET; Board_DelayMs(5u);
    P4DIR |= LCD_MOSI | LCD_CLK;
    P4SEL |= LCD_MOSI | LCD_CLK;
    UCB1CTL1 = UCSWRST | UCSSEL_2;
    UCB1CTL0 = UCCKPH | UCMSB | UCMST | UCSYNC;
    UCB1BR0 = 2; UCB1BR1 = 0;
    UCB1CTL1 &= ~UCSWRST;
    (void)UCB1RXBUF;
    UCB1IFG &= ~UCRXIFG;
    P7OUT &= ~LCD_CS;
    for (i = 0u; i < sizeof(init); i++) Send(init[i]);
    P7OUT |= LCD_CS;
    return bus_ok;
}
uint8_t LCD_Port_Flush(const uint8_t *buffer, uint16_t size)
{
    uint8_t page, column;
    if (buffer == 0 || size != 816u) return 0u;
    bus_ok = 1u;
    for (page = 0u; page < 8u; page++)
    {
        P7OUT &= ~LCD_CS; P5OUT &= ~LCD_DC;
        Send((uint8_t)(0xB0u + 7u - page));
        Send(0x00u); Send(0x10u);
        P5OUT |= LCD_DC;
        for (column = 0u; column < 102u; column++)
            Send(buffer[(uint16_t)page * 102u + column]);
        P7OUT |= LCD_CS;
        if (!bus_ok) return 0u;
    }
    return 1u;
}
