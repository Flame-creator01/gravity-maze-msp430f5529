#include <string.h>
#include "save.h"
#include "save_port.h"

/* Record layout (little-endian):
   0..1 magic, 2..3 sequence, 4 version, 5 unlocked,
   6.. steps[], then ms[], then a 16-bit checksum. */
#define REC_MAGIC 0u
#define REC_SEQ 2u
#define REC_VERSION 4u
#define REC_UNLOCK 5u
#define REC_STEPS 6u
#define REC_MS (REC_STEPS + 2u * MAZE_LEVEL_COUNT)
#define REC_CHECK (REC_MS + 4u * MAZE_LEVEL_COUNT)
#define REC_SIZE (REC_CHECK + 2u)
#define CHECK_SEED 0xA5C3u
/* Progress lives in slots 0/1 (INFO D/C); slot 2 (INFO B) holds tuning. */
#define PROGRESS_SLOTS 2u

#if (REC_SIZE > SAVE_SLOT_SIZE)
#error "save record does not fit the info-flash slot"
#endif

static uint8_t active_slot;
static uint16_t active_seq;
static uint8_t active_valid;

static void Put16(uint8_t *p, uint16_t value)
{
    p[0] = (uint8_t)(value & 255u);
    p[1] = (uint8_t)(value >> 8);
}
static uint16_t Get16(const uint8_t *p)
{
    return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}
static void Put32(uint8_t *p, uint32_t value)
{
    p[0] = (uint8_t)(value & 255u);
    p[1] = (uint8_t)((value >> 8) & 255u);
    p[2] = (uint8_t)((value >> 16) & 255u);
    p[3] = (uint8_t)((value >> 24) & 255u);
}
static uint32_t Get32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}
static uint16_t Checksum(const uint8_t *p)
{
    uint16_t sum = CHECK_SEED;
    uint8_t i;
    for (i = 0u; i < (uint8_t)REC_CHECK; i++) sum = (uint16_t)(sum + p[i]);
    return sum;
}
void Save_Defaults(SaveData *data)
{
    uint8_t i;
    if (data == 0) return;
    data->unlocked = 1u;
    for (i = 0u; i < MAZE_LEVEL_COUNT; i++)
    {
        data->best_steps[i] = 0u;
        data->best_ms[i] = 0UL;
    }
}
static void Pack(const SaveData *data, uint16_t seq, uint8_t *buf)
{
    uint8_t i;
    memset(buf, 0, SAVE_SLOT_SIZE);
    Put16(buf + REC_MAGIC, SAVE_MAGIC);
    Put16(buf + REC_SEQ, seq);
    buf[REC_VERSION] = SAVE_VERSION;
    buf[REC_UNLOCK] = data->unlocked;
    for (i = 0u; i < MAZE_LEVEL_COUNT; i++)
    {
        Put16(buf + REC_STEPS + 2u * i, data->best_steps[i]);
        Put32(buf + REC_MS + 4u * i, data->best_ms[i]);
    }
    Put16(buf + REC_CHECK, Checksum(buf));
}
static uint8_t Unpack(const uint8_t *buf, SaveData *data)
{
    uint8_t i;
    if (Get16(buf + REC_MAGIC) != SAVE_MAGIC) return 0u;
    if (buf[REC_VERSION] != SAVE_VERSION) return 0u;
    if (Get16(buf + REC_CHECK) != Checksum(buf)) return 0u;
    data->unlocked = buf[REC_UNLOCK];
    for (i = 0u; i < MAZE_LEVEL_COUNT; i++)
    {
        data->best_steps[i] = Get16(buf + REC_STEPS + 2u * i);
        data->best_ms[i] = Get32(buf + REC_MS + 4u * i);
    }
    return 1u;
}
/* (int16_t) difference is positive when a is newer than b, also across wrap. */
static uint8_t SeqNewer(uint16_t a, uint16_t b)
{
    return (uint8_t)((int16_t)(a - b) > 0);
}
uint8_t Save_Load(SaveData *data)
{
    uint8_t buf[SAVE_SLOT_SIZE];
    SaveData slot_data[PROGRESS_SLOTS];
    uint16_t slot_seq[PROGRESS_SLOTS];
    uint8_t slot_ok[PROGRESS_SLOTS];
    uint8_t i, chosen;
    if (data == 0) return 0u;
    Save_Defaults(data);
    active_valid = 0u; active_slot = 0u; active_seq = 0u;
    for (i = 0u; i < PROGRESS_SLOTS; i++)
    {
        slot_ok[i] = 0u; slot_seq[i] = 0u;
        Save_Defaults(&slot_data[i]);
        if (SavePort_Read(i, 0u, buf, (uint8_t)REC_SIZE))
        {
            if (Unpack(buf, &slot_data[i]))
            {
                slot_ok[i] = 1u;
                slot_seq[i] = Get16(buf + REC_SEQ);
            }
        }
    }
    chosen = 0u;
    if (slot_ok[0] != 0u && slot_ok[1] != 0u)
        chosen = (uint8_t)(SeqNewer(slot_seq[1], slot_seq[0]) ? 1u : 0u);
    else if (slot_ok[1] != 0u) chosen = 1u;
    else if (slot_ok[0] == 0u) return 0u;
    *data = slot_data[chosen];
    if (data->unlocked < 1u) data->unlocked = 1u;
    if (data->unlocked > (uint8_t)MAZE_LEVEL_COUNT) data->unlocked = (uint8_t)MAZE_LEVEL_COUNT;
    active_valid = 1u;
    active_slot = chosen;
    active_seq = slot_seq[chosen];
    return 1u;
}
uint8_t Save_Store(const SaveData *data)
{
    uint8_t buf[SAVE_SLOT_SIZE];
    uint8_t slot;
    uint16_t seq;
    if (data == 0) return 0u;
    slot = (uint8_t)(active_valid ? (uint8_t)(active_slot ^ 1u) : 0u);
    seq = (uint16_t)(active_valid ? (uint16_t)(active_seq + 1u) : 1u);
    Pack(data, seq, buf);
    if (!SavePort_Erase(slot)) return 0u;
    if (!SavePort_Write(slot, 0u, buf, (uint8_t)REC_SIZE)) return 0u;
    active_slot = slot; active_seq = seq; active_valid = 1u;
    return 1u;
}
uint8_t Save_ApplyResult(SaveData *data, uint8_t level, uint32_t ms, uint16_t steps)
{
    uint8_t flags = SAVE_RESULT_NONE;
    if (data == 0 || level >= MAZE_LEVEL_COUNT) return SAVE_RESULT_NONE;
    if (data->best_ms[level] == 0UL || ms < data->best_ms[level])
    {
        data->best_ms[level] = ms;
        data->best_steps[level] = steps;
        flags |= SAVE_RESULT_BEST;
    }
    else if (ms == data->best_ms[level] && steps < data->best_steps[level])
    {
        data->best_steps[level] = steps;
        flags |= SAVE_RESULT_BEST;
    }
    if (data->unlocked < (uint8_t)(level + 2u) &&
        (uint8_t)(level + 1u) < (uint8_t)MAZE_LEVEL_COUNT)
    {
        data->unlocked = (uint8_t)(level + 2u);
        flags |= SAVE_RESULT_UNLOCK;
    }
    return flags;
}
