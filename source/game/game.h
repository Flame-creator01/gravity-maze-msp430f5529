#ifndef GAME_H
#define GAME_H
#include "app_types.h"
#define GAME_NOTICE_NONE 0u
#define GAME_NOTICE_TRAP 1u
#define GAME_NOTICE_LOCKED 2u
void Game_Init(uint8_t level);
void Game_SetRunning(uint8_t running, uint32_t now);
void Game_Update(MotionDirection direction, uint16_t tilt_mg, uint32_t now);
uint8_t Game_IsCleared(void);
uint8_t Game_GetLevel(void);
uint8_t Game_GetPlayerX(void);
uint8_t Game_GetPlayerY(void);
uint16_t Game_GetSteps(void);
uint32_t Game_GetElapsedMilliseconds(void);
uint16_t Game_GetElapsedSeconds(void);
uint8_t Game_GetItemsLeft(void);
uint8_t Game_GetItemsTotal(void);
uint8_t Game_IsExitOpen(void);
uint8_t Game_ItemAt(uint8_t x, uint8_t y);
uint8_t Game_GetNotice(void);
uint8_t Game_GetTrapHits(void);
#endif
