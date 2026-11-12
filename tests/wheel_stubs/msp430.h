/* Host-only ADC/port stubs so source/drivers/wheel.c can be compiled
   (syntax/C89 check) with a normal C compiler. Never used by CCS. */
#ifndef MSP430_H
#define MSP430_H
#include <stdint.h>

#define BIT0 0x01u
#define BIT5 0x20u

#define ADC12ON 0x0010u
#define ADC12SHT02 0x0400u
#define ADC12SHP 0x0200u
#define ADC12INCH_5 0x0005u
#define ADC12ENC 0x0002u
#define ADC12SC 0x0001u

extern volatile uint16_t ADC12CTL0, ADC12CTL1, ADC12IFG, ADC12MEM0;
extern volatile uint8_t ADC12MCTL0;
extern volatile uint8_t P6SEL, P8OUT, P8DIR;
#endif
