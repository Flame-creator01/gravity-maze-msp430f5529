#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "app_config.h"
#include "app.h"
#include "accel.h"
#include "motion_input.h"
#include "key_filter.h"
#include "game.h"
#include "game_map.h"
#include "lcd.h"
#include "lcd_port.h"
#include "timer.h"
#include "key_input.h"
#include "save.h"
#include "tuning.h"
#include "ui.h"

/* Host-only hardware substitutes. Never included in the CCS source build. */
static uint32_t fake_now;
static uint8_t fake_sensor_ok = 1u;
static uint8_t fake_read = ACCEL_READ_OK;
static AccelData fake_sample = {-72,36,1008};
static KeyEvent fake_key;
static int8_t fake_wheel; /* ticks returned by the next Wheel_Poll */
static uint32_t flush_count;
static AccelDiagnostics fake_diagnostics;
static uint16_t fake_init_calls;
extern void HostSave_Reset(void);
extern void HostSave_FailErase(uint8_t fail);
extern void HostSave_Corrupt(uint8_t slot, uint8_t offset);
uint32_t Timer_GetMilliseconds(void) { return fake_now; }
uint8_t Timer_Elapsed(uint32_t now, uint32_t *last, uint16_t period)
{
    if ((uint32_t)(now-*last) < period) return 0u;
    *last = now; return 1u;
}
uint8_t Accel_Init(void) { fake_init_calls++; return fake_sensor_ok; }
uint8_t Accel_GetType(void) { return fake_sensor_ok ? 1u : 0u; }
uint8_t Accel_Read(AccelData *data) { *data=fake_sample; return fake_read; }
const AccelDiagnostics *Accel_GetDiagnostics(void) { return &fake_diagnostics; }
KeyEvent KeyInput_GetEvent(void) { KeyEvent k=fake_key; fake_key=KEY_EVENT_NONE; return k; }
void Wheel_Init(void) { fake_wheel=0; }
int8_t Wheel_Poll(void) { int8_t v=fake_wheel; fake_wheel=0; return v; }
void Led_Set(uint8_t on) { (void)on; }
uint8_t LCD_Port_Init(void) { return 1u; }
uint8_t LCD_Port_Flush(const uint8_t *buffer, uint16_t size)
{ assert(buffer!=0 && size==816u); flush_count++; return 1u; }

#define TEST_MAX_ITEMS 4
#define TEST_MASKS (1 << TEST_MAX_ITEMS)
#define TEST_CELLS ((int)(MAZE_COLUMNS * MAZE_ROWS))
#define TEST_STATES (TEST_CELLS * TEST_MASKS)

static int16_t bfs_prev[TEST_STATES];
static int16_t bfs_queue[TEST_STATES];

static MotionDirection DirTo(uint8_t cx, uint8_t cy, uint8_t nx, uint8_t ny)
{
    if (nx > cx) return MOTION_RIGHT;
    if (nx < cx) return MOTION_LEFT;
    if (ny > cy) return MOTION_DOWN;
    return MOTION_UP;
}
static int AbsI(int v) { return v < 0 ? -v : v; }
static int BuildItems(const MazeLevel *map, uint8_t *ix, uint8_t *iy)
{
    uint8_t x, y;
    int n = 0;
    for (y = 0u; y < MAZE_ROWS; y++)
        for (x = 0u; x < MAZE_COLUMNS; x++)
            if (map->rows[y][x] == MAP_ITEM && n < TEST_MAX_ITEMS)
            { ix[n] = x; iy[n] = y; n++; }
    return n;
}
static int ReversePath(int16_t *prev, int start, int target, uint8_t *px, uint8_t *py, int max_len)
{
    int count = 0, cur = target, i;
    uint8_t tx, ty;
    while (count < max_len)
    {
        px[count] = (uint8_t)(cur % (int)MAZE_COLUMNS);
        py[count] = (uint8_t)(cur / (int)MAZE_COLUMNS);
        count++;
        if (cur == start) break;
        cur = prev[cur];
    }
    if (cur != start) return 0;
    for (i = 0; i < count / 2; i++)
    {
        tx = px[i]; px[i] = px[count-1-i]; px[count-1-i] = tx;
        ty = py[i]; py[i] = py[count-1-i]; py[count-1-i] = ty;
    }
    return count;
}
static int ReverseStatePath(int16_t *prev, int start_state, int target_state,
                            uint8_t *px, uint8_t *py, int max_len)
{
    int count = 0, cur = target_state, i, cell;
    uint8_t tx, ty;
    while (count < max_len)
    {
        cell = cur / TEST_MASKS;
        px[count] = (uint8_t)(cell % (int)MAZE_COLUMNS);
        py[count] = (uint8_t)(cell / (int)MAZE_COLUMNS);
        count++;
        if (cur == start_state) break;
        cur = prev[cur];
    }
    if (cur != start_state) return 0;
    for (i = 0; i < count / 2; i++)
    {
        tx = px[i]; px[i] = px[count-1-i]; px[count-1-i] = tx;
        ty = py[i]; py[i] = py[count-1-i]; py[count-1-i] = ty;
    }
    return count;
}
/* BFS over (cell, collected-mask); traps are blocked for this route check. */
static int PathBFS(const MazeLevel *map, const uint8_t *ix, const uint8_t *iy, int item_count,
                   uint8_t *px, uint8_t *py, int max_len)
{
    int head = 0, tail = 0, i, d, cell, mask, cx, cy, nx, ny, nmask, nst;
    int start = (int)map->start_y * (int)MAZE_COLUMNS + (int)map->start_x;
    int goal = (int)map->end_y * (int)MAZE_COLUMNS + (int)map->end_x;
    int full = (1 << item_count) - 1;
    int dx[4] = {1,-1,0,0}, dy[4] = {0,0,1,-1};
    for (i = 0; i < TEST_STATES; i++) bfs_prev[i] = -1;
    i = start * TEST_MASKS;
    bfs_prev[i] = (int16_t)i; bfs_queue[tail++] = (int16_t)i;
    while (head < tail)
    {
        int st = (int)bfs_queue[head++];
        cell = st / TEST_MASKS; mask = st % TEST_MASKS;
        if (cell == goal && mask == full)
            return ReverseStatePath(bfs_prev, start * TEST_MASKS, st, px, py, max_len);
        cx = cell % (int)MAZE_COLUMNS; cy = cell / (int)MAZE_COLUMNS;
        for (d = 0; d < 4; d++)
        {
            nx = cx + dx[d]; ny = cy + dy[d];
            if (nx < 0 || ny < 0 || nx >= (int)MAZE_COLUMNS || ny >= (int)MAZE_ROWS) continue;
            if (GameMap_IsWall(map, (uint8_t)nx, (uint8_t)ny)) continue;
            if (GameMap_IsTrap(map, (uint8_t)nx, (uint8_t)ny)) continue;
            nmask = mask;
            for (i = 0; i < item_count; i++)
                if ((int)ix[i] == nx && (int)iy[i] == ny) nmask |= (1 << i);
            if (GameMap_IsExit(map, (uint8_t)nx, (uint8_t)ny) && nmask != full) continue;
            nst = (ny * (int)MAZE_COLUMNS + nx) * TEST_MASKS + nmask;
            if (bfs_prev[nst] != -1) continue;
            bfs_prev[nst] = (int16_t)st; bfs_queue[tail++] = (int16_t)nst;
        }
    }
    return 0;
}
static void TestMapsAndCompletion(void)
{
    uint8_t level, ix[TEST_MAX_ITEMS], iy[TEST_MAX_ITEMS];
    uint8_t px[512], py[512];
    uint32_t now;
    for (level = 0u; level < MAZE_LEVEL_COUNT; level++)
    {
        const MazeLevel *map = GameMap_Get(level);
        int y, x, item_count, path_len, i, traps = 0;
        for (y = 0; y < (int)MAZE_ROWS; y++)
        {
            assert(strlen(map->rows[y]) == MAZE_COLUMNS);
            assert(map->rows[y][0] == MAP_WALL && map->rows[y][MAZE_COLUMNS-1u] == MAP_WALL);
        }
        assert(map->rows[map->start_y][map->start_x] != MAP_WALL);
        assert(map->rows[map->end_y][map->end_x] == MAP_EXIT);
        for (y = 0; y < (int)MAZE_ROWS; y++)
            for (x = 0; x < (int)MAZE_COLUMNS; x++)
                if (map->rows[y][x] == MAP_TRAP) traps++;
        assert(traps >= 1);
        item_count = BuildItems(map, ix, iy);
        assert(item_count >= 1 && item_count <= TEST_MAX_ITEMS);
        Game_Init(level);
        assert(Game_GetItemsTotal() == (uint8_t)item_count);
        assert(Game_GetItemsLeft() == (uint8_t)item_count);
        assert(!Game_IsExitOpen());
        assert(Game_GetPlayerX() == map->start_x && Game_GetPlayerY() == map->start_y);
        path_len = PathBFS(map, ix, iy, item_count, px, py, 512);
        assert(path_len > 1);
        now = 1000UL;
        Game_SetRunning(1u, now);
        for (i = 1; i < path_len; i++)
        {
            now += GAME_MOVE_SLOW_MS;
            Game_Update(DirTo(px[i-1], py[i-1], px[i], py[i]), 0u, now);
            assert(Game_GetPlayerX() == px[i] && Game_GetPlayerY() == py[i]);
        }
        assert(Game_IsCleared() && Game_GetItemsLeft() == 0u && Game_IsExitOpen());
        assert(Game_GetSteps() == (uint16_t)(path_len - 1));
        printf("level %u: %d items, %d traps, %d steps solved\n",
               level+1u, item_count, traps, path_len-1);
    }
    assert(GameMap_IsWall(GameMap_Get(0u), 255u, 255u));
}
static void TestGameTimeAndRate(void)
{
    uint16_t before;
    Game_Init(0u); Game_SetRunning(1u, 0UL);
    Game_Update(MOTION_STOP, 0u, 500UL); Game_SetRunning(0u, 1000UL);
    Game_Update(MOTION_DOWN, 0u, 9000UL);
    assert(Game_GetElapsedMilliseconds() == 1000UL);
    Game_SetRunning(1u, 10000UL); Game_Update(MOTION_STOP, 0u, 11000UL);
    assert(Game_GetElapsedMilliseconds() == 2000UL);

    /* Light tilt uses the slow interval. */
    Game_Init(0u); Game_SetRunning(1u, 0UL);
    before = Game_GetSteps();
    Game_Update(MOTION_DOWN, 0u, 0UL);
    assert(Game_GetSteps() == (uint16_t)(before + 1u));
    Game_Update(MOTION_DOWN, 0u, 100UL);
    assert(Game_GetSteps() == (uint16_t)(before + 1u));
    /* Strong tilt shortens the interval to GAME_MOVE_FAST_MS. */
    Game_Update(MOTION_DOWN, (uint16_t)GAME_TILT_FULL_MG, 130UL);
    assert(Game_GetSteps() == (uint16_t)(before + 2u));
    /* Medium tilt lies strictly between fast and slow. */
    Game_Init(0u); Game_SetRunning(1u, 0UL);
    before = Game_GetSteps();
    Game_Update(MOTION_DOWN, 0u, 0UL);
    Game_Update(MOTION_DOWN, 300u, 130UL);
    assert(Game_GetSteps() == (uint16_t)(before + 1u));
    Game_Update(MOTION_DOWN, 300u, 140UL);
    assert(Game_GetSteps() == (uint16_t)(before + 2u));
    /* Stop/start must not bypass the rate limit. */
    Game_Init(0u); Game_SetRunning(1u, 0UL);
    Game_Update(MOTION_DOWN, 0u, 0UL);
    before = Game_GetSteps();
    Game_Update(MOTION_STOP, 0u, 10UL);
    Game_Update(MOTION_DOWN, 0u, 20UL);
    assert(Game_GetSteps() == before);
    /* Tick rollover. */
    Game_Init(0u); Game_SetRunning(1u, 0xFFFFFFF0UL);
    Game_Update(MOTION_STOP, 0u, 0x00000054UL);
    assert(Game_GetElapsedMilliseconds() == 100UL);
    puts("timing: pause/resume, restart, tilt speed, rate limit and tick rollover passed");
}
static void TestMotion(void)
{
    /* Raw +X maps to screen left on the user's board. */
    AccelData sample={72,36,1008};
    uint16_t i;
    MotionInput_Init();MotionInput_CalibrateStart();
    for (i=0u;i<CALIBRATION_SAMPLES;i++) (void)MotionInput_CalibrateSample(&sample);
    assert(MotionInput_IsCalibrated());
    assert(MotionInput_GetStatus()->offset_x==-72 && MotionInput_GetStatus()->offset_y==36);
    for (i=0u;i<100u;i++) MotionInput_Update(&sample);
    assert(MotionInput_GetDirection()==MOTION_STOP);
    assert(MotionInput_GetTiltMagnitude() < MOTION_TRIGGER_MG);
    sample.x=-500;
    for (i=0u;i<20u;i++) MotionInput_Update(&sample);
    assert(MotionInput_GetDirection()==MOTION_RIGHT);
    assert(MotionInput_GetTiltMagnitude() >= MOTION_TRIGGER_MG);
    sample.x=700;
    for (i=0u;i<20u;i++) MotionInput_Update(&sample);
    assert(MotionInput_GetDirection()==MOTION_LEFT);
    sample.x=72;sample.y=650;
    for (i=0u;i<20u;i++) MotionInput_Update(&sample);
    assert(MotionInput_GetDirection()==MOTION_DOWN);
    MotionInput_ResetDirection();
    assert(MotionInput_GetTiltMagnitude()==0u);
    MotionInput_CalibrateStart();
    for (i=0u;i<CALIBRATION_SAMPLES;i++)
    { sample.x=(i&1u)?-400:400;(void)MotionInput_CalibrateSample(&sample); }
    assert(!MotionInput_IsCalibrated() && MotionInput_GetStatus()->calibration==CAL_UNSTABLE);
    sample.x=0;sample.y=1000;sample.z=0;
    MotionInput_CalibrateStart();
    for (i=0u;i<CALIBRATION_SAMPLES;i++) (void)MotionInput_CalibrateSample(&sample);
    assert(!MotionInput_IsCalibrated());
    puts("motion: negative offsets, tilt magnitude, calibration and directions passed");
}
static void CalibrateConstant(const AccelData *sample)
{
    uint16_t i;
    MotionInput_CalibrateStart();
    for (i=0u;i<CALIBRATION_SAMPLES;i++) (void)MotionInput_CalibrateSample(sample);
}
static void TestCalibrationReasons(void)
{
    AccelData sample={0,0,1008};
    uint16_t i;
    const MotionStatus *motion;
    MotionInput_CalibratePrepare();
    assert(MotionInput_GetStatus()->calibration==CAL_SETTLING);
    assert(!MotionInput_CalibrateSample(&sample));
    assert(MotionInput_GetStatus()->samples==0u);
    MotionInput_CalibrateStart();
    for (i=0u;i<CALIBRATION_SAMPLES;i++)
    { sample.x=(i & 1u) ? -54 : 54; (void)MotionInput_CalibrateSample(&sample); }
    motion=MotionInput_GetStatus();
    assert(MotionInput_IsCalibrated() && motion->span_x==108u);
    assert(motion->mean_x==0 && motion->failure==CAL_FAILURE_NONE);
    sample.x=0; sample.z=0; CalibrateConstant(&sample);
    assert(MotionInput_GetStatus()->failure==CAL_FAILURE_GRAVITY);
    assert(!MotionInput_IsCalibrated());
    sample.z=162; CalibrateConstant(&sample);
    assert(MotionInput_GetStatus()->failure==CAL_FAILURE_GRAVITY);
    sample.z=-162; CalibrateConstant(&sample);
    assert(MotionInput_GetStatus()->failure==CAL_FAILURE_GRAVITY);
    sample.x=650; sample.z=800; CalibrateConstant(&sample);
    assert(MotionInput_GetStatus()->failure==CAL_FAILURE_TILT);
    sample.x=0; sample.z=-1008; CalibrateConstant(&sample);
    assert(MotionInput_IsCalibrated());
    MotionInput_CalibrateStart();
    for (i=0u;i<CALIBRATION_SAMPLES;i++)
    { sample.x=i==0u ? 144 : 0; (void)MotionInput_CalibrateSample(&sample); }
    assert(MotionInput_GetStatus()->failure==CAL_FAILURE_MOTION);
    assert(MotionInput_GetStatus()->span_x==144u);
    assert(!MotionInput_IsCalibrated());
    MotionInput_CalibrateStart();
    for (i=0u;i<CALIBRATION_SAMPLES;i++)
    { sample.y=(i & 1u) ? -32767-1 : 32767; (void)MotionInput_CalibrateSample(&sample); }
    assert(MotionInput_GetStatus()->span_y==65535u);
    assert(MotionInput_GetStatus()->failure==CAL_FAILURE_MOTION);
    puts("calibration: settling ignores samples, bounded noise, motion/gravity/tilt reasons and full-span arithmetic passed");
}
static void TestKeys(void)
{
    KeyFilter key;
    KeyFilter_Init(&key);
    assert(KeyFilter_Update(&key,1u,0UL)==0u);
    assert(KeyFilter_Update(&key,0u,10UL)==0u);
    assert(KeyFilter_Update(&key,1u,20UL)==0u);
    assert(KeyFilter_Update(&key,1u,50UL)==0u);
    assert(KeyFilter_Update(&key,0u,100UL)==0u);
    assert(KeyFilter_Update(&key,1u,110UL)==0u);
    assert(KeyFilter_Update(&key,0u,120UL)==0u);
    assert(KeyFilter_Update(&key,0u,150UL)==1u);
    assert(KeyFilter_Update(&key,0u,200UL)==0u);
    assert(KeyFilter_Update(&key,1u,300UL)==0u);
    assert(KeyFilter_Update(&key,1u,330UL)==0u);
    assert(KeyFilter_Update(&key,1u,1130UL)==2u);
    assert(KeyFilter_Update(&key,1u,1200UL)==0u);
    assert(KeyFilter_Update(&key,0u,1250UL)==0u);
    assert(KeyFilter_Update(&key,0u,1280UL)==0u);
    puts("keys: bounce rejection, one short event, one long event, no duplicate passed");
}
static void TestLCD(void)
{
    const uint8_t *buf;
    uint16_t i;
    LCD_Init();LCD_DrawPixel(101u,63u,1u);buf=LCD_GetBuffer();
    assert(buf[815]==1u);
    LCD_DrawPixel(102u,64u,1u);assert(buf[815]==1u);
    LCD_Clear();LCD_DrawText(0u,0u,"MAZE 123");
    for (i=0u;i<816u && buf[i]==0u;i++) {}
    assert(i<816u);LCD_DrawRect(250u,250u,20u,20u,1u);
    assert(LCD_Flush());
    puts("LCD: ASCII, bottom-right pixel, clipping and full frame passed");
}
static void RunApp(uint32_t now,KeyEvent event)
{ fake_now=now;fake_key=event;App_RunOnce(now); }
static void TestApp(void)
{
    uint32_t now;
    HostSave_Reset();
    App_Init();assert(g_debug.lcd_ok);
    RunApp(20UL,KEY_EVENT_1_PRESS);
    for (now=40UL;now<=1000UL;now+=20UL)
    {
        fake_sample.x=(now & 0x10u) ? -900 : 900;
        RunApp(now,now==500UL ? KEY_EVENT_1_PRESS : KEY_EVENT_NONE);
        assert(g_debug.calibration_samples==0u);
        assert(MotionInput_GetStatus()->calibration==CAL_SETTLING);
    }
    fake_sample.x=-72;
    for (now=1020UL;now<=2300UL;now+=20UL) RunApp(now,KEY_EVENT_NONE);
    assert(g_debug.state==APP_STATE_MENU);
    RunApp(2320UL,KEY_EVENT_1_PRESS);assert(g_debug.state==APP_STATE_PLAYING);
    RunApp(3320UL,KEY_EVENT_1_PRESS);assert(g_debug.state==APP_STATE_PAUSED);
    RunApp(9320UL,KEY_EVENT_NONE);assert(Game_GetElapsedMilliseconds()==1000UL);
    RunApp(9340UL,KEY_EVENT_1_PRESS);assert(g_debug.state==APP_STATE_PLAYING);
    RunApp(10340UL,KEY_EVENT_NONE);assert(Game_GetElapsedMilliseconds()==2000UL);
    fake_read=ACCEL_READ_ERROR;
    RunApp(10360UL,KEY_EVENT_NONE);assert(g_debug.state==APP_STATE_ERROR);
    fake_read=ACCEL_READ_OK;
    RunApp(10380UL,KEY_EVENT_1_PRESS);assert(g_debug.state==APP_STATE_CALIBRATION);
    fake_read=ACCEL_READ_WAIT;
    RunApp(10700UL,KEY_EVENT_NONE);assert(g_debug.state==APP_STATE_ERROR);
    fake_read=ACCEL_READ_OK;
    puts("application: calibration/menu/play/pause, sensor failure and stale-data handling passed");
}
static void TestCalibrationAppRollover(void)
{
    uint32_t start=0xFFFFFF00UL;
    uint16_t passed;
    HostSave_Reset();
    fake_now=start; App_Init();
    RunApp(start,KEY_EVENT_1_PRESS);
    for (passed=20u;passed<CALIBRATION_SETTLE_MS;passed+=20u)
    {
        RunApp(start+passed,KEY_EVENT_NONE);
        assert(g_debug.calibration_samples==0u);
    }
    RunApp(start+CALIBRATION_SETTLE_MS,KEY_EVENT_NONE);
    assert(g_debug.calibration_samples==1u);
    RunApp(start+CALIBRATION_SETTLE_MS+20UL,KEY_EVENT_1_LONG);
    assert(MotionInput_GetStatus()->calibration==CAL_IDLE);
    assert(g_debug.calibration_samples==0u);
    RunApp(start+CALIBRATION_SETTLE_MS+40UL,KEY_EVENT_1_PRESS);
    assert(MotionInput_GetStatus()->calibration==CAL_SETTLING);
    assert(g_debug.calibration_samples==0u);
    fake_sample.z=0;
    for (passed=20u;passed<=2300u;passed+=20u)
        RunApp(start+CALIBRATION_SETTLE_MS+40UL+passed,KEY_EVENT_NONE);
    assert(g_debug.state==APP_STATE_CALIBRATION);
    assert(g_debug.calibration_failure==CAL_FAILURE_GRAVITY);
    assert(g_debug.calibration_mean_mg.z==0);
    fake_sample.z=1008;
    start=start+CALIBRATION_SETTLE_MS+2400UL;
    RunApp(start,KEY_EVENT_1_PRESS);
    assert(g_debug.calibration_failure==CAL_FAILURE_NONE);
    for (passed=20u;passed<=2300u;passed+=20u)
        RunApp(start+passed,KEY_EVENT_NONE);
    assert(g_debug.state==APP_STATE_MENU);
    assert(g_debug.calibration_mean_mg.z==1008);
    puts("application calibration: one-second settling, repeated short press ignored, cancellation and tick rollover passed");
    puts("application calibration: gravity failure is reported, retry clears failure and reaches menu passed");
}
static void TestSensorDiagnosticsApp(void)
{
    uint16_t before;
    fake_sensor_ok=1u; fake_read=ACCEL_READ_OK; fake_now=40000UL;
    memset(&fake_diagnostics,0,sizeof(fake_diagnostics));
    fake_diagnostics.ctrl=0x94u; fake_diagnostics.id=0x12u;
    HostSave_Reset();
    App_Init();
    assert(g_debug.accel_diag.ctrl==0x94u && g_debug.accel_diag.id==0x12u);
    RunApp(40020UL,KEY_EVENT_2_LONG);
    assert(g_debug.state==APP_STATE_DIAGNOSTICS);
    fake_sample.z=162; fake_diagnostics.raw_counts.z=9;
    fake_diagnostics.sample_valid=1u; fake_diagnostics.bus_ok=1u;
    RunApp(40040UL,KEY_EVENT_NONE);
    assert(g_debug.raw_mg.z==162 && g_debug.accel_diag.raw_counts.z==9);
    assert(g_debug.accel_diag.sample_valid && g_debug.accel_diag.bus_ok);
    before=fake_init_calls;
    RunApp(40060UL,KEY_EVENT_1_PRESS);
    assert(fake_init_calls==before+1u && g_debug.state==APP_STATE_CALIBRATION);
    assert(!MotionInput_IsCalibrated() && MotionInput_GetStatus()->calibration==CAL_IDLE);
    RunApp(40080UL,KEY_EVENT_2_LONG);
    fake_sensor_ok=0u; fake_diagnostics.ctrl=0x14u;
    before=fake_init_calls;
    RunApp(40100UL,KEY_EVENT_1_PRESS);
    assert(fake_init_calls==before+1u && g_debug.state==APP_STATE_ERROR);
    assert(g_debug.error==1u && g_debug.accel_diag.ctrl==0x14u && g_debug.sensor_type==0u);
    fake_sensor_ok=1u; fake_diagnostics.ctrl=0x94u;
    RunApp(40120UL,KEY_EVENT_1_PRESS);
    assert(g_debug.state==APP_STATE_CALIBRATION && g_debug.error==0u);
    fake_sample.z=1008;
    puts("application diagnostics: raw/register snapshot, S1 reinitialization, failure and recovery passed");
}
/* ---- trap / locked exit / save / level lock --------------------------------- */
static int FindTrapRoute(const MazeLevel *map, uint8_t *px, uint8_t *py, int max_len)
{
    int16_t prev[TEST_CELLS];
    int16_t queue[TEST_CELLS];
    int head=0, tail=0, i, d, start, cell, cx, cy, nx, ny, next;
    int dx[4]={1,-1,0,0}, dy[4]={0,0,1,-1};
    for (i=0;i<TEST_CELLS;i++) prev[i]=-1;
    start=(int)map->start_y*(int)MAZE_COLUMNS+(int)map->start_x;
    prev[start]=(int16_t)start; queue[tail++]=(int16_t)start;
    while (head<tail)
    {
        cell=(int)queue[head++];
        cx=cell%(int)MAZE_COLUMNS; cy=cell/(int)MAZE_COLUMNS;
        for (d=0;d<4;d++)
        {
            nx=cx+dx[d]; ny=cy+dy[d];
            if (nx<0||ny<0||nx>=(int)MAZE_COLUMNS||ny>=(int)MAZE_ROWS) continue;
            next=ny*(int)MAZE_COLUMNS+nx;
            if (prev[next]!=-1) continue;
            if (GameMap_IsWall(map,(uint8_t)nx,(uint8_t)ny)) continue;
            if (GameMap_IsExit(map,(uint8_t)nx,(uint8_t)ny)) continue;
            prev[next]=(int16_t)cell;
            if (GameMap_IsTrap(map,(uint8_t)nx,(uint8_t)ny))
                return ReversePath(prev,start,next,px,py,max_len);
            queue[tail++]=(int16_t)next;
        }
    }
    return 0;
}
static int FindExitNeighbourRoute(const MazeLevel *map, uint8_t *px, uint8_t *py, int max_len)
{
    int16_t prev[TEST_CELLS];
    int16_t queue[TEST_CELLS];
    int head=0, tail=0, i, d, start, cell, cx, cy, nx, ny, next;
    int ex=(int)map->end_x, ey=(int)map->end_y;
    int dx[4]={1,-1,0,0}, dy[4]={0,0,1,-1};
    for (i=0;i<TEST_CELLS;i++) prev[i]=-1;
    start=(int)map->start_y*(int)MAZE_COLUMNS+(int)map->start_x;
    prev[start]=(int16_t)start; queue[tail++]=(int16_t)start;
    while (head<tail)
    {
        cell=(int)queue[head++];
        cx=cell%(int)MAZE_COLUMNS; cy=cell/(int)MAZE_COLUMNS;
        for (d=0;d<4;d++)
        {
            nx=cx+dx[d]; ny=cy+dy[d];
            if (nx<0||ny<0||nx>=(int)MAZE_COLUMNS||ny>=(int)MAZE_ROWS) continue;
            next=ny*(int)MAZE_COLUMNS+nx;
            if (prev[next]!=-1) continue;
            if (GameMap_IsWall(map,(uint8_t)nx,(uint8_t)ny)) continue;
            if (GameMap_IsTrap(map,(uint8_t)nx,(uint8_t)ny)) continue;
            if (GameMap_IsItem(map,(uint8_t)nx,(uint8_t)ny)) continue;
            if (GameMap_IsExit(map,(uint8_t)nx,(uint8_t)ny)) continue;
            prev[next]=(int16_t)cell;
            if (AbsI(nx-ex)+AbsI(ny-ey)==1)
                return ReversePath(prev,start,next,px,py,max_len);
            queue[tail++]=(int16_t)next;
        }
    }
    return 0;
}
static void TestTrapAndExitLock(void)
{
    uint8_t level, px[256], py[256];
    int found_trap=0, found_lock=0;
    for (level=0u; level<MAZE_LEVEL_COUNT && (!found_trap || !found_lock); level++)
    {
        const MazeLevel *map=GameMap_Get(level);
        int len, i;
        uint32_t now;
        if (!found_trap)
        {
            len=FindTrapRoute(map,px,py,256);
            if (len>1)
            {
                found_trap=1;
                Game_Init(level); Game_SetRunning(1u,1000UL);
                now=1000UL;
                for (i=1;i<len;i++)
                {
                    now+=GAME_MOVE_SLOW_MS;
                    Game_Update(DirTo(px[i-1],py[i-1],px[i],py[i]),0u,now);
                }
                assert(Game_GetPlayerX()==map->start_x && Game_GetPlayerY()==map->start_y);
                assert(Game_GetTrapHits()==1u);
                assert(Game_GetNotice()==GAME_NOTICE_TRAP);
                assert(Game_GetElapsedMilliseconds()>=GAME_TRAP_PENALTY_MS);
            }
        }
        if (!found_lock)
        {
            len=FindExitNeighbourRoute(map,px,py,256);
            if (len>1)
            {
                found_lock=1;
                Game_Init(level); Game_SetRunning(1u,1000UL);
                assert(Game_GetItemsLeft()>0u);
                now=1000UL;
                for (i=1;i<len;i++)
                {
                    now+=GAME_MOVE_SLOW_MS;
                    Game_Update(DirTo(px[i-1],py[i-1],px[i],py[i]),0u,now);
                }
                assert(Game_GetPlayerX()==px[len-1] && Game_GetPlayerY()==py[len-1]);
                now+=GAME_MOVE_SLOW_MS;
                Game_Update(DirTo(px[len-1],py[len-1],map->end_x,map->end_y),0u,now);
                assert(Game_GetPlayerX()==px[len-1] && Game_GetPlayerY()==py[len-1]);
                assert(Game_GetNotice()==GAME_NOTICE_LOCKED);
                assert(!Game_IsCleared());
            }
        }
    }
    assert(found_trap && found_lock);
    puts("gameplay: trap teleport + time penalty and locked exit notice passed");
}
static void TestSavePersistence(void)
{
    SaveData a, b;
    uint8_t flags;
    HostSave_Reset();
    Save_Defaults(&a);
    assert(a.unlocked==1u && a.best_ms[0]==0UL);
    assert(!Save_Load(&b));
    assert(b.unlocked==1u);
    a.unlocked=2u; a.best_ms[0]=12345UL; a.best_steps[0]=26u;
    assert(Save_Store(&a));
    assert(Save_Load(&b));
    assert(b.unlocked==2u && b.best_ms[0]==12345UL && b.best_steps[0]==26u);
    a.best_ms[1]=777UL; a.unlocked=3u;
    assert(Save_Store(&a));
    assert(Save_Load(&b));
    assert(b.unlocked==3u && b.best_ms[1]==777UL);
    /* Corrupt the newest slot: load falls back to the older valid record. */
    HostSave_Corrupt(1u,0u);
    assert(Save_Load(&b));
    assert(b.unlocked==2u && b.best_ms[0]==12345UL);
    /* Corrupt both: no valid record, defaults are returned. */
    HostSave_Corrupt(0u,0u);
    assert(!Save_Load(&b));
    assert(b.unlocked==1u);
    /* ApplyResult: best time, best steps and unlock progression. */
    Save_Defaults(&a);
    flags=Save_ApplyResult(&a,0u,9000UL,30u);
    assert((flags&SAVE_RESULT_BEST)!=0u && (flags&SAVE_RESULT_UNLOCK)!=0u);
    assert(a.unlocked==2u && a.best_ms[0]==9000UL && a.best_steps[0]==30u);
    flags=Save_ApplyResult(&a,0u,12000UL,20u);
    assert(flags==SAVE_RESULT_NONE);
    flags=Save_ApplyResult(&a,0u,8000UL,40u);
    assert((flags&SAVE_RESULT_BEST)!=0u && a.best_ms[0]==8000UL);
    /* A failed erase must not corrupt the in-RAM state or hang. */
    HostSave_Reset();
    assert(Save_Store(&a));
    HostSave_FailErase(1u);
    assert(!Save_Store(&a));
    HostSave_FailErase(0u);
    assert(Save_Load(&b));
    assert(b.best_ms[0]==8000UL);
    puts("save: defaults, double-buffer store/load, checksum fallback, apply-result and erase failure passed");
}
static void AppToMenu(uint32_t start)
{
    uint32_t now;
    fake_sample.x=0; fake_sample.y=0; fake_sample.z=1008;
    fake_now=start;
    App_Init();
    RunApp(start+20UL,KEY_EVENT_1_PRESS);
    for (now=start+40UL; now<=start+2500UL; now+=20UL)
        RunApp(now,KEY_EVENT_NONE);
    assert(g_debug.state==APP_STATE_MENU);
}
static void TestMenuLock(void)
{
    SaveData d;
    HostSave_Reset();
    Save_Defaults(&d);
    assert(Save_Store(&d));
    AppToMenu(70000UL);
    assert(g_debug.unlocked==1u);
    RunApp(75000UL,KEY_EVENT_2_PRESS);
    assert(g_debug.state==APP_STATE_MENU);
    RunApp(75020UL,KEY_EVENT_1_PRESS);
    assert(g_debug.state==APP_STATE_MENU); /* level 2 is still locked */
    Save_Defaults(&d);
    d.unlocked=MAZE_LEVEL_COUNT;
    assert(Save_Store(&d));
    AppToMenu(80000UL);
    assert(g_debug.unlocked==MAZE_LEVEL_COUNT);
    RunApp(85000UL,KEY_EVENT_2_PRESS);
    RunApp(85020UL,KEY_EVENT_1_PRESS);
    assert(g_debug.state==APP_STATE_PLAYING);
    puts("menu: locked level refused, saved unlock allows selection passed");
}
static void TestTuning(void)
{
    HostSave_Reset();
    Tuning_Defaults();
    assert(g_tuning.trigger_mg==(uint16_t)MOTION_TRIGGER_MG);
    assert(g_tuning.release_mg==(uint16_t)MOTION_RELEASE_MG);
    assert(g_tuning.margin_mg==(uint16_t)MOTION_AXIS_MARGIN_MG);
    assert(g_tuning.slow_ms==(uint16_t)GAME_MOVE_SLOW_MS);
    assert(g_tuning.fast_ms==(uint16_t)GAME_MOVE_FAST_MS);
    assert(g_tuning.full_mg==(uint16_t)GAME_TILT_FULL_MG);
    assert(g_tuning.filter_num==(uint16_t)MOTION_FILTER_WEIGHT);
    assert(Tuning_Interval(g_tuning.trigger_mg)==g_tuning.slow_ms);
    assert(Tuning_Interval(g_tuning.full_mg)==g_tuning.fast_ms);
    assert(Tuning_Interval(300u)>g_tuning.fast_ms && Tuning_Interval(300u)<g_tuning.slow_ms);
    /* Clamp keeps release<trigger, fast<slow, full>trigger and filter 1..3. */
    g_tuning.trigger_mg=10u; g_tuning.release_mg=300u; g_tuning.margin_mg=0u;
    g_tuning.slow_ms=500u; g_tuning.fast_ms=10u; g_tuning.full_mg=100u; g_tuning.filter_num=0u;
    Tuning_Clamp();
    assert(g_tuning.trigger_mg==50u && g_tuning.release_mg==30u && g_tuning.margin_mg==10u);
    assert(g_tuning.slow_ms==400u && g_tuning.fast_ms==60u && g_tuning.full_mg==100u);
    assert(g_tuning.filter_num==1u);
    /* Save/load through INFO B (save-port slot 2). */
    Tuning_Defaults();
    g_tuning.trigger_mg=150u; g_tuning.release_mg=90u; g_tuning.margin_mg=50u;
    g_tuning.slow_ms=200u; g_tuning.fast_ms=100u; g_tuning.full_mg=500u;
    g_tuning.filter_num=3u;
    assert(Tuning_Save());
    Tuning_Defaults();
    assert(Tuning_Load());
    assert(g_tuning.trigger_mg==150u && g_tuning.release_mg==90u);
    assert(g_tuning.margin_mg==50u && g_tuning.slow_ms==200u);
    assert(g_tuning.fast_ms==100u && g_tuning.full_mg==500u);
    assert(g_tuning.filter_num==3u);
    /* Corrupt record: load falls back to defaults. */
    HostSave_Corrupt(2u,0u);
    Tuning_Defaults();
    assert(!Tuning_Load());
    assert(g_tuning.trigger_mg==(uint16_t)MOTION_TRIGGER_MG);
    Tuning_Defaults();
    puts("tuning: defaults, interval, clamp, filter weight, INFO B save/load and corrupt fallback passed");
}
static void TestTuningApp(void)
{
    SaveData d;
    HostSave_Reset();
    Save_Defaults(&d);
    assert(Save_Store(&d));
    Tuning_Defaults();
    AppToMenu(90000UL);
    RunApp(95000UL,KEY_EVENT_2_LONG);   /* menu -> diagnostics */
    assert(g_debug.state==APP_STATE_DIAGNOSTICS);
    RunApp(95020UL,KEY_EVENT_2_LONG);   /* diagnostics -> tuning */
    assert(g_debug.state==APP_STATE_TUNING);
    assert(g_debug.tuning_item==0u);
    RunApp(95040UL,KEY_EVENT_2_PRESS);  /* select RELEASE */
    assert(g_debug.tuning_item==1u);
    RunApp(95060UL,KEY_EVENT_1_PRESS);  /* S1 short: back to TRIGGER */
    assert(g_debug.tuning_item==0u);
    RunApp(95080UL,KEY_EVENT_2_PRESS);  /* next: RELEASE again */
    assert(g_debug.tuning_item==1u);
    fake_wheel=1;
    RunApp(95100UL,KEY_EVENT_NONE);     /* wheel +1 tick: +10 mg */
    assert(g_tuning.release_mg==(uint16_t)(MOTION_RELEASE_MG+10));
    fake_wheel=-1;
    RunApp(95120UL,KEY_EVENT_NONE);     /* wheel -1 tick */
    assert(g_tuning.release_mg==(uint16_t)MOTION_RELEASE_MG);
    fake_wheel=1;
    RunApp(95140UL,KEY_EVENT_NONE);     /* +10 again, then save */
    assert(g_tuning.release_mg==(uint16_t)(MOTION_RELEASE_MG+10));
    RunApp(95160UL,KEY_EVENT_2_LONG);   /* hold S2: save & back to menu */
    assert(g_debug.state==APP_STATE_MENU);
    assert(g_debug.tuning_saved==1u);
    Tuning_Defaults();
    assert(Tuning_Load());
    assert(g_tuning.release_mg==(uint16_t)(MOTION_RELEASE_MG+10));
    Tuning_Defaults();
    puts("tuning app: diagnostics->TUNE, wheel adjust, S1/S2 select, hold-S2 save and reload passed");
}
static void TestScreens(void)
{
    uint8_t i;
    g_debug.unlocked = 3u;
    g_debug.new_best = 1u;
    for (i = 0u; i < MAZE_LEVEL_COUNT; i++)
    {
        Game_Init(i);
        Game_SetRunning(1u, 0UL);
        (void)UI_DrawState(APP_STATE_PLAYING, i, 42u, 12345UL);
        (void)UI_DrawState(APP_STATE_MENU, i, 0u, 12345UL);
        (void)UI_DrawState(APP_STATE_CLEAR, i, 42u, 12345UL);
        (void)UI_DrawState(APP_STATE_PAUSED, i, 0u, 0UL);
    }
    g_debug.new_best = 0u;
    (void)UI_DrawState(APP_STATE_CALIBRATION, 0u, 0u, 0UL);
    (void)UI_DrawState(APP_STATE_DIAGNOSTICS, 0u, 0u, 0UL);
    g_debug.tuning_item = 0u;
    g_debug.tuning_saved = 0u;
    (void)UI_DrawState(APP_STATE_TUNING, 0u, 0u, 0UL);
    g_debug.sensor_type = 0u;
    (void)UI_DrawState(APP_STATE_ERROR, 0u, 0u, 0UL);
    puts("screens: gameplay/menu/clear/paused/calibration/diagnostics/tuning/error frames rendered");
}
int main(void)
{
    TestMapsAndCompletion();TestGameTimeAndRate();TestMotion();TestCalibrationReasons();
    TestKeys();TestLCD();TestApp();TestCalibrationAppRollover();
    TestSensorDiagnosticsApp();
    TestTrapAndExitLock();TestSavePersistence();TestMenuLock();
    TestTuning();TestTuningApp();TestScreens();
    puts("ALL HOST TESTS PASSED (no physical hardware is exercised)");
    return 0;
}
