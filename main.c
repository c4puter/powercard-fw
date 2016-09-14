#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stddef.h>
#include <avr/pgmspace.h>
#include <esh.h>
#include <ctype.h>
#include <stdio.h>

#include "hardware.h"
#include "leds.h"

static int uart_putchar(char c, FILE *stream);
static FILE uart_stdout = FDEV_SETUP_STREAM(uart_putchar, NULL,
                                                 _FDEV_SETUP_WRITE);

static volatile bool supply_enabled[6] = {false};
static volatile bool power_good[6] = {false};

static int uart_putchar(char c, FILE *stream)
{
    (void) stream;
    if (c == '\n') {
        uart_putchar('\r', stream);
    }
    uart_transmit(c);
    return 0;
}


// Resolves a power supply to an int.
// The power supply name is edited in place!
int resolve_supply(char const * supply)
{
    if (!strcasecmp_P(supply, PSTR("5VA"))) {
        return 1;
    } else if (!strcasecmp_P(supply, PSTR("5VB"))) {
        return 2;
    } else if (!strcasecmp_P(supply, PSTR("3VA"))) {
        return 3;
    } else if (!strcasecmp_P(supply, PSTR("3VB"))) {
        return 4;
    } else if (!strcasecmp_P(supply, PSTR("N12"))) {
        return 5;
    } else {
        return -1;
    }
}


void esh_cb(esh_t * esh, int argc, char ** argv, void * arg)
{
    (void) esh;
    (void) arg;
    if (argc < 1) {
        return;
    }
    int supply = resolve_supply(argv[1]);

    if (!strcmp_P(argv[0], PSTR("en"))) {
        if (argc < 2) {
            return;
        }
        printf_P(PSTR("enable supply %d\n"), supply);
        switch (supply) {
        case 1: ENABLE_DCDC(P5A); break;
        case 2: ENABLE_DCDC(P5B); break;
        case 3: ENABLE_DCDC(P3A); break;
        case 4: ENABLE_DCDC(P3B); break;
        case 5: enable_n12(); break;
        }
        if (supply > 0) {
            supply_enabled[supply] = true;
        }
    } else if (!strcmp_P(argv[0], PSTR("dis"))) {
        if (argc < 2) {
            return;
        }
        printf_P(PSTR("disable supply %d\n"), supply);
        switch (supply) {
        case 1: DISABLE_DCDC(P5A); break;
        case 2: DISABLE_DCDC(P5B); break;
        case 3: DISABLE_DCDC(P3A); break;
        case 4: DISABLE_DCDC(P3B); break;
        case 5: disable_n12(); break;
        }
        if (supply > 0) {
            supply_enabled[supply] = false;
        }
    } else if (!strcmp_P(argv[0], PSTR("stat"))) {
        if (argc < 2) {
            return;
        }
        if (supply > 0 && supply < 6) {
            printf_P(PSTR("enabled: %c  power good: %c\n"),
                    supply_enabled[supply] ? 'Y' : 'N',
                    power_good[supply] ? 'Y' : 'N');
        }
    } else if (!strcmp_P(argv[0], PSTR("standby"))) {
        standby();
    } else if (!strcmp_P(argv[0], PSTR("help"))) {
        puts_P(PSTR("en SUPPLY"));
        puts_P(PSTR("dis SUPPLY"));
        puts_P(PSTR("stat SUPPLY"));
        puts_P(PSTR("standby"));
        puts_P(PSTR(""));
        puts_P(PSTR("supplies: 3VA, 3VB, 5VA, 5VB, N12"));
    }
}


void monitor_task(void)
{
    static uint8_t supply = 1;
    static bool found_bad = false;

    bool pg = false;
    switch (supply) {
    case 1: pg = P5A_PG_PORT.IN & bm(P5A_PG_bp); break;
    case 2: pg = P5B_PG_PORT.IN & bm(P5B_PG_bp); break;
    case 3: pg = P3A_PG_PORT.IN & bm(P3A_PG_bp); break;
    case 4: pg = P3B_PG_PORT.IN & bm(P3B_PG_bp); break;
    case 5:
        // N12 has special conditions. It requires 3VB for its clock, and
        // one of 5VA or 5VB for its power.
        pg = N12_PG_PORT.IN & bm(N12_PG_bp)
            && (supply_enabled[4] && power_good[4])
            && (   (supply_enabled[1] && power_good[1])
                || (supply_enabled[2] && power_good[2]));
        break;
    }

    power_good[supply] = pg;

    if (supply == 1) {
        found_bad = false;
    }

    if (!power_good[supply] && supply_enabled[supply]) {
        found_bad = true;
    }

    set_led(LED_SFY, found_bad);

    uint16_t led = 0;
    switch (supply) {
    case 1: led = LED_P5A; break;
    case 2: led = LED_P5B; break;
    case 3: led = LED_P3A; break;
    case 4: led = LED_P3B; break;
    case 5: led = LED_N12; break;
    }
    set_led(led, pg && supply_enabled[supply]);

    supply += 1;
    if (supply == 6) {
        supply = 1;
    }
}


void esh_printer(esh_t * esh, char const * s, void * arg)
{
    (void) esh;
    (void) arg;
    while (s && *s) {
        putchar(*s);
        ++s;
    }
}


void fuck(void)
{
    LED_A_PORT.OUTSET = bm(LED_A_bp);
    LED_B_PORT.OUTCLR = bm(LED_B_bp);
    LED_A_PORT.DIRSET = bm(LED_A_bp) | bm(LED_B_bp);
    for(;;);
}


int main(void)
{
    init_ports();
    init_clock();
    init_timer();
    init_uart();
    stdout = &uart_stdout;
    sei();
    PMIC.CTRL = PMIC_LOLVLEN_bm | PMIC_MEDLVLEN_bm | PMIC_HILVLEN_bm;

    esh_t * esh = esh_init();
    esh_register_command(esh, &esh_cb, NULL);
    esh_register_print(esh, &esh_printer, NULL);

    for(;;) {
        monitor_task();
        led_cycle();

        int c = uart_receive();
        if (c > 0) {
            if (c == '\r') c = '\n';
            esh_rx(esh, (char) c);
        }

        if (in_standby()) {
            // Delay at least one baud cycle to avoid instant wakeup.
            // The clock has been cut by 256, so this is a much longer
            // delay than what it looks like.
            _delay_us(100);
            enable_wake();
        }
    }
}
