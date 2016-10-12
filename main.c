#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <util/atomic.h>
#include <stddef.h>
#include <avr/pgmspace.h>
#include <esh.h>
#include <ctype.h>
#include <stdio.h>

#include "hardware.h"
#include "regulator.h"
#include "leds.h"

#define CTRL_BIT_ENABLED    (1 << 0)
#define CTRL_BIT_POWER_GOOD (1 << 1)
#define CTRL_BIT_L_ENABLED  (1 << 2)
#define CTRL_BIT_INVALID    (1 << 7)
static volatile uint8_t CONTROL[6] = {
    CTRL_BIT_INVALID,
    0,  // P5A
    0,  // P5B
    0,  // P3A
    CTRL_BIT_ENABLED,   // P3B, enabled at startup
    0,  // N12
};

// Slight hack: if this supply is shut down, delay and then turn it back on.
// This allows the EC to do a full reboot of the whole system including itself.
#define SUPPLY_KEEP_ALIVE   reg_P3B
#define KEEP_ALIVE_DELAY_MS 1000

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


static void en_dis(char const * supply, bool enabled)
{
    int nsupply = resolve_supply(supply);
    if (nsupply > 0 && nsupply < 6) {
        if (enabled) {
            printf_P(PSTR("enable supply %d\n"), nsupply);
            reg_enable(map_supply(nsupply), true);
        } else {
            printf_P(PSTR("disable supply %d\n"), nsupply);
            reg_disable(map_supply(nsupply));
        }
    } else {
        printf_P(PSTR("unrecognized supply: %s\n"), supply);
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
        for (int i = 1; i < argc; ++i) {
            en_dis(argv[i], true);
        }
    } else if (!strcmp_P(argv[0], PSTR("dis"))) {
        for (int i = 1; i < argc; ++i) {
            en_dis(argv[i], false);
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
    static uint8_t nsupply = 1;
    static bool found_bad = false;

    reg_type * supply = map_supply(nsupply);
    bool en = reg_is_enabled(supply);
    bool pg = reg_is_power_good(supply);
    bool sw = false;
    bool enable = false;

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        uint8_t ctrl = CONTROL[nsupply];
        if (pg) {
            ctrl |= CTRL_BIT_POWER_GOOD;
        } else {
            ctrl &= ~CTRL_BIT_POWER_GOOD;
        }

        enable = ctrl & CTRL_BIT_ENABLED;
        bool was_enabled = ctrl & CTRL_BIT_L_ENABLED;
        sw = (enable && !was_enabled) || (!enable && was_enabled);
        if (enable) {
            ctrl |= CTRL_BIT_L_ENABLED;
        } else {
            ctrl &= ~CTRL_BIT_L_ENABLED;
        }
        CONTROL[nsupply] = ctrl;
    }

    if (sw) {
        if (enable) {
            reg_enable(supply, true);
        } else {
            reg_disable(supply);

            if (supply == SUPPLY_KEEP_ALIVE) {
                _delay_ms(KEEP_ALIVE_DELAY_MS);
                CONTROL[nsupply] |= CTRL_BIT_ENABLED;
            }
        }
    }

    if (nsupply == 1) {
        found_bad = false;
    }

    if (!pg && en) {
        found_bad = true;
    }

    set_led(LED_SFY, found_bad);

    uint16_t led = 0;
    switch (nsupply) {
    case 1: led = LED_P5A; break;
    case 2: led = LED_P5B; break;
    case 3: led = LED_P3A; break;
    case 4: led = LED_P3B; break;
    case 5: led = LED_N12; break;
    }
    set_led(led, pg && en);

    nsupply += 1;
    if (nsupply == 6) {
        nsupply = 1;
    }
}


void esh_printer(esh_t * esh, char c, void * arg)
{
    (void) esh;
    (void) arg;
    putchar(c);
}


// I2C interface follows the usual "address, data" form, where the addresses
// are the supply numbers (1-indexed here as everywhere else). Each byte is
// a bitfield with:
//      ENABLED     = 1 << 0
//      POWER_GOOD  = 1 << 1
//      RESERVED    = 1 << 2    // used to track previous ENABLED state
//      INVALID     = 1 << 7

void twi_callback(TWI_Slave_t * packet)
{
    static uint8_t active_supply = 0;
    uint8_t supply = packet->receivedData[0];
    if (supply >= 1 && supply <= 5) {
        active_supply = supply;
    } else {
        active_supply = 0;
    }

    uint8_t outindex = packet->bytesReceived;
    if (outindex == 0) {
        if (active_supply >= 1 && active_supply <= 5) {
            packet->sendData[outindex] = CONTROL[active_supply];
        } else {
            packet->sendData[outindex] = CTRL_BIT_INVALID;
        }
    } else {
        // Only allow certain bits to be changed
        const uint8_t allowed_bits = CTRL_BIT_ENABLED;
        uint8_t bits_to_set = packet->receivedData[outindex] & allowed_bits;
        CONTROL[active_supply] = (CONTROL[active_supply] & ~allowed_bits) | bits_to_set;
    }
}


int main(void)
{
    init_ports();
    init_clock();
    reg_probe(reg_P5A);
    reg_probe(reg_P5B);
    reg_probe(reg_P3A);
    reg_probe(reg_P3B);
    reg_probe(reg_N12);
    init_uart();
    stdout = &uart_stdout;
    init_twi(&twi_callback);
    PMIC.CTRL = PMIC_LOLVLEN_bm | PMIC_MEDLVLEN_bm | PMIC_HILVLEN_bm;
    sei();

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
