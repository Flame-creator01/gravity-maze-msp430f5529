#include "ui.h"
#include "app_config.h"
#include "game_map.h"
#include "game.h"
#include "lcd.h"
#include "motion_input.h"
#include "app.h"
#include "tuning.h"
static void Label(uint8_t row, const char *text) { LCD_DrawText(0u,(uint8_t)(row*8u),text); }
static void Unsigned(uint8_t x, uint8_t y, uint32_t value, uint8_t width)
{
    char text[6]; uint8_t i;
    if (width>5u) width=5u;
    text[width]='\0';
    for (i=0u;i<width;i++) { text[width-i-1u]=(char)('0'+value%10UL); value/=10UL; }
    LCD_DrawText(x,y,text);
}
static void Signed(uint8_t x, uint8_t y, int16_t value)
{
    LCD_DrawText(x,y,value<0 ? "-" : "+");
    Unsigned((uint8_t)(x+6u),y,value<0 ? (uint32_t)(-(int32_t)value) : (uint32_t)value,4u);
}
static void HexByte(uint8_t x, uint8_t y, uint8_t value)
{
    static const char digits[] = "0123456789ABCDEF";
    char text[3];
    text[0] = digits[value >> 4]; text[1] = digits[value & 15u]; text[2] = '\0';
    LCD_DrawText(x,y,text);
}
static void SensorRegisters(uint8_t y)
{
    LCD_DrawText(0u,y,"I"); HexByte(6u,y,g_debug.accel_diag.id);
    LCD_DrawText(24u,y,"C"); HexByte(30u,y,g_debug.accel_diag.ctrl);
    LCD_DrawText(48u,y,"S"); HexByte(54u,y,g_debug.accel_diag.status);
    LCD_DrawText(72u,y,"B"); Unsigned(78u,y,g_debug.accel_diag.bus_ok,1u);
}
/* Row 0: level, steps, collected energy points and time. A notice temporarily
   replaces the right-hand section (item counter + timer). */
static void DrawHud(uint8_t level, uint16_t seconds)
{
    LCD_DrawText(0u,0u,"L"); Unsigned(6u,0u,level+1u,1u);
    LCD_DrawText(18u,0u,"S"); Unsigned(24u,0u,Game_GetSteps(),4u);
    if (Game_GetNotice()==GAME_NOTICE_TRAP) LCD_DrawText(48u,0u,GAME_TRAP_NOTICE);
    else if (Game_GetNotice()==GAME_NOTICE_LOCKED) LCD_DrawText(48u,0u,"NEED KEYS");
    else
    {
        LCD_DrawText(48u,0u,"K"); Unsigned(54u,0u,Game_GetItemsLeft(),1u);
        LCD_DrawText(60u,0u,"/"); Unsigned(66u,0u,Game_GetItemsTotal(),1u);
        LCD_DrawText(72u,0u,"T"); Unsigned(78u,0u,seconds,4u);
    }
}
static void DrawMaze(uint8_t level)
{
    uint8_t x,y,px,py,i;
    const MazeLevel *map=GameMap_Get(level);
    for (y=0u;y<MAZE_ROWS;y++) for (x=0u;x<MAZE_COLUMNS;x++)
    {
        px=(uint8_t)(MAZE_ORIGIN_X+x*MAZE_CELL_WIDTH);
        py=(uint8_t)(MAZE_ORIGIN_Y+y*MAZE_CELL_HEIGHT);
        if (GameMap_IsWall(map,x,y)) LCD_DrawRect(px,py,6u,6u,1u);
        else if (GameMap_IsExit(map,x,y))
        {
            LCD_DrawRect((uint8_t)(px+1u),(uint8_t)(py+1u),4u,4u,1u);
            /* Open exit: hollow ring. Locked exit: ring with a solid centre. */
            if (Game_IsExitOpen()) LCD_DrawRect((uint8_t)(px+2u),(uint8_t)(py+2u),2u,2u,0u);
            else LCD_DrawRect((uint8_t)(px+2u),(uint8_t)(py+2u),2u,2u,1u);
        }
        else if (Game_ItemAt(x,y)) LCD_DrawRect((uint8_t)(px+2u),(uint8_t)(py+2u),2u,2u,1u);
        else if (GameMap_IsTrap(map,x,y))
            for (i=0u;i<4u;i++)
            {
                LCD_DrawPixel((uint8_t)(px+1u+i),(uint8_t)(py+1u+i),1u);
                LCD_DrawPixel((uint8_t)(px+4u-i),(uint8_t)(py+1u+i),1u);
            }
    }
    px=(uint8_t)(MAZE_ORIGIN_X+Game_GetPlayerX()*6u+1u);
    py=(uint8_t)(MAZE_ORIGIN_Y+Game_GetPlayerY()*6u+1u);
    LCD_DrawRect(px,py,4u,4u,1u);
}
/* ---- TUNE screen helpers -------------------------------------------------- */
static const char *TuningName(uint8_t item)
{
    if (item==0u) return "TRIGGER";
    if (item==1u) return "RELEASE";
    if (item==2u) return "MARGIN";
    if (item==3u) return "SLOW MS";
    if (item==4u) return "FAST MS";
    if (item==5u) return "FULL MG";
    if (item==6u) return "FILTER";
    return "DEFAULT";
}
static uint16_t TuningValue(uint8_t item)
{
    if (item==0u) return g_tuning.trigger_mg;
    if (item==1u) return g_tuning.release_mg;
    if (item==2u) return g_tuning.margin_mg;
    if (item==3u) return g_tuning.slow_ms;
    if (item==4u) return g_tuning.fast_ms;
    if (item==5u) return g_tuning.full_mg;
    return g_tuning.filter_num;
}
static const char *DirectionName(MotionDirection direction)
{
    if (direction==MOTION_UP) return "UP";
    if (direction==MOTION_DOWN) return "DOWN";
    if (direction==MOTION_LEFT) return "LEFT";
    if (direction==MOTION_RIGHT) return "RIGHT";
    return "STOP";
}
static uint8_t TiltBarX(uint16_t mg)
{
    if (mg > 900u) mg = 900u;
    return (uint8_t)(2u + (uint16_t)(((uint32_t)mg * 97UL) / 900UL));
}
/* 0..900 mg scale: trigger and full-tilt marks, plus the live tilt marker. */
static void DrawTiltBar(uint16_t tilt_mg)
{
    LCD_DrawRect(2u,46u,98u,1u,1u);
    LCD_DrawRect(TiltBarX(g_tuning.trigger_mg),42u,1u,5u,1u);
    LCD_DrawRect(TiltBarX(g_tuning.full_mg),42u,1u,5u,1u);
    LCD_DrawRect(TiltBarX(tilt_mg),41u,2u,6u,1u);
}
uint8_t UI_DrawState(AppState state, uint8_t level, uint16_t seconds, uint32_t best_ms)
{
    const MotionStatus *motion=MotionInput_GetStatus();
    LCD_Clear();
    if (state==APP_STATE_PLAYING)
    {
        DrawMaze(level);
        DrawHud(level,seconds);
    }
    else if (state==APP_STATE_CALIBRATION)
    {
        Label(0u,"CALIBRATE"); Label(2u,"LAY BOARD FLAT");
        if (motion->calibration==CAL_SETTLING)
        { Label(4u,"HANDS OFF 1 SEC"); Label(6u,"WAIT THEN SAMPLE"); }
        else if (motion->calibration==CAL_COLLECTING)
        { Label(4u,"KEEP STILL"); Unsigned(66u,32u,motion->samples,2u); }
        else if (motion->calibration==CAL_UNSTABLE)
        {
            if (motion->failure==CAL_FAILURE_MOTION) Label(2u,"TOO MUCH MOTION");
            else if (motion->failure==CAL_FAILURE_GRAVITY) Label(2u,"CHECK Z / POSTURE");
            else Label(2u,"BOARD NOT FLAT");
            Label(4u,"DX"); Unsigned(18u,32u,motion->span_x,4u);
            LCD_DrawText(54u,32u,"DY"); Unsigned(72u,32u,motion->span_y,4u);
            Label(5u,"DZ"); Unsigned(18u,40u,motion->span_z,4u);
            LCD_DrawText(54u,40u,"Z"); Signed(66u,40u,motion->mean_z);
            Label(6u,"S1 RETRY"); Label(7u,"HOLD S2 DIAG");
        }
        else Label(4u,"S1 BEGIN");
    }
    else if (state==APP_STATE_MENU)
    {
        Label(0u,"GRAVITY MAZE");
        LCD_DrawText(0u,8u,"LEVEL"); Unsigned(36u,8u,level+1u,1u);
        LCD_DrawText(42u,8u,"/"); Unsigned(48u,8u,MAZE_LEVEL_COUNT,1u);
        LCD_DrawText(0u,16u,"BEST");
        if (best_ms==0UL) LCD_DrawText(30u,16u,"----");
        else { Unsigned(30u,16u,best_ms/1000UL,4u); LCD_DrawText(54u,16u,"S"); }
        LCD_DrawText(0u,24u,"PROGRESS"); Unsigned(54u,24u,g_debug.unlocked,1u);
        LCD_DrawText(60u,24u,"/"); Unsigned(66u,24u,MAZE_LEVEL_COUNT,1u);
        Label(4u,"S1 START S2 NEXT"); Label(5u,"HOLD S1 CAL"); Label(6u,"HOLD S2 DIAG");
        if (g_debug.tuning_saved==1u) Label(7u,"TUNING SAVED");
        else if (g_debug.tuning_saved==2u) Label(7u,"SAVE FAILED");
        else if (GAME_LEVEL_LOCK && level >= g_debug.unlocked) Label(7u,"LOCKED");
    }
    else if (state==APP_STATE_PAUSED)
    { Label(0u,"PAUSED"); Label(2u,"S1 RESUME"); Label(3u,"S2 RESTART"); Label(5u,"HOLD S1 CAL"); Label(6u,"HOLD S2 MENU"); }
    else if (state==APP_STATE_CLEAR)
    {
        if (level+1u==MAZE_LEVEL_COUNT) Label(0u,"ALL CLEAR"); else Label(0u,"LEVEL CLEAR");
        Label(2u,"TIME"); Unsigned(36u,16u,seconds,5u);
        Label(3u,"STEPS"); Unsigned(36u,24u,Game_GetSteps(),5u);
        Label(4u,"BEST");
        if (best_ms==0UL) LCD_DrawText(36u,32u,"----");
        else { Unsigned(36u,32u,best_ms/1000UL,4u); LCD_DrawText(60u,32u,"S"); }
        if (g_debug.new_best) Label(5u,"NEW BEST!");
        Label(6u,"S1 NEXT S2 RETRY");
    }
    else if (state==APP_STATE_ERROR)
    {
        Label(0u,"SENSOR ERROR"); SensorRegisters(16u);
        Label(3u,"CHECK SPI / POWER"); Label(5u,"S1 RETRY");
    }
    else if (state==APP_STATE_DIAGNOSTICS)
    {
        Label(0u,"ACCEL DIAG TYPE"); Unsigned(90u,0u,g_debug.sensor_type,1u);
        SensorRegisters(8u);
        Label(2u,"X"); Signed(18u,16u,g_debug.raw_mg.x);
        Signed(66u,16u,g_debug.accel_diag.raw_counts.x);
        Label(3u,"Y"); Signed(18u,24u,g_debug.raw_mg.y);
        Signed(66u,24u,g_debug.accel_diag.raw_counts.y);
        Label(4u,"Z"); Signed(18u,32u,g_debug.raw_mg.z);
        Signed(66u,32u,g_debug.accel_diag.raw_counts.z);
        Label(5u,"MG     RAW COUNTS");
        Label(6u,"S1 REINIT S2 MENU");
        if (g_debug.accel_diag.sample_valid) Label(7u,"OK HOLD2 TUNE");
        else Label(7u,"NO NEW SAMPLE");
    }
    else if (state==APP_STATE_TUNING)
    {
        uint8_t item = g_debug.tuning_item;
        uint16_t tilt = MotionInput_GetTiltMagnitude();
        uint16_t interval = Tuning_Interval(tilt);
        Label(0u,"TUNE"); Unsigned(30u,0u,(uint32_t)item+1u,1u);
        LCD_DrawText(36u,0u,"/"); Unsigned(42u,0u,(uint32_t)TUNING_ITEM_COUNT,1u);
        LCD_DrawText(0u,8u,TuningName(item));
        if (item==7u) LCD_DrawText(48u,8u,"S1 RESET");
        else if (item==6u)
        { Unsigned(48u,8u,g_tuning.filter_num,1u); LCD_DrawText(54u,8u,"/4"); }
        else Unsigned(48u,8u,TuningValue(item),4u);
        Label(2u,"NOW"); Unsigned(30u,16u,tilt,4u); LCD_DrawText(54u,16u,"MG");
        Label(3u,"INT"); Unsigned(30u,24u,interval,4u); LCD_DrawText(54u,24u,"MS");
        Label(4u,"DIR"); LCD_DrawText(30u,32u,DirectionName(MotionInput_GetDirection()));
        DrawTiltBar(tilt);
        Label(6u,"WHEEL ADJ  S1/S2");
        if (g_debug.tuning_saved==3u) Label(7u,"DEFAULT LOADED");
        else if (g_debug.tuning_saved==2u) Label(7u,"SAVE FAILED");
        else Label(7u,"SEL  HOLD2 SAVE");
    }
    return LCD_Flush();
}
