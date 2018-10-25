/**
 * @brief Frederic Drago logmapping operator
 *
 * Adaptive logarithmic mapping for displaying high contrast
 * scenes.
 * F. Drago, K. Myszkowski, T. Annen, and N. Chiba. In Eurographics 2003.
 *
 * This file is a part of LuminanceHDR package, based on pfstmo.
 * ----------------------------------------------------------------------
 * Copyright (C) 2003,2004 Grzegorz Krawczyk
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
 * @author Grzegorz Krawczyk, <krawczyk@mpi-sb.mpg.de>
 *
 * $Id: tmo_drago03.cpp,v 1.4 2008/11/04 23:43:08 rafm Exp $
 */

#include "tmo_drago03.h"

#include <cassert>
#include <cmath>

#include <boost/math/special_functions/fpclassify.hpp>

#include "Libpfs/utils/msec_timer.h"
#include "Libpfs/frame.h"
#include "Libpfs/progress.h"
#include "TonemappingOperators/pfstmo.h"
#include "../../opthelper.h"
#include "../../sleef.c"

namespace {
inline float biasFunc(float b, float x) {
    return b == 0.f ? xexpf( x * xlogf(b)) : 1.f;
}

const float LOG05 = -0.693147f;  // log(0.5)
}

void calculateLuminance(unsigned int width, unsigned int height, const float *Y,
                        float &avLum, float &maxLum) {
    avLum = 0.0f;
    maxLum = 0.0f;

    int size = width * height;

#ifdef __SSE2__
    vfloat maxLumv = ZEROV;
    vfloat avLumv = ZEROV;
#endif // __SSE2__

#pragma omp parallel
{
    float eps = 1e-4f;
    float avLumThr = 0.f;
    float maxLumThr = 0.f;
#ifdef __SSE2__
    vfloat epsv = F2V(eps);
    vfloat maxLumThrv = ZEROV;
    vfloat avLumThrv = ZEROV;
#endif // __SSE2__
    #pragma omp for nowait
    for(size_t i = 0; i < height; ++i) {
        size_t j = 0;
#ifdef __SSE2__
        for (; j < width - 3; j+=4) {
            vfloat Ywv = LVFU(Y[i * width + j]);
            avLumThrv += xlogf(Ywv + epsv);
            maxLumThrv = vmaxf(Ywv, maxLumThrv);
        }
#endif
        for (; j < width; j++) {
            float Yw = Y[i * width + j];
            avLumThr += xlogf(Yw + eps);
            maxLumThr = std::max(Yw, maxLumThr);
        }
    }
#pragma omp critical
{
#ifdef __SSE2__
    avLumv += avLumThrv;
    maxLumv = vmaxf(maxLumv, maxLumThrv);
#endif
    avLum += avLumThr;
    maxLum = std::max(maxLum, maxLumThr);
}
}
#ifdef __SSE2__
    avLum += vhadd(avLumv);
    maxLum = std::max(maxLum, vhmax(maxLumv));
#endif
    avLum = exp(avLum / size);
}

void tmo_drago03(const pfs::Array2Df &Y, pfs::Array2Df &L, float maxLum,
                 float avLum, float bias, pfs::Progress &ph) {
    assert(Y.getRows() == L.getRows());
    assert(Y.getCols() == L.getCols());
#ifdef TIMER_PROFILING
    msec_timer stop_watch;
    stop_watch.start();
#endif

    // normalize maximum luminance by average luminance
    maxLum /= avLum;

    float divider = std::log10(maxLum + 1.0f);
    float biasP = (log(bias) / LOG05);
    float logmaxLum = log(maxLum);

    // Normal tone mapping of every pixel
    #pragma omp parallel
    {
    int yEnd = Y.getRows();
    int xEnd = Y.getCols();
#ifdef __SSE2__
    vfloat avLumv = F2V(avLum);
    vfloat onev = F2V(1.f);
    vfloat twov = F2V(2.f);
    vfloat eightv = F2V(8.f);
    vfloat biasPv = F2V(biasP);
    vfloat logmaxLumv = F2V(logmaxLum);
    vfloat dividerv = F2V(divider);
#endif
    #pragma omp for
    for (int y = 0; y < yEnd; y++) {
        int x = 0;
#ifdef __SSE2__
        for (; x < xEnd - 3; x+=4) {
            vfloat Ywv = LVFU(Y(x, y)) / avLumv;
            vfloat interpolv = xlogf( twov + eightv * xexpf(biasPv * (xlogf(Ywv) - logmaxLumv)));
            STVFU(L(x, y), xlogf(Ywv + onev) / (interpolv * dividerv));  // avoid loss of precision
        }
#endif
        for (; x < xEnd; x++) {
            float Yw = Y(x, y) / avLum;
            float interpol = xlogf( 2.f + 8.f * xexpf(biasP * (xlogf(Yw) - logmaxLum)));
            L(x, y) = xlogf(Yw + 1.f) / (interpol * divider);  // avoid loss of precision

            assert(!boost::math::isnan(L(x, y)));
        }
    }
    }
#ifdef TIMER_PROFILING
    stop_watch.stop_and_update();
    cout << endl;
    cout << "tmo_drago03 = " << stop_watch.get_time() << " msec" << endl;
#endif

}
