// HALLSCAN MODULE - Hall effect sensor matrix scanning implementation
// Based on shego75_breadboard approach

#include "hallscan.h"
#include "hallscan_config.h"
#include "quantum.h"
#include "analog.h"
#include "wait.h"
#include "timer.h"
#include "print.h"

// ========================================
// INTERNAL STATE
// ========================================

// Key state tracking for debouncing
static bool key_pressed[MAX_KEYS];
static uint32_t key_timer[MAX_KEYS];
static uint32_t last_debug_time = 0;

// Sensor name strings for debug output
static const char *sensor_names[SENSOR_COUNT] = {
    "Esc", "Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P", "Bspc",
    "Tab", "A", "S", "D", "F", "G", "H", "J", "K", "L", "Scln", "Ent",
    "LShft", "Z", "X", "C", "V", "B", "N", "M", "Comm", "Dot", "Up", "RShft",
    "LCtrl", "Win", "LAlt", "MO1", "TG3", "Spc1", "Spc2", "Fn", "RAlt", "Left", "Down", "Rght"
};

// ========================================
// MUX CHANNEL MAPPINGS
// ========================================
// These map physical MUX channels to logical sensor IDs
// The actual mappings are defined in hallscan_keymap.h

#include "hallscan_keymap.h"

// ========================================
// HELPER FUNCTIONS
// ========================================

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
    for (uint8_t i = 0; i < MAX_KEYS; i++) {
        key_pressed[i] = false;
        key_timer[i] = 0;
    }
    
    uprintf("[HALLSCAN] Matrix initialized - 4 MUXes, %d sensors max\n", SENSOR_COUNT);
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
            
            // Skip unmapped sensors
            if (key_mapping->sensor == SENSOR_UNMAPPED || key_mapping->sensor >= SENSOR_COUNT) {
                continue;
            }
            
            sensor_id_t sensor = key_mapping->sensor;
            
            // Convert sensor ID to 0-based index (enum is 1-based: S_ESC=1, S_Q=2, etc.)
            uint8_t sensor_idx = sensor - 1;
            
            // Convert sensor index to matrix position (0-based index into 4x12 matrix)
            uint8_t matrix_row = sensor_idx / MATRIX_COLS;
            uint8_t matrix_col = sensor_idx % MATRIX_COLS;
            
            // Check if this is within our 4x12 matrix
            if (matrix_row >= MATRIX_ROWS || matrix_col >= MATRIX_COLS) continue;
            
            // Calculate key index for debounce tracking (use 0-based index)
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
                        sensor_names[sensor_idx],
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
