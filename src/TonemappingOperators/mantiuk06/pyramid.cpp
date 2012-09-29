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

#include <cmath>
#include <cassert>
#include <algorithm>
#include <numeric>
#include <functional>
#include <boost/bind.hpp>

#ifdef _OPENMP
#include <omp.h>
#endif

#include "Libpfs/vex.h"
#include "Libpfs/vex/vex.h"

namespace
{
inline
size_t downscaleBy2(size_t value)
{
    return (value >> 1);
    // return ((value + 1) >> 1);
}
}

PyramidS::PyramidS(size_t rows, size_t cols)
    : m_rows(rows)
    , m_cols(cols)
    , m_Gx(rows*cols)
    , m_Gy(rows*cols)
{}

PyramidS::PyramidS(const PyramidS &lhs)
    : m_rows(lhs.m_rows)
    , m_cols(lhs.m_cols)
    , m_Gx( lhs.m_Gx.size() )
    , m_Gy( lhs.m_Gy.size() )
{}

void PyramidS::transformToR(float detailFactor)
{
    ::transformToR(m_Gx.data(), detailFactor, m_rows*m_cols);
    ::transformToR(m_Gy.data(), detailFactor, m_rows*m_cols);
}

void PyramidS::transformToG(float detailFactor)
{
    ::transformToG(m_Gx.data(), detailFactor, m_rows*m_cols);
    ::transformToG(m_Gy.data(), detailFactor, m_rows*m_cols);
}

void PyramidS::scale(float multiplier)
{
    VEX_vsmul(m_Gx.data(), multiplier, m_Gx.data(), m_rows*m_cols);
    VEX_vsmul(m_Gy.data(), multiplier, m_Gy.data(), m_rows*m_cols);
}

void PyramidS::multiply(const PyramidS& multiplier)
{
    vex::vmul(m_Gx.data(), multiplier.gX(), m_Gx.data(), m_rows*m_cols);
    vex::vmul(m_Gy.data(), multiplier.gY(), m_Gy.data(), m_rows*m_cols);
}

namespace
{
const size_t PYRAMID_MIN_PIXELS = 3;
}

PyramidT::PyramidT(size_t rows, size_t cols)
    : m_rows(rows)
    , m_cols(cols)
{
    size_t referenceSize = std::min( rows, cols );
    while ( referenceSize >= PYRAMID_MIN_PIXELS )
    {
        m_pyramid.push_back( PyramidS(rows, cols) );

        rows = downscaleBy2(rows);                     // division by 2
        cols = downscaleBy2(cols);                     // division by 2
        referenceSize = downscaleBy2(referenceSize);   // division by 2
    }
}

typedef PyramidS::Vector Vector;

void PyramidT::computeGradients(const float *Y)
{
    if ( !m_pyramid.size() ) return;

#ifndef NDEBUG
    assert((getElems()/4) > 0);
    assert((getElems()/16) > 0);
#endif

    Vector buffer1( getElems()/4 );
    Vector buffer2( getElems()/16 );

    calculateGradients(m_pyramid[0].getCols(), m_pyramid[0].getRows(),
                       Y,
                       m_pyramid[0].gX(), m_pyramid[0].gY());

    if ( m_pyramid.size() > 1 )
    {
        matrixDownsample(m_pyramid[0].getCols(), m_pyramid[0].getRows(),
                         Y,
                         buffer1.data());
        calculateGradients(m_pyramid[1].getCols(), m_pyramid[1].getRows(),
                           buffer1.data(),
                           m_pyramid[1].gX(), m_pyramid[1].gY());
    }

    for (size_t idx = 2; idx < m_pyramid.size(); ++idx)
    {
        matrixDownsample(m_pyramid[idx-1].getCols(), m_pyramid[idx-1].getRows(),
                         buffer1.data(),
                         buffer2.data());
        calculateGradients(m_pyramid[idx].getCols(), m_pyramid[idx].getRows(),
                           buffer2.data(),
                           m_pyramid[idx].gX(), m_pyramid[idx].gY());

        buffer1.swap( buffer2 );
    }
}

void PyramidT::computeSumOfDivergence(float* sumOfdivG)
{
    Vector tempMatrix( getElems() >> 2 );

    float* temp = tempMatrix.data();

    if ( (numLevels() % 2) )
    {
        std::swap(sumOfdivG, temp);
        std::fill(temp, temp + getElems(), 0.0f);
    } else
    {
        std::fill(temp, temp + (getElems() >> 2), 0.0f);
    }

    PyramidContainer::const_reverse_iterator it = m_pyramid.rbegin();
    PyramidContainer::const_reverse_iterator itEnd = m_pyramid.rend();

    if (it != itEnd)
    {
        calculateAndAddDivergence(it->getCols(), it->getRows(),
                                  it->gX(), it->gY(), temp);

        std::swap(sumOfdivG, temp);
        ++it;
    }

    while (it != itEnd)
    {
        matrixUpsample(it->getCols(), it->getRows(), sumOfdivG, temp);
        calculateAndAddDivergence(it->getCols(), it->getRows(),
                                  it->gX(), it->gY(), temp);

        std::swap(sumOfdivG, temp);
        ++it;
    }

}

// possible improvements: parallel_while!
void PyramidT::computeScaleFactors( PyramidT& result ) const
{
    PyramidContainer::const_iterator inCurr = m_pyramid.begin();
    PyramidContainer::const_iterator inEnd = m_pyramid.end();

    PyramidContainer::iterator outCurr = result.m_pyramid.begin();

    while ( inCurr != inEnd )
    {
        calculateScaleFactor(inCurr->gX(), outCurr->gX(),
                             inCurr->getRows()*inCurr->getCols());
        calculateScaleFactor(inCurr->gY(), outCurr->gY(),
                             inCurr->getRows()*inCurr->getCols());

        ++outCurr;
        ++inCurr;
    }
}

void PyramidT::transformToR(float detailFactor)
{
    const int iEnd = static_cast<int>(m_pyramid.size());
#pragma omp parallel for
    for (int i = 0; i < iEnd; i++)
    {
        m_pyramid[i].transformToR( detailFactor );
    }
}

void PyramidT::transformToG(float detailFactor)
{
    const int iEnd = static_cast<int>(m_pyramid.size());
#pragma omp parallel for
    for (int i = 0; i < iEnd; i++)
    {
        m_pyramid[i].transformToG( detailFactor );
    }
}

void PyramidT::scale(float multiplier)
{
    const int iEnd = static_cast<int>(m_pyramid.size());
#pragma omp parallel for
    for (int i = 0; i < iEnd; i++)
    {
        m_pyramid[i].scale( multiplier );
    }
}

// scale gradients for the whole one pyramid with the use of (Cx,Cy)
// from the other pyramid
void PyramidT::multiply(const PyramidT &multiplier)
{
    PyramidContainer::const_iterator inCurr = multiplier.m_pyramid.begin();
    PyramidContainer::const_iterator inEnd = multiplier.m_pyramid.end();

    PyramidContainer::iterator outCurr = m_pyramid.begin();

    for ( ; inCurr != inEnd ; ++outCurr, ++inCurr)
    {
        outCurr->multiply(*inCurr);
    }
}

// downsample the matrix
void matrixDownsampleFull(size_t inCols, size_t inRows,
                          const float* inputData, float* outputData)
{
    const size_t outRows = inRows / 2;
    const size_t outCols = inCols / 2;

    const float dx = static_cast<float>(inCols) / outCols;
    const float dy = static_cast<float>(inRows) / outRows;
    const float normalize = 1.0f/(dx*dy);

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
    for (int y = 0; y < outRows; y++)
    {
        const size_t iy1 = (  y   * inRows) / outRows;
        const size_t iy2 = ((y+1) * inRows) / outRows;
        const float fy1 = (iy1+1) - y * dy;
        const float fy2 = (y+1) * dy - iy2;

        for (size_t x = 0; x < outCols; x++)
        {
            const size_t ix1 = (  x   * inCols) / outCols;
            const size_t ix2 = ((x+1) * inCols) / outCols;
            const float fx1 = (ix1+1) - x * dx;
            const float fx2 = (x+1) * dx - ix2;

            float pixVal = 0.0f;
            float factorx, factory;
            for (size_t i = iy1; i <= iy2 && i < inRows; i++)
            {
                if (i == iy1)
                    factory = fy1;  // We're just getting the bottom edge of this pixel
                else if (i == iy2)
                    factory = fy2;  // We're just gettting the top edge of this pixel
                else
                    factory = 1.0f; // We've got the full height of this pixel
                for (size_t j = ix1; j <= ix2 && j < inCols; j++)
                {
                    if (j == ix1)
                        factorx = fx1;  // We've just got the right edge of this pixel
                    else if (j == ix2)
                        factorx = fx2; // We've just got the left edge of this pixel
                    else
                        factorx = 1.0f; // We've got the full width of this pixel

                    pixVal += inputData[j + i*inCols] * factorx * factory;
                }
            }

            outputData[x + y * outCols] = pixVal * normalize;  // Normalize by the area of the new pixel
        }
    }
}

void matrixDownsampleSimple(size_t inCols, size_t inRows,
                            const float* inputData, float* outputData)
{
    const size_t outRows = inRows / 2;
    const size_t outCols = inCols / 2;

    // Simplified downsampling by Bruce Guenter:
    //
    // Follows exactly the same math as the full downsampling above,
    // except that inRows and inCols are known to be even.  This allows
    // for all of the boundary cases to be eliminated, reducing the
    // sampling to a simple average.

#pragma omp parallel for
    for (int y = 0; y < outRows; y++)
    {
        const int iy1 = y * 2;
        const float* datap = inputData + iy1 * inCols;
        float* resp = outputData + y * outCols;

        for (size_t x = 0; x < outCols; x++)
        {
            const size_t ix1 = x*2;

            resp[x] = ( datap[ix1] +
                        datap[(ix1+1)] +
                        datap[ix1     + inCols] +
                        datap[(ix1+1) + inCols]) / 4.0f; // * 0.25f; //  /4.0f;
        }
    }
}

void matrixDownsample(size_t inCols, size_t inRows,
                      const float* inputData, float* outputData)
{
    if ( !(inCols % 2) && !(inRows % 2) )
    {
        matrixDownsampleSimple(inCols, inRows, inputData, outputData);
    }
    else
    {
        matrixDownsampleFull(inCols, inRows, inputData, outputData);
    }
}

// upsample the matrix
// upsampled matrix is twice bigger in each direction than data[]
// res should be a pointer to allocated memory for bigger matrix
// cols and rows are the dimensions of the output matrix
void matrixUpsampleFull(const size_t outCols, const size_t outRows,
                        const float* inputData, float* outputData)
{
    const size_t inRows = outRows/2;
    const size_t inCols = outCols/2;

    // Transpose of experimental downsampling matrix (theoretically the correct thing to do)
    const float dx = static_cast<float>(inCols)/outCols;
    const float dy = static_cast<float>(inRows)/outRows;

    const float factor = 1.0f / (dx*dy); // This gives a genuine upsampling matrix, not the transpose of the downsampling matrix
    // const float factor = 1.0f; // Theoretically, this should be the best.

#pragma omp parallel for
    for (int y = 0; y < outRows; y++)
    {
        const float sy = y * dy;
        const int iy1 =      (  y   * inRows) / outRows;
        const int iy2 = std::min(((y+1) * inRows) / outRows, inRows-1);

        for (size_t x = 0; x < outCols; x++)
        {
            const float sx = x * dx;
            const int ix1 =      (  x   * inCols) / outCols;
            const int ix2 = std::min(((x+1) * inCols) / outCols, inCols-1);

            outputData[x + y*outCols] = (((ix1+1) - sx)*((iy1+1 - sy)) * inputData[ix1 + iy1*inCols] +
                                         ((ix1+1) - sx)*(sy+dy - (iy1+1)) * inputData[ix1 + iy2*inCols] +
                                         (sx+dx - (ix1+1))*((iy1+1 - sy)) * inputData[ix2 + iy1*inCols] +
                                         (sx+dx - (ix1+1))*(sy+dx - (iy1+1)) * inputData[ix2 + iy2*inCols])*factor;
        }
    }
}

void matrixUpsampleSimple(const int outCols, const int outRows,
                          const float* const inputData, float* const outputData)
{
#pragma omp parallel for
    for (int y = 0; y < outRows; y++)
    {
        const int iy1 = y / 2;
        float* outp = outputData + y*outCols;
        const float* inp = inputData + iy1*(outCols/2);
        for (int x = 0; x < outCols; x+=2)
        {
            const int ix1 = x / 2;
            outp[x] = outp[x+1] = inp[ix1];
        }
    }
}

void matrixUpsample(size_t outCols, size_t outRows,
                    const float* inputData, float* outputData)
{
    if ( !(outRows%2) && !(outCols%2) )
    {
        matrixUpsampleSimple(outCols, outRows, inputData, outputData);
    }
    else
    {
        matrixUpsampleFull(outCols, outRows, inputData, outputData);
    }
}

namespace
{
void xGradient(size_t ROWS, size_t COLS, const float* lum, float* Gx)
{
#pragma omp parallel for
    for (int ky = 0; ky < ROWS; ky++)
    {
        float* currGx = Gx + ky*COLS;
        float* endGx = currGx + COLS - 1;
        const float* currLum = lum + ky*COLS;

        while ( currGx != endGx )
        {
            *currGx++ = *(currLum + 1) - *currLum; currLum++;
        }
        *currGx = 0.0f;
    }
}

void yGradient(size_t ROWS, size_t COLS, const float* lum, float* Gy)
{
#pragma omp parallel for
    for (int ky = 0; ky < ROWS-1; ++ky)
    {
        float* currGy = Gy + ky*COLS;
        float* endGy = currGy + COLS;
        const float* currLumU = lum + ky*COLS;
        const float* currLumL = lum + (ky + 1)*COLS;

        while ( currGy != endGy )
        {
            *currGy++ = *currLumL++ - *currLumU++;
        }
    }

    // ...and last row!
    float* GyLastRow = Gy + COLS*(ROWS-1);
    std::fill(GyLastRow, GyLastRow + COLS, 0.0f);
}
}

// calculate gradients
void calculateGradients(size_t cols, size_t rows,
                        const float* inputData,
                        float* gxData, float* gyData)
{
    xGradient(rows, cols, inputData, gxData);
    yGradient(rows, cols, inputData, gyData);
}



namespace
{
const float LOG10 = 2.3025850929940456840179914546844f;
const size_t LOOKUP_W_TO_R = 107;

static float W_table[] = {
    0.000000f,0.010000f,0.021180f,0.031830f,0.042628f,0.053819f,0.065556f,
    0.077960f,0.091140f,0.105203f,0.120255f,0.136410f,0.153788f,0.172518f,
    0.192739f,0.214605f,0.238282f,0.263952f,0.291817f,0.322099f,0.355040f,
    0.390911f,0.430009f,0.472663f,0.519238f,0.570138f,0.625811f,0.686754f,
    0.753519f,0.826720f,0.907041f,0.995242f,1.092169f,1.198767f,1.316090f,
    1.445315f,1.587756f,1.744884f,1.918345f,2.109983f,2.321863f,2.556306f,
    2.815914f,3.103613f,3.422694f,3.776862f,4.170291f,4.607686f,5.094361f,
    5.636316f,6.240338f,6.914106f,7.666321f,8.506849f,9.446889f,10.499164f,
    11.678143f,13.000302f,14.484414f,16.151900f,18.027221f,20.138345f,
    22.517282f,25.200713f,28.230715f,31.655611f,35.530967f,39.920749f,
    44.898685f,50.549857f,56.972578f,64.280589f,72.605654f,82.100619f,
    92.943020f,105.339358f,119.530154f,135.795960f,154.464484f,175.919088f,
    200.608905f,229.060934f,261.894494f,299.838552f,343.752526f,394.651294f,
    453.735325f,522.427053f,602.414859f,695.706358f,804.693100f,932.229271f,
    1081.727632f,1257.276717f,1463.784297f,1707.153398f,1994.498731f,
    2334.413424f,2737.298517f,3215.770944f,3785.169959f,4464.187290f,
    5275.653272f,6247.520102f,7414.094945f,8817.590551f,10510.080619f};
static float R_table[] = {
    0.000000f,0.009434f,0.018868f,0.028302f,0.037736f,0.047170f,0.056604f,
    0.066038f,0.075472f,0.084906f,0.094340f,0.103774f,0.113208f,0.122642f,
    0.132075f,0.141509f,0.150943f,0.160377f,0.169811f,0.179245f,0.188679f,
    0.198113f,0.207547f,0.216981f,0.226415f,0.235849f,0.245283f,0.254717f,
    0.264151f,0.273585f,0.283019f,0.292453f,0.301887f,0.311321f,0.320755f,
    0.330189f,0.339623f,0.349057f,0.358491f,0.367925f,0.377358f,0.386792f,
    0.396226f,0.405660f,0.415094f,0.424528f,0.433962f,0.443396f,0.452830f,
    0.462264f,0.471698f,0.481132f,0.490566f,0.500000f,0.509434f,0.518868f,
    0.528302f,0.537736f,0.547170f,0.556604f,0.566038f,0.575472f,0.584906f,
    0.594340f,0.603774f,0.613208f,0.622642f,0.632075f,0.641509f,0.650943f,
    0.660377f,0.669811f,0.679245f,0.688679f,0.698113f,0.707547f,0.716981f,
    0.726415f,0.735849f,0.745283f,0.754717f,0.764151f,0.773585f,0.783019f,
    0.792453f,0.801887f,0.811321f,0.820755f,0.830189f,0.839623f,0.849057f,
    0.858491f,0.867925f,0.877358f,0.886792f,0.896226f,0.905660f,0.915094f,
    0.924528f,0.933962f,0.943396f,0.952830f,0.962264f,0.971698f,0.981132f,
    0.990566f,1.000000f};

// in_tab and out_tab should contain inccreasing float values
inline
float lookup_table(const size_t n,
                   const float* in_tab, const float* out_tab,
                   const float val)
{
    if ( val < in_tab[0] ) return out_tab[0];

    for (size_t j = 1; j < n; j++)
    {
        if (val < in_tab[j])
        {
            const float dd = (val - in_tab[j-1]) / (in_tab[j] - in_tab[j-1]);
            return out_tab[j-1] + (out_tab[j] - out_tab[j-1]) * dd;
        }
    }

    return out_tab[n-1];
}
}

// transform gradient (Gx,Gy) to R
void transformToR(float* G, float detailFactor, size_t size)
{
    const float log10 = 2.3025850929940456840179914546844*detailFactor;

#pragma omp parallel for
    for (int j = 0; j < size; j++)
    {
        // G to W
        float Curr_G = G[j];

        if (Curr_G < 0.0f)
        {
            Curr_G = -(powf(10, (-Curr_G) * log10) - 1.0f);
        } else {
            Curr_G = (powf(10, Curr_G * log10) - 1.0f);
        }
        // W to RESP
        if (Curr_G < 0.0f)
        {
            Curr_G = -lookup_table(LOOKUP_W_TO_R, W_table, R_table, -Curr_G);
        } else {
            Curr_G = lookup_table(LOOKUP_W_TO_R, W_table, R_table, Curr_G);
        }

        G[j] = Curr_G;
    }
}

// transform from R to G
void transformToG(float* R, float detailFactor, size_t size)
{
    //here we are actually changing the base of logarithm
    const float log10 = 2.3025850929940456840179914546844*detailFactor;

#pragma omp parallel for
    for (int j = 0; j < size; j++)
    {
        float Curr_R = R[j];

        // RESP to W
        if (Curr_R < 0.0f)
        {
            Curr_R = -lookup_table(LOOKUP_W_TO_R, R_table, W_table, -Curr_R);
        } else {
            Curr_R = lookup_table(LOOKUP_W_TO_R, R_table, W_table, Curr_R);
        }

        // W to G
        if (Curr_R < 0.0f)
        {
            Curr_R = -log((-Curr_R) + 1.0f) / log10;
        } else {
            Curr_R = log(Curr_R + 1.0f) / log10;
        }

        R[j] = Curr_R;
    }
}

//! \brief calculate divergence of two gradient maps (Gx and Gy)
//! divG(x,y) = [Gx(x,y) - Gx(x-1,y)] + [Gy(x,y) - Gy(x,y-1)]
void calculateAndAddDivergence(size_t COLS, size_t ROWS,
                               const float* Gx, const float* Gy, float* divG)
{
    float divGx, divGy;
#pragma omp parallel sections private(divGx, divGy)
    {
#pragma omp section
        {
            // kx = 0 AND ky = 0;
            divG[0] += Gx[0] + Gy[0];                       // OUT

            // ky = 0
            for (size_t kx = 1; kx < COLS; kx++)
            {
                divGx = Gx[kx] - Gx[kx - 1];
                divGy = Gy[kx];
                divG[kx] += divGx + divGy;                    // OUT
            }
        }
#pragma omp section
        {
#pragma omp parallel for schedule(static) private(divGx, divGy)
            for (int ky = 1; ky < ROWS; ky++)
            {
                // kx = 0
                divGx = Gx[ky*COLS];
                divGy = Gy[ky*COLS] - Gy[ky*COLS - COLS];
                divG[ky*COLS] += divGx + divGy;               // OUT

                // kx > 0
                for (size_t kx = 1; kx < COLS; kx++)
                {
                    divGx = Gx[kx + ky*COLS] - Gx[kx + ky*COLS-1];
                    divGy = Gy[kx + ky*COLS] - Gy[kx + ky*COLS - COLS];
                    divG[kx + ky*COLS] += divGx + divGy;        // OUT
                }
            }
        }
    }   // END PARALLEL SECTIONS
}

void calculateScaleFactor(const float* G, float* C, size_t size)
{
    //  float GFIXATE = 0.1f;
    //  float EDGE_WEIGHT = 0.01f;
    const float detectT = 0.001f;
    const float a = 0.038737f;
    const float b = 0.537756f;

#pragma omp parallel for schedule(static)
    for (int i = 0; i < size; i++)
    {
        //#if 1
        const float g = std::max( detectT, std::fabs(G[i]) );
        C[i] = 1.0f / (a* std::pow(g,b));
        //#else
        //    if(fabsf(G[i]) < GFIXATE)
        //      C[i] = 1.0f / EDGE_WEIGHT;
        //    else
        //      C[i] = 1.0f;
        //#endif
    }
}




