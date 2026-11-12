#ifndef APP_TYPES_H
#define APP_TYPES_H
#include <stdint.h>
typedef struct { int16_t x, y, z; } AccelData; /* unit: nominal mg */
typedef enum { MOTION_STOP=0, MOTION_UP, MOTION_DOWN, MOTION_LEFT, MOTION_RIGHT } MotionDirection;
typedef enum { KEY_EVENT_NONE=0, KEY_EVENT_1_PRESS, KEY_EVENT_1_LONG,
    KEY_EVENT_2_PRESS, KEY_EVENT_2_LONG } KeyEvent;
typedef enum { APP_STATE_INIT=0, APP_STATE_CALIBRATION, APP_STATE_MENU,
    APP_STATE_PLAYING, APP_STATE_PAUSED, APP_STATE_CLEAR, APP_STATE_ERROR,
    APP_STATE_DIAGNOSTICS, APP_STATE_TUNING } AppState;
#endif
