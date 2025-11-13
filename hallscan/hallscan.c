#include "hallscan.h"
#include <stddef.h>


/* Internal state and hooks */
static hallscan_set_mux_t   s_set_mux = NULL;
static hallscan_set_mux_en_t s_set_en = NULL;
static hallscan_read_int_adc_t s_read_int = NULL;
static hallscan_read_ext_adc_t s_read_ext = NULL;
static hallscan_delay_us_t s_delay_us = NULL;

#ifndef HC4067_COUNT
#error "HC4067_COUNT must be defined in hallscan_config.h"
#endif

#if HC4067_COUNT == 0
#error "HC4067_COUNT must be at least 1"
#endif

static uint16_t s_buf[HC4067_COUNT * 16u];

static inline size_t idx(uint8_t mux, uint8_t sel) {
    return ((size_t)mux * 16u) + (size_t)(sel & 0x0Fu);
}

void hallscan_init(hallscan_set_mux_t set_mux,
                   hallscan_set_mux_en_t set_en,
                   hallscan_read_int_adc_t read_int,
                   hallscan_read_ext_adc_t read_ext,
                   hallscan_delay_us_t delay_us) {
    s_set_mux = set_mux;
    s_set_en = set_en;
    s_read_int = read_int;
    s_read_ext = read_ext;
    s_delay_us = delay_us;
    for (size_t i = 0; i < (size_t)HC4067_COUNT * 16u; ++i) s_buf[i] = 0;

#ifdef ADC_MATRIX_TEST_MUX_MAPS_H
    hallscan_map_register_mux((const mux16_ref_t *)mux_maps, HC4067_COUNT);
#endif
}

void hallscan_scan_blocking(void) {
    if (!s_set_mux || !s_delay_us) return;

    for (uint8_t sel = 0; sel < 16; ++sel) {
        s_set_mux(sel);
        s_delay_us((uint32_t)HALLSCAN_SETTLE_US);

        for (uint8_t m = 0; m < (uint8_t)HC4067_COUNT; ++m) {
            if (s_set_en) s_set_en(m, true);

            const hallscan_mux_map_t map = HALLSCAN_MUX_TO_ADC[m];
            uint16_t val = 0;

            if (map.src == HALLSCAN_ADC_INT) {
                if (s_read_int) {
                    val = s_read_int(map.channel);
                }
            } else {
                if (s_read_ext) {
                    val = s_read_ext(map.channel);
                }
            }

            s_buf[idx(m, sel)] = val;

            if (s_set_en) s_set_en(m, false);
        }
    }
}

uint16_t hallscan_get(uint8_t mux_index, uint8_t select_index) {
    if (mux_index >= (uint8_t)HC4067_COUNT) return 0;
    select_index &= 0x0Fu;
    return s_buf[idx(mux_index, select_index)];
}

const uint16_t *hallscan_get_buffer(void) {
    return s_buf;
}

size_t hallscan_get_buffer_size(void) {
    return (size_t)HC4067_COUNT * 16u;
}

/* ============================================================================
 * Keycode mapping implementation (sensor → keycode with deviation detection)
 * ============================================================================ */

#include <string.h>
#include <stdbool.h>

// QMK headers for emitting keycodes and timers
#include "quantum.h"
#include "timer.h"
#include "wait.h"

// Internal state
static const hallscan_map_payload_t *s_map = NULL; // flat map pointer
static uint16_t s_threshold = 0u;
static uint32_t s_debounce_ms = 200u;
static uint16_t s_release_threshold = 0u;

// per-sensor state arrays (HC4067_COUNT is compile-time constant)
static bool s_pressed[HC4067_COUNT * 16u];
static uint32_t s_last_change_ms[HC4067_COUNT * 16u];
// Internal owned copy used when callers register per-mux arrays
static hallscan_map_payload_t s_map_internal[HC4067_COUNT * 16u];

/* Shego-style calibration state */
static uint32_t s_accum[HC4067_COUNT * 16u];
static uint16_t s_baseline[HC4067_COUNT * 16u];
static uint16_t s_min[HC4067_COUNT * 16u];
static uint16_t s_max[HC4067_COUNT * 16u];
static uint8_t s_sensitivity_percent[HC4067_COUNT * 16u];
static bool s_calibrated = false;


void hallscan_map_register_flat(const hallscan_map_payload_t *flat_map) {
    s_map = flat_map;
    /* If s_threshold wasn't explicitly set, compute a sensible default.
     * Prefer percent-based configuration (HALLSCAN_THRESHOLD_PERCENT +
     * HALLSCAN_ADC_MAX). Fall back to HALLSCAN_DISPLAY_THRESHOLD.
     */
    if (s_threshold == 0u) {
#if defined(HALLSCAN_THRESHOLD_PERCENT) && defined(HALLSCAN_ADC_MAX)
        uint32_t t = ((uint32_t)HALLSCAN_THRESHOLD_PERCENT * (uint32_t)HALLSCAN_ADC_MAX) / 100u;
        if (t > 0xFFFFu) t = 0xFFFFu;
        s_threshold = (uint16_t)t;
#if defined(HALLSCAN_HYST_PERCENT)
    uint32_t rel = (uint32_t)s_threshold;
    rel = rel - ((rel * (uint32_t)HALLSCAN_HYST_PERCENT) / 100u);
    if (rel > 0xFFFFu) rel = 0xFFFFu;
    s_release_threshold = (uint16_t)rel;
#else
    s_release_threshold = s_threshold - (s_threshold / 10u); /* default 10% */
#endif
#else
        s_threshold = (uint16_t)HALLSCAN_DISPLAY_THRESHOLD;
#if defined(HALLSCAN_HYST_PERCENT)
    uint32_t rel = (uint32_t)s_threshold;
    rel = rel - ((rel * (uint32_t)HALLSCAN_HYST_PERCENT) / 100u);
    if (rel > 0xFFFFu) rel = 0xFFFFu;
    s_release_threshold = (uint16_t)rel;
#else
    s_release_threshold = s_threshold - (s_threshold / 10u);
#endif
#endif
    }
    // initialize state: no pending change timestamp (0 == none)
    for (size_t i = 0; i < (size_t)HC4067_COUNT * 16u; ++i) {
        s_pressed[i] = false;
        s_last_change_ms[i] = 0u;
        s_accum[i] = 0u;
        s_baseline[i] = 0u;
        s_min[i] = 0xFFFFu;
        s_max[i] = 0u;
        s_sensitivity_percent[i] = 20u; /* default sens: 20% tolerance (less sensitive) */
        s_calibrated = false;
    }
}

void hallscan_map_register_mux(const mux16_ref_t *maps, size_t mux_count) {
    if (!maps) return;
    size_t copy_count = mux_count;
    if (copy_count > (size_t)HC4067_COUNT) copy_count = (size_t)HC4067_COUNT;

    // copy mux_count * 16 entries into internal flat buffer
    for (size_t m = 0; m < copy_count; ++m) {
        for (size_t s = 0; s < 16u; ++s) {
            size_t src_idx = m * 16u + s;
            size_t dst_idx = src_idx;
            uint16_t sensor = maps[src_idx].sensor; // 1-based sensor id (0 == unconnected)
            if (sensor >= 1u && sensor <= (uint16_t)SENSOR_COUNT) {
                s_map_internal[dst_idx] = sensor; // store sensor id (1-based)
            } else {
                s_map_internal[dst_idx] = 0u; // unmapped
            }
        }
    }

    // for any remaining mux slots up to HC4067_COUNT, mark as unmapped (0)
    for (size_t i = copy_count * 16u; i < (size_t)HC4067_COUNT * 16u; ++i) {
        s_map_internal[i] = 0u;
    }

    // point s_map to our internal buffer and initialize state
    hallscan_map_register_flat(s_map_internal);
}

void hallscan_map_set_threshold(uint16_t threshold) {
    s_threshold = threshold;
}

void hallscan_map_set_debounce_ms(uint32_t ms) {
    s_debounce_ms = ms;
}

/* Shego-style calibration: perform `samples` full scans with given delay
 * between scans, accumulate averages and compute per-sensor baseline/min/max.
 */
void hallscan_map_calibrate(size_t samples, uint32_t sample_delay_ms) {
    if (samples == 0) return;
    for (size_t i = 0; i < (size_t)HC4067_COUNT * 16u; ++i) {
        s_accum[i] = 0u;
        s_min[i] = 0xFFFFu;
        s_max[i] = 0u;
    }

    for (size_t pass = 0; pass < samples; ++pass) {
        hallscan_scan_blocking();
        for (uint8_t m = 0; m < (uint8_t)HC4067_COUNT; ++m) {
            for (uint8_t s = 0; s < 16u; ++s) {
                size_t idx = (size_t)m * 16u + (size_t)s;
                uint16_t v = hallscan_get(m, s);
                s_accum[idx] += (uint32_t)v;
                if (v < s_min[idx]) s_min[idx] = v;
                if (v > s_max[idx]) s_max[idx] = v;
            }
        }
        if (sample_delay_ms) wait_ms((int)sample_delay_ms);
    }

    for (size_t i = 0; i < (size_t)HC4067_COUNT * 16u; ++i) {
        s_baseline[i] = (uint16_t)(s_accum[i] / samples);
        /* clamp baseline */
        if (s_baseline[i] == 0) s_baseline[i] = 1;
        /* leave sensitivity at default unless caller changes it */
    }
    s_calibrated = true;
}

void hallscan_map_set_sensitivity_percent(size_t sensor_idx, uint8_t percent) {
    if (sensor_idx >= (size_t)HC4067_COUNT * 16u) return;
    if (percent < 1) percent = 1;
    if (percent > 90) percent = 90;
    s_sensitivity_percent[sensor_idx] = percent;
}

void hallscan_map_set_global_sensitivity_percent(uint8_t percent) {
    if (percent < 1) percent = 1;
    if (percent > 90) percent = 90;
    for (size_t i = 0; i < (size_t)HC4067_COUNT * 16u; ++i) {
        s_sensitivity_percent[i] = percent;
    }
}

void hallscan_map_process_once(void) {
    if (!s_map) return;

    const uint32_t now = timer_read();

    for (uint8_t m = 0; m < (uint8_t)HC4067_COUNT; ++m) {
        for (uint8_t s = 0; s < 16u; ++s) {
            size_t idx = (size_t)m * 16u + (size_t)s;
            uint16_t raw = hallscan_get(m, s);

            bool should_press = false;
            if (s_calibrated) {
                uint16_t base = s_baseline[idx] ? s_baseline[idx] : 1u;
                uint8_t sens = s_sensitivity_percent[idx] ? s_sensitivity_percent[idx] : 10u;

                uint32_t lower = ((uint32_t)base * (100u - (uint32_t)sens)) / 100u;
                uint32_t upper = ((uint32_t)base * (100u + (uint32_t)sens)) / 100u;
                if (lower < 1u) lower = 1u;
                if (upper > 0xFFFFu) upper = 0xFFFFu;

                should_press = (raw < (uint16_t)lower) || (raw > (uint16_t)upper);
            } else {
                /* Use hysteresis: when currently pressed, require raw >= release_threshold
                 * to remain pressed. When currently not pressed, require raw >= press_threshold
                 * to become pressed.
                 */
                should_press = s_pressed[idx] ? (raw >= s_release_threshold) : (raw >= s_threshold);
            }

            if (should_press != s_pressed[idx]) {
                /* Potential state change: start debounce timer if not started,
                 * otherwise, if timer expired commit the change.
                 */
                if (s_last_change_ms[idx] == 0u) {
                    s_last_change_ms[idx] = now; // start debounce window
                } else if ((now - s_last_change_ms[idx]) >= s_debounce_ms) {
                    // commit change
                    s_pressed[idx] = should_press;
                    s_last_change_ms[idx] = 0u; // clear pending

                    /* Payload is a sensor id (1-based), not a keycode. We record
                     * the pressed state and let the matrix-fill helper expose
                     * the pressed sensors as matrix bits for VIA/keymap lookup.
                     */
                    hallscan_map_payload_t payload = s_map[idx];
#if defined(CONSOLE_ENABLE) || defined(PRINT_ENABLE)
                    /* Only log PRESS events to reduce spam */
                    if (should_press && payload != 0) {
                        if (s_calibrated) {
                            uprintf("[KEY PRESS] sensor=%u mux=%u sel=%u raw=%u base=%u sens=%u%%\n",
                                    (unsigned)payload, (unsigned)m, (unsigned)s, (unsigned)raw,
                                    (unsigned)s_baseline[idx], (unsigned)s_sensitivity_percent[idx]);
                        } else {
                            uprintf("[KEY PRESS] sensor=%u mux=%u sel=%u raw=%u\n",
                                    (unsigned)payload, (unsigned)m, (unsigned)s, (unsigned)raw);
                        }
                    }
#endif
                    /* store pressed state only; payload==0 means unmapped */
                    (void)payload;
                }
            } else {
                /* Stable state: clear any pending debounce timer */
                s_last_change_ms[idx] = 0u;
            }
        }
    }
}

bool hallscan_fill_matrix(matrix_row_t current_matrix[]) {
    if (!current_matrix) return false;
    bool changed = false;

    // clear output
    for (uint8_t r = 0; r < MATRIX_ROWS; ++r) current_matrix[r] = 0;

    for (size_t i = 0; i < (size_t)HC4067_COUNT * 16u; ++i) {
        if (!s_pressed[i]) continue;
        uint16_t sensor = s_map[i]; /* 1-based sensor id */
        if (sensor == 0u) continue;
        uint16_t sensor_id = sensor - 1u; /* 0-based index */
        uint8_t row = (uint8_t)(sensor_id / MATRIX_COLS);
        uint8_t col = (uint8_t)(sensor_id % MATRIX_COLS);
        if (row >= MATRIX_ROWS || col >= MATRIX_COLS) continue;
        uint16_t before = current_matrix[row];
        current_matrix[row] |= (1UL << col);
        if (current_matrix[row] != before) changed = true;
    }

    return changed;
}
