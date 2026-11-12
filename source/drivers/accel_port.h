#ifndef ACCEL_PORT_H
#define ACCEL_PORT_H
#include "accel.h"
uint8_t Accel_Port_Init(void);
uint8_t Accel_Port_Read(AccelData *data);
const AccelDiagnostics *Accel_Port_GetDiagnostics(void);
#endif
