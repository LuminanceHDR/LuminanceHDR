////////////////////////////////////////////////////////////////
//
//  Fast Xtrans demosaic algorithm
//
//  code dated: January 01, 2019
//
//  xtransfast.cc is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
////////////////////////////////////////////////////////////////

#include "librtprocess.h"
#include "StopWatch.h"
#include "xtranshelper.h"

using namespace librtprocess;

rpError xtransfast_demosaic (int width, int height, const float * const *rawData, float **red, float **green, float **blue, const unsigned xtrans[6][6], const std::function<bool(double)> &setProgCancel)
{
BENCHFUN

    if (!validateXtransCfa(xtrans)) {
        return RP_WRONG_CFA;
    }

    setProgCancel(0.0);

    xtransborder_demosaic(width, height, 1, rawData, red, green, blue, xtrans);

    const float weight[3][3] = {
                                {0.25f, 0.5f, 0.25f},
                                {0.5f,  0.f,  0.5f},
                                {0.25f, 0.5f, 0.25f}
                               };
#ifdef _OPENMP
    #pragma omp parallel for schedule(dynamic, 16)
#endif
    for (int row = 1; row < height - 1; ++row) {
        for (int col = 1; col < width - 1; ++col) {
            float sum[3] = {};

            for (int v = -1; v <= 1; v++) {
                for (int h = -1; h <= 1; h++) {
                    sum[fc(xtrans, row + v, col + h)] += rawData[row + v][(col + h)] * weight[v + 1][h + 1];
                }
            }

            switch(fc(xtrans, row, col)) {
            case 0: // red pixel
                red[row][col] = rawData[row][col];
                green[row][col] = sum[1] * 0.5f;
                blue[row][col] = sum[2];
                break;

            case 1: // green pixel
                green[row][col] = rawData[row][col];
                if (fc(xtrans, row, col - 1) == fc(xtrans, row, col + 1)) { // Solitary green pixel always has exactly two direct red and blue neighbors in 3x3 grid
                    red[row][col] = sum[0];
                    blue[row][col] = sum[2];
                } else { // Non solitary green pixel always has one direct and one diagonal red and blue neighbor in 3x3 grid
                    red[row][col] = sum[0] * 1.3333333f;
                    blue[row][col] = sum[2] * 1.3333333f;
                }
                break;

            case 2: // blue pixel
                red[row][col] = sum[0];
                green[row][col] = sum[1] * 0.5f;
                blue[row][col] = rawData[row][col];
                break;
            }
        }
    }

    setProgCancel(1.0);

    return RP_NO_ERROR;
}
