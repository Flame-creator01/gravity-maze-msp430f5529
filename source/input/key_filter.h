#ifndef KEY_FILTER_H
#define KEY_FILTER_H
#include <stdint.h>
typedef struct {
    uint32_t candidate_since, down_since;
    uint8_t candidate, stable, long_sent;
} KeyFilter;
/* result: 0 none, 1 short on debounced release, 2 long while held */
void KeyFilter_Init(KeyFilter *key);
uint8_t KeyFilter_Update(KeyFilter *key, uint8_t pressed, uint32_t now);
#endif
