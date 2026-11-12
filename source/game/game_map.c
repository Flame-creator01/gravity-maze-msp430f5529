#include "game_map.h"
/* Six fixed levels. 'o' = energy point, 'x' = trap, 'E' = exit.
   Every level is checked by tools/test_host.ps1: all items are reachable and the
   exit can be reached after collecting them without stepping on a trap. */
static const char l0[MAZE_ROWS][MAZE_COLUMNS+1u] = {
    "###############",
    "# #          o#",
    "# # # # # # # #",
    "# # #         #",
    "# # ##    ### #",
    "#             #",
    "# ## #  # #x  #",
    "#            E#",
    "###############"
};
/* level 1: 1 item at (13,1); 1 trap at (11,6) */
static const char l1[MAZE_ROWS][MAZE_COLUMNS+1u] = {
    "###############",
    "#             #",
    "# # ##### # # #",
    "#         #   #",
    "# #### ## # # #",
    "#   #       # #",
    "# #     #   #o#",
    "# #x  #    o E#",
    "###############"
};
/* level 2: 2 items at (13,6),(11,7); 1 trap at (3,7) */
static const char l2[MAZE_ROWS][MAZE_COLUMNS+1u] = {
    "###############",
    "#         #  o#",
    "### ### # # ###",
    "# #       #   #",
    "# ### # # # # #",
    "#           #o#",
    "# #x#  #### # #",
    "# #     x    E#",
    "###############"
};
/* level 3: 2 items at (13,5),(13,1); 2 traps at (3,6),(8,7) */
static const char l3[MAZE_ROWS][MAZE_COLUMNS+1u] = {
    "###############",
    "# #         o #",
    "# # # # ## ## #",
    "# # # #     #o#",
    "# ###   ### # #",
    "#   # #   #   #",
    "# #   ### #####",
    "#x x  #     oE#",
    "###############"
};
/* level 4: 3 items at (13,3),(12,7),(12,1); 2 traps at (1,7),(3,7) */
static const char l4[MAZE_ROWS][MAZE_COLUMNS+1u] = {
    "###############",
    "# #  o  o   # #",
    "# # ####### # #",
    "# #          x#",
    "# ### ### ###x#",
    "#   #  o#   x #",
    "### ##### # ###",
    "#         #  E#",
    "###############"
};
/* level 5: 3 items at (5,1),(7,5),(8,1); 3 traps at (13,4),(13,3),(12,5) */
static const char l5[MAZE_ROWS][MAZE_COLUMNS+1u] = {
    "###############",
    "# #o  o       #",
    "# ##### ##### #",
    "#     # #o#   #",
    "##### # # # # #",
    "# x # # o # # #",
    "#x#x# ##### # #",
    "# #         #E#",
    "###############"
};
/* level 6: 4 items at (9,3),(8,5),(3,1),(6,1); 3 traps at (2,5),(3,6),(1,6) */
static const MazeLevel levels[MAZE_LEVEL_COUNT] = {
    {1u,1u,13u,7u,{l0[0],l0[1],l0[2],l0[3],l0[4],l0[5],l0[6],l0[7],l0[8]}},
    {1u,1u,13u,7u,{l1[0],l1[1],l1[2],l1[3],l1[4],l1[5],l1[6],l1[7],l1[8]}},
    {1u,1u,13u,7u,{l2[0],l2[1],l2[2],l2[3],l2[4],l2[5],l2[6],l2[7],l2[8]}},
    {1u,1u,13u,7u,{l3[0],l3[1],l3[2],l3[3],l3[4],l3[5],l3[6],l3[7],l3[8]}},
    {1u,1u,13u,7u,{l4[0],l4[1],l4[2],l4[3],l4[4],l4[5],l4[6],l4[7],l4[8]}},
    {1u,1u,13u,7u,{l5[0],l5[1],l5[2],l5[3],l5[4],l5[5],l5[6],l5[7],l5[8]}}
};

static char Cell(const MazeLevel *map, uint8_t x, uint8_t y)
{
    if (map == 0 || x >= MAZE_COLUMNS || y >= MAZE_ROWS) return MAP_WALL;
    return map->rows[y][x];
}
const MazeLevel *GameMap_Get(uint8_t level)
{
    return &levels[level < MAZE_LEVEL_COUNT ? level : 0u];
}
uint8_t GameMap_IsWall(const MazeLevel *map, uint8_t x, uint8_t y)
{
    return (uint8_t)(Cell(map, x, y) == MAP_WALL);
}
uint8_t GameMap_IsItem(const MazeLevel *map, uint8_t x, uint8_t y)
{
    return (uint8_t)(Cell(map, x, y) == MAP_ITEM);
}
uint8_t GameMap_IsTrap(const MazeLevel *map, uint8_t x, uint8_t y)
{
    return (uint8_t)(Cell(map, x, y) == MAP_TRAP);
}
uint8_t GameMap_IsExit(const MazeLevel *map, uint8_t x, uint8_t y)
{
    return (uint8_t)(Cell(map, x, y) == MAP_EXIT);
}
