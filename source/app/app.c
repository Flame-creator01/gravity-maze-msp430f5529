#include "app_config.h"
#include "app.h"
#include "timer.h"
#include "accel.h"
#include "lcd.h"
#include "key_input.h"
#include "motion_input.h"
#include "game.h"
#include "ui.h"
#include "led.h"
#include "save.h"
#include "tuning.h"
#include "wheel.h"
volatile AppDebug g_debug;
static AppState state;
static uint8_t selected_level, calibration_started;
static uint32_t last_accel, last_lcd, last_valid;
static uint32_t calibration_requested;
static SaveData save;
static uint8_t tuning_item;

static void ChangeState(AppState next, uint32_t now)
{
    Game_SetRunning(next == APP_STATE_PLAYING, now);
    MotionInput_ResetDirection(); state = next;
    if (next == APP_STATE_CALIBRATION)
    { calibration_started = 0u; MotionInput_Init(); }
    last_lcd = now-LCD_REFRESH_PERIOD_MS;
}
static void SensorInit(uint32_t now)
{
    if (Accel_Init())
    {
        last_valid = Timer_GetMilliseconds(); last_accel = last_valid;
        g_debug.error = 0u; MotionInput_Init();
        ChangeState(APP_STATE_CALIBRATION, last_valid);
    }
    else { g_debug.error = 1u; ChangeState(APP_STATE_ERROR, now); }
    g_debug.sensor_type = Accel_GetType();
    g_debug.accel_diag = *Accel_GetDiagnostics();
}
void App_Init(void)
{
    uint32_t now = Timer_GetMilliseconds();
    selected_level = 0u; state = APP_STATE_INIT;
    last_accel = now; last_lcd = now; last_valid = now;
    Save_Defaults(&save);
    (void)Save_Load(&save);
    Tuning_Defaults();
    (void)Tuning_Load();
    g_debug.unlocked = save.unlocked;
    g_debug.new_best = 0u;
    g_debug.save_ok = 0u;
    tuning_item = 0u;
    g_debug.tuning_item = 0u;
    g_debug.tuning_saved = 0u;
    Game_Init(selected_level); MotionInput_Init();
    g_debug.lcd_ok = LCD_Init();
    Wheel_Init();
    SensorInit(Timer_GetMilliseconds());
}
static uint8_t LevelUnlocked(uint8_t level)
{
#if GAME_LEVEL_LOCK
    return (uint8_t)(level < save.unlocked);
#else
    (void)level; return 1u;
#endif
}
static void StartGame(uint32_t now)
{
    if (!LevelUnlocked(selected_level)) return;
    g_debug.new_best = 0u;
    Game_Init(selected_level); ChangeState(APP_STATE_PLAYING, now);
}
static uint16_t StepU16(uint16_t value, int16_t step, uint16_t low, uint16_t high)
{
    int32_t v = (int32_t)value + (int32_t)step;
    if (v < (int32_t)low) v = (int32_t)low;
    if (v > (int32_t)high) v = (int32_t)high;
    return (uint16_t)v;
}
static void TuningEdit(int16_t ticks)
{
    switch (tuning_item)
    {
    case 0u: g_tuning.trigger_mg = StepU16(g_tuning.trigger_mg, (int16_t)(ticks*10), 50u, 400u); break;
    case 1u: g_tuning.release_mg = StepU16(g_tuning.release_mg, (int16_t)(ticks*10), 30u, 300u); break;
    case 2u: g_tuning.margin_mg = StepU16(g_tuning.margin_mg, (int16_t)(ticks*5), 10u, 120u); break;
    case 3u: g_tuning.slow_ms = StepU16(g_tuning.slow_ms, (int16_t)(ticks*10), 120u, 400u); break;
    case 4u: g_tuning.fast_ms = StepU16(g_tuning.fast_ms, (int16_t)(ticks*10), 60u, 250u); break;
    case 5u: g_tuning.full_mg = StepU16(g_tuning.full_mg, (int16_t)(ticks*25), 250u, 1000u); break;
    case 6u: g_tuning.filter_num = StepU16(g_tuning.filter_num, ticks, 1u, 3u); break;
    default: Tuning_Defaults(); g_debug.tuning_saved = 3u; return;
    }
    Tuning_Clamp();
}
/* The wheel adjusts the value; the buttons only navigate and save. */
static void HandleTuningEvent(KeyEvent event, uint32_t now)
{
    if (event == KEY_EVENT_1_PRESS || event == KEY_EVENT_2_PRESS)
    {
        if (event == KEY_EVENT_1_PRESS)
            tuning_item = (uint8_t)((tuning_item + TUNING_ITEM_COUNT - 1u) % TUNING_ITEM_COUNT);
        else
            tuning_item = (uint8_t)((tuning_item + 1u) % TUNING_ITEM_COUNT);
        g_debug.tuning_item = tuning_item;
        g_debug.tuning_saved = 0u;
    }
    else if (event == KEY_EVENT_1_LONG)
    { ChangeState(APP_STATE_MENU, now); return; } /* leave without saving */
    else if (event == KEY_EVENT_2_LONG)
    {
        g_debug.tuning_saved = (uint8_t)(Tuning_Save() ? 1u : 2u);
        ChangeState(APP_STATE_MENU, now);
        return;
    }
    last_lcd = now - LCD_REFRESH_PERIOD_MS;
}
static void HandleEvent(KeyEvent event, uint32_t now)
{
    if (event == KEY_EVENT_NONE) return;
    if (state == APP_STATE_ERROR)
    {
        if (event == KEY_EVENT_1_PRESS) SensorInit(now);
        return;
    }
    if (state == APP_STATE_TUNING)
    { HandleTuningEvent(event, now); return; }
    if (state == APP_STATE_MENU) g_debug.tuning_saved = 0u;
    if (state == APP_STATE_DIAGNOSTICS && event == KEY_EVENT_2_LONG)
    {
        tuning_item = 0u;
        g_debug.tuning_item = 0u;
        g_debug.tuning_saved = 0u;
        ChangeState(APP_STATE_TUNING, now);
        return;
    }
    if (event == KEY_EVENT_1_LONG)
    { ChangeState(APP_STATE_CALIBRATION, now); return; }
    if (event == KEY_EVENT_2_LONG)
    {
        if (state == APP_STATE_MENU || state == APP_STATE_CALIBRATION)
            ChangeState(APP_STATE_DIAGNOSTICS, now);
        else ChangeState(APP_STATE_MENU, now);
        return;
    }
    if (state == APP_STATE_CALIBRATION)
    {
        if (event == KEY_EVENT_1_PRESS && !calibration_started)
        {
            MotionInput_CalibratePrepare(); calibration_started = 1u;
            calibration_requested = now;
        }
    }
    else if (state == APP_STATE_MENU)
    {
        if (event == KEY_EVENT_1_PRESS)
        {
            if (MotionInput_IsCalibrated()) StartGame(now);
            else ChangeState(APP_STATE_CALIBRATION, now);
        }
        else if (event == KEY_EVENT_2_PRESS) selected_level=(uint8_t)((selected_level+1u)%MAZE_LEVEL_COUNT);
    }
    else if (state == APP_STATE_PLAYING)
    {
        if (event == KEY_EVENT_1_PRESS) ChangeState(APP_STATE_PAUSED, now);
        else if (event == KEY_EVENT_2_PRESS) StartGame(now);
    }
    else if (state == APP_STATE_PAUSED)
    {
        if (event == KEY_EVENT_1_PRESS) ChangeState(APP_STATE_PLAYING, now);
        else if (event == KEY_EVENT_2_PRESS) StartGame(now);
    }
    else if (state == APP_STATE_CLEAR)
    {
        if (event == KEY_EVENT_1_PRESS) selected_level=(uint8_t)((selected_level+1u)%MAZE_LEVEL_COUNT);
        if (event == KEY_EVENT_1_PRESS || event == KEY_EVENT_2_PRESS) StartGame(now);
    }
    else if (state == APP_STATE_DIAGNOSTICS)
    {
        if (event == KEY_EVENT_1_PRESS) SensorInit(now);
        else if (event == KEY_EVENT_2_PRESS) ChangeState(APP_STATE_MENU, now);
    }
    last_lcd=now-LCD_REFRESH_PERIOD_MS;
}
void App_RunOnce(uint32_t now)
{
    uint8_t result, flags;
    AccelData sample;
    const MotionStatus *motion;
    uint32_t finished;
    int8_t wheel;
    HandleEvent(KeyInput_GetEvent(), now);
    /* Sensor retry may delay for power-up; do not compare old now to new timestamps. */
    now=Timer_GetMilliseconds();
    if (state == APP_STATE_CALIBRATION && calibration_started &&
        MotionInput_GetStatus()->calibration == CAL_SETTLING &&
        (uint32_t)(now-calibration_requested) >= CALIBRATION_SETTLE_MS)
        MotionInput_CalibrateStart();
    if (state != APP_STATE_ERROR && Timer_Elapsed(now,&last_accel,ACCEL_SAMPLE_PERIOD_MS))
    {
        /* Poll the thumb wheel every sample; it only changes TUNE values. */
        wheel = Wheel_Poll();
        if (state == APP_STATE_TUNING && wheel != 0)
        { g_debug.tuning_saved = 0u; TuningEdit((int16_t)wheel); }
        result=Accel_Read(&sample);
        g_debug.accel_diag = *Accel_GetDiagnostics();
        if (result == ACCEL_READ_OK)
        {
            last_valid=now; g_debug.raw_mg=sample; g_debug.valid_samples++;
            if (state == APP_STATE_CALIBRATION && calibration_started)
            {
                if (MotionInput_CalibrateSample(&sample))
                { calibration_started=0u; ChangeState(APP_STATE_MENU,now); }
                else if (MotionInput_GetStatus()->calibration == CAL_UNSTABLE) calibration_started=0u;
            }
            else if (state == APP_STATE_PLAYING || state == APP_STATE_TUNING)
                MotionInput_Update(&sample);
        }
        if (result == ACCEL_READ_ERROR || (uint32_t)(now-last_valid) >= ACCEL_STALE_MS)
        { g_debug.error=2u; ChangeState(APP_STATE_ERROR,now); }
    }
    if (state == APP_STATE_PLAYING)
    {
        Game_Update(MotionInput_GetDirection(), MotionInput_GetTiltMagnitude(), now);
        if (Game_IsCleared())
        {
            /* Best time and unlock progress are written to INFO flash. */
            finished=Game_GetElapsedMilliseconds();
            flags=Save_ApplyResult(&save, selected_level, finished, Game_GetSteps());
            g_debug.new_best = (uint8_t)((flags & SAVE_RESULT_BEST) != 0u);
            if (flags != 0u) g_debug.save_ok = Save_Store(&save);
            ChangeState(APP_STATE_CLEAR,now);
        }
    }
    Led_Set(state==APP_STATE_PAUSED || state==APP_STATE_CLEAR || state==APP_STATE_ERROR);
    motion=MotionInput_GetStatus();
    g_debug.filtered_x=motion->filtered_x; g_debug.filtered_y=motion->filtered_y;
    g_debug.offset_x=motion->offset_x; g_debug.offset_y=motion->offset_y;
    g_debug.calibration_samples=motion->samples;
    g_debug.calibration_mean_mg.x=motion->mean_x;
    g_debug.calibration_mean_mg.y=motion->mean_y;
    g_debug.calibration_mean_mg.z=motion->mean_z;
    g_debug.calibration_span_x=motion->span_x;
    g_debug.calibration_span_y=motion->span_y;
    g_debug.calibration_span_z=motion->span_z;
    g_debug.calibration_failure=motion->failure;
    g_debug.direction=(uint8_t)MotionInput_GetDirection(); g_debug.state=(uint8_t)state;
    g_debug.unlocked = save.unlocked;
    if (Timer_Elapsed(now,&last_lcd,LCD_REFRESH_PERIOD_MS))
        g_debug.lcd_ok=UI_DrawState(state,selected_level,Game_GetElapsedSeconds(),save.best_ms[selected_level]);
}
