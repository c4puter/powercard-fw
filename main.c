#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stddef.h>
#include <avr/pgmspace.h>
#include <esh.h>

#include "hardware.h"

void esh_cb(esh_t * esh, int argc, char ** argv, void * arg)
{
    (void) esh;
    (void) arg;
    if (argc < 1) {
        return;
    }

    if (!strcmp_P(argv[0], PSTR("en"))) {
        if (argc < 2) {
            return;
        }
        switch (argv[1][0]) {
        case '1': ENABLE_DCDC(P5A); break;
        case '2': ENABLE_DCDC(P5B); break;
        case '3': ENABLE_DCDC(P3A); break;
        case '4': ENABLE_DCDC(P3B); break;
        //case '5': ENABLE_DCDC(N12); break;
        }
    } else if (!strcmp_P(argv[0], PSTR("dis"))) {
        if (argc < 2) {
            return;
        }
        switch (argv[1][0]) {
        case '1': DISABLE_DCDC(P5A); break;
        case '2': DISABLE_DCDC(P5B); break;
        case '3': DISABLE_DCDC(P3A); break;
        case '4': DISABLE_DCDC(P3B); break;
        //case '5': DISABLE_DCDC(N12); break;
        }
    }
}

void esh_printer(esh_t * esh, char const * s, void * arg)
{
    (void) esh;
    (void) arg;
    while (s && *s) {
        if (*s == '\n') {
            uart_transmit('\r');
        }
        uart_transmit(*s);
        ++s;
    }
}

int main(void)
{
    init_ports();
    init_clock();
    init_timer();
    init_uart();
    sei();

    esh_t * esh = esh_init();
    esh_register_command(esh, &esh_cb, NULL);
    esh_register_print(esh, &esh_printer, NULL);

    for(;;) {
        int c = uart_receive();
        if (c > 0) {
            if (c == '\r') c = '\n';
            esh_rx(esh, (char) c);
        }
    }
}
