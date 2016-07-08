#include <assert.h>
#include "hardware.h"


// Include bits not defined in 128A1U header (128A1U was used for devel)
#ifdef __AVR_ATxmega128A1U__
#define PORT_SPI_bm  0x20  /* SPI bit mask. */
#define PORT_SPI_bp  5  /* SPI bit position. */

#define PORT_USART0_bm  0x10  /* Usart0 bit mask. */
#define PORT_USART0_bp  4  /* Usart0 bit position. */

#define PORT_TC0D_bm  0x08  /* Timer/Counter 0 Output Compare D bit mask. */
#define PORT_TC0D_bp  3  /* Timer/Counter 0 Output Compare D bit position. */

#define PORT_TC0C_bm  0x04  /* Timer/Counter 0 Output Compare C bit mask. */
#define PORT_TC0C_bp  2  /* Timer/Counter 0 Output Compare C bit position. */

#define PORT_TC0B_bm  0x02  /* Timer/Counter 0 Output Compare B bit mask. */
#define PORT_TC0B_bp  1  /* Timer/Counter 0 Output Compare B bit position. */

#define PORT_TC0A_bm  0x01  /* Timer/Counter 0 Output Compare A bit mask. */
#define PORT_TC0A_bp  0  /* Timer/Counter 0 Output Compare A bit position. */
#endif


_Static_assert(&LED_A_PORT == &LED_B_PORT && &LED_B_PORT == &LED_C_PORT,
        "LED matrix must be on a single port");

_Static_assert(&SDA_PORT == &SCL_PORT,
        "I2C must be on a single port");
_Static_assert(SDA_bp == 0 && SCL_bp == 1,
        "I2C must be on a hardware TWI port");

_Static_assert(&TX_PORT == &RX_PORT,
        "UART must be on a single port");
_Static_assert((TX_bp == 7 || TX_bp == 3) && (RX_bp == 6 || RX_bp == 2),
        "UART must be on a hardware USART port");
_Static_assert(TX_bp == RX_bp + 1,
        "UART pins must be either both remapped or neither");

_Static_assert(&P5A_SYNC_PORT == &P5B_SYNC_PORT &&
               &P5A_SYNC_PORT == &P3A_SYNC_PORT &&
               &P5A_SYNC_PORT == &P3B_SYNC_PORT,
        "DC-DC sync pins must all be on the same port");
_Static_assert(DCDC_SYNC_gm == 0xf0 || DCDC_SYNC_gm == 0x0f,
        "DC-DC sync pins must be either all remapped or none");
_Static_assert(&P5A_PG_PORT == &P5B_PG_PORT &&
               &P5A_PG_PORT == &P3A_PG_PORT &&
               &P5A_PG_PORT == &P3B_PG_PORT,
        "DC-DC power-good pins must all be on the same port");

void init_ports(void)
{
    PORTCFG.MPCMASK = LED_gm;
    LED_A_PORT.PIN0CTRL = PORT_SRLEN_bm | PORT_OPC_TOTEM_gc;
    LED_A_PORT.DIRSET = LED_gm;

    SDA_PORT.DIRCLR = I2C_gm;

    INT_PORT.PINCTRL(INT_bp) = PORT_OPC_WIREDANDPULL_gc | PORT_INVEN_bm;
    INT_PORT.OUTCLR = bm(INT_bp);

    RX_PORT.DIRCLR = bm(RX_bp);
    TX_PORT.OUTSET = bm(TX_bp);
    TX_PORT.DIRSET = bm(TX_bp);
    TX_PORT.PINCTRL(TX_bp) = PORT_SRLEN_bm | PORT_OPC_TOTEM_gc;
    if (TX_bp > 3) {
        // UART pins are in the high half of the port - remap them
        TX_PORT.REMAP |= PORT_USART0_bm;
    }

    N12_EN_PORT.PINCTRL(N12_EN_bp) = PORT_SRLEN_bm | PORT_OPC_TOTEM_gc;
    N12_EN_PORT.OUTCLR = bm(N12_EN_bp);
    N12_EN_PORT.DIRSET = bm(N12_EN_bp);
    N12_PG_PORT.PINCTRL(N12_PG_bp) = PORT_OPC_PULLUP_gc;
    N12_PG_PORT.DIRCLR = bm(N12_PG_bp);

    PORTCFG.MPCMASK = DCDC_SYNC_gm;
    DCDC_SYNC_PORT.PIN0CTRL = PORT_SRLEN_bm | PORT_OPC_TOTEM_gc;
    DCDC_SYNC_PORT.OUTCLR = DCDC_SYNC_gm;
    DCDC_SYNC_PORT.DIRSET = DCDC_SYNC_gm;

    PORTCFG.MPCMASK = DCDC_PG_gm;
    DCDC_PG_PORT.PIN0CTRL = PORT_OPC_PULLUP_gc;
    DCDC_PG_PORT.DIRCLR = DCDC_PG_gm;

    if (DCDC_SYNC_gm == 0xf0) {
        P5A_SYNC_PORT.REMAP |= (PORT_TC0A_bm | PORT_TC0B_bm | PORT_TC0C_bm | PORT_TC0D_bm);
    }

}


_Static_assert(F_CPU == 32000000uLL, "F_CPU is expected to be 32 MHz");
void init_clock(void)
{
    OSC.CTRL |= OSC_RC32MEN_bm;
    while (!(OSC.STATUS & OSC_RC32MRDY_bm));

    _PROTECTED_WRITE(CLK.PSCTRL, CLK_PSADIV_1_gc | CLK_PSBCDIV_1_1_gc);
    _PROTECTED_WRITE(CLK.CTRL, CLK_SCLKSEL_RC32M_gc);
}


void init_timer(void)
{
    DCDC_TIMER.CTRLA = TC_CLKSEL_DIV1_gc;
    DCDC_TIMER.CTRLB = TC_WGMODE_FRQ_gc;

    // From XMEGA AU manual, p 172:
    // fFRQ = fclkper / (2 PRESC (CCA + 1))
    DCDC_TIMER.CCA = (uint16_t) (F_CPU / (2 * /* presc */ 1 * DCDC_FREQUENCY) - 1 + 0.5);
}


void _enable_dcdc(uint8_t sync_bm, uint8_t sync_cc_bm, bool phase_180)
{
    PORTCFG.MPCMASK = sync_bm;
    DCDC_SYNC_PORT.PIN0CTRL =
        PORT_OPC_TOTEM_gc | PORT_SRLEN_bm |
        (phase_180 ? PORT_INVEN_bm : 0);
    DCDC_TIMER.CTRLB |= sync_cc_bm;
}


void _disable_dcdc(uint8_t sync_bm, uint8_t sync_cc_bm)
{
    DCDC_TIMER.CTRLB &= ~sync_cc_bm;
    PORTCFG.MPCMASK = sync_bm;
    DCDC_SYNC_PORT.PIN0CTRL = PORT_OPC_TOTEM_gc | PORT_SRLEN_bm;
    DCDC_SYNC_PORT.OUTCLR = sync_bm;
}
