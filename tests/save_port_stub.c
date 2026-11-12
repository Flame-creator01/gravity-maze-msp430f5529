/* Host model of the two info-flash slots. Only used by the host tests. */
#include <string.h>
#include "save_port.h"

static uint8_t slots[SAVE_SLOT_COUNT][SAVE_SLOT_SIZE];
static uint8_t erase_failure;

void HostSave_Reset(void)
{
    memset(slots, 0xFF, sizeof(slots));
    erase_failure = 0u;
}
void HostSave_FailErase(uint8_t fail) { erase_failure = fail; }
void HostSave_Corrupt(uint8_t slot, uint8_t offset)
{
    if (slot < SAVE_SLOT_COUNT && offset < SAVE_SLOT_SIZE) slots[slot][offset] ^= 0xFFu;
}
uint8_t HostSave_Byte(uint8_t slot, uint8_t offset)
{
    if (slot >= SAVE_SLOT_COUNT || offset >= SAVE_SLOT_SIZE) return 0u;
    return slots[slot][offset];
}
uint8_t SavePort_Read(uint8_t slot, uint8_t offset, uint8_t *buffer, uint8_t length)
{
    if (buffer == 0 || slot >= SAVE_SLOT_COUNT) return 0u;
    if ((uint16_t)offset + (uint16_t)length > (uint16_t)SAVE_SLOT_SIZE) return 0u;
    memcpy(buffer, &slots[slot][offset], length);
    return 1u;
}
uint8_t SavePort_Erase(uint8_t slot)
{
    if (slot >= SAVE_SLOT_COUNT || erase_failure) return 0u;
    memset(slots[slot], 0xFF, SAVE_SLOT_SIZE);
    return 1u;
}
uint8_t SavePort_Write(uint8_t slot, uint8_t offset, const uint8_t *buffer, uint8_t length)
{
    if (buffer == 0 || slot >= SAVE_SLOT_COUNT) return 0u;
    if ((uint16_t)offset + (uint16_t)length > (uint16_t)SAVE_SLOT_SIZE) return 0u;
    memcpy(&slots[slot][offset], buffer, length);
    return 1u;
}
