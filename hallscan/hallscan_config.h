#ifndef HALLSCAN_CONFIG_H
#define HALLSCAN_CONFIG_H

#include <stdint.h>
#include "quantum.h" // for KC_NO / keycode constants used in SENSOR_TO_KEYCODE

// Number of HC4067 mux chips (one per ADC channel)
#define HC4067_COUNT 4  // Only using 4 muxes (MUX5 disabled for now)

// ADC source for each mux: external MCP3208 (EXT) or RP2040 internal ADC (INT).
// Use the HALLSCAN_* names to avoid accidental collisions with other symbols.
typedef enum { HALLSCAN_ADC_EXT = 0, HALLSCAN_ADC_INT = 1 } hallscan_adc_t;

typedef struct {
    hallscan_adc_t src;    // HALLSCAN_ADC_EXT or HALLSCAN_ADC_INT
    uint8_t channel;       // MCP3208 channel 0..7 for EXT, rp2040 ADC 0..2 for INT
} hallscan_mux_map_t;

// Map each HC4067 (index 0..HC4067_COUNT-1) to an ADC source + channel.
// Keep this static const in the header for easy per-board configuration.
static const hallscan_mux_map_t HALLSCAN_MUX_TO_ADC[HC4067_COUNT] = {
    { HALLSCAN_ADC_INT, 0 },
    { HALLSCAN_ADC_INT, 1 },
    { HALLSCAN_ADC_INT, 2 },
    { HALLSCAN_ADC_INT, 3 },
    // { HALLSCAN_ADC_EXT, 0 },  // MUX5 disabled
};

/* The board-specific 1-based sensor enum lives here. This enum provides
 * human-friendly sensor identifiers (S_ESC = 1 ...). It must be kept in
 * this config header so all compilation units see the names without
 * include-order issues.
 *
 * If you edit the named sensors, use the literal 0 for unmapped
 * channels. The sentinel SENSOR_COUNT_PLUS_1 is the last enumerator and
 * can be used as a compile-time count (enum constant). Use
 * SENSOR_COUNT for the usable count (SENSOR_COUNT_PLUS_1 - 1).
 */
typedef enum sensor_names {
    S_ESC = 1, S_Q, S_W, S_E, S_R, S_T, S_Y, S_U, S_I, S_O, S_P, S_BSPC,
    S_TAB, S_A, S_S, S_D, S_F, S_G, S_H, S_J, S_K, S_L, S_SCLN, S_ENT,
    S_LSFT, S_Z, S_X, S_C, S_V, S_B, S_N, S_M, S_COMM, S_DOT, S_UP, S_RSFT,
    S_LCTL, S_WIN, S_LALT, S_MO1, S_TG3, S_SPC1, S_SPC2, S_FN, S_RALT, S_LEFT, S_DOWN, S_RGHT,

    SENSOR_COUNT_PLUS_1,
} sensor_names_t;

#define SENSOR_COUNT (SENSOR_COUNT_PLUS_1 - 1)


#define SENSOR_COUNT_U ((uint16_t)(SENSOR_COUNT))
#define SENSOR_ID_TO_INDEX(id) \
        ( ((uint16_t)(id) == 0u) ? (size_t)-1 : \
            ( ((uint16_t)(id) <= SENSOR_COUNT_U) ? (size_t)((uint16_t)(id) - 1u) : (size_t)-1 ) )


/* Sensor -> keycode mapping is board-specific and must be provided by the
 * board-level header `hallscan_keymap.h`. That file should define the
 * concrete mapping (e.g. which sensor id corresponds to which KC_*), and
 * only it (plus this `hallscan_config.h`) should need editing when
 * adding support for another board.
 */
/* Sensor -> keycode mapping is defined in the board-level header
 * `hallscan_keymap.h`. `SENSOR_TO_KEYCODE` is sized to
 * SENSOR_COUNT_PLUS_1 so boards can index it directly by 1-based sensor id.
 */

/* Board-level mapping array should be big enough to index by the 1-based
 * sensor id directly (i.e. SENSOR_TO_KEYCODE[sensor_id]). Use the
 * SENSOR_COUNT_PLUS_1 size so index 0 is available as the unmapped (0)
 * sentinel.
 */
extern const uint16_t SENSOR_TO_KEYCODE[SENSOR_COUNT_PLUS_1];

// ADC pins for each MUX (1-based: MUX1-MUX5)
#define MUX1_ADC_PIN GP26
#define MUX2_ADC_PIN GP27
#define MUX3_ADC_PIN GP28
#define MUX4_ADC_PIN GP29
// #define MUX5_ADC_PIN GP29  // Note: MUX5 uses external MCP3208, not RP2040 ADC

// MUX select pins (S0..S3)
#define MUX_S0_PIN  (10)
#define MUX_S1_PIN  (11)
#define MUX_S2_PIN  (12)
#define MUX_S3_PIN  (13)

// Optional: per-mux enable pins (active-low). Use HALLSCAN_EN_PIN_UNUSED if the
// MUX_EN is tied to GND. Length must equal HC4067_COUNT.
#define HALLSCAN_EN_PIN_UNUSED  (-1)
static const int HALLSCAN_EN_PINS[HC4067_COUNT] = {
    /* MUX 0 */ HALLSCAN_EN_PIN_UNUSED,
    /* MUX 1 */ HALLSCAN_EN_PIN_UNUSED,
    /* MUX 2 */ HALLSCAN_EN_PIN_UNUSED,
    /* MUX 3 */ HALLSCAN_EN_PIN_UNUSED,
    // /* MUX 4 */ HALLSCAN_EN_PIN_UNUSED,  // MUX5 disabled
};

// SPI pins for MCP3208 (only used when a mapping entry uses HALL_ADC_EXT)
// Allow board or platform config to override these by checking for prior
// definitions to avoid redefinition warnings/errors.
#ifndef SPI_SCK_PIN
#define SPI_SCK_PIN   (18)
#endif
#ifndef SPI_MOSI_PIN
#define SPI_MOSI_PIN  (19)
#endif
#ifndef SPI_MISO_PIN
#define SPI_MISO_PIN  (16)
#endif
#ifndef SPI_CS_PIN
#define SPI_CS_PIN    (17)
#endif

// Threshold: only values >= this will be considered active and printed
#define HALLSCAN_DISPLAY_THRESHOLD  (200u)

// Sensor threshold: percentage deviation from calibrated baseline (1-100)
// Keys are detected when ADC reading deviates by this percentage from baseline
// Example: 30% means a key press is detected when reading is ±30% from baseline
// NOTE: Set higher (30-40%) if you see false triggers from ADC noise
#define SENSOR_THRESHOLD  (25u)  // 25% deviation required for key press

// Timing (tunable)
#define HALLSCAN_SETTLE_US     (200u) // microseconds to wait after changing mux selects
#define HALLSCAN_SCAN_DELAY_MS (80u)  // ms between full scans

#endif // HALLSCAN_CONFIG_H
