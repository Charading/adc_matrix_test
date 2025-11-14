/* adc_matrix_test.c - Keyboard integration with hallscan_simple module
 * 
 * This file provides the glue between QMK and the hallscan module:
 * - Hardware initialization (pins, ADC)
 * - Hardware callbacks (mux select, ADC read)
 * - Sensor-to-mux channel mappings
 * - QMK matrix integration
 */

#include "quantum.h"
#include "hallscan/hallscan_simple.h"
#include "analog.h"
#include "wait.h"

// MUX control pins
#define MUX_S0_PIN  GP10
#define MUX_S1_PIN  GP11
#define MUX_S2_PIN  GP12
#define MUX_S3_PIN  GP13

// ADC pins for each MUX
#define MUX1_ADC_PIN GP26
#define MUX2_ADC_PIN GP27
#define MUX3_ADC_PIN GP28
#define MUX4_ADC_PIN GP29

// ============================================================================
// MUX CHANNEL MAPPINGS
// ============================================================================
// Each mux has 16 channels (0-15), and each channel maps to a sensor or is unmapped

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

static const mux16_ref_t mux4_channels[16] = {
    [0]  = { SENSOR_UNMAPPED },
    [1]  = { SENSOR_UNMAPPED },
    [2]  = { SENSOR_UNMAPPED },
    [3]  = { SENSOR_UNMAPPED },
    [4]  = { SENSOR_UNMAPPED },
    [5]  = { SENSOR_UNMAPPED },
    [6]  = { SENSOR_UNMAPPED },
    [7]  = { SENSOR_UNMAPPED },
    [8]  = { SENSOR_UNMAPPED },
    [9]  = { SENSOR_UNMAPPED },
    [10] = { SENSOR_UNMAPPED },
    [11] = { SENSOR_UNMAPPED },
    [12] = { SENSOR_UNMAPPED },
    [13] = { SENSOR_UNMAPPED },
    [14] = { SENSOR_UNMAPPED },
    [15] = { SENSOR_UNMAPPED },
};

// ============================================================================
// HARDWARE CALLBACKS
// ============================================================================

// Select mux channel (0-15) by setting S0-S3 pins
static void hw_set_mux_channel(uint8_t channel) {
    writePin(MUX_S0_PIN, (channel & 0x01) ? 1 : 0);
    writePin(MUX_S1_PIN, (channel & 0x02) ? 1 : 0);
    writePin(MUX_S2_PIN, (channel & 0x04) ? 1 : 0);
    writePin(MUX_S3_PIN, (channel & 0x08) ? 1 : 0);
    wait_us(100);
}

// Read ADC for given mux index (0 = MUX1, 1 = MUX2, etc.)
static uint16_t hw_read_adc(uint8_t mux_idx) {
    switch (mux_idx) {
        case 0: return analogReadPin(MUX1_ADC_PIN);
        case 1: return analogReadPin(MUX2_ADC_PIN);
        case 2: return analogReadPin(MUX3_ADC_PIN);
        case 3: return analogReadPin(MUX4_ADC_PIN);
        default: return 0;
    }
}

// ============================================================================
// QMK MATRIX INTEGRATION
// ============================================================================

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
    
    // Initialize hallscan module
    hallscan_init(hw_set_mux_channel, hw_read_adc);
    
    // Register mux channel mappings
    const mux16_ref_t *tables[4] = {
        mux1_channels,
        mux2_channels,
        mux3_channels,
        mux4_channels
    };
    hallscan_register_mux_tables(tables, 4);
    
    // Configure scanning parameters
    hallscan_set_threshold(440);  // ADC threshold for key press
    hallscan_set_debounce_ms(50); // Debounce time
    
    uprintf("[KEYBOARD] Matrix init complete\n");
}

bool matrix_scan_custom(matrix_row_t current_matrix[]) {
    return hallscan_scan_matrix(current_matrix);
}
