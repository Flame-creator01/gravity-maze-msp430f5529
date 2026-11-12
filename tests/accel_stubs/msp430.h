#ifndef HOST_ACCEL_MSP430_H
#define HOST_ACCEL_MSP430_H
#include "../stubs/msp430.h"
#define UCSWRST 0x01u
#define UCSSEL_2 0x80u
#define UCMST 0x08u
#define UCSYNC 0x01u
#define UCMSB 0x20u
#define UCCKPL 0x40u
#define UCCKPH 0x80u
#define UCTXIFG 0x02u
#define UCRXIFG 0x01u
#define UCBUSY 0x01u
extern volatile uint8_t P2SEL, P2IE, P3SEL, P3OUT, P3DIR;
extern volatile uint8_t UCA0CTL0, UCA0CTL1, UCA0BR0, UCA0BR1, UCA0MCTL;
/* Wider host mailbox provides a sentinel to detect a newly written byte. */
extern volatile uint16_t host_txbuf;
volatile uint8_t *Host_SpiFlagsRegister(void);
uint8_t Host_SpiReceive(void);
uint8_t Host_SpiStatus(void);
void Host_DelayCycles(uint32_t cycles);
#define UCA0TXBUF host_txbuf
#define UCA0RXBUF Host_SpiReceive()
#define UCA0IFG (*Host_SpiFlagsRegister())
extern volatile uint8_t host_spi_ifg;
#define UCA0STAT Host_SpiStatus()
#define __delay_cycles(cycles) Host_DelayCycles(cycles)
#endif
