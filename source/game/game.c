#include "app_config.h"
#include "game_map.h"
#include "game.h"
#include "tuning.h"
static const MazeLevel *map;
static uint8_t level, x, y, cleared, running;
static uint16_t steps;
static uint32_t elapsed, last_tick, last_move;
static uint8_t item_x[GAME_MAX_ITEMS], item_y[GAME_MAX_ITEMS], item_taken[GAME_MAX_ITEMS];
static uint8_t item_count, item_left;
static uint8_t notice, trap_hits;
static uint32_t notice_until;

static void AdvanceTime(uint32_t now)
{
    if (running && !cleared) elapsed += (uint32_t)(now-last_tick);
    last_tick = now;
}
/* Scan the level once; item order matches the row-major scan order used by tests. */
static void ScanItems(void)
{
    uint8_t cx, cy;
    item_count = 0u; item_left = 0u;
    for (cy = 0u; cy < MAZE_ROWS; cy++)
        for (cx = 0u; cx < MAZE_COLUMNS; cx++)
            if (map->rows[cy][cx] == MAP_ITEM && item_count < GAME_MAX_ITEMS)
            {
                item_x[item_count] = cx; item_y[item_count] = cy;
                item_taken[item_count] = 0u;
                item_count++; item_left++;
            }
}
static int16_t FindItem(uint8_t cx, uint8_t cy)
{
    uint8_t i;
    for (i = 0u; i < item_count; i++)
        if (item_taken[i] == 0u && item_x[i] == cx && item_y[i] == cy) return (int16_t)i;
    return -1;
}
/* 0 = slowest (light tilt), 1 = fastest (strong tilt). */
void Game_Init(uint8_t selected)
{
    uint8_t i;
    level = selected < MAZE_LEVEL_COUNT ? selected : 0u;
    map = GameMap_Get(level); x = map->start_x; y = map->start_y;
    steps = 0u; cleared = 0u; running = 0u; elapsed = 0UL;
    last_tick = 0UL; last_move = 0UL;
    notice = GAME_NOTICE_NONE; notice_until = 0UL; trap_hits = 0u;
    ScanItems();
    for (i = 0u; i < item_count; i++) item_taken[i] = 0u;
}
void Game_SetRunning(uint8_t enabled, uint32_t now)
{
    AdvanceTime(now);
    running = (uint8_t)(enabled != 0u && !cleared);
    last_move = now - GAME_MOVE_SLOW_MS;
    if (!running) notice = GAME_NOTICE_NONE;
}
void Game_Update(MotionDirection direction, uint16_t tilt_mg, uint32_t now)
{
    int16_t nx, ny, idx;
    uint16_t interval;
    AdvanceTime(now);
    if (notice != GAME_NOTICE_NONE && (int32_t)(now-notice_until) >= 0) notice = GAME_NOTICE_NONE;
    if (!running || cleared) return;
    if (direction == MOTION_STOP) return;
    interval = Tuning_Interval(tilt_mg);
    if ((uint32_t)(now-last_move) < (uint32_t)interval) return;
    last_move = now;
    nx = x; ny = y;
    if (direction == MOTION_UP) ny--;
    else if (direction == MOTION_DOWN) ny++;
    else if (direction == MOTION_LEFT) nx--;
    else if (direction == MOTION_RIGHT) nx++;
    else return;
    if (nx < 0 || ny < 0 || nx >= (int16_t)MAZE_COLUMNS || ny >= (int16_t)MAZE_ROWS) return;
    if (GameMap_IsWall(map, (uint8_t)nx, (uint8_t)ny)) return;
    /* The exit stays shut until every energy point has been collected. */
    if (GameMap_IsExit(map, (uint8_t)nx, (uint8_t)ny) && item_left != 0u)
    {
        notice = GAME_NOTICE_LOCKED;
        notice_until = now + 1000UL;
        return;
    }
    if (steps < 65535u) steps++;
    if (GameMap_IsTrap(map, (uint8_t)nx, (uint8_t)ny))
    {
        trap_hits++;
        elapsed += GAME_TRAP_PENALTY_MS;
#if GAME_TRAP_RESET
        x = map->start_x; y = map->start_y;
#else
        x = (uint8_t)nx; y = (uint8_t)ny;
#endif
        notice = GAME_NOTICE_TRAP;
        notice_until = now + 1200UL;
        return;
    }
    x = (uint8_t)nx; y = (uint8_t)ny;
    idx = FindItem(x, y);
    if (idx >= 0)
    {
        item_taken[idx] = 1u;
        if (item_left != 0u) item_left--;
    }
    if (GameMap_IsExit(map, x, y) && item_left == 0u)
    {
        cleared = 1u; running = 0u;
    }
}
uint8_t Game_IsCleared(void) { return cleared; }
uint8_t Game_GetLevel(void) { return level; }
uint8_t Game_GetPlayerX(void) { return x; }
uint8_t Game_GetPlayerY(void) { return y; }
uint16_t Game_GetSteps(void) { return steps; }
uint32_t Game_GetElapsedMilliseconds(void) { return elapsed; }
uint16_t Game_GetElapsedSeconds(void)
{
    uint32_t seconds = elapsed / 1000UL;
    return (uint16_t)(seconds > 65535UL ? 65535UL : seconds);
}
uint8_t Game_GetItemsLeft(void) { return item_left; }
uint8_t Game_GetItemsTotal(void) { return item_count; }
uint8_t Game_IsExitOpen(void) { return (uint8_t)(item_left == 0u); }
uint8_t Game_ItemAt(uint8_t cx, uint8_t cy)
{
    return (uint8_t)(FindItem(cx, cy) >= 0);
}
uint8_t Game_GetNotice(void) { return notice; }
uint8_t Game_GetTrapHits(void) { return trap_hits; }
