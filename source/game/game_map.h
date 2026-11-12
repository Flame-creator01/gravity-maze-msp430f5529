#ifndef GAME_MAP_H
#define GAME_MAP_H
#include <stdint.h>
#include "app_config.h"
#define MAP_WALL '#'
#define MAP_FLOOR ' '
#define MAP_ITEM 'o'
#define MAP_TRAP 'x'
#define MAP_EXIT 'E'
typedef struct { uint8_t start_x, start_y, end_x, end_y; const char *rows[MAZE_ROWS]; } MazeLevel;
const MazeLevel *GameMap_Get(uint8_t level);
uint8_t GameMap_IsWall(const MazeLevel *map, uint8_t x, uint8_t y);
uint8_t GameMap_IsItem(const MazeLevel *map, uint8_t x, uint8_t y);
uint8_t GameMap_IsTrap(const MazeLevel *map, uint8_t x, uint8_t y);
uint8_t GameMap_IsExit(const MazeLevel *map, uint8_t x, uint8_t y);
#endif
