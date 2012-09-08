#ifndef TESTMANTIUK06REFERENCE_H
#define TESTMANTIUK06REFERENCE_H

#include <cstddef>

namespace reference
{

// header for Mantiuk06
void matrix_upsample_full(int outCols, int outRows,
                          const float* in, float* out);
void matrix_upsample_simple(const int outCols, const int outRows,
                            const float* const in, float* const out);

void calculate_gradient(const int COLS, const int ROWS,
                        const float* const lum, float* const Gx, float* const Gy);

void calculate_and_add_divergence(int COLS, int ROWS,
                                  const float* Gx,
                                  const float* Gy,
                                  float* divG);

}

#endif
