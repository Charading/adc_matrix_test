// SIMPLIFIED MUX ADC IMPLEMENTATION - Direct copy of shego75_breadboard approach
// This bypasses the hallscan module entirely and does everything in matrix_scan_custom

#include QMK_KEYBOARD_H
#include "quantum.h"
#include "matrix.h"
#include "analog.h"
#include "wait.h"
#include "timer.h"
#include "print.h"

// Hall effect threshold - key is pressed when ADC is BELOW this value
#define SENSOR_THRESHOLD 440
#define DEBOUNCE_MS 50

// MUX control pins
#define MUX_S0_PIN  GP10
#define MUX_S1_PIN  GP11
#define MUX_S2_PIN  GP12
#define MUX_S3_PIN  GP13

// ADC pins for each MUX (1-based naming)
#define MUX1_ADC_PIN GP26
#define MUX2_ADC_PIN GP27
#define MUX3_ADC_PIN GP28
#define MUX4_ADC_PIN GP29

// Key state tracking for 48 keys (4 rows x 12 cols)
static bool key_pressed[48];
static uint32_t key_timer[48];
static uint32_t last_debug_time = 0;

// Sensor enum (1-based to match hallscan_config.h)
typedef enum {
    S_ESC = 1, S_Q, S_W, S_E, S_R, S_T, S_Y, S_U, S_I, S_O, S_P, S_BSPC,
    S_TAB, S_A, S_S, S_D, S_F, S_G, S_H, S_J, S_K, S_L, S_SCLN, S_ENT,
    S_LSFT, S_Z, S_X, S_C, S_V, S_B, S_N, S_M, S_COMM, S_DOT, S_UP, S_RSFT,
    S_LCTL, S_WIN, S_LALT, S_MO1, S_TG3, S_SPC1, S_SPC2, S_FN, S_RALT, S_LEFT, S_DOWN, S_RGHT,
    SENSOR_COUNT
} sensor_id_t;

// Sensor name strings for debug
static const char *sensor_names[SENSOR_COUNT] = {
    "Esc", "Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P", "Bspc",
    "Tab", "A", "S", "D", "F", "G", "H", "J", "K", "L", "Scln", "Ent",
    "LShft", "Z", "X", "C", "V", "B", "N", "M", "Comm", "Dot", "Up", "RShft",
    "LCtrl", "Win", "LAlt", "MO1", "TG3", "Spc1", "Spc2", "Fn", "RAlt", "Left", "Down", "Rght"
};

// MUX channel mapping structure
typedef struct {
    sensor_id_t sensor;
} mux16_ref_t;

// MUX1 mappings (copy from your hallscan_keymap.h)
static const mux16_ref_t mux1_channels[16] = {
    [0]  = { S_E },
    [1]  = { S_D },
    [2]  = { S_C },
    [3]  = { S_MO1 },
    [4]  = { S_W },
    [5]  = { S_S },
    [6]  = { S_X },
    [7]  = { S_LALT },
    [8]  = { S_Q },
    [9]  = { S_A },
    [10] = { S_Z },
    [11] = { S_WIN },
    [12] = { S_ESC },
    [13] = { S_TAB },
    [14] = { S_LSFT },
    [15] = { S_LCTL },
};

// MUX2 mappings
static const mux16_ref_t mux2_channels[16] = {
    [0]  = { S_U },
    [1]  = { S_J },
    [2]  = { S_M },
    [3]  = { S_FN },
    [4]  = { S_Y },
    [5]  = { S_H },
    [6]  = { S_N },
    [7]  = { S_SPC2 },
    [8]  = { S_T },
    [9]  = { S_G },
    [10] = { S_B },
    [11] = { S_SPC1 },
    [12] = { S_R },
    [13] = { S_F },
    [14] = { S_V },
    [15] = { S_TG3 },
};

// MUX3 mappings
static const mux16_ref_t mux3_channels[16] = {
    [0]  = { S_BSPC },
    [1]  = { S_ENT },
    [2]  = { S_RSFT },
    [3]  = { S_RGHT },
    [4]  = { S_P },
    [5]  = { S_SCLN },
    [6]  = { S_UP },
    [7]  = { S_DOWN },
    [8]  = { S_O },
    [9]  = { S_L },
    [10] = { S_DOT },
    [11] = { S_LEFT },
    [12] = { S_I },
    [13] = { S_K },
    [14] = { S_COMM },
    [15] = { S_RALT },
};

// MUX4 mappings (all unmapped for now; use 0 to indicate unmapped)
static const mux16_ref_t mux4_channels[16] = {
    [0]  = { 0 },
    [1]  = { 0 },
    [2]  = { 0 },
    [3]  = { 0 },
    [4]  = { 0 },
    [5]  = { 0 },
    [6]  = { 0 },
    [7]  = { 0 },
    [8]  = { 0 },
    [9]  = { 0 },
    [10] = { 0 },
    [11] = { 0 },
    [12] = { 0 },
    [13] = { 0 },
    [14] = { 0 },
    [15] = { 0 },
};

static void select_mux_channel(uint8_t channel) {
    writePin(MUX_S0_PIN, (channel & 0x01) ? 1 : 0);
    writePin(MUX_S1_PIN, (channel & 0x02) ? 1 : 0);
    writePin(MUX_S2_PIN, (channel & 0x04) ? 1 : 0);
    writePin(MUX_S3_PIN, (channel & 0x08) ? 1 : 0);
    wait_us(100);
}

void matrix_init_custom(void) {
    // Setup MUX control pins
    setPinOutput(MUX_S0_PIN);
    setPinOutput(MUX_S1_PIN);
    setPinOutput(MUX_S2_PIN);
    setPinOutput(MUX_S3_PIN);
    
    // Initialize ADC pins
    setPinInputHigh(MUX1_ADC_PIN);
    setPinInputHigh(MUX2_ADC_PIN);
    setPinInputHigh(MUX3_ADC_PIN);
    setPinInputHigh(MUX4_ADC_PIN);
    
    // Initialize state
    for (uint8_t i = 0; i < 48; i++) {
        key_pressed[i] = false;
        key_timer[i] = 0;
    }
    
    uprintf("[MUX_INIT] Simple MUX ADC initialized - 4 muxes, 48 sensors\n");
}

bool matrix_scan_custom(matrix_row_t current_matrix[]) {
    bool changed = false;
    uint32_t now = timer_read32();
    
    // Clear matrix
    for (uint8_t row = 0; row < MATRIX_ROWS; row++) {
        current_matrix[row] = 0;
    }
    
    // Debug every 1 second
    bool debug_this_scan = (timer_elapsed32(last_debug_time) >= 1000);
    if (debug_this_scan) {
        last_debug_time = now;
        uprintf("[MUX_SCAN] Scanning...\n");
    }
    
    // Array of ADC pins and MUX tables
    pin_t adc_pins[4] = {MUX1_ADC_PIN, MUX2_ADC_PIN, MUX3_ADC_PIN, MUX4_ADC_PIN};
    const mux16_ref_t* mux_tables[4] = {mux1_channels, mux2_channels, mux3_channels, mux4_channels};
    
    // Scan all 4 MUXes
    for (uint8_t mux_idx = 0; mux_idx < 4; mux_idx++) {
        // Scan all 16 channels on this MUX
        for (uint8_t ch = 0; ch < 16; ch++) {
            select_mux_channel(ch);
            wait_us(100);
            
            uint16_t adc_val = analogReadPin(adc_pins[mux_idx]);
            
            // Get key mapping from the table
            const mux16_ref_t* key_mapping = &mux_tables[mux_idx][ch];
            
            // Skip unmapped or out-of-range sensors (0 = unmapped)
            if (key_mapping->sensor == 0 || key_mapping->sensor > SENSOR_COUNT) {
                continue;
            }
            
            sensor_id_t sensor = key_mapping->sensor;

            // Convert sensor ID to 0-based index (enum is 1-based)
            uint8_t sensor_idx = sensor - 1;

            // Convert sensor index to matrix position (0-based index into 4x12 matrix)
            uint8_t matrix_row = sensor_idx / MATRIX_COLS;
            uint8_t matrix_col = sensor_idx % MATRIX_COLS;
            
            // Check if this is within our 4x12 matrix
            if (matrix_row >= MATRIX_ROWS || matrix_col >= MATRIX_COLS) continue;
            
            // Calculate key index for debounce tracking (0-based)
            uint8_t key_idx = sensor_idx;
            
            // KEY LOGIC: Key is pressed when ADC value is BELOW threshold
            bool should_press = (adc_val < SENSOR_THRESHOLD);
            
            if (debug_this_scan && mux_idx == 0 && ch < 4) {
                    uprintf("  MUX%d CH%d: %s ADC=%d %s\n", 
                    mux_idx+1, ch, sensor_names[sensor_idx], adc_val,
                    should_press ? "PRESS" : "");
            }
            
            // Debounce check
            if (timer_elapsed32(key_timer[key_idx]) > DEBOUNCE_MS) {
                if (should_press != key_pressed[key_idx]) {
                    key_pressed[key_idx] = should_press;
                    key_timer[key_idx] = now;
                    changed = true;
                    
                    uprintf("Key %s: %s (R%d C%d) ADC=%d\n", 
                        sensor_names[sensor],
                        should_press ? "PRESS" : "RELEASE", 
                        matrix_row, matrix_col, adc_val);
                }
            }
            
            // Set key in matrix if pressed
            if (key_pressed[key_idx]) {
                current_matrix[matrix_row] |= (1 << matrix_col);
            }
        }
    }
    
    return changed;
}
