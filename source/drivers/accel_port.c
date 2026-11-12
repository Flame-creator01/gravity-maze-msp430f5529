/*******************************************************************************
 *
 *  Adapted from TI HAL_Cma3000.c for the GravityMaze accelerometer driver
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
#include <string.h>
#include "board_config.h"
#include "board.h"
#include "accel.h"
#include "accel_port.h"

static uint8_t type;
static uint8_t bus_ok;
static AccelDiagnostics diagnostics;

static void BeginTransaction(void)
{
    /* About 50 us at the configured 1.048576 MHz MCLK. */
    __delay_cycles(53);
    (void)UCA0RXBUF;
    P3OUT &= ~ACCEL_CS;
    __delay_cycles(3);
}
static void EndTransaction(void)
{
    uint16_t limit = SPI_WAIT_LIMIT;
    while ((UCA0STAT & UCBUSY) != 0u && limit != 0u) limit--;
    if (limit == 0u) bus_ok = 0u;
    P3OUT |= ACCEL_CS;
}

static void SpiMode(uint8_t lis_mode)
{
    UCA0CTL1 = UCSWRST | UCSSEL_2;
    UCA0CTL0 = UCMST | UCSYNC | UCMSB;
    if (lis_mode != 0u) UCA0CTL0 |= UCCKPL; /* SPI mode 3 */
    else UCA0CTL0 |= UCCKPH; /* SPI mode 0, TI CMA3000 HAL */
    UCA0BR0 = 8; UCA0BR1 = 0; UCA0MCTL = 0;
    UCA0CTL1 &= ~UCSWRST;
    (void)UCA0RXBUF;
    UCA0IFG &= ~UCRXIFG;
}
static uint8_t Transfer(uint8_t value)
{
    uint16_t limit;
    if (!bus_ok) return 0u;
    limit = SPI_WAIT_LIMIT;
    while ((UCA0IFG & UCTXIFG) == 0u && limit != 0u) limit--;
    if (limit == 0u) { bus_ok = 0u; return 0u; }
    UCA0TXBUF = value;
    limit = SPI_WAIT_LIMIT;
    while ((UCA0IFG & UCRXIFG) == 0u && limit != 0u) limit--;
    if (limit == 0u) { bus_ok = 0u; return 0u; }
    return UCA0RXBUF;
}
static uint8_t ReadRegister(uint8_t command)
{
    uint8_t result;
    BeginTransaction();
    (void)Transfer(command);
    result = Transfer(0u);
    EndTransaction();
    return result;
}
static void WriteRegister(uint8_t command, uint8_t value)
{
    BeginTransaction();
    (void)Transfer(command); (void)Transfer(value);
    EndTransaction();
}

uint8_t Accel_Port_Init(void)
{
    uint8_t value;
    type = ACCEL_TYPE_NONE; bus_ok = 1u;
    memset(&diagnostics, 0, sizeof(diagnostics));
    P3SEL &= ~(ACCEL_CS | ACCEL_PWR);
    P3OUT |= ACCEL_CS | ACCEL_PWR;
    P3DIR |= ACCEL_CS | ACCEL_PWR | ACCEL_MOSI;
    P3DIR &= ~ACCEL_MISO;
    P3SEL |= ACCEL_MOSI | ACCEL_MISO;
    P2SEL |= ACCEL_CLK; P2DIR |= ACCEL_CLK;
    P2SEL &= ~ACCEL_INT; P2DIR &= ~ACCEL_INT;
    P2IE &= ~ACCEL_INT;
    Board_DelayMs(30u);

    if (ACCEL_FORCE_TYPE != ACCEL_TYPE_CMA3000)
    {
        SpiMode(1u);
        value = ReadRegister(0x8Fu); /* LIS3DH WHO_AM_I = 0x33 */
        diagnostics.id = value;
        if (value == 0x33u && ReadRegister(0x8Fu) == 0x33u && bus_ok)
        {
            WriteRegister(0x20u, 0x47u); /* 50 Hz, XYZ enabled */
            WriteRegister(0x23u, 0x88u); /* BDU, HR, +/-2g */
            Board_DelayMs(30u);
            diagnostics.ctrl = ReadRegister(0xA0u);
            diagnostics.ctrl4 = ReadRegister(0xA3u);
            if (diagnostics.ctrl == 0x47u && diagnostics.ctrl4 == 0x88u && bus_ok)
                type = ACCEL_TYPE_LIS3DH;
        }
    }
    if (type == ACCEL_TYPE_NONE && ACCEL_FORCE_TYPE != ACCEL_TYPE_LIS3DH)
    {
        bus_ok = 1u;
        SpiMode(0u);
        /* CMA3000 command: register << 2, write flag in bit1. */
        /* RSTR protocol values cross-checked with Linux cma3000_d0x;
         * see docs/sources.md. Register facts only, no Linux code copied. */
        WriteRegister(0x12u, 0x02u);
        WriteRegister(0x12u, 0x0Au);
        WriteRegister(0x12u, 0x04u);
        Board_DelayMs(30u);
        diagnostics.id = ReadRegister(0x04u); /* REVID, register 0x01. */
        diagnostics.status = ReadRegister(0x0Cu); /* STATUS, register 0x03. */
        diagnostics.ctrl4 = 0u;
        WriteRegister((uint8_t)((0x02u << 2) | 2u), 0x94u);
        Board_DelayMs(30u);
        value = ReadRegister((uint8_t)(0x02u << 2));
        diagnostics.ctrl = value;
        if (value == 0x94u && (diagnostics.status & 0x01u) == 0u && bus_ok)
            type = ACCEL_TYPE_CMA3000;
    }
    diagnostics.bus_ok = bus_ok;
    return type;
}

uint8_t Accel_Port_Read(AccelData *data)
{
    uint8_t values[6];
    uint8_t i;
    int16_t raw;
    int16_t axes[3];
    diagnostics.sample_valid = 0u;
    if (data == 0 || type == ACCEL_TYPE_NONE) return ACCEL_READ_ERROR;
    bus_ok = 1u;
    if (type == ACCEL_TYPE_CMA3000)
    {
        diagnostics.ctrl = ReadRegister(0x08u);
        diagnostics.status = ReadRegister(0x0Cu);
        if (diagnostics.ctrl != 0x94u || (diagnostics.status & 0x01u) != 0u || !bus_ok)
        { diagnostics.bus_ok = bus_ok; return ACCEL_READ_ERROR; }
        /* CMA3000 +/-2g nominal sensitivity: 18 mg per signed LSB. */
        for (i = 0u; i < 3u; i++)
        {
            values[i] = ReadRegister((uint8_t)((0x06u + i) << 2));
            raw = values[i] < 128u ? (int16_t)values[i] : (int16_t)values[i] - 256;
            if (i == 0u) diagnostics.raw_counts.x = raw;
            else if (i == 1u) diagnostics.raw_counts.y = raw;
            else diagnostics.raw_counts.z = raw;
            axes[i] = (int16_t)(raw * 18);
        }
    }
    else
    {
        diagnostics.id = ReadRegister(0x8Fu);
        diagnostics.ctrl = ReadRegister(0xA0u);
        diagnostics.ctrl4 = ReadRegister(0xA3u);
        diagnostics.status = ReadRegister(0xA7u);
        diagnostics.bus_ok = bus_ok;
        if (diagnostics.id != 0x33u || diagnostics.ctrl != 0x47u ||
            diagnostics.ctrl4 != 0x88u || !bus_ok) return ACCEL_READ_ERROR;
        if ((diagnostics.status & 0x08u) == 0u) return ACCEL_READ_WAIT;
        BeginTransaction();
        (void)Transfer(0xE8u); /* read, auto-increment, OUT_X_L */
        for (i = 0u; i < 6u; i++) values[i] = Transfer(0u);
        EndTransaction();
        for (i = 0u; i < 3u; i++)
        {
            raw = (int16_t)((uint16_t)values[i * 2u] | ((uint16_t)values[i * 2u + 1u] << 8));
            axes[i] = (int16_t)(raw / 16); /* HR 12 bit, 1 mg per LSB */
            if (i == 0u) diagnostics.raw_counts.x = axes[i];
            else if (i == 1u) diagnostics.raw_counts.y = axes[i];
            else diagnostics.raw_counts.z = axes[i];
        }
    }
    diagnostics.bus_ok = bus_ok;
    if (!bus_ok) return ACCEL_READ_ERROR;
    data->x = axes[0]; data->y = axes[1]; data->z = axes[2];
    diagnostics.sample_valid = 1u;
    return ACCEL_READ_OK;
}
const AccelDiagnostics *Accel_Port_GetDiagnostics(void) { return &diagnostics; }
