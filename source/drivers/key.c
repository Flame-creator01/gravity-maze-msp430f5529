#include "board_config.h"
#include "app_config.h"
#include "key_filter.h"
#include "key.h"
#define KEY_QUEUE_SIZE 8u
static KeyFilter filters[2];
/* Single producer (timer ISR), single consumer (main loop). */
static volatile uint8_t events[KEY_QUEUE_SIZE];
static volatile uint8_t read_index, write_index;
static void QueueEvent(KeyEvent event)
{
    uint8_t next = (uint8_t)((write_index + 1u) % KEY_QUEUE_SIZE);
    if (next == read_index) return; /* Full: retain events already waiting. */
    events[write_index] = (uint8_t)event;
    write_index = next;
}
void Key_Init(void)
{
    KEY1_PORT_DIR &= ~KEY1_BIT; KEY1_PORT_REN |= KEY1_BIT; KEY1_PORT_OUT |= KEY1_BIT;
    KEY2_PORT_DIR &= ~KEY2_BIT; KEY2_PORT_REN |= KEY2_BIT; KEY2_PORT_OUT |= KEY2_BIT;
    KeyFilter_Init(&filters[0]); KeyFilter_Init(&filters[1]);
    read_index = 0u; write_index = 0u;
}
void Key_Scan(uint32_t now)
{
    uint8_t a, b;
    a = KeyFilter_Update(&filters[0], (KEY1_PORT_IN & KEY1_BIT) == 0u, now);
    b = KeyFilter_Update(&filters[1], (KEY2_PORT_IN & KEY2_BIT) == 0u, now);
    if (a != 0u) QueueEvent(a == 1u ? KEY_EVENT_1_PRESS : KEY_EVENT_1_LONG);
    if (b != 0u) QueueEvent(b == 1u ? KEY_EVENT_2_PRESS : KEY_EVENT_2_LONG);
}
KeyEvent Key_GetEvent(void)
{
    uint16_t gie = __get_SR_register() & GIE;
    KeyEvent event = KEY_EVENT_NONE;
    __disable_interrupt();
    if (read_index != write_index)
    {
        event = (KeyEvent)events[read_index];
        read_index = (uint8_t)((read_index + 1u) % KEY_QUEUE_SIZE);
    }
    if (gie != 0u) __enable_interrupt();
    return event;
}
