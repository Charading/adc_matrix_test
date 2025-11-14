#ifndef HALLSCAN_H
#define HALLSCAN_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "hallscan_config.h"
#include "matrix.h"

/* ============================================================================
 * Low-level scanning API (ADC reading + mux control)
 * ============================================================================ */

typedef void (*hallscan_set_mux_t)(uint8_t select_index);
typedef void (*hallscan_set_mux_en_t)(uint8_t mux_index, bool enabled);
typedef uint16_t (*hallscan_read_int_adc_t)(uint8_t channel);
typedef uint16_t (*hallscan_read_ext_adc_t)(uint8_t channel);
typedef void (*hallscan_delay_us_t)(uint32_t us);

void hallscan_init(hallscan_set_mux_t set_mux,
                   hallscan_set_mux_en_t set_en,
                   hallscan_read_int_adc_t read_int,
                   hallscan_read_ext_adc_t read_ext,
                   hallscan_delay_us_t delay_us);

void hallscan_scan_blocking(void);
uint16_t hallscan_get(uint8_t mux_index, uint8_t select_index);
const uint16_t *hallscan_get_buffer(void);
size_t hallscan_get_buffer_size(void);

static inline uint32_t hallscan_get_scan_delay_ms(void) {
    return (uint32_t)HALLSCAN_SCAN_DELAY_MS;
}

/* ============================================================================
 * Keycode mapping API (sensor → keycode mapping with deviation detection)
 * ============================================================================ */

/* Per-channel payload: stores the 1-based sensor id (uint16_t). A value of
 * 0 indicates an unmapped channel. This is intentionally a small integer so
 * the module can live in flash and be copied into the internal flat buffer.
 */
typedef uint16_t hallscan_map_payload_t;  // 1-based sensor id (0 == unmapped)

/* Per-channel mux reference now stores a sensor id (index into
 * SENSOR_TO_KEYCODE) instead of a direct QMK keycode. This allows
 * wiring to be defined as sensor IDs (logical positions) and the
 * sensor->keycode table to be changed independently.
 */
typedef struct { uint16_t sensor; } mux16_ref_t;

void hallscan_map_register_mux(const mux16_ref_t *maps, size_t mux_count);
void hallscan_map_register_flat(const hallscan_map_payload_t *flat_map);
void hallscan_map_set_threshold(uint16_t threshold);
void hallscan_map_set_debounce_ms(uint32_t ms);
void hallscan_map_calibrate(size_t samples, uint32_t delay_ms);
void hallscan_map_set_sensitivity_percent(size_t sensor_idx, uint8_t percent);
void hallscan_map_set_global_sensitivity_percent(uint8_t percent);
void hallscan_map_process_once(void);

/* Fill a QMK matrix buffer from the internal sensor state.
 * This allows the keyboard-level custom matrix code to expose hall sensors
 * as regular matrix positions so VIA and the keymap pipeline work normally.
 *
 * The function returns true when the matrix changed since the last call.
 */
bool hallscan_fill_matrix(matrix_row_t current_matrix[]);

#endif // HALLSCAN_H
