#ifndef SAVE_H
#define SAVE_H
#include <stdint.h>
#include "app_config.h"
#define SAVE_MAGIC 0x5341u
#define SAVE_VERSION 1u
#define SAVE_RESULT_NONE 0u
#define SAVE_RESULT_BEST 1u
#define SAVE_RESULT_UNLOCK 2u
typedef struct {
    uint8_t unlocked; /* number of unlocked levels, 1..MAZE_LEVEL_COUNT */
    uint16_t best_steps[MAZE_LEVEL_COUNT];
    uint32_t best_ms[MAZE_LEVEL_COUNT];
} SaveData;
void Save_Defaults(SaveData *data);
uint8_t Save_Load(SaveData *data);
uint8_t Save_Store(const SaveData *data);
uint8_t Save_ApplyResult(SaveData *data, uint8_t level, uint32_t ms, uint16_t steps);
#endif
