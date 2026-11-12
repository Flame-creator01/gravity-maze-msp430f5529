#include "accel.h"
#include "accel_port.h"
static uint8_t sensor_type;
uint8_t Accel_Init(void)
{
    sensor_type = Accel_Port_Init();
    return sensor_type != ACCEL_TYPE_NONE;
}
uint8_t Accel_Read(AccelData *data)
{
    if (data == 0 || sensor_type == ACCEL_TYPE_NONE) return ACCEL_READ_ERROR;
    return Accel_Port_Read(data);
}
uint8_t Accel_GetType(void) { return sensor_type; }
const AccelDiagnostics *Accel_GetDiagnostics(void) { return Accel_Port_GetDiagnostics(); }
