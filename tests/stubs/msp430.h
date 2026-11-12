#ifndef HOST_KEY_MSP430_H
#define HOST_KEY_MSP430_H
/* Only for the host key-driver test; never use as a target device header. */
#include <stdint.h>
#define BIT0 0x01u
#define BIT1 0x02u
#define BIT2 0x04u
#define BIT3 0x08u
#define BIT4 0x10u
#define BIT5 0x20u
#define BIT6 0x40u
#define BIT7 0x80u
#define GIE 0x0008u
extern volatile uint8_t P1IN, P1DIR, P1REN, P1OUT;
extern volatile uint8_t P2IN, P2DIR, P2REN, P2OUT;
uint16_t __get_SR_register(void);
void __disable_interrupt(void);
void __enable_interrupt(void);
#endif
