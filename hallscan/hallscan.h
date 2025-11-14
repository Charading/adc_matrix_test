// Lightweight header for hallscan module
#ifndef HALLSCAN_H
#define HALLSCAN_H

#include "matrix.h"

void matrix_init_custom(void);
bool matrix_scan_custom(matrix_row_t current_matrix[]);

#endif // HALLSCAN_H
