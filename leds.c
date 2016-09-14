#include "leds.h"
#include "hardware.h"
#include <stdbool.h>

// Matrix representing the on/off state of each LED.
// each LED is row,col = anode,cathode

#define N_PINS 3
static volatile bool led_states[N_PINS + 1][N_PINS + 1] = {{0}};

void set_led(uint16_t led, bool state)
{
    const uint8_t row = (led & 0xff00u) >> 8;
    const uint8_t col = (led & 0x00ffu);
    led_states[row][col] = state;
}


static PORT_t volatile * map_to_port(uint8_t pin)
{
    switch (pin) {
    case 1:
        return &LED_A_PORT;
    case 2:
        return &LED_B_PORT;
    case 3:
        return &LED_C_PORT;
    default:
        // We're not using port R, so accidental pin pokes here
        // are safe
        return &PORTR;
    }
}


static uint8_t map_to_bit(uint8_t pin)
{
    switch (pin) {
    case 1:
        return bm(LED_A_bp);
    case 2:
        return bm(LED_B_bp);
    case 3:
        return bm(LED_C_bp);
    default:
        // PR7 doesn't even exist, so pokes here do nothing
        return 0x80;
    }
}


static void config_pos(uint8_t row, uint8_t col)
{
    map_to_port(row)->OUTSET = map_to_bit(row);
    map_to_port(row)->DIRSET = map_to_bit(row);
    map_to_port(col)->OUTCLR = map_to_bit(col);
    map_to_port(col)->DIRSET = map_to_bit(col);
}


static void config_blank(void)
{
    for (uint8_t i = 1; i <= N_PINS; ++i) {
        map_to_port(i)->DIRCLR = map_to_bit(i);
    }
}


enum led_cycle_states {
    STATE_CONFIG,
    STATE_HOLD,
    STATE_BLANK,
};


void led_cycle(void)
{
    static enum led_cycle_states state = STATE_BLANK;
    static const uint16_t hold_limit = 100u;
    static uint16_t hold_val = 0;
    static uint8_t row = 1;
    static uint8_t col = 1;

    switch (state) {
    case STATE_CONFIG:
        if (led_states[row][col]) {
            config_pos(row, col);
        }
        hold_val = 0;
        state = STATE_HOLD;
        break;

    case STATE_HOLD:
        if (hold_val == hold_limit) {
            state = STATE_BLANK;
        } else {
            ++hold_val;
        }
        break;

    case STATE_BLANK:
        config_blank();
        ++col;
        if (col == N_PINS + 1) {
            ++row;
            col = 1;
        }
        if (row == N_PINS + 1) {
            row = 1;
        }
        state = STATE_CONFIG;
        break;
    }
}
