#ifndef APP_H
#define APP_H
#include "app_types.h"
#include "accel.h"
typedef struct {
    AccelData raw_mg;
    AccelDiagnostics accel_diag;
    int16_t filtered_x, filtered_y, offset_x, offset_y;
    uint32_t valid_samples;
    uint16_t calibration_samples;
    AccelData calibration_mean_mg;
    uint16_t calibration_span_x, calibration_span_y, calibration_span_z;
    uint8_t calibration_failure;
    uint8_t sensor_type, state, direction, error, lcd_ok;
    uint8_t unlocked;  /* levels unlocked by saved progress */
    uint8_t new_best;  /* last clear improved the level record */
    uint8_t save_ok;   /* last info-flash store succeeded */
    uint8_t tuning_item;  /* selected parameter on the TUNE screen */
    uint8_t tuning_saved; /* 0 none, 1 saved, 2 save failed, 3 defaults loaded */
} AppDebug;
extern volatile AppDebug g_debug;
void App_Init(void);
void App_RunOnce(uint32_t now);
#endif
