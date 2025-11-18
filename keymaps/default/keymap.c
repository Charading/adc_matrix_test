// Copyright 2023 QMK
// SPDX-License-Identifier: GPL-2.0-or-later

#include QMK_KEYBOARD_H

/* hallscan test harness includes */
#include "hallscan/hallscan.h"
#include "hallscan/hallscan_reports.h"
#include "print.h"    // uprintf
#include "timer.h"    // timer_read / timer_elapsed
// raw_hid_send declaration
#include "raw_hid.h"

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    /*
     * ┌───┬───┬───┬───┬───┬───┬───┬───┬───┬───┬───┬───┐
     * │ A │ B │ C │ D │ E │ F │ G │ H │ I │ J │ K │ L │
     * ├───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┤
     * │ A │ B │ C │ D │ E │ F │ G │ H │ I │ J │ K │ L │
     * ├───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┤
     * │ A │ B │ C │ D │ E │ F │ G │ H │ I │ J │ K │ L │
     * ├───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┤
     * │ A │ B │ C │ D │ E │ F │ G │ H │ I │ J │ K │ L │
     * └───┴───┴───┴───┴───┴───┴───┴───┴───┴───┴───┴───┘
     */
    [0] = LAYOUT_ortho_4x12(
        KC_1,    KC_2,    KC_3,    KC_4,    KC_5,    KC_6,    KC_7,    KC_8,    KC_9,    KC_0,    KC_MINUS, KC_EQUAL,
        KC_Q,    KC_W,    KC_E,    KC_R,    KC_T,    KC_Y,    KC_U,    KC_I,    KC_O,    KC_P,    KC_LBRC, KC_RBRC,
        KC_A,    KC_S,    KC_D,    KC_F,    KC_G,    KC_H,    KC_J,    KC_K,    KC_L,    KC_SCLN, KC_QUOT, KC_BSLS,
        KC_Z,    KC_X,    KC_C,    KC_V,    KC_B,    KC_N,    KC_M,    KC_COMM, KC_DOT,  KC_SLSH, KC_SPC, KC_CAPS
    )
};

// --- minimal user-space state -------------------------

static uint32_t hallscan_last_ms = 0;

void matrix_init_user(void) {
    // NOTE: Initialization moved to adc_matrix_test.c keyboard_post_init_user()
    // to avoid duplicate registration that would reset calibration state.
    // Sensor mapping is configured via hallscan/hallscan_keymap.h
    // (mux1_channels through mux5_channels arrays)
    hallscan_last_ms = timer_read();
}

// matrix_scan_user is implemented in the keyboard-level source (adc_matrix_test.c)
// to avoid duplicate definitions when building the keyboard. Keep this file
// light-weight and rely on the keyboard-level scan implementation.

// --- GPIO8 control for HID commands ---
#define LED_TOG_PIN GP8
static bool gpio_state = false;

void keyboard_post_init_user(void) {
    setPinOutput(LED_TOG_PIN);
    writePinHigh(LED_TOG_PIN);
    gpio_state = true;
    uprintf("[KEYMAP] GPIO8 initialized HIGH\n");

    // Quick boot blink to verify LED/transistor wiring (2 pulses)
    writePinLow(LED_TOG_PIN);
    wait_ms(150);
    writePinHigh(LED_TOG_PIN);
    wait_ms(150);
    writePinLow(LED_TOG_PIN);
    wait_ms(150);
    writePinHigh(LED_TOG_PIN);
    // restore state variable reflects final pin state (HIGH)
    gpio_state = true;
}

static void gpio8_set(bool on) {
    gpio_state = on;
    writePin(LED_TOG_PIN, gpio_state);
}

static void gpio8_toggle(void) {
    gpio8_set(!gpio_state);
}

// --- Raw HID handler (VIA forwards unrecognized commands here) ---
// Use a shared processor that returns handled=true if this module handled the
// incoming packet. This allows both `raw_hid_receive_kb` and `via_command_kb`
// to call the same logic depending on how VIA/QMK forwards packets.
static bool process_rawhid_command(uint8_t *data, uint8_t length) {
    uprintf("[KEYMAP] raw_hid_receive_kb CALLED! len=%d\n", length);
    if (length == 0) {
        uprintf("[KEYMAP] len=0\n");
        return false;
    }

    // Log first 8 bytes
    uprintf("[KEYMAP] RX: %02X %02X %02X %02X %02X %02X %02X %02X\n",
           data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7]);

    if (length < 32) {
        uprintf("[KEYMAP] Note: received %d bytes (expected 32)\n", length);
        // continue - some hosts/platforms send shorter reports
    }

    // Heuristic: commands may arrive in different positions depending on host.
    // Search first 8 bytes for a known command byte, prefer that. Fall back
    // to data[0] if none found.
    uint8_t cmd = 0;
    const uint8_t known_cmds[] = { 'T','S','C','Q','B' };
    for (uint8_t i = 0; i < 8; ++i) {
        for (uint8_t k = 0; k < sizeof(known_cmds); ++k) {
            if (data[i] == known_cmds[k]) {
                cmd = data[i];
                uprintf("[KEYMAP] Found cmd at offset %d: 0x%02X\n", i, cmd);
                break;
            }
        }
        if (cmd) break;
    }
    if (!cmd) {
        // If data[0] is 0xFF (VIA custom marker), command is in data[1]
        if (data[0] == 0xFF && data[1] != 0x00) {
            cmd = data[1];
            uprintf("[KEYMAP] Using VIA marker cmd at data[1]=0x%02X\n", cmd);
        } else if (data[1] != 0x00) {
            // Try position 1 
            cmd = data[1];
            uprintf("[KEYMAP] Using data[1]=0x%02X\n", cmd);
        } else {
            cmd = data[0];
            uprintf("[KEYMAP] Defaulting to data[0]=0x%02X\n", cmd);
        }
    }
    bool handled = true;

    uprintf("[KEYMAP] Processing cmd=0x%02X ('%c')\n", cmd, (cmd >= 32 && cmd < 127) ? cmd : '?');
    switch (cmd) {
        case 'T':
            uprintf("[KEYMAP] Toggling GPIO8 (was %d)\n", gpio_state);
            gpio8_toggle();
            uprintf("[KEYMAP] GPIO8 now %d\n", gpio_state);
            break;
        case 'B':
            // Debug blink: pulse the pin visibly so user can confirm wiring
            uprintf("[KEYMAP] BLINK test - pulsing GPIO8\n");
            for (int i = 0; i < 4; ++i) {
                writePinLow(LED_TOG_PIN);
                wait_ms(120);
                writePinHigh(LED_TOG_PIN);
                wait_ms(120);
            }
            // leave gpio_state reflecting pin HIGH
            gpio_state = true;
            break;
        case 'S':
            uprintf("[KEYMAP] Setting GPIO8 HIGH\n");
            gpio8_set(true);
            break;
        case 'C':
            uprintf("[KEYMAP] Setting GPIO8 LOW\n");
            gpio8_set(false);
            break;
        case 'Q':
            uprintf("[KEYMAP] Query only\n");
            break;
        default:
            uprintf("[KEYMAP] Unknown cmd=0x%02X\n", cmd);
            handled = false;
            break;
    }

    if (!handled) return false;

    uint8_t response[32];
    memset(response, 0, sizeof(response));
    response[0] = 0x01; // Success marker
    response[1] = cmd;  // Echo command
    response[2] = gpio_state ? 1 : 0; // GPIO state
    // Echo first 8 bytes of received payload for debugging on host side
    for (uint8_t i = 0; i < 8; ++i) response[3 + i] = data[i];

    uprintf("[KEYMAP] Sending response: SUCCESS cmd=%c state=%d\n", cmd, gpio_state);
    uprintf("[KEYMAP] TX: %02X %02X %02X %02X %02X %02X %02X %02X\n",
           response[0], response[1], response[2], response[3], 
           response[4], response[5], response[6], response[7]);

    raw_hid_send(response, sizeof(response));
    uprintf("[KEYMAP] Response sent\n");
    return true;
}

// QMK will call this when VIA forwards unrecognized commands to the keyboard
void raw_hid_receive_kb(uint8_t *data, uint8_t length) {
    (void)process_rawhid_command(data, length);
}

#if defined(VIA_ENABLE)
// Some VIA-enabled QMK forks call `via_command_kb` instead — implement it
// so our handler is invoked regardless of which hook VIA uses.
bool via_command_kb(uint8_t *data, uint8_t length) {
    uprintf("[KEYMAP] via_command_kb ENTRY! len=%d\n", length);
    return process_rawhid_command(data, length);
}
#endif