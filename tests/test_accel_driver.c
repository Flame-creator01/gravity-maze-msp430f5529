#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <msp430.h>
#include "board_config.h"
#include "board.h"
#include "accel.h"

volatile uint8_t P2SEL, P2DIR, P2IE, P3SEL, P3OUT, P3DIR;
volatile uint8_t UCA0CTL0, UCA0CTL1, UCA0BR0, UCA0BR1, UCA0MCTL;
volatile uint16_t host_txbuf;
volatile uint8_t host_spi_ifg;
static uint8_t registers[64], model, selected, phase, command, rx;
static uint8_t reset_values[3], reset_count, new_data, cma_parity;
static uint8_t fail_rx, fail_tx, stuck_busy;
static uint32_t transaction_count, busy_checks, delay_total;

static void TrackSelection(void)
{
    uint8_t active=(P3OUT & ACCEL_CS)==0u;
    if (active && !selected) { phase=0u; transaction_count++; }
    selected=active;
}
static uint8_t Exchange(uint8_t value)
{
    uint8_t address, result;
    if (phase==0u) { command=value; phase=1u; return 0u; }
    if ((model==ACCEL_TYPE_CMA3000 && (UCA0CTL0 & UCCKPL)!=0u) ||
        (model==ACCEL_TYPE_LIS3DH && (UCA0CTL0 & UCCKPL)==0u) ||
        model==ACCEL_TYPE_NONE) return 0u;
    if (model==ACCEL_TYPE_CMA3000)
    {
        address=command >> 2;
        if ((command & 2u)!=0u)
        {
            registers[address]=value;
            if (address==4u && reset_count<3u)
            {
                reset_values[reset_count++]=value;
                if (reset_count==3u) registers[2]=0u;
            }
            return 0u;
        }
        return registers[address];
    }
    address=(uint8_t)((command & 0x3Fu)+(((command & 0x40u)!=0u) ? phase-1u : 0u));
    phase++;
    if ((command & 0x80u)==0u) { registers[address]=value; return 0u; }
    result=registers[address];
    return result;
}
volatile uint8_t *Host_SpiFlagsRegister(void)
{
    TrackSelection();
    if (host_txbuf<=255u)
    {
        assert(selected);
        rx=Exchange((uint8_t)host_txbuf); host_txbuf=256u;
        host_spi_ifg=fail_rx ? UCTXIFG : (UCTXIFG | UCRXIFG);
    }
    if (fail_tx) host_spi_ifg=0u;
    return &host_spi_ifg;
}
uint8_t Host_SpiReceive(void)
{
    TrackSelection(); host_spi_ifg &= (uint8_t)~UCRXIFG; return rx;
}
uint8_t Host_SpiStatus(void)
{
    TrackSelection(); busy_checks++;
    return stuck_busy ? UCBUSY : 0u;
}
void Host_DelayCycles(uint32_t cycles)
{
    TrackSelection(); delay_total+=cycles;
}
void Board_DelayMs(uint16_t ms) { delay_total+=(uint32_t)ms*1049UL; }
static void ResetModel(uint8_t type)
{
    memset(registers,0,sizeof(registers)); memset(reset_values,0,sizeof(reset_values));
    model=type; selected=0u; phase=0u; command=0u; rx=0u;
    reset_count=0u; fail_rx=0u; fail_tx=0u; stuck_busy=0u;
    transaction_count=0UL; busy_checks=0UL; delay_total=0UL;
    host_txbuf=256u; host_spi_ifg=UCTXIFG;
    P3OUT=ACCEL_CS; P3SEL=0u; P3DIR=0u; P2SEL=0u; P2IE=0u;
    if (type==ACCEL_TYPE_CMA3000)
    { registers[1]=0x12u; registers[3]=cma_parity ? 1u : 0u; }
    else if (type==ACCEL_TYPE_LIS3DH)
    { registers[0x0F]=0x33u; registers[0x27]=new_data ? 0x08u : 0u; }
}
static void LisAxis(uint8_t address, int16_t count)
{
    uint16_t value=(uint16_t)(count*16);
    registers[address]=(uint8_t)(value & 255u);
    registers[address+1u]=(uint8_t)(value >> 8);
}
int main(void)
{
    AccelData data;
    const AccelDiagnostics *diag;
    uint32_t before;
    cma_parity=0u; new_data=1u;
    ResetModel(ACCEL_TYPE_CMA3000); assert(Accel_Init());
    assert(Accel_GetType()==ACCEL_TYPE_CMA3000);
    assert(reset_count==3u && reset_values[0]==2u && reset_values[1]==10u && reset_values[2]==4u);
    registers[6]=0xC8u; registers[7]=4u; registers[8]=9u;
    before=busy_checks;
    assert(Accel_Read(&data)==ACCEL_READ_OK);
    assert(data.x==-1008 && data.y==72 && data.z==162);
    diag=Accel_GetDiagnostics();
    assert(diag->raw_counts.x==-56 && diag->raw_counts.y==4 && diag->raw_counts.z==9);
    assert(diag->id==0x12u && diag->ctrl==0x94u && diag->status==0u);
    assert(diag->bus_ok && diag->sample_valid);
    assert(busy_checks-before==5UL); /* CTRL, STATUS, then three axes. */
    registers[8]=0xC8u; assert(Accel_Read(&data)==ACCEL_READ_OK); assert(data.z==-1008);
    registers[8]=56u; assert(Accel_Read(&data)==ACCEL_READ_OK); assert(data.z==1008);
    registers[2]=0x14u; data.x=123;
    assert(Accel_Read(&data)==ACCEL_READ_ERROR && data.x==123);
    assert(!diag->sample_valid && diag->ctrl==0x14u);
    registers[2]=0x94u; registers[3]=1u;
    assert(Accel_Read(&data)==ACCEL_READ_ERROR);
    registers[3]=0u; fail_rx=1u;
    assert(Accel_Read(&data)==ACCEL_READ_ERROR && !diag->bus_ok);
    assert((P3OUT & ACCEL_CS)!=0u);
    fail_rx=0u; host_spi_ifg=UCTXIFG; stuck_busy=1u;
    assert(Accel_Read(&data)==ACCEL_READ_ERROR && !diag->bus_ok);
    assert((P3OUT & ACCEL_CS)!=0u);
    stuck_busy=0u; fail_tx=1u;
    assert(Accel_Read(&data)==ACCEL_READ_ERROR && !diag->bus_ok);
    assert((P3OUT & ACCEL_CS)!=0u);
    cma_parity=1u; ResetModel(ACCEL_TYPE_CMA3000);
    assert(!Accel_Init()); assert(Accel_GetDiagnostics()->status==1u); cma_parity=0u;

    ResetModel(ACCEL_TYPE_LIS3DH); assert(Accel_Init());
    assert(Accel_GetType()==ACCEL_TYPE_LIS3DH && reset_count==0u);
    LisAxis(0x28u,-1000); LisAxis(0x2Au,50); LisAxis(0x2Cu,1008);
    assert(Accel_Read(&data)==ACCEL_READ_OK);
    assert(data.x==-1000 && data.y==50 && data.z==1008);
    diag=Accel_GetDiagnostics();
    assert(diag->ctrl==0x47u && diag->ctrl4==0x88u && diag->sample_valid);
    registers[0x27]=0u; data.x=123;
    assert(Accel_Read(&data)==ACCEL_READ_WAIT && data.x==123 && !diag->sample_valid);
    registers[0x23]=0u;
    assert(Accel_Read(&data)==ACCEL_READ_ERROR);
    ResetModel(ACCEL_TYPE_NONE); assert(!Accel_Init());
    assert(Accel_Read(&data)==ACCEL_READ_ERROR && !Accel_GetDiagnostics()->sample_valid);
    puts("accelerometer: reset sequence, register selection, signed XYZ, raw diagnostics, config/parity rejection, bounded RX/TX/busy timeouts and LIS3DH regression passed");
    puts("ALL ACCELEROMETER HOST TESTS PASSED (SPI/device responses are simulated)");
    return 0;
}
