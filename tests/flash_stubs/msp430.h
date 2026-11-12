/* Host-only flash-controller register stubs so source/drivers/save_port.c can be
   compiled (syntax/C89 check) with a normal C compiler. Names/values mirror the
   MSP430F5529 device header, where FCTL2 is not implemented and is commented
   out; keep it absent here so accidental FCTL2 use is caught by the host check.
   Never used by CCS. */
#ifndef MSP430_H
#define MSP430_H
#include <stdint.h>

#define FWKEY 0xA500u
#define ERASE 0x0002u      /* FCTL1 */
#define WRT 0x0040u        /* FCTL1 */
#define BUSY 0x0001u       /* FCTL3 */
#define LOCK 0x0010u       /* FCTL3 */
#define LOCKA 0x0040u      /* FCTL3 */
#define LOCKINFO 0x0080u   /* FCTL4 */
#define GIE 0x0008u

extern volatile uint16_t FCTL1, FCTL3, FCTL4;
uint16_t __get_SR_register(void);
void __bic_SR_register(uint16_t mask);
void __bis_SR_register(uint16_t mask);
#endif
