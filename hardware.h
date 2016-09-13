#ifndef HARDWARE_H
#define HARDWARE_H

#include <avr/io.h>
#include <stdbool.h>

#define CONCAT__2(x,y)  x ## y
#define CONCAT(x,y)     CONCAT__2(x,y)

#define bm(bp)      (1 << (bp))
#define PINCTRL(bp) CONCAT(PIN, CONCAT(bp, CTRL))

#define PORTA_UNUSED    (bm(1) | bm(5) | bm(6) | bm(7))
#define PORTC_UNUSED    (bm(3))
#define PORTD_UNUSED    (bm(5))
#define PORTR_UNUSED    (bm(0) | bm(1))

#define LED_A_PORT  PORTA
#define LED_A_bp    2
#define LED_B_PORT  PORTA
#define LED_B_bp    4
#define LED_C_PORT  PORTA
#define LED_C_bp    3
#define LED_gm      (bm(LED_A_bp) | bm(LED_B_bp) | bm(LED_C_bp))

#define SDA_PORT    PORTC
#define SDA_bp      0
#define SCL_PORT    PORTC
#define SCL_bp      1
#define I2C_gm      (bm(SDA_bp) | bm(SCL_bp))
#define I2C_TWI     TWIC

#define INT_PORT    PORTC
#define INT_bp      2

#define TX_PORT     PORTD
#define TX_bp       7
#define RX_PORT     PORTD
#define RX_bp       6
#define UART_gm     (bm(TX_bp) | bm(RX_bp))
#define UART_USART  USARTD0
#define UART_DREINT USARTD0_DRE_vect
#define UART_BSEL   12u
#define UART_BSCALE 4
#define UART_2X     0
#define UART_CHSIZE 8
#define UART_PARITY 'N'
#define UART_STOP   1
#define UART_DREINTLVL  USART_DREINTLVL_LO_gc

#define N12_EN_PORT PORTA
#define N12_EN_bp   0
#define N12_PG_PORT PORTD
#define N12_PG_bp   0

#define P5A_SYNC_PORT   PORTC
#define P5A_SYNC_bp     4
#define P5A_SYNC_CC_bp  0   // A
#define P5A_PG_PORT     PORTD
#define P5A_PG_bp       4
#define P5A_PHASE_180   0

#define P5B_SYNC_PORT   PORTC
#define P5B_SYNC_bp     5
#define P5B_SYNC_CC_bp  2   // B
#define P5B_PG_PORT     PORTD
#define P5B_PG_bp       3
#define P5B_PHASE_180   1

#define P3A_SYNC_PORT   PORTC
#define P3A_SYNC_bp     6
#define P3A_SYNC_CC_bp  4   // C
#define P3A_PG_PORT     PORTD
#define P3A_PG_bp       2
#define P3A_PHASE_180   1

#define P3B_SYNC_PORT   PORTC
#define P3B_SYNC_bp     7
#define P3B_SYNC_CC_bp  6   // D
#define P3B_PG_PORT     PORTD
#define P3B_PG_bp       1
#define P3B_PHASE_180   0

#define DCDC_SYNC_gm    (bm(P3A_SYNC_bp) | bm(P3B_SYNC_bp) | bm(P5A_SYNC_bp) | bm(P5B_SYNC_bp))
#define DCDC_PG_gm      (bm(P3A_PG_bp) | bm(P3B_PG_bp) | bm(P5A_PG_bp) | bm(P5B_PG_bp))
#define DCDC_FREQUENCY  600e3
#define DCDC_SYNC_PORT  P5A_SYNC_PORT
#define DCDC_PG_PORT    P5A_PG_PORT
#define DCDC_TIMER      TCC4

void init_ports(void);
void init_clock(void);
void init_timer(void);

void init_uart(void);

/**
 * Transmit one character through the UART.
 *
 * @param ch - character to be transmitted
 */
void uart_transmit(char ch);

/**
 * Receive one character through the UART. Returns -1 if one is
 * not available.
 */
int uart_receive(void);

void _enable_dcdc(uint8_t sync_bm, uint8_t sync_cc_bm, bool phase_180);
void _disable_dcdc(uint8_t sync_bm, uint8_t sync_cc_bm);

#define ENABLE_DCDC(dcdc) _enable_dcdc(bm(CONCAT(dcdc, _SYNC_bp)),      \
                                       TC45_CCAMODE_COMP_gc << (CONCAT(dcdc, _SYNC_CC_bp)),   \
                                       CONCAT(dcdc, _PHASE_180))

#define DISABLE_DCDC(dcdc) _disable_dcdc(bm(CONCAT(dcdc, _SYNC_bp)),    \
                                         TC45_CCAMODE_COMP_gc << (CONCAT(dcdc, _SYNC_CC_bp)))

#endif // HARDWARE_H
