#include "quantum.h"
#include "raw_hid.h"
#include "print.h"
#include "config.h"

#define LED_TOG_PIN GP8
static bool gpio_state = false;

void led_toggle_init(void) {
    setPinOutput(LED_TOG_PIN);
    writePinHigh(LED_TOG_PIN);
    gpio_state = true;
}

// Initialization only here; GPIO control lives in keymap.c (raw_hid handler)
// Keep a small API if other keyboard-level code needs to read state later.
bool adc_matrix_get_gpio8_state(void) {
    return gpio_state;
}

void keyboard_post_init_kb(void) {
    led_toggle_init();
    keyboard_post_init_user(); // Call user-level init from keymap
}
