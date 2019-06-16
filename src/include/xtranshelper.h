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
    return cfa[row % 6][col % 6];
}

inline bool isgreen(const  unsigned cfa[6][6], unsigned row,  unsigned col) {
    return cfa[row % 3][col % 3] & 1;
}

inline bool validateXtransCfa(const  unsigned cfa[6][6]) {
    // rough check for xtrans cfa correctness, to be completed
    int count[3] = {};
    for (int i = 0; i < 6; ++i) {
        for (int j = 0; j < 6; ++j) {
            ++count[cfa[i][j]];
        }
    }
    // xtrans 6x6 grid always has 8 reds, 20 greens and 8 blues
    if(count[0] == 8 && count[2] == 8 && count[1] == 20) {
        bool perRoworColumn = true;
        // for each row/column of xtrans 6x6 grid the following has to be valid
        // 1) number of reds == number of blues
        // 2) number of reds is either one or two
        // 3) number of reds + number of greens + number of blues == 6
        // First check rows
        for (int i = 0; i < 6; ++i) {
            int countpr[3] = {};
            for (int j = 0; j < 6; ++j) {
                ++countpr[cfa[i][j]];
            }
            if (countpr[0] != countpr[2] || countpr[0] < 1 || countpr[0] > 2 || (countpr[1] + 2 * countpr[0] != 6)) {
                perRoworColumn = false;
                break;
            }
        }
        if (perRoworColumn) {
            // Rows are valid, check columns
            for (int i = 0; i < 6; ++i) {
                int countpc[3] = {};
                for (int j = 0; j < 6; ++j) {
                    ++countpc[cfa[j][i]];
                }
                if (countpc[0] != countpc[2] || countpc[0] < 1 || countpc[0] > 2 || (countpc[1] + 2 * countpc[0] != 6)) {
                    perRoworColumn = false;
                    break;
                }
            }
            if (perRoworColumn) {
                // Columns are also valid
                // now check green neighborhood
                bool checkGreens = true;
                for (int i = 1; i < 5; ++i) {
                    for (int j = 1; j < 5; ++j) {
                        if (cfa[i][j] == 1) {
                            // for each cross around a green pixel the following has to be valid
                            // 1) number of reds == number blues
                            // 2) number of reds is either one or two
                            // 3) if number of reds is two, then right and left pixel of cross must have same color
                            int lcount[3] = {};
                            lcount[cfa[i - 1][j]]++;
                            lcount[cfa[i + 1][j]]++;
                            lcount[cfa[i][j - 1]]++;
                            lcount[cfa[i][j + 1]]++;
                            if (lcount[0] != lcount[2] || (lcount[0] != 1 && lcount[0] != 2) || (lcount[0] == 2 && cfa[i][j - 1] != cfa[i][j + 1])) {
                                checkGreens = false;
                                break;
                            }
                        }
                    }
                    if (!checkGreens) {
                        break;
                    }
                }
                if (checkGreens) {
                    return true;
                }
            }
        }
    }
    
    std::cerr << "librtprocess : Wrong color filter for xtrans array: " << std::endl;
    for (int i = 0; i < 6; ++i) {
        std::cerr << cfa[i][0] << " " << cfa[i][1] << " " << cfa[i][2] << " " << cfa[i][3] << " " << cfa[i][4] << " " << cfa[i][5] << std::endl;
    }

    return false;
}
}
