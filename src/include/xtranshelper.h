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

inline int fc(const unsigned cfa[6][6], unsigned row,  unsigned col) {
    return cfa[(row)%6][(col)%6];
}

inline bool isgreen(const  unsigned cfa[6][6], unsigned row,  unsigned col) {
    return cfa[row % 3][col % 3] & 1;
}

inline bool validateXtransCfa(const  unsigned cfa[6][6]) {
    // rough check for xtrans cfa correctness
    int count[3] = {};
    for (int i = 0; i < 6; ++i) {
        for (int j = 0; j < 6; ++j) {
            ++count[cfa[i][j]];
        }
    }
    if(count[0] == 8 && count[2] == 8 && count[1] == 20) {
        bool perLine = true;
        for (int i = 0; i < 6; ++i) {
            int countpl[3] = {};
            for (int j = 0; j < 6; ++j) {
                ++countpl[cfa[i][j]];
            }
            if (countpl[0] != countpl[2] || countpl[0] < 1 || countpl[0] > 2 || (countpl[1] + 2* countpl[0] != 6)) {
                perLine = false;
                break;
            }
        }
        if (perLine) {
            return true;
        }
    }
    
    std::cerr << "librtprocess : Wrong color filter for xtrans array: " << std::endl;
    for (int i = 0; i < 6; ++i) {
        std::cerr << cfa[i][0] << " " << cfa[i][1] << " " << cfa[i][2] << " " << cfa[i][3] << " " << cfa[i][4] << " " << cfa[i][5] << std::endl;
    }

    return false;
}
}
