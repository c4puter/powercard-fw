#include <avr/io.h>
#include <util/delay.h>

#include "hardware.h"

int main(void)
{
    init_ports();
    init_clock();
    init_timer();

    PORTF.DIRSET = 0xf0;
    PORTF.OUTSET = 0xa0;

    uint8_t lastmask = 0;
    for(;;) {
        uint8_t mask = ~(PORTF.IN & 0x0f);
        if (mask != lastmask) {
            lastmask = mask;
            if (mask & 0x01) {
                ENABLE_DCDC(P3A);
            } else {
                DISABLE_DCDC(P3A);
            }
            if (mask & 0x02) {
                ENABLE_DCDC(P3B);
            } else {
                DISABLE_DCDC(P3B);
            }
            if (mask & 0x04) {
                ENABLE_DCDC(P5A);
            } else {
                DISABLE_DCDC(P5A);
            }
            if (mask & 0x08) {
                ENABLE_DCDC(P5B);
            } else {
                DISABLE_DCDC(P5B);
            }
        }
        PORTF.OUTTGL = 0xf0;
        _delay_ms(100);
    }
}
