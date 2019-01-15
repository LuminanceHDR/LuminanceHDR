/*
 * This file is part of librtprocess.
 *
 * Copyright (c) 2018 Carlo Vaccari
 * Copyright (c) 2019 Ingo Weyrich
 *
 * librtprocess is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the license, or
 * (at your option) any later version.
 *
 * librtprocess is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with librtprocess.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <iostream>

namespace librtprocess {

inline unsigned fc(const unsigned cfa[2][2], unsigned row, unsigned col)
{
    return cfa[row & 1][col & 1];
}

inline bool validateBayerCfa(unsigned colors, const unsigned cfa[2][2]) {
    if (colors == 3) {
        int count[3] = {};
        for (int i = 0; i < 2; ++i) {
            for (int j = 0; j < 2; ++j) {
                if(cfa[i][j] < colors) {
                    ++count[cfa[i][j]];
                }
            }
        }
        if(count[0] == 1 && count[2] == 1 && count[1] == 2 && ((cfa[0][0] & 1) == (cfa[1][1] & 1))) {
            return true;
        }
    } else if (colors == 4) {
        int count[4] = {};
        for (int i = 0; i < 2; ++i) {
            for (int j = 0; j < 2; ++j) {
                if(cfa[i][j] < colors) {
                    ++count[cfa[i][j]];
                }
            }
        }
        if(count[0] == 1 && count[2] == 1 && count[1] == 1 && count[3] == 1 && ((cfa[0][0] & 1) == (cfa[1][1] & 1))) {
            return true;
        }
    }

    std::cerr << "librtprocess : Wrong color filter for " << colors << " colors array: " << cfa[0][0] << " " << cfa[0][1] << " " << cfa[1][0] << " " << cfa[1][1] << std::endl;

    return false;
}
}