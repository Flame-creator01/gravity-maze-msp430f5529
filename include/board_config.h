#ifndef BOARD_CONFIG_H
#define BOARD_CONFIG_H
#include <msp430.h>
/* TI SLAU330B p22/p24 and HAL reference; user board is SEEED V1.0.
 * SEEED board revision is distinct from the TI revision numbers below. */
#define BOARD_SMCLK_HZ 1048576UL
#define KEY1_PORT_IN P1IN
#define KEY1_PORT_DIR P1DIR
#define KEY1_PORT_REN P1REN
#define KEY1_PORT_OUT P1OUT
#define KEY1_BIT BIT7
#define KEY2_PORT_IN P2IN
#define KEY2_PORT_DIR P2DIR
#define KEY2_PORT_REN P2REN
#define KEY2_PORT_OUT P2OUT
#define KEY2_BIT BIT2
#define LED_PORT_DIR P1DIR
#define LED_PORT_OUT P1OUT
#define LED_BIT BIT0
#define ACCEL_CS BIT5 /* P3.5 */
#define ACCEL_PWR BIT6 /* P3.6 */
#define ACCEL_MOSI BIT3 /* P3.3 / UCA0SIMO */
#define ACCEL_MISO BIT4 /* P3.4 / UCA0SOMI */
#define ACCEL_CLK BIT7 /* P2.7 / UCA0CLK */
#define ACCEL_INT BIT5 /* P2.5 */
/* 0 auto, 1 CMA3000-D01 (Rev2.0/2.1), 2 LIS3DH (Rev2.2). */
#define ACCEL_FORCE_TYPE 0
#define LCD_CS BIT4 /* P7.4 */
#define LCD_RESET BIT7 /* P5.7 */
#define LCD_DC BIT6 /* P5.6 */
#define LCD_BACKLIGHT BIT6 /* P7.6 */
#define LCD_MOSI BIT1 /* P4.1 / UCB1SIMO */
#define LCD_CLK BIT3 /* P4.3 / UCB1CLK */
#define SPI_WAIT_LIMIT 6000u
#endif
