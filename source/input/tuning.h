#ifndef TUNING_H
#define TUNING_H
#include <stdint.h>
#include "app_config.h"

/* Runtime sensitivity settings. Defaults come from app_config.h; the TUNE
   screen edits g_tuning live and stores it in INFO B (see tuning.c). */
typedef struct {
    uint16_t trigger_mg; /* start moving above this tilt */
    uint16_t release_mg; /* stop below this tilt */
    uint16_t margin_mg;  /* extra tilt needed to switch axes */
    uint16_t slow_ms;    /* step interval at/below trigger */
    uint16_t fast_ms;    /* step interval at/above full */
    uint16_t full_mg;    /* tilt that reaches the fastest interval */
    uint16_t filter_num; /* low-pass weight out of 4: 1..3 */
} TuningParams;

extern TuningParams g_tuning;

#define TUNING_ITEM_COUNT 8u

void Tuning_Defaults(void);
void Tuning_Clamp(void);
uint8_t Tuning_Load(void);
uint8_t Tuning_Save(void);
uint16_t Tuning_Interval(uint16_t tilt_mg);
#endif
