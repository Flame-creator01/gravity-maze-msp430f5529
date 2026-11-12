#ifndef MOTION_INPUT_H
#define MOTION_INPUT_H
#include "app_types.h"
#define CAL_IDLE 0u
#define CAL_COLLECTING 1u
#define CAL_OK 2u
#define CAL_UNSTABLE 3u
#define CAL_SETTLING 4u
#define CAL_FAILURE_NONE 0u
#define CAL_FAILURE_MOTION 1u
#define CAL_FAILURE_GRAVITY 2u
#define CAL_FAILURE_TILT 3u
typedef struct {
    int16_t offset_x, offset_y, filtered_x, filtered_y;
    uint16_t tilt_mg; /* magnitude of the dominant filtered axis after offset */
    int16_t mean_x, mean_y, mean_z;
    uint16_t span_x, span_y, span_z;
    uint16_t samples;
    uint8_t calibration, failure;
} MotionStatus;
void MotionInput_Init(void);
void MotionInput_CalibratePrepare(void);
void MotionInput_CalibrateStart(void);
uint8_t MotionInput_CalibrateSample(const AccelData *data);
void MotionInput_Update(const AccelData *data);
void MotionInput_ResetDirection(void);
MotionDirection MotionInput_GetDirection(void);
uint16_t MotionInput_GetTiltMagnitude(void);
uint8_t MotionInput_IsCalibrated(void);
const MotionStatus *MotionInput_GetStatus(void);
#endif
