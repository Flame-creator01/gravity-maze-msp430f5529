#include <string.h>
#include "app_config.h"
#include "motion_input.h"
#include "tuning.h"
static MotionStatus status;
static int32_t sum_x, sum_y, sum_z;
static int16_t min_x, max_x, min_y, max_y, min_z, max_z;
static MotionDirection direction;
static int32_t Absolute(int32_t v) { return v < 0 ? -v : v; }
static void MapAxes(const AccelData *data, int16_t *x, int16_t *y)
{
#if MOTION_SWAP_XY
    *x = (int16_t)((int32_t)data->y * MOTION_X_SIGN);
    *y = (int16_t)((int32_t)data->x * MOTION_Y_SIGN);
#else
    *x = (int16_t)((int32_t)data->x * MOTION_X_SIGN);
    *y = (int16_t)((int32_t)data->y * MOTION_Y_SIGN);
#endif
}
void MotionInput_ResetDirection(void)
{
    status.filtered_x = 0; status.filtered_y = 0; status.tilt_mg = 0u;
    direction = MOTION_STOP;
}
void MotionInput_Init(void)
{
    memset(&status, 0, sizeof(status)); MotionInput_ResetDirection();
}
void MotionInput_CalibrateStart(void)
{
    sum_x = 0L; sum_y = 0L; sum_z = 0L; status.samples = 0u;
    status.mean_x = status.mean_y = status.mean_z = 0;
    status.span_x = status.span_y = status.span_z = 0u;
    status.failure = CAL_FAILURE_NONE;
    min_x = min_y = min_z = 32767; max_x = max_y = max_z = -32767;
    status.calibration = CAL_COLLECTING; MotionInput_ResetDirection();
}
void MotionInput_CalibratePrepare(void)
{
    MotionInput_CalibrateStart();
    status.calibration = CAL_SETTLING;
}
uint8_t MotionInput_CalibrateSample(const AccelData *data)
{
    int16_t x, y, z;
    int32_t average_z;
    if (data == 0 || status.calibration != CAL_COLLECTING) return 0u;
    MapAxes(data, &x, &y); z = data->z;
    sum_x += x; sum_y += y; sum_z += z;
    if (x < min_x) min_x = x;
    if (x > max_x) max_x = x;
    if (y < min_y) min_y = y;
    if (y > max_y) max_y = y;
    if (z < min_z) min_z = z;
    if (z > max_z) max_z = z;
    status.samples++;
    if (status.samples < CALIBRATION_SAMPLES) return 0u;
    status.mean_x = (int16_t)(sum_x / (int32_t)CALIBRATION_SAMPLES);
    status.mean_y = (int16_t)(sum_y / (int32_t)CALIBRATION_SAMPLES);
    status.mean_z = (int16_t)(sum_z / (int32_t)CALIBRATION_SAMPLES);
    status.span_x = (uint16_t)((int32_t)max_x-min_x);
    status.span_y = (uint16_t)((int32_t)max_y-min_y);
    status.span_z = (uint16_t)((int32_t)max_z-min_z);
    average_z = Absolute(status.mean_z);
    if (status.span_x > CALIBRATION_SPAN_MG ||
        status.span_y > CALIBRATION_SPAN_MG ||
        status.span_z > CALIBRATION_SPAN_MG)
        status.failure = CAL_FAILURE_MOTION;
    else if (average_z < 600L || average_z > 1400L)
        status.failure = CAL_FAILURE_GRAVITY;
    else if (Absolute(status.mean_x) > 600L || Absolute(status.mean_y) > 600L)
        status.failure = CAL_FAILURE_TILT;
    if (status.failure != CAL_FAILURE_NONE)
    { status.calibration = CAL_UNSTABLE; return 0u; }
    status.offset_x = status.mean_x;
    status.offset_y = status.mean_y;
    status.calibration = CAL_OK; MotionInput_ResetDirection();
    return 1u;
}
void MotionInput_Update(const AccelData *data)
{
    int16_t x, y;
    int32_t ax, ay, active;
    uint8_t current_is_x;
    if (data == 0 || status.calibration != CAL_OK) return;
    MapAxes(data, &x, &y);
    /* Low-pass with a runtime weight (1..3 of 4): larger = faster response and
       quicker direction reversal / stop, at the cost of a little more noise. */
    status.filtered_x = (int16_t)(((int32_t)status.filtered_x * (4L - (int32_t)g_tuning.filter_num) +
        ((int32_t)x - (int32_t)status.offset_x) * (int32_t)g_tuning.filter_num) / 4L);
    status.filtered_y = (int16_t)(((int32_t)status.filtered_y * (4L - (int32_t)g_tuning.filter_num) +
        ((int32_t)y - (int32_t)status.offset_y) * (int32_t)g_tuning.filter_num) / 4L);
    ax = Absolute(status.filtered_x); ay = Absolute(status.filtered_y);
    status.tilt_mg = (uint16_t)(ax > ay ? ax : ay);
    current_is_x = direction == MOTION_LEFT || direction == MOTION_RIGHT;
    active = current_is_x ? ax : ay;
    /* Keep the current axis until its release threshold, or a stronger axis wins. */
    if (direction != MOTION_STOP && active >= (int32_t)g_tuning.release_mg)
    {
        if (current_is_x && ay < ax + (int32_t)g_tuning.margin_mg)
        { direction = status.filtered_x > 0 ? MOTION_RIGHT : MOTION_LEFT; return; }
        if (!current_is_x && ax < ay + (int32_t)g_tuning.margin_mg)
        { direction = status.filtered_y > 0 ? MOTION_DOWN : MOTION_UP; return; }
    }
    if (ax < (int32_t)g_tuning.trigger_mg && ay < (int32_t)g_tuning.trigger_mg)
        direction = MOTION_STOP;
    else if (ax >= ay) direction = status.filtered_x > 0 ? MOTION_RIGHT : MOTION_LEFT;
    else direction = status.filtered_y > 0 ? MOTION_DOWN : MOTION_UP;
}
MotionDirection MotionInput_GetDirection(void) { return direction; }
uint16_t MotionInput_GetTiltMagnitude(void) { return status.tilt_mg; }
uint8_t MotionInput_IsCalibrated(void) { return status.calibration == CAL_OK; }
const MotionStatus *MotionInput_GetStatus(void) { return &status; }
