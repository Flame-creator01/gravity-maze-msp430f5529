#ifndef ACCEL_H
#define ACCEL_H
#include "app_types.h"
#define ACCEL_TYPE_NONE 0u
#define ACCEL_TYPE_CMA3000 1u
#define ACCEL_TYPE_LIS3DH 2u
#define ACCEL_READ_ERROR 0u
#define ACCEL_READ_OK 1u
#define ACCEL_READ_WAIT 2u
typedef struct {
    AccelData raw_counts;
    uint8_t id, ctrl, ctrl4, status;
    uint8_t bus_ok, sample_valid;
} AccelDiagnostics;
uint8_t Accel_Init(void);
uint8_t Accel_Read(AccelData *data);
uint8_t Accel_GetType(void);
const AccelDiagnostics *Accel_GetDiagnostics(void);
#endif
