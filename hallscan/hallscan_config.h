#ifndef HALLSCAN_CONFIG_H
#define HALLSCAN_CONFIG_H

#include <stdint.h>
#include "quantum.h"

// ========================================
// PIN ASSIGNMENTS - Change these for your board
// ========================================

// MUX control pins (HC4067 S0-S3)
#define MUX_S0_PIN  GP10
#define MUX_S1_PIN  GP11
#define MUX_S2_PIN  GP12
#define MUX_S3_PIN  GP13

// ADC pins for each MUX (RP2040 internal ADC)
#define MUX1_ADC_PIN GP26  // ADC0
#define MUX2_ADC_PIN GP27  // ADC1
#define MUX3_ADC_PIN GP28  // ADC2
#define MUX4_ADC_PIN GP29  // ADC3

// ========================================
// SENSOR BEHAVIOR SETTINGS
// ========================================

// Hall effect threshold - key is pressed when ADC is BELOW this value
#define SENSOR_THRESHOLD 440

// Debounce time in milliseconds
#define DEBOUNCE_MS 50

// Number of keys to track (should match your matrix size)
#define MAX_KEYS 48  // 4 rows x 12 cols

// ========================================
// SENSOR DEFINITIONS
// ========================================

// Sensor enum - These are the logical key names
// Note: Using 1-based numbering to match shego75 approach
// S_ESC = 1 means first sensor, maps to matrix position (0,0)
typedef enum sensor_names {
    S_ESC = 1, S_Q, S_W, S_E, S_R, S_T, S_Y, S_U, S_I, S_O, S_P, S_BSPC,
    S_TAB, S_A, S_S, S_D, S_F, S_G, S_H, S_J, S_K, S_L, S_SCLN, S_ENT,
    S_LSFT, S_Z, S_X, S_C, S_V, S_B, S_N, S_M, S_COMM, S_DOT, S_UP, S_RSFT,
    S_LCTL, S_WIN, S_LALT, S_MO1, S_TG3, S_SPC1, S_SPC2, S_FN, S_RALT, S_LEFT, S_DOWN, S_RGHT,

    SENSOR_COUNT_PLUS_1,
    
} sensor_names_t;

// Alias for compatibility
typedef sensor_names_t sensor_id_t;

#define SENSOR_COUNT (SENSOR_COUNT_PLUS_1 - 1)

// ========================================
// MUX CHANNEL MAPPING STRUCTURE
// ========================================

// Maps a MUX channel to a sensor ID
typedef struct {
    sensor_id_t sensor;
} mux16_ref_t;

#endif // HALLSCAN_CONFIG_H