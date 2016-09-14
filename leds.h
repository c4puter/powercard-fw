#ifndef LEDS_H
#define LEDS_H

#include <stdbool.h>
#include <inttypes.h>

// LED IDs are 0xAB where A is the anode pin and B is the cathode pin
static const uint16_t LED_P5A = 0x0102u;
static const uint16_t LED_P5B = 0x0203u;
static const uint16_t LED_P3A = 0x0201u;
static const uint16_t LED_P3B = 0x0302u;
static const uint16_t LED_N12 = 0x0103u;
static const uint16_t LED_SFY = 0x0301u;

void set_led(uint16_t led, bool value);

void led_cycle(void);

#endif // LEDS_H
