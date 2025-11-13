/* adc_matrix_test.h - Custom matrix declarations */
#pragma once

#include "quantum.h"

/* Custom matrix functions required by QMK when CUSTOM_MATRIX = lite */
void matrix_init_custom(void);
bool matrix_scan_custom(matrix_row_t current_matrix[]);
