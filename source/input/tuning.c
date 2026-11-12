#include <string.h>
#include "tuning.h"
#include "save_port.h"

/* Tuning record in save-port slot 2 (INFO B). Layout (little-endian):
   0..1 magic, 2 version, 3 reserved, 4..17 seven uint16 parameters,
   18..19 checksum over the first 18 bytes. Progress keeps INFO C/D. */
#define TUNE_MAGIC 0x5455u
#define TUNE_VERSION 2u
#define TUNE_SLOT 2u
#define TUNE_OFF_MAGIC 0u
#define TUNE_OFF_VERSION 2u
#define TUNE_OFF_PARAMS 4u
#define TUNE_OFF_CHECK (TUNE_OFF_PARAMS + 14u)
#define TUNE_REC_SIZE (TUNE_OFF_CHECK + 2u)
#define TUNE_CHECK_SEED 0x3C5Au

#if (TUNE_REC_SIZE > SAVE_SLOT_SIZE)
#error "tuning record does not fit the info-flash slot"
#endif

TuningParams g_tuning = {
    (uint16_t)MOTION_TRIGGER_MG,
    (uint16_t)MOTION_RELEASE_MG,
    (uint16_t)MOTION_AXIS_MARGIN_MG,
    (uint16_t)GAME_MOVE_SLOW_MS,
    (uint16_t)GAME_MOVE_FAST_MS,
    (uint16_t)GAME_TILT_FULL_MG,
    (uint16_t)MOTION_FILTER_WEIGHT
};

static void Put16(uint8_t *p, uint16_t value)
{
    p[0] = (uint8_t)(value & 255u);
    p[1] = (uint8_t)(value >> 8);
}
static uint16_t Get16(const uint8_t *p)
{
    return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}
static uint16_t Checksum(const uint8_t *p)
{
    uint16_t sum = TUNE_CHECK_SEED;
    uint8_t i;
    for (i = 0u; i < (uint8_t)TUNE_OFF_CHECK; i++) sum = (uint16_t)(sum + p[i]);
    return sum;
}
static int32_t ClampInt(int32_t value, int32_t low, int32_t high)
{
    if (value < low) return low;
    if (value > high) return high;
    return value;
}
void Tuning_Defaults(void)
{
    g_tuning.trigger_mg = (uint16_t)MOTION_TRIGGER_MG;
    g_tuning.release_mg = (uint16_t)MOTION_RELEASE_MG;
    g_tuning.margin_mg = (uint16_t)MOTION_AXIS_MARGIN_MG;
    g_tuning.slow_ms = (uint16_t)GAME_MOVE_SLOW_MS;
    g_tuning.fast_ms = (uint16_t)GAME_MOVE_FAST_MS;
    g_tuning.full_mg = (uint16_t)GAME_TILT_FULL_MG;
    g_tuning.filter_num = (uint16_t)MOTION_FILTER_WEIGHT;
}
/* Keep all seven values inside safe ranges and keep the curve consistent:
   release < trigger, fast < slow, full > trigger, filter in 1..3. */
void Tuning_Clamp(void)
{
    g_tuning.trigger_mg = (uint16_t)ClampInt((int32_t)g_tuning.trigger_mg, 50L, 400L);
    g_tuning.release_mg = (uint16_t)ClampInt((int32_t)g_tuning.release_mg, 30L,
                                             (int32_t)g_tuning.trigger_mg - 20L);
    g_tuning.margin_mg = (uint16_t)ClampInt((int32_t)g_tuning.margin_mg, 10L, 120L);
    g_tuning.slow_ms = (uint16_t)ClampInt((int32_t)g_tuning.slow_ms, 120L, 400L);
    g_tuning.fast_ms = (uint16_t)ClampInt((int32_t)g_tuning.fast_ms, 60L,
                                          (int32_t)g_tuning.slow_ms - 20L);
    g_tuning.full_mg = (uint16_t)ClampInt((int32_t)g_tuning.full_mg,
                                          (int32_t)g_tuning.trigger_mg + 50L, 1000L);
    g_tuning.filter_num = (uint16_t)ClampInt((int32_t)g_tuning.filter_num, 1L, 3L);
}
uint16_t Tuning_Interval(uint16_t tilt_mg)
{
    uint32_t span;
    uint32_t range;
    if (tilt_mg <= g_tuning.trigger_mg) return g_tuning.slow_ms;
    if (tilt_mg >= g_tuning.full_mg) return g_tuning.fast_ms;
    span = (uint32_t)(g_tuning.slow_ms - g_tuning.fast_ms);
    range = (uint32_t)(g_tuning.full_mg - g_tuning.trigger_mg);
    return (uint16_t)(g_tuning.slow_ms -
        (span * (uint32_t)(tilt_mg - g_tuning.trigger_mg)) / range);
}
uint8_t Tuning_Load(void)
{
    uint8_t buf[SAVE_SLOT_SIZE];
    Tuning_Defaults();
    if (!SavePort_Read(TUNE_SLOT, 0u, buf, (uint8_t)TUNE_REC_SIZE)) return 0u;
    if (Get16(buf + TUNE_OFF_MAGIC) != TUNE_MAGIC) return 0u;
    if (buf[TUNE_OFF_VERSION] != TUNE_VERSION) return 0u;
    if (Checksum(buf) != Get16(buf + TUNE_OFF_CHECK)) return 0u;
    g_tuning.trigger_mg = Get16(buf + TUNE_OFF_PARAMS + 0u);
    g_tuning.release_mg = Get16(buf + TUNE_OFF_PARAMS + 2u);
    g_tuning.margin_mg = Get16(buf + TUNE_OFF_PARAMS + 4u);
    g_tuning.slow_ms = Get16(buf + TUNE_OFF_PARAMS + 6u);
    g_tuning.fast_ms = Get16(buf + TUNE_OFF_PARAMS + 8u);
    g_tuning.full_mg = Get16(buf + TUNE_OFF_PARAMS + 10u);
    g_tuning.filter_num = Get16(buf + TUNE_OFF_PARAMS + 12u);
    Tuning_Clamp();
    return 1u;
}
uint8_t Tuning_Save(void)
{
    uint8_t buf[SAVE_SLOT_SIZE];
    Tuning_Clamp();
    memset(buf, 0, SAVE_SLOT_SIZE);
    Put16(buf + TUNE_OFF_MAGIC, TUNE_MAGIC);
    buf[TUNE_OFF_VERSION] = TUNE_VERSION;
    Put16(buf + TUNE_OFF_PARAMS + 0u, g_tuning.trigger_mg);
    Put16(buf + TUNE_OFF_PARAMS + 2u, g_tuning.release_mg);
    Put16(buf + TUNE_OFF_PARAMS + 4u, g_tuning.margin_mg);
    Put16(buf + TUNE_OFF_PARAMS + 6u, g_tuning.slow_ms);
    Put16(buf + TUNE_OFF_PARAMS + 8u, g_tuning.fast_ms);
    Put16(buf + TUNE_OFF_PARAMS + 10u, g_tuning.full_mg);
    Put16(buf + TUNE_OFF_PARAMS + 12u, g_tuning.filter_num);
    Put16(buf + TUNE_OFF_CHECK, Checksum(buf));
    if (!SavePort_Erase(TUNE_SLOT)) return 0u;
    if (!SavePort_Write(TUNE_SLOT, 0u, buf, (uint8_t)TUNE_REC_SIZE)) return 0u;
    return 1u;
}
