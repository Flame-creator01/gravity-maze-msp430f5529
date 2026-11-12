#ifndef APP_CONFIG_H
#define APP_CONFIG_H
#define ACCEL_SAMPLE_PERIOD_MS 20u
#define LCD_REFRESH_PERIOD_MS 100u
/* Tilt magnitude controls the repeat interval: light tilt = slow, strong tilt = fast.
   Responsive preset: starts moving at 130 mg (~7.5 deg); 190 ms/step at 130 mg,
   linearly down to 90 ms/step at 450 mg (~26 deg). Lower the two tilt values to
   make it more sensitive; raise MOTION_TRIGGER_MG if the ball drifts when still. */
#define GAME_MOVE_SLOW_MS 190u
#define GAME_MOVE_FAST_MS 90u
#define GAME_TILT_FULL_MG 450
/* Low-pass weight out of 4: 1 = 1/4 (smooth, laggy), 2 = 1/2 (default),
   3 = 3/4 (snappy, slightly noisier). Directions reverse/stop faster with a
   larger weight. */
#define MOTION_FILTER_WEIGHT 2
/* Board thumb wheel (analog potentiometer, TI reference design): P8.0 enables
   it, P6.5 is ADC12 input A5. One adjustment tick = WHEEL_STEP_ADC counts;
   increase WHEEL_STEP_ADC if noise triggers, set WHEEL_INVERT 1 to flip. */
#define WHEEL_STEP_ADC 96
#define WHEEL_INVERT 0
#define WHEEL_WAIT_LIMIT 20000u
/* Trap: teleport back to the start (configurable) and add a time penalty. */
#define GAME_TRAP_RESET 1
#define GAME_TRAP_PENALTY_MS 3000UL
#define GAME_TRAP_NOTICE "TRAP! +3S" /* keep in sync with GAME_TRAP_PENALTY_MS */
/* 1 = levels unlock in order and progress is saved; 0 = every level selectable. */
#define GAME_LEVEL_LOCK 1
#define GAME_MAX_ITEMS 6u
#define ACCEL_STALE_MS 300u
#define CALIBRATION_SAMPLES 64u
#define CALIBRATION_SETTLE_MS 1000u
#define CALIBRATION_SPAN_MG 120
/* Start moving at about 7.5 deg of tilt; stop below about 5.7 deg. */
#define MOTION_TRIGGER_MG 130
#define MOTION_RELEASE_MG 100
#define MOTION_AXIS_MARGIN_MG 35
/* Change these after the first physical tilt test, not the sensor driver. */
#define MOTION_SWAP_XY 0
#define MOTION_X_SIGN (-1)
#define MOTION_Y_SIGN 1
#define MAZE_COLUMNS 15u
#define MAZE_ROWS 9u
#define MAZE_LEVEL_COUNT 6u
#define MAZE_CELL_WIDTH 6u
#define MAZE_CELL_HEIGHT 6u
#define MAZE_ORIGIN_X 6u
#define MAZE_ORIGIN_Y 10u
#define LCD_WIDTH 102u
#define LCD_HEIGHT 64u
#define LCD_CONTRAST 15u
#define KEY_SCAN_PERIOD_MS 5u
#define KEY_DEBOUNCE_MS 20u
#define KEY_LONG_MS 800u
#endif
