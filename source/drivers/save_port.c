/*******************************************************************************
 *
 *  Info-flash persistence for the GravityMaze progress record.
 *
 *  MSP430F5529 notes (SLAU208 Rev. Q):
 *  - The F5xx flash controller has no FCTL2 (the TI device header comments it
 *    out and SLAU208 says it is "currently not implemented"); flash timing is
 *    generated inside the controller, no divider setup is required.
 *  - FCTL3.LOCKA resets to 1. While LOCKA = 1 all information memory is
 *    protected from segment erase, so it must be toggled to 0 (write 1 to the
 *    bit) around an INFO C/D erase and restored afterwards.
 *  - FCTL4.LOCKINFO also protects information memory; clear it for the
 *    operation and set it again afterwards (SLAU208 7.2.1).
 *  - INFO A holds the factory TLV data, so only INFO B (0x1900, sensitivity
 *    tuning), INFO C (0x1880) and INFO D (0x1800, progress copies) are used.
 *    Every erase/write is bounded so a broken flash cannot hang the main loop.
 *
 ******************************************************************************/

#include <msp430.h>
#include "save_port.h"

/* Bit values from the MSP430F5529 device header. The fallbacks only exist so
   an older header pack missing these two newer bit names still compiles; the
   numeric positions come from SLAU208 (FCTL3.LOCKA=bit6, FCTL4.LOCKINFO=bit7). */
#ifndef LOCKA
#define LOCKA 0x0040u
#endif
#ifndef LOCKINFO
#define LOCKINFO 0x0080u
#endif

#define INFO_D_BASE 0x1800u
#define INFO_C_BASE 0x1880u
#define INFO_B_BASE 0x1900u
#define FLASH_WAIT_LIMIT 50000u

static uint16_t FlashBase(uint8_t slot)
{
    if (slot == 2u) return INFO_B_BASE;
    return (uint16_t)(slot != 0u ? INFO_C_BASE : INFO_D_BASE);
}
static uint8_t WaitBusy(void)
{
    uint16_t limit = FLASH_WAIT_LIMIT;
    while ((FCTL3 & BUSY) != 0u && limit != 0u) limit--;
    return (uint8_t)(limit != 0u);
}
/* Unlock LOCK, LOCKINFO and (if set) LOCKA. Returns whether LOCKA was set. */
static uint8_t UnlockInfo(void)
{
    uint8_t was_locked;
    FCTL3 = FWKEY;                    /* clear LOCK */
    FCTL4 = FWKEY;                    /* clear LOCKINFO */
    was_locked = (uint8_t)((FCTL3 & LOCKA) != 0u);
    if (was_locked) FCTL3 = FWKEY | LOCKA; /* write 1 toggles LOCKA to 0 */
    return was_locked;
}
static void LockInfo(uint8_t was_locked)
{
    if (was_locked) FCTL3 = FWKEY | LOCKA; /* toggle LOCKA back to 1 */
    FCTL3 = FWKEY | LOCK;
    FCTL4 = FWKEY | LOCKINFO;              /* re-protect information memory */
}
static uint8_t EraseSegment(uint8_t slot)
{
    uint16_t base = FlashBase(slot);
    uint16_t sr;
    uint8_t ok, was_locked;
    if (!WaitBusy()) return 0u;
    sr = __get_SR_register();
    if ((sr & GIE) != 0u) __bic_SR_register(GIE);
    was_locked = UnlockInfo();
    FCTL1 = FWKEY | ERASE;            /* segment erase */
    *((volatile uint16_t *)base) = 0u;
    ok = WaitBusy();
    FCTL1 = FWKEY;
    LockInfo(was_locked);
    if ((sr & GIE) != 0u) __bis_SR_register(GIE);
    return ok;
}
static uint8_t WriteWords(uint16_t address, const uint8_t *buffer, uint8_t length)
{
    uint16_t i;
    uint16_t sr;
    uint8_t ok = 1u, was_locked;
    if (!WaitBusy()) return 0u;
    sr = __get_SR_register();
    if ((sr & GIE) != 0u) __bic_SR_register(GIE);
    was_locked = UnlockInfo();
    FCTL1 = FWKEY | WRT;              /* byte/word write mode */
    for (i = 0u; (uint16_t)(i + 1u) < (uint16_t)length; i = (uint16_t)(i + 2u))
    {
        uint16_t word = (uint16_t)buffer[i] | ((uint16_t)buffer[i + 1u] << 8);
        *((volatile uint16_t *)(address + i)) = word;
        if (!WaitBusy()) { ok = 0u; break; }
    }
    FCTL1 = FWKEY;
    LockInfo(was_locked);
    if ((sr & GIE) != 0u) __bis_SR_register(GIE);
    return ok;
}
uint8_t SavePort_Read(uint8_t slot, uint8_t offset, uint8_t *buffer, uint8_t length)
{
    uint16_t base;
    uint16_t i;
    if (buffer == 0 || slot >= SAVE_SLOT_COUNT) return 0u;
    if ((uint16_t)offset + (uint16_t)length > (uint16_t)SAVE_SLOT_SIZE) return 0u;
    base = FlashBase(slot);
    for (i = 0u; i < (uint16_t)length; i++)
        buffer[i] = *((volatile uint8_t *)(base + (uint16_t)offset + i));
    return 1u;
}
uint8_t SavePort_Erase(uint8_t slot)
{
    if (slot >= SAVE_SLOT_COUNT) return 0u;
    return EraseSegment(slot);
}
uint8_t SavePort_Write(uint8_t slot, uint8_t offset, const uint8_t *buffer, uint8_t length)
{
    uint16_t address;
    if (buffer == 0 || slot >= SAVE_SLOT_COUNT) return 0u;
    if ((uint16_t)offset + (uint16_t)length > (uint16_t)SAVE_SLOT_SIZE) return 0u;
    if ((length & 1u) != 0u) return 0u;
    address = (uint16_t)(FlashBase(slot) + (uint16_t)offset);
    return WriteWords(address, buffer, length);
}
