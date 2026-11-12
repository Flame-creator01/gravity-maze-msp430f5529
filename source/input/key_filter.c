#include "key_filter.h"
#include "app_config.h"
void KeyFilter_Init(KeyFilter *key)
{
    key->candidate_since = 0UL; key->down_since = 0UL;
    key->candidate = 0u; key->stable = 0u; key->long_sent = 0u;
}
uint8_t KeyFilter_Update(KeyFilter *key, uint8_t pressed, uint32_t now)
{
    if (pressed != key->candidate) { key->candidate = pressed; key->candidate_since = now; }
    if (key->stable != key->candidate && (uint32_t)(now-key->candidate_since) >= KEY_DEBOUNCE_MS)
    {
        key->stable = key->candidate;
        if (key->stable) { key->down_since = now; key->long_sent = 0u; }
        else if (!key->long_sent) return 1u;
    }
    if (key->stable && key->candidate && !key->long_sent &&
        (uint32_t)(now-key->down_since) >= KEY_LONG_MS)
    { key->long_sent = 1u; return 2u; }
    return 0u;
}
