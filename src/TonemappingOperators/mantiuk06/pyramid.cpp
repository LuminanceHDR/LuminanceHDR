/*
 * This file is a part of Luminance HDR package
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

//! \brief Pyramid implementation for Mantiuk06 tonemapping operator
//! \author Davide Anastasia <davideanastasia@users.sourceforge.net>
//! \date 09 Sept 2012
//! \note Implementation of this class is insired by the original implementation
//! of the Mantiuk06 operator found in PFSTMO. However, the actual
//! implementation found in this file (and its .cpp file) is based on STL
//! containers

#include "pyramid.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <functional>
#include <numeric>

#ifdef _OPENMP
#include <omp.h>
#endif

#include "Libpfs/array2d.h"
#include "Libpfs/utils/numeric.h"
#include "Libpfs/utils/sse.h"

using namespace pfs;

namespace {
inline size_t downscaleBy2(size_t value) { return (value >> 1); }
}

namespace {
const size_t PYRAMID_MIN_PIXELS = 3;
}

PyramidT::PyramidT(size_t rows, size_t cols) : m_rows(rows), m_cols(cols) {
    size_t referenceSize = std::min(rows, cols);
    while (referenceSize >= PYRAMID_MIN_PIXELS) {
        m_pyramid.push_back(PyramidS(cols, rows));

        rows = downscaleBy2(rows);                    // division by 2
        cols = downscaleBy2(cols);                    // division by 2
        referenceSize = downscaleBy2(referenceSize);  // division by 2
    }
}

void PyramidT::computeGradients(const pfs::Array2Df &Y) {
    assert(this->getCols() == Y.getCols());
    assert(this->getRows() == Y.getRows());

    if (!m_pyramid.size()) return;

#ifndef NDEBUG
    assert((getElems() / 4) > 0);
    assert((getElems() / 16) > 0);
#endif

    Array2Df buffer1(downscaleBy2(Y.getCols()), downscaleBy2(getRows()));
    Array2Df buffer2(downscaleBy2(buffer1.getCols()),
                     downscaleBy2(buffer1.getRows()));

    calculateGradients(Y.data(), m_pyramid[0]);

    if (m_pyramid.size() > 1) {
        matrixDownsample(m_pyramid[0].getCols(), m_pyramid[0].getRows(),
                         Y.data(), buffer1.data());
        calculateGradients(buffer1.data(), m_pyramid[1]);
    }

    for (size_t idx = 2; idx < m_pyramid.size(); ++idx) {
        matrixDownsample(m_pyramid[idx - 1].getCols(),
                         m_pyramid[idx - 1].getRows(), buffer1.data(),
                         buffer2.data());
        calculateGradients(buffer2.data(), m_pyramid[idx]);

        buffer1.swap(buffer2);
    }
}

void PyramidT::computeSumOfDivergence(pfs::Array2Df &sumOfdivG) {
    // zero dimension Array2D
    pfs::Array2Df tempSumOfdivG(downscaleBy2(sumOfdivG.getCols()),
                                downscaleBy2(sumOfdivG.getRows()));

    if ((numLevels() % 2)) {
        sumOfdivG.swap(tempSumOfdivG);
        tempSumOfdivG.fill(0.0f);
    } else {
        tempSumOfdivG.fill(0.0f);
    }

    if (numLevels() != 0) {
        calculateAndAddDivergence(m_pyramid[m_pyramid.size() - 1],
                                  tempSumOfdivG.data());
        tempSumOfdivG.swap(sumOfdivG);
    }

    for (int idx = numLevels() - 2; idx >= 0; idx--) {
        matrixUpsample(m_pyramid[idx].getCols(), m_pyramid[idx].getRows(),
                       sumOfdivG.data(), tempSumOfdivG.data());
        calculateAndAddDivergence(m_pyramid[idx], tempSumOfdivG.data());
        tempSumOfdivG.swap(sumOfdivG);
    }
}

namespace {
struct CalculateScaleFactor {
    XYGradient operator()(const XYGradient &xyGrad) const {
        return XYGradient(calculateScaleFactor(xyGrad.gX()),
                          calculateScaleFactor(xyGrad.gY()));
    }
};
}

void PyramidT::computeScaleFactors(PyramidT &result) const {
    PyramidContainer::const_iterator inCurr = m_pyramid.begin();
    PyramidContainer::const_iterator inEnd = m_pyramid.end();

    PyramidContainer::iterator outCurr = result.m_pyramid.begin();

    while (inCurr != inEnd) {
        std::transform(inCurr->begin(), inCurr->end(), outCurr->begin(),
                       CalculateScaleFactor());

        ++outCurr;
        ++inCurr;
    }
}

namespace {
template <typename Traits>
struct TransformObj {
    TransformObj(float f) : t_(f) {}

    XYGradient operator()(const XYGradient &xyGrad) const {
        return XYGradient(t_(xyGrad.gX()), t_(xyGrad.gY()));
    }

   private:
    Traits t_;
};
}

void PyramidT::transformToR(float detailFactor) {
    PyramidContainer::iterator itCurr = m_pyramid.begin();
    PyramidContainer::iterator itEnd = m_pyramid.end();

    TransformObj<TransformToR> transformFunctor(detailFactor);

    while (itCurr != itEnd) {
        std::transform(itCurr->begin(), itCurr->end(), itCurr->begin(),
                       transformFunctor);

        ++itCurr;
    }
}

void PyramidT::transformToG(float detailFactor) {
    PyramidContainer::iterator itCurr = m_pyramid.begin();
    PyramidContainer::iterator itEnd = m_pyramid.end();

    TransformObj<TransformToG> transformFunctor(detailFactor);

    while (itCurr != itEnd) {
        std::transform(itCurr->begin(), itCurr->end(), itCurr->begin(),
                       transformFunctor);

        ++itCurr;
    }
}

struct ScalePyramidS {
    ScalePyramidS(float multiplier) : multiplier_(multiplier) {}

    void operator()(PyramidS &multiply) {
        pfs::utils::vsmul(multiply.data(), multiplier_, multiply.data(),
                          multiply.size());
    }

   private:
    float multiplier_;
};

void PyramidT::scale(float multiplier) {
    for_each(m_pyramid.begin(), m_pyramid.end(), ScalePyramidS(multiplier));
}

// scale gradients for the whole one pyramid with the use of (Cx,Cy)
// from the other pyramid
void PyramidT::multiply(const PyramidT &other) {
    // check that the pyramids have the same number of levels
    assert(this->numLevels() == other.numLevels());
    // check that the first level of the pyramid has the same size
    assert(this->getCols() == other.getCols());
    assert(this->getRows() == other.getRows());

    PyramidContainer::const_iterator inCurr = other.m_pyramid.begin();
    PyramidContainer::const_iterator inEnd = other.m_pyramid.end();
    PyramidContainer::iterator outCurr = m_pyramid.begin();

    for (; inCurr != inEnd; ++outCurr, ++inCurr) {
        PyramidContainer::value_type::const_iterator innerItOther =
            inCurr->begin();
        PyramidContainer::value_type::iterator innerIt = outCurr->begin();
        PyramidContainer::value_type::iterator innerItEnd = outCurr->end();

        while (innerIt != innerItEnd) {
            *innerIt *= *innerItOther;

            innerIt++;
            innerItOther++;
        }
    }
}

// downsample the matrix
void matrixDownsampleFull(size_t inCols, size_t inRows, const float *inputData,
                          float *outputData) {
    const size_t outRows = inRows / 2;
    const size_t outCols = inCols / 2;

    const float dx = static_cast<float>(inCols) / outCols;
    const float dy = static_cast<float>(inRows) / outRows;
    const float normalize = 1.0f / (dx * dy);

// New downsampling by Ed Brambley:
// Experimental downsampling that assumes pixels are square and
// integrates over each new pixel to find the average value of the
// underlying pixels.
//
// Consider the original pixels laid out, and the new (larger)
// pixels layed out over the top of them.  Then the new value for
// the larger pixels is just the integral over that pixel of what
// shows through; i.e., the values of the pixels underneath
// multiplied by how much of that pixel is showing.
//
// (ix1, iy1) is the coordinate of the top left visible pixel.
// (ix2, iy2) is the coordinate of the bottom right visible pixel.
// (fx1, fy1) is the fraction of the top left pixel showing.
// (fx2, fy2) is the fraction of the bottom right pixel showing.

#pragma omp parallel for
    for (int y = 0; y < static_cast<int>(outRows); y++) {
        const size_t iy1 = (y * inRows) / outRows;
        const size_t iy2 = ((y + 1) * inRows) / outRows;
        const float fy1 = (iy1 + 1) - y * dy;
        const float fy2 = (y + 1) * dy - iy2;

        for (size_t x = 0; x < outCols; x++) {
            const size_t ix1 = (x * inCols) / outCols;
            const size_t ix2 = ((x + 1) * inCols) / outCols;
            const float fx1 = (ix1 + 1) - x * dx;
            const float fx2 = (x + 1) * dx - ix2;

            float pixVal = 0.0f;
            float factorx, factory;
            for (size_t i = iy1; i <= iy2 && i < inRows; i++) {
                if (i == iy1)
                    factory = fy1;  // We're just getting the bottom edge of
                                    // this pixel
                else if (i == iy2)
                    factory =
                        fy2;  // We're just gettting the top edge of this pixel
                else
                    factory = 1.0f;  // We've got the full height of this pixel
                for (size_t j = ix1; j <= ix2 && j < inCols; j++) {
                    if (j == ix1)
                        factorx =
                            fx1;  // We've just got the right edge of this pixel
                    else if (j == ix2)
                        factorx =
                            fx2;  // We've just got the left edge of this pixel
                    else
                        factorx =
                            1.0f;  // We've got the full width of this pixel

                    pixVal += inputData[j + i * inCols] * factorx * factory;
                }
            }

            outputData[x + y * outCols] =
                pixVal * normalize;  // Normalize by the area of the new pixel
        }
    }
}

void matrixDownsampleSimple(size_t inCols, size_t inRows,
                            const float *inputData, float *outputData) {
    const size_t outRows = inRows / 2;
    const size_t outCols = inCols / 2;

// Simplified downsampling by Bruce Guenter:
//
// Follows exactly the same math as the full downsampling above,
// except that inRows and inCols are known to be even.  This allows
// for all of the boundary cases to be eliminated, reducing the
// sampling to a simple average.

#pragma omp parallel for
    for (int y = 0; y < static_cast<int>(outRows); y++) {
        const int iy1 = y * 2;
        const float *datap = inputData + iy1 * inCols;
        float *resp = outputData + y * outCols;

        for (size_t x = 0; x < outCols; x++) {
            const size_t ix1 = x * 2;

            resp[x] = (datap[ix1] + datap[(ix1 + 1)] + datap[ix1 + inCols] +
                       datap[(ix1 + 1) + inCols]) *
                      0.25f;  // / 4.0f;
        }
    }
}

void matrixDownsample(size_t inCols, size_t inRows, const float *inputData,
                      float *outputData) {
    if (!(inCols % 2) && !(inRows % 2)) {
        matrixDownsampleSimple(inCols, inRows, inputData, outputData);
    } else {
        matrixDownsampleFull(inCols, inRows, inputData, outputData);
    }
}

// upsample the matrix
// upsampled matrix is twice bigger in each direction than data[]
// res should be a pointer to allocated memory for bigger matrix
// cols and rows are the dimensions of the output matrix
void matrixUpsampleFull(const size_t outCols, const size_t outRows,
                        const float *inputData, float *outputData) {
    const size_t inRows = outRows / 2;
    const size_t inCols = outCols / 2;

    // Transpose of experimental downsampling matrix (theoretically the correct
    // thing to do)
    const float dx = static_cast<float>(inCols) / outCols;
    const float dy = static_cast<float>(inRows) / outRows;

    // This gives a genuine upsampling matrix, not the transpose of the
    // downsampling matrix
    const float factor = 1.0f / (dx * dy);
// Theoretically, this should be the best.
// const float factor = 1.0f;

#pragma omp parallel for
    for (int y = 0; y < static_cast<int>(outRows); y++) {
        const float sy = y * dy;
        const int iy1 = (y * inRows) / outRows;
        const int iy2 = std::min(((y + 1) * inRows) / outRows, inRows - 1);

        for (size_t x = 0; x < outCols; x++) {
            const float sx = x * dx;
            const int ix1 = (x * inCols) / outCols;
            const int ix2 = std::min(((x + 1) * inCols) / outCols, inCols - 1);

            outputData[x + y * outCols] =
                (((ix1 + 1) - sx) * ((iy1 + 1 - sy)) *
                     inputData[ix1 + iy1 * inCols] +
                 ((ix1 + 1) - sx) * (sy + dy - (iy1 + 1)) *
                     inputData[ix1 + iy2 * inCols] +
                 (sx + dx - (ix1 + 1)) * ((iy1 + 1 - sy)) *
                     inputData[ix2 + iy1 * inCols] +
                 (sx + dx - (ix1 + 1)) * (sy + dx - (iy1 + 1)) *
                     inputData[ix2 + iy2 * inCols]) *
                factor;
        }
    }
}

void matrixUpsampleSimple(const int outCols, const int outRows,
                          const float *const inputData,
                          float *const outputData) {
#pragma omp parallel for
    for (int y = 0; y < outRows; y++) {
        const int iy1 = y / 2;
        float *outp = outputData + y * outCols;
        const float *inp = inputData + iy1 * (outCols / 2);
        for (int x = 0; x < outCols; x += 2) {
            const int ix1 = x / 2;
            outp[x] = outp[x + 1] = inp[ix1];
        }
    }
}

void matrixUpsample(size_t outCols, size_t outRows, const float *inputData,
                    float *outputData) {
    if (!(outRows % 2) && !(outCols % 2)) {
        matrixUpsampleSimple(outCols, outRows, inputData, outputData);
    } else {
        matrixUpsampleFull(outCols, outRows, inputData, outputData);
    }
}

// calculate gradients
void calculateGradients(const float *inputData, PyramidS &gradient) {
    const int COLS = gradient.getCols();
    const int ROWS = gradient.getRows();

#pragma omp parallel  // shared(COLS, ROWS)
    {
#pragma omp for nowait
        for (int ky = 0; ky < (ROWS - 1); ++ky) {
            PyramidS::iterator currGxy = gradient.row_begin(ky);
            PyramidS::iterator endGxy = gradient.row_end(ky) - 1;

            const float *currLumU = inputData + ky * COLS;
            const float *currLumL = inputData + (ky + 1) * COLS;

            while (currGxy != endGxy) {
                currGxy->gX() = *(currLumU + 1) - *currLumU;
                currGxy->gY() = *currLumL - *currLumU;

                ++currLumL;
                ++currLumU;
                ++currGxy;
            }
            // last sample of the row...
            currGxy->gX() = 0.0f;
            currGxy->gY() = *currLumL - *currLumU;
        }

#pragma omp single
        {
            PyramidS::iterator currGxy = gradient.row_begin(ROWS - 1);
            PyramidS::iterator endGxy = gradient.row_end(ROWS - 1) - 1;

            const float *currLumU = inputData + (ROWS - 1) * COLS;

            while (currGxy != endGxy) {
                currGxy->gX() = *(currLumU + 1) - *currLumU;
                currGxy->gY() = 0.0f;

                ++currLumU;
                ++currGxy;
            }
            // last sample of the row...
            currGxy->gX() = 0.0f;
            currGxy->gY() = 0.0f;
        }  // pragma omp single
    }      // pragma omp parallel
}

namespace {
const float LOG10FACTOR = 2.3025850929940456840179914546844f;
const size_t LOOKUP_W_TO_R = 107;

static float W_table[] = {
    0.000000f,    0.010000f,    0.021180f,    0.031830f,    0.042628f,
    0.053819f,    0.065556f,    0.077960f,    0.091140f,    0.105203f,
    0.120255f,    0.136410f,    0.153788f,    0.172518f,    0.192739f,
    0.214605f,    0.238282f,    0.263952f,    0.291817f,    0.322099f,
    0.355040f,    0.390911f,    0.430009f,    0.472663f,    0.519238f,
    0.570138f,    0.625811f,    0.686754f,    0.753519f,    0.826720f,
    0.907041f,    0.995242f,    1.092169f,    1.198767f,    1.316090f,
    1.445315f,    1.587756f,    1.744884f,    1.918345f,    2.109983f,
    2.321863f,    2.556306f,    2.815914f,    3.103613f,    3.422694f,
    3.776862f,    4.170291f,    4.607686f,    5.094361f,    5.636316f,
    6.240338f,    6.914106f,    7.666321f,    8.506849f,    9.446889f,
    10.499164f,   11.678143f,   13.000302f,   14.484414f,   16.151900f,
    18.027221f,   20.138345f,   22.517282f,   25.200713f,   28.230715f,
    31.655611f,   35.530967f,   39.920749f,   44.898685f,   50.549857f,
    56.972578f,   64.280589f,   72.605654f,   82.100619f,   92.943020f,
    105.339358f,  119.530154f,  135.795960f,  154.464484f,  175.919088f,
    200.608905f,  229.060934f,  261.894494f,  299.838552f,  343.752526f,
    394.651294f,  453.735325f,  522.427053f,  602.414859f,  695.706358f,
    804.693100f,  932.229271f,  1081.727632f, 1257.276717f, 1463.784297f,
    1707.153398f, 1994.498731f, 2334.413424f, 2737.298517f, 3215.770944f,
    3785.169959f, 4464.187290f, 5275.653272f, 6247.520102f, 7414.094945f,
    8817.590551f, 10510.080619f};
static float R_table[] = {
    0.000000f, 0.009434f, 0.018868f, 0.028302f, 0.037736f, 0.047170f, 0.056604f,
    0.066038f, 0.075472f, 0.084906f, 0.094340f, 0.103774f, 0.113208f, 0.122642f,
    0.132075f, 0.141509f, 0.150943f, 0.160377f, 0.169811f, 0.179245f, 0.188679f,
    0.198113f, 0.207547f, 0.216981f, 0.226415f, 0.235849f, 0.245283f, 0.254717f,
    0.264151f, 0.273585f, 0.283019f, 0.292453f, 0.301887f, 0.311321f, 0.320755f,
    0.330189f, 0.339623f, 0.349057f, 0.358491f, 0.367925f, 0.377358f, 0.386792f,
    0.396226f, 0.405660f, 0.415094f, 0.424528f, 0.433962f, 0.443396f, 0.452830f,
    0.462264f, 0.471698f, 0.481132f, 0.490566f, 0.500000f, 0.509434f, 0.518868f,
    0.528302f, 0.537736f, 0.547170f, 0.556604f, 0.566038f, 0.575472f, 0.584906f,
    0.594340f, 0.603774f, 0.613208f, 0.622642f, 0.632075f, 0.641509f, 0.650943f,
    0.660377f, 0.669811f, 0.679245f, 0.688679f, 0.698113f, 0.707547f, 0.716981f,
    0.726415f, 0.735849f, 0.745283f, 0.754717f, 0.764151f, 0.773585f, 0.783019f,
    0.792453f, 0.801887f, 0.811321f, 0.820755f, 0.830189f, 0.839623f, 0.849057f,
    0.858491f, 0.867925f, 0.877358f, 0.886792f, 0.896226f, 0.905660f, 0.915094f,
    0.924528f, 0.933962f, 0.943396f, 0.952830f, 0.962264f, 0.971698f, 0.981132f,
    0.990566f, 1.000000f};

// in_tab and out_tab should contain inccreasing float values
static float lookup_table(size_t n, const float *in_tab, const float *out_tab,
                          float val) {
    if (val < in_tab[0]) return out_tab[0];

    for (size_t j = 1; j < n; j++) {
        if (val < in_tab[j]) {
            const float dd =
                (val - in_tab[j - 1]) / (in_tab[j] - in_tab[j - 1]);
            return out_tab[j - 1] + (out_tab[j] - out_tab[j - 1]) * dd;
        }
    }

    return out_tab[n - 1];
}
}

TransformToR::TransformToR(float detailFactor)
    : m_detailFactor(LOG10FACTOR * detailFactor) {}

// transform gradient G to R
float TransformToR::operator()(float currG) const {
    if (currG < 0.0f) {
        // G to W
        currG = std::pow(10, (-currG) * m_detailFactor) - 1.0f;
        // W to RESP
        return -lookup_table(LOOKUP_W_TO_R, W_table, R_table, currG);
    } else {
        // G to W
        currG = std::pow(10, currG * m_detailFactor) - 1.0f;
        // W to RESP
        return lookup_table(LOOKUP_W_TO_R, W_table, R_table, currG);
    }
}

TransformToG::TransformToG(float detailFactor)
    : m_detailFactor(1.0f / (LOG10FACTOR * detailFactor)) {}

// transform from R to G
float TransformToG::operator()(float currR) const {
    if (currR < 0.0f) {
        // RESP to W
        currR = lookup_table(LOOKUP_W_TO_R, R_table, W_table, -currR);
        // W to G
        // return -std::log(currR + 1.0f) * m_detailFactor;
        return -std::log1p(currR) * m_detailFactor;  // avoid loss of precision
    } else {
        // RESP to W
        currR = lookup_table(LOOKUP_W_TO_R, R_table, W_table, currR);
        // W to G
        // return std::log(currR + 1.0f) * m_detailFactor;
        return std::log1p(currR) * m_detailFactor;  // avoid loss of precision
    }
}

//! \brief calculate divergence of two gradient maps (Gx and Gy)
//! divG(x,y) = [Gx(x,y) - Gx(x-1,y)] + [Gy(x,y) - Gy(x,y-1)]
//! \note \a divG will be used purely as a temporary vector of data, to store
//! the result. The only requirement is that \a divG size is bigger or equal
//! to \a G size
void calculateAndAddDivergence(const PyramidS &G, float *divG) {
    const int ROWS = G.getRows();
    const int COLS = G.getCols();

    // kx = 0 AND ky = 0;
    divG[0] += G[0][0].gX() + G[0][0].gY();  // OUT

    float divGx, divGy;
// ky = 0
#pragma omp parallel for private(divGx, divGy)
    for (int kx = 1; kx < COLS; kx++) {
        divGx = G[0][kx].gX() - G[0][kx - 1].gX();
        divGy = G[0][kx].gY();
        divG[kx] += divGx + divGy;  // OUT
    }
#pragma omp parallel for private(divGx, divGy)
    for (int ky = 1; ky < ROWS; ky++) {
        // kx = 0
        divGx = G[ky][0].gX();
        divGy = G[ky][0].gY() - G[ky - 1][0].gY();
        divG[ky * COLS] += divGx + divGy;  // OUT

        // kx > 0
        for (int kx = 1; kx < COLS; kx++) {
            divGx = G[ky][kx].gX() - G[ky][kx - 1].gX();
            divGy = G[ky][kx].gY() - G[ky - 1][kx].gY();
            divG[kx + ky * COLS] += divGx + divGy;  // OUT
        }
    }
}

namespace {
//  static const float GFIXATE = 0.1f;
//  static const float EDGE_WEIGHT = 0.01f;
static const float detectT = 0.001f;
static const float a = 0.038737f;
static const float b = 0.537756f;
}

float calculateScaleFactor(float g) {
#if 1
    return 1.0 / (a * std::pow(std::max(detectT, std::fabs(g)), b));
#else
    if (std::fabs(G[i]) < GFIXATE)
        C[i] = 1.0f / EDGE_WEIGHT;
    else
        C[i] = 1.0f;
#endif
}
