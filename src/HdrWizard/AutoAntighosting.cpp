/**
 * This file is a part of Luminance HDR package
 * ---------------------------------------------------------------------- 
 * Copyright (C) 2013 Franco Comida
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
 *
 * Manual and auto anti-ghosting functions
 * @author Franco Comida <fcomida@users.sourceforge.net>
 *
 */

#include <QDebug>

#include <boost/bind.hpp>
#include <boost/math/constants/constants.hpp>
#include <cmath>

#include "Common/CommonFunctions.h"
#include <Libpfs/frame.h>
#include <Libpfs/colorspace/colorspace.h>
#include <Libpfs/manip/copy.h>
#include <Libpfs/utils/minmax.h>

#include <fftw3.h>

#include "AutoAntighosting.h"
// --- LEGACY CODE ---

float max(const Array2Df& u)
{
    return *std::max_element(u.begin(), u.end());
}

float min(const Array2Df& u)
{
    return *std::min_element(u.begin(), u.end());
}

void solve_pde_dct(Array2Df &F, Array2Df &U)
{
#ifdef TIMER_PROFILING
    msec_timer stop_watch;
    stop_watch.start();
#endif
    const int width = U.getCols();
    const int height = U.getRows();
    assert((int)F.getCols()==width && (int)F.getRows()==height);

    Array2Df Ftr(width, height);

    fftwf_plan p;
    #pragma omp parallel for private(p) schedule(static)
    for ( int j = 0; j < height; j++ ) {
        #pragma omp critical (make_plan)
        p = fftwf_plan_r2r_1d(width, F.data()+width*j, Ftr.data()+width*j, FFTW_REDFT00, FFTW_ESTIMATE);
        fftwf_execute(p); 
    }
    
  #pragma omp parallel 
  {
    vector<float> c(height);
    #pragma omp for
    for ( int i = 0; i < width; i++ ) {
        for (int j = 0; j < height; j++) {
            c[j] = 1.0f; 
        }
        float b = 2.0f*(cos(boost::math::double_constants::pi*i/width) - 2.0f);
        c[0] /= b;
        Ftr(i, 0) /= b;
        for (int j = 1; j < height - 1; j++ ) {
            float m = (b - c[j-1]);
            c[j] /= m;
            Ftr(i, j) = (Ftr(i, j) - Ftr(i, j-1))/m;   
        }
        Ftr(i, height - 1) = (Ftr(i, height - 1) - Ftr(i, height - 2))/(b - c[height - 2]);
        U(i, height - 1) = Ftr(i, height - 1);
        for (int j = height - 2; j >= 0; j--) {
            U(i, j) = Ftr(i, j) - c[j]*U(i, j+1);
        }
    }
  }

    const float invDivisor = 1.0f / (2.0f*(width-1));
    #pragma omp parallel for schedule(static) lastprivate(p)
    for ( int j = 0; j < height; j++ ) {
        #pragma omp critical (make_plan)
        p = fftwf_plan_r2r_1d(width, U.data()+width*j, U.data()+width*j, FFTW_REDFT00, FFTW_ESTIMATE);
        fftwf_execute(p); 
        
        for ( int i = 0; i < width; i++ ) {
            U(i, j) *= invDivisor;
        }
    }

    fftwf_destroy_plan(p);
#ifdef TIMER_PROFILING
    stop_watch.stop_and_update();
    std::cout << "solve_pde_dct = " << stop_watch.get_time() << " msec" << std::endl;
#endif
}

void clampToZero(Array2Df &R, Array2Df &G, Array2Df &B, float m)
{
  const int width = R.getCols();
  const int height = R.getRows();
  
  #pragma omp parallel for
  for ( int i = 0; i < width*height; i++ )
  {
    R(i) -= m;
    G(i) -= m;
    B(i) -= m;
  }
}

int findIndex(const float* data, int size)
{
    assert(size > 0);

    int idx = 0;
    float currentMax = data[0];
    for (int i = 0; i < size; i++) {
        if ( data[i] > currentMax ) {
            currentMax = data[i];
            idx = i;
        }
    }
    return idx;
}

typedef vector<float> BufferF;

inline
float hueMean(const BufferF& data)
{
    return std::accumulate(data.begin(), data.end(), 0.0f)/data.size();
}

void hueSquaredMean(const HdrCreationItemContainer& data,
                    vector<float>& HE)
{
    size_t width = data[0].frame()->getWidth();
    size_t height = data[0].frame()->getHeight();
    size_t numItems = data.size();

    float r, g, b, h, s, l;

    BufferF hues(numItems, 0.f);
    BufferF HS(numItems, 0.f);

    const Channel *X, *Y, *Z;

    for (size_t j = 0; j < height; j++) {
        for (size_t i = 0; i < width; i++) {
            for (size_t w = 0; w < numItems; w++) {
                data[w].frame()->getXYZChannels( X, Y, Z );

                r = (*X)(i, j);
                g = (*Y)(i, j);
                b = (*Z)(i, j);
                rgb2hsl(r, g, b, h, s, l);
                hues[w] = h;
            }

            float hueMean_ = hueMean(hues);

            for (size_t w = 0; w < numItems; w++) {
                float H = hueMean_ - hues[w];
                HS[w] += H*H;
            }
        }
    }

    for (size_t w = 0; w < numItems; w++) {
        HE[w] = HS[w] / (width*height);

        qDebug() << "HE[" << w << "]: " << HE[w];
    }
}

bool comparePatches(const HdrCreationItem& item1,
                    const HdrCreationItem& item2,
                    int i, int j, int gridX, int gridY, float threshold, float deltaEV, int dx, int dy)
{
    int gridSize = gridX * gridY;
    vector<float> logRed(gridSize);
    vector<float> logGreen(gridSize);
    vector<float> logBlue(gridSize);

    Channel *X1, *Y1, *Z1, *X2, *Y2, *Z2;
    item1.frame()->getXYZChannels( X1, Y1, Z1 );
    item2.frame()->getXYZChannels( X2, Y2, Z2 );
    Array2Df& R1 = *X1;
    Array2Df& G1 = *Y1;
    Array2Df& B1 = *Z1;
    Array2Df& R2 = *X2;
    Array2Df& G2 = *Y2;
    Array2Df& B2 = *Z2;
    
    int width = gridX*agGridSize;
    int height = gridY*agGridSize;
    int count = 0;
    for (int y = j * gridY; y < (j+1) * gridY; y++) {
        if (y+dy < 0 || y+dy > height-1)
            continue;
        for (int x = i * gridX; x < (i+1) * gridX; x++) {
            if (x+dx < 0 || x+dx > width-1)
                continue;
            if (R1(x, y) >= 1.0f || R2(x+dx, y+dy) >= 1.0f) {
                    logRed[count] = 0.0f;
                    logGreen[count] = 0.0f;
                    logBlue[count++] = 0.0f;
                    continue;
            }
            if (G1(x, y) >= 1.0f || G2(x+dx, y+dy) >= 1.0f) {
                    logRed[count] = 0.0f;
                    logGreen[count] = 0.0f;
                    logBlue[count++] = 0.0f;
                    continue;
            }
            if (B1(x, y) >= 1.0f || B2(x+dx, y+dy) >= 1.0f) {
                    logRed[count] = 0.0f;
                    logGreen[count] = 0.0f;
                    logBlue[count++] = 0.0f;
                    continue;
            }
            if (R1(x, y) <= 0.0f || R2(x+dx, y+dy) <= 0.0f) {
                    logRed[count] = 0.0f;
                    logGreen[count] = 0.0f;
                    logBlue[count++] = 0.0f;
                    continue;
            }
            if (G1(x, y) <= 0.0f || G2(x+dx, y+dy) <= 0.0f) {
                    logRed[count] = 0.0f;
                    logGreen[count] = 0.0f;
                    logBlue[count++] = 0.0f;
                    continue;
            }
            if (B1(x, y) <= 0.0f || B2(x+dx, y+dy) <= 0.0f) {
                    logRed[count] = 0.0f;
                    logGreen[count] = 0.0f;
                    logBlue[count++] = 0.0f;
                    continue;
            }

            float logDeltaEV = log(std::abs(deltaEV));
            if (deltaEV > 0) {
                logRed[count] = log(R1(x, y)) - log(R2(x+dx, y+dy)) - logDeltaEV;
                logGreen[count] = log(G1(x, y)) - log(G2(x+dx, y+dy)) - logDeltaEV;
                logBlue[count++] = log(B1(x, y)) - log(B2(x+dx, y+dy)) - logDeltaEV;
            }
            else {
                logRed[count] = log(R2(x, y)) - log(R1(x+dx, y+dy)) - logDeltaEV;
                logGreen[count] = log(G2(x, y)) - log(G1(x+dx, y+dy)) - logDeltaEV;
                logBlue[count++] = log(B2(x, y)) - log(B1(x+dx, y+dy)) - logDeltaEV;
            }
        }
    }
  
    float threshold1 = 0.75f;
    count = 0;
    for (int h = 0; h < gridSize; h++) {
        if (std::abs(logRed[h]) > threshold1 && std::abs(logGreen[h]) > threshold1 && std::abs(logBlue[h]) > threshold1)
            count++;
    }

    return (static_cast<float>(count) / static_cast<float>(gridX*gridY)) > threshold;
}

void computeIrradiance(Array2Df& irradiance, const Array2Df& in)
{
#ifdef TIMER_PROFILING
    msec_timer stop_watch;
    stop_watch.start();
#endif

    const int width = in.getCols();
    const int height = in.getRows();

#pragma omp parallel for schedule(static)
    for (int i = 0; i < width * height; ++i) {
        irradiance(i) = std::exp( in(i) );
    }

#ifdef TIMER_PROFILING
    stop_watch.stop_and_update();
    std::cout << "computeIrradiance = " << stop_watch.get_time() << " msec" << std::endl;
#endif
}

void computeLogIrradiance(Array2Df &logIrradiance, const Array2Df &u)
{
#ifdef TIMER_PROFILING
    msec_timer stop_watch;
    stop_watch.start();
#endif
    const int width = u.getCols();
    const int height = u.getRows();
    
    float ir, logIr;
    #pragma omp parallel for private (ir, logIr) schedule(static)
    for (int i = 0; i < width*height; i++) {
            ir = u(i);
            if (ir == 0.0f)
                logIr = -11.09f;
            else
                logIr = std::log(ir);

            logIrradiance(i) = logIr;
    }
 
#ifdef TIMER_PROFILING
    stop_watch.stop_and_update();
    std::cout << "computeLogIrradiance = " << stop_watch.get_time() << " msec" << std::endl;
#endif
}

void computeGradient(Array2Df &gradientX, Array2Df &gradientY, const Array2Df& in)
{
#ifdef TIMER_PROFILING
    msec_timer stop_watch;
    stop_watch.start();
#endif

    const int width = in.getCols();
    const int height = in.getRows();

    #pragma omp parallel for schedule(static)
    for (int j = 1; j < height-1; j++) {
        for (int i = 1; i < width-1; i++) {
            gradientX(i, j) = 0.5f*(in(i+1, j) - in(i-1, j));
            gradientY(i, j) = 0.5f*(in(i, j+1) - in(i, j-1));
        }
    }
    #pragma omp parallel for schedule(static)
    for (int i = 1; i < width-1; i++) {
        gradientX(i, 0) = 0.5f*(in(i+1, 0) - in(i-1, 0));
        gradientX(i, height-1) = 0.5f*(in(i+1, height-1) - in(i-1, height-1));
        gradientY(i, 0) = 0.0f;
        gradientY(i, height-1) = 0.0f;
    }
    #pragma omp parallel for schedule(static)
    for (int j = 1; j < height-1; j++) {
        gradientX(0, j) = 0.0f;
        gradientX(width-1, j) = 0.0f;
        gradientY(0, j) = 0.5f*(in(0, j+1) - in(0, j-1));
        gradientY(width-1, j) = 0.5f*(in(width-1, j+1) - in(width-1, j-1));
    }
    gradientX(0, 0) = gradientX(0, height-1) = gradientX(width-1, 0) = gradientX(width-1, height-1) = 0.0f;
    gradientY(0, 0) = gradientY(0, height-1) = gradientY(width-1, 0) = gradientY(width-1, height-1) = 0.0f;
#ifdef TIMER_PROFILING
    stop_watch.stop_and_update();
    std::cout << "computeGradient = " << stop_watch.get_time() << " msec" << std::endl;
#endif
}

void computeDivergence(Array2Df &divergence, const Array2Df& gradientX, const Array2Df& gradientY)
{
#ifdef TIMER_PROFILING
    msec_timer stop_watch;
    stop_watch.start();
#endif
    const int width = gradientX.getCols();
    const int height = gradientX.getRows();

    divergence(0, 0) = gradientX(0, 0) + gradientY(0, 0);
    #pragma omp parallel for schedule(static)
    for (int j = 1; j < height-1; j++) {
        for (int i = 1; i < width-1; i++) {
            divergence(i, j) = 0.5f*(gradientX(i+1, j) - gradientX(i-1, j)) +
                                  0.5f*(gradientY(i, j+1) - gradientY(i, j-1));
        }
    }
    #pragma omp parallel for schedule(static)
    for (int j = 1; j < height-1; j++) {
        divergence(0, j) = gradientX(1, j) - gradientX(0, j) + 0.5f*(gradientY(0, j+1) - gradientY(0, j-1));
        divergence(width-1, j) = gradientX(width-1, j) - gradientX(width-2, j) +
                                    0.5f*(gradientY(width-1, j) - gradientY(width-1, j-1));
    }
    #pragma omp parallel for schedule(static)
    for (int i = 1; i < width-1; i++) {
        divergence(i, 0) = 0.5f*(gradientX(i, 0) - gradientX(i-1, 0)) + gradientY(i, 0);
        divergence(i, height-1) = 0.5f*(gradientX(i+1, height-1) - gradientX(i-1, height-1)) +
                                     gradientY(i, height-1) - gradientY(i, height-2);
    }
#ifdef TIMER_PROFILING
    stop_watch.stop_and_update();
    std::cout << "computeDivergence = " << stop_watch.get_time() << " msec" << std::endl;
#endif
}

void blendGradients(Array2Df &gradientXBlended, Array2Df &gradientYBlended,
                    const Array2Df &gradientX, const Array2Df &gradientY,
                    const Array2Df &gradientXGood, const Array2Df &gradientYGood,
                    bool patches[agGridSize][agGridSize], const int gridX, const int gridY)
{
#ifdef TIMER_PROFILING
    msec_timer stop_watch;
    stop_watch.start();
#endif
    int width = gradientX.getCols();
    int height = gradientY.getRows();

    int x, y;
    #pragma omp parallel for private(x, y) schedule(static)
    for (int j = 0; j < height; j++) {
        y = floor(static_cast<float>(j)/gridY);
        for (int i = 0; i < width; i++) {
            x = floor(static_cast<float>(i)/gridX);
            if (patches[x][y] == true) {
                gradientXBlended(i, j) = gradientXGood(i, j);
                gradientYBlended(i, j) = gradientYGood(i, j);
                if (i % gridX == 0) {
                    gradientXBlended(i+1, j) = gradientXGood(i+1, j);
                    gradientYBlended(i+1, j) = gradientYGood(i+1, j);
                }
                if (j % gridY == 0) {
                    gradientXBlended(i, j+1) = gradientXGood(i, j+1);
                    gradientYBlended(i, j+1) = gradientYGood(i, j+1);
                }
            }
            else {
                gradientXBlended(i, j) = gradientX(i, j);
                gradientYBlended(i, j) = gradientY(i, j);
            }
        }
    }
#ifdef TIMER_PROFILING
    stop_watch.stop_and_update();
    std::cout << "blendGradients = " << stop_watch.get_time() << " msec" << std::endl;
#endif
}

void blendGradients(Array2Df &gradientXBlended, Array2Df &gradientYBlended,
                    const Array2Df &gradientX, const Array2Df &gradientY,
                    const Array2Df &gradientXGood, const Array2Df &gradientYGood,
                    const QImage& agMask)
{
#ifdef TIMER_PROFILING
    msec_timer stop_watch;
    stop_watch.start();
#endif
    int width = gradientX.getCols();
    int height = gradientY.getRows();

    #pragma omp parallel for schedule(static)
    for (int j = 0; j < height; j++) {
        for (int i = 0; i < width; i++) {
            if (qAlpha(agMask.pixel(i,j))  != 0) {
                gradientXBlended(i, j) = gradientXGood(i, j);
                gradientYBlended(i, j) = gradientYGood(i, j);
            }
            else {
                gradientXBlended(i, j) = gradientX(i, j);
                gradientYBlended(i, j) = gradientY(i, j);
            }
        }
    }
#ifdef TIMER_PROFILING
    stop_watch.stop_and_update();
    std::cout << "blendGradients = " << stop_watch.get_time() << " msec" << std::endl;
#endif
}

void colorBalance(pfs::Array2Df& U, const pfs::Array2Df& F, const int x, const int y)
{
    const int width = U.getCols();
    const int height = U.getRows();
    
    float sf = F(x, y) / U(x, y);
    #pragma omp parallel for schedule(static)
    for (int i = 0; i < width * height; i++)
        U(i) *= sf;
} 
