/*
 * This file is a part of Luminance HDR package.
 * ----------------------------------------------------------------------
 * Copyright (C) 2012 Davide Anastasia
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * ----------------------------------------------------------------------
 */

#ifndef PFS_ROTATE_HXX
#define PFS_ROTATE_HXX

#include "rotate.h"

namespace pfs {

template <typename Type>
void rotate(const pfs::Array2D<Type> *in, pfs::Array2D<Type> *out,
            bool clockwise) {
    const Type *Vin = in->data();
    Type *Vout = out->data();

    const int I_ROWS = in->getRows();
    const int I_COLS = in->getCols();

    // const int O_ROWS = out->getRows();
    const int O_COLS = out->getCols();

    if (clockwise) {
#pragma omp parallel for
        for (int j = 0; j < I_ROWS; j++) {
            for (int i = 0; i < I_COLS; i++) {
                Vout[(i + 1) * O_COLS - 1 - j] = Vin[j * I_COLS + i];
            }
        }
    } else {
#pragma omp parallel for
        for (int j = 0; j < I_ROWS; j++) {
            for (int i = 0; i < I_COLS; i++) {
                Vout[(I_COLS - i - 1) * O_COLS + j] = Vin[j * I_COLS + i];
            }
        }
    }
}
}

#endif  // #ifndef PFS_ROTATE_HXX
