#include <msp430.h>
#include "app.h"
#include "board.h"
#include "timer.h"

int main(void)
{
    uint32_t now;
    WDTCTL = WDTPW | WDTHOLD;
    Board_Init();
    Timer_Init();
    App_Init();
    __bis_SR_register(GIE);
    while (1)
    {
        now = Timer_GetMilliseconds();
        App_RunOnce(now);
    }
}
