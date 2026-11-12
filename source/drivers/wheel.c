/*******************************************************************************
 *
 *  Analog thumb wheel driver (MSP430F5529 experimenter board / SEEED clone).
 *
 *  The wheel is a potentiometer, not a quadrature encoder: P8.0 high enables
 *  it and the wiper is ADC12 input A5 on P6.5 (see TI HAL_Wheel.c and SLAU330).
 *  The absolute ADC value is converted to relative "ticks": every
 *  WHEEL_STEP_ADC counts of movement produces one tick in Wheel_Poll().
 *  There is no absolute position dependency, so no wheel calibration is needed.
 *
 ******************************************************************************/

#include <msp430.h>
#include "app_config.h"
#include "wheel.h"

#define WHEEL_ENABLE BIT0  /* P8.0: high enables the wheel */
#define WHEEL_ADC BIT5     /* P6.5: ADC12 input channel 5 */

static uint16_t wheel_ref;
static uint8_t wheel_ready;

static uint16_t ReadRaw(void)
{
    uint16_t limit = WHEEL_WAIT_LIMIT;
    ADC12IFG &= (uint16_t)~BIT0;
    ADC12CTL0 |= ADC12SC;
    while ((ADC12IFG & BIT0) == 0u && limit != 0u) limit--;
    if (limit == 0u) return wheel_ref; /* keep the last value on a stuck ADC */
    return (uint16_t)(ADC12MEM0 & 0x0FFFu);
}
void Wheel_Init(void)
{
    P8DIR |= WHEEL_ENABLE;
    P8OUT |= WHEEL_ENABLE;             /* enable the wheel */
    P6SEL |= WHEEL_ADC;                /* P6.5 as ADC input */
    ADC12CTL0 = ADC12SHT02 | ADC12ON;  /* 16 ADC12CLK sample time, ADC on */
    ADC12CTL1 = ADC12SHP;              /* sampling timer, ADC12OSC default */
    ADC12MCTL0 = ADC12INCH_5;          /* A5 = wheel wiper */
    ADC12CTL0 |= ADC12ENC;
    wheel_ref = ReadRaw();
    wheel_ready = 1u;
}
int8_t Wheel_Poll(void)
{
    int32_t diff, steps;
    int8_t out;
    if (wheel_ready == 0u) return 0;
    diff = (int32_t)ReadRaw() - (int32_t)wheel_ref;
    steps = diff / (int32_t)WHEEL_STEP_ADC;
    if (steps > 6L) steps = 6L;
    if (steps < -6L) steps = -6L;
    if (steps == 0L) return 0;
    wheel_ref = (uint16_t)((int32_t)wheel_ref + steps * (int32_t)WHEEL_STEP_ADC);
    out = (int8_t)steps;
#if WHEEL_INVERT
    out = (int8_t)(-out);
#endif
    return out;
}
