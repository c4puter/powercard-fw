#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stddef.h>
#include <avr/pgmspace.h>
#include <esh.h>
#include <ctype.h>
#include <stdio.h>

#include "hardware.h"
#include "regulator.h"
#include "leds.h"

static int uart_putchar(char c, FILE *stream);
static FILE uart_stdout = FDEV_SETUP_STREAM(uart_putchar, NULL,
                                                 _FDEV_SETUP_WRITE);

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
static int resolve_supply(char const * supply)
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

static reg_type * map_supply(int num)
{
    switch (num) {
    case 1: return reg_P5A;
    case 2: return reg_P5B;
    case 3: return reg_P3A;
    case 4: return reg_P3B;
    case 5: return reg_N12;
    default: return NULL;
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
        if (supply > 0 && supply < 6) {
            reg_enable(map_supply(supply), true);
        }
    } else if (!strcmp_P(argv[0], PSTR("dis"))) {
        if (argc < 2) {
            return;
        }
        printf_P(PSTR("disable supply %d\n"), supply);
        if (supply > 0 && supply < 6) {
            reg_disable(map_supply(supply));
        }
    } else if (!strcmp_P(argv[0], PSTR("stat"))) {
        if (argc < 2) {
            return;
        }
        if (supply > 0 && supply < 6) {
            bool enabled = reg_is_enabled(map_supply(supply));
            bool pg = reg_is_power_good(map_supply(supply));
            printf_P(PSTR("enabled: %c  power good: %c\n"),
                    enabled ? 'Y' : 'N',
                    pg ? 'Y' : 'N');
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

    bool pg = reg_is_power_good(map_supply(supply));

    if (supply == 1) {
        found_bad = false;
    }

    if (pg) {
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
    set_led(led, pg);

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
