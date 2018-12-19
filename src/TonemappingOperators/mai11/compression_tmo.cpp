/**
 * @brief Tone-mapping optimized for backward-compatible HDR image and video
 * compression
 *
 * From:
 * Mai, Z., Mansour, H., Mantiuk, R., Nasiopoulos, P., Ward, R., & Heidrich, W.
 * Optimizing a tone curve for backward-compatible high dynamic range image and
 * video compression.
 * IEEE Transactions on Image Processing, 20(6), 1558 – 1571.
 * doi:10.1109/TIP.2010.2095866, 2011
 *
 * This file is a part of LuminanceHDR package, based on pfstmo.
 * ----------------------------------------------------------------------
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
 * @author Rafal Mantiuk, <mantiuk@gmail.com>
 *
 * $Id: pfstmo_mantiuk08.cpp,v 1.19 2013/12/28 14:00:54 rafm Exp $
 */

#ifdef _OPENMP
#include <omp.h>
#endif
#include "opthelper.h"
#include "sleef.c"

#include <math.h>
#include <string.h>
#include <algorithm>
#include <iostream>

#include "Libpfs/utils/msec_timer.h"
#include "compression_tmo.h"

#ifdef BRANCH_PREDICTION
#define likely(x) __builtin_expect((x), 1)
#define unlikely(x) __builtin_expect((x), 0)
#else
#define likely(x) (x)
#define unlikely(x) (x)
#endif

/**
 * Lookup table on a uniform array & interpolation
 *
 * x_i must be at least two elements
 * y_i must be initialized after creating an object
 */
namespace mai {
class UniformArrayLUT {
    double start_v;
    size_t lut_size;
    double delta;

    bool own_y_i;

   public:
    double *y_i;

    UniformArrayLUT(double from, double to, int lut_size, double *y_i = NULL)
        : start_v(from),
          lut_size(lut_size),
          delta((to - from) / (double)lut_size) {
        if (y_i == NULL) {
            this->y_i = new double[lut_size];
            own_y_i = true;
        } else {
            this->y_i = y_i;
            own_y_i = false;
        }
    }

    UniformArrayLUT() : start_v(0.), lut_size(0), delta(0.), own_y_i(false), y_i(NULL) {}

    UniformArrayLUT(const UniformArrayLUT &other)
        : start_v(other.start_v), lut_size(other.lut_size), delta(other.delta) {
        this->y_i = new double[lut_size];
        own_y_i = true;
        memcpy(this->y_i, other.y_i, lut_size * sizeof(double));
    }

    UniformArrayLUT &operator=(const UniformArrayLUT &other) {
        if (&other != this) {
            this->lut_size = other.lut_size;
            this->delta = other.delta;
            this->start_v = other.start_v;
            this->y_i = new double[lut_size];
            own_y_i = true;
            memcpy(this->y_i, other.y_i, lut_size * sizeof(double));
        }
        return *this;
    }

    ~UniformArrayLUT() {
        if (own_y_i) delete[] y_i;
    }

    double interp(double x) {
        const double ind_f = (x - start_v) / delta;
        const size_t ind_low = (size_t)(ind_f);
        const size_t ind_hi = (size_t)ceil(ind_f);

        if (unlikely(ind_f < 0))  // Out of range checks
            return y_i[0];
        if (unlikely(ind_hi >= lut_size)) return y_i[lut_size - 1];

        if (unlikely(ind_low == ind_hi))
            return y_i[ind_low];  // No interpolation necessary

        return y_i[ind_low] +
               (y_i[ind_hi] - y_i[ind_low]) *
                   (ind_f - (double)ind_low);  // Interpolation
    }
};

class ImgHistogram {
   public:
    const float L_min, L_max;
    const float delta;
    int *bins;
    double *p;
    int bin_count;

    ImgHistogram() : L_min(-6.f), L_max(9.f), delta(0.1), bins(NULL), p(NULL) {
        bin_count = (int)ceil((L_max - L_min) / delta);
        bins = new int[bin_count];
        p = new double[bin_count];
    }

    ~ImgHistogram() {
        delete[] bins;
        delete[] p;
    }

    void compute(const float *img, size_t pixel_count) {
        std::fill(bins, bins + bin_count, 0);

        int pp_count = 0;

#pragma omp parallel
{
        int *binsThr = new int[bin_count];
        std::fill(binsThr, binsThr + bin_count, 0);

        int pp_countThr = 0;
        #pragma omp for nowait
        for (size_t pp = 0; pp < pixel_count; pp++) {
            int bin_index = (img[pp] - L_min) / delta;
            // ignore anything outside the range
            if (bin_index < 0 || bin_index >= bin_count) continue;
            binsThr[bin_index]++;
            pp_countThr++;
        }
        #pragma omp critical
        {
            pp_count += pp_countThr;
            for (int bb = 0; bb < bin_count; bb++) {
                bins[bb] += binsThr[bb];
            }
            delete [] binsThr;
        }
}
        for (int bb = 0; bb < bin_count; bb++) {
            p[bb] = (double)bins[bb] / (double)pp_count;
        }
    }
};

inline float safelog10f(float x) {
    return xlogf(std::max(x, 1e-5f)) * 0.43429448190325182765112891891661f;
}

#ifdef __SSE2__
inline vfloat safelog10f(vfloat xv, vfloat log10v, vfloat minv) {
    return xlogf(vmaxf(xv, minv)) * log10v;
}
#endif

void CompressionTMO::tonemap(const float *R_in, const float *G_in, float *B_in,
                             int width, int height, float *R_out, float *G_out,
                             float *B_out, const float *L_in,
                             pfs::Progress &ph) {
#ifdef TIMER_PROFILING
    msec_timer stop_watch;
    stop_watch.start();
#endif
    const size_t pix_count = width * height;

    ph.setValue(2);
    // Compute log of Luminance
    float *logL = new float[pix_count];

#ifdef __SSE2__
    const vfloat log10v = F2V(0.43429448190325182765112891891661f);
    const vfloat minv = F2V(1e-5f);
    #pragma omp parallel for
    for (size_t pp = 0; pp < pix_count - 3; pp += 4) {
        STVFU(logL[pp], safelog10f(LVFU(L_in[pp]), log10v, minv));
    }

    for (size_t pp = pix_count - (pix_count % 4); pp < pix_count; pp++) {
        logL[pp] = safelog10f(L_in[pp]);
    }
#else
    #pragma omp parallel for
    for (size_t pp = 0; pp < pix_count; pp++) {
        logL[pp] = safelog10f(L_in[pp]);
    }

#endif
    ImgHistogram H;
    H.compute(logL, pix_count);

    // Instantiate LUT
    UniformArrayLUT lut(H.L_min, H.L_max, H.bin_count);

    // Compute slopes
    //    std::unique_ptr<double[]> s(new double[H.bin_count]);
    double *s = new double[H.bin_count];
    {
        double d = 0;

        for (int bb = 0; bb < H.bin_count; bb++) {
            d += cbrt(H.p[bb]);
        }

        d *= H.delta;

        for (int bb = 0; bb < H.bin_count; bb++) {
            s[bb] = cbrt(H.p[bb]) / d;
        }
    }
    ph.setValue(33);

#if 0
    // TODO: Handling of degenerated cases, e.g. when an image contains uniform color
    const double s_max = 2.; // Maximum slope, to avoid enhancing noise
    double s_renorm = 1;
    for( int bb = 0; bb < H.bin_count; bb++ ) {
        if( s[bb] >= s_max ) {
            s[bb] = s_max;
            s_renorm -= s_max * H.delta;
        }
    }
    for( int bb = 0; bb < H.bin_count; bb++ ) {
        if( s[bb] < s_max ) {
            s[bb] = s_max;
            s_renorm -= s_max * H.delta;
        }

    }

#endif

    // Create a tone-curve
    lut.y_i[0] = 0;
    for (int bb = 1; bb < H.bin_count; bb++) {
        lut.y_i[bb] = lut.y_i[bb - 1] + s[bb] * H.delta;
    }
    ph.setValue(66);

// Apply the tone-curve

#pragma omp parallel for
    for (int pp = 0; pp < static_cast<int>(pix_count); pp++) {
#ifdef __SSE2__
        vfloat rgbv = _mm_set_ps(R_in[pp], G_in[pp], B_in[pp], 0);
        rgbv = safelog10f(rgbv, log10v, minv);
        float temp[4];
        STVFU(temp[0], rgbv);
        R_out[pp] = lut.interp(temp[3]);
        G_out[pp] = lut.interp(temp[2]);
        B_out[pp] = lut.interp(temp[1]);
#else
        R_out[pp] = lut.interp(safelog10f(R_in[pp]));
        G_out[pp] = lut.interp(safelog10f(G_in[pp]));
        B_out[pp] = lut.interp(safelog10f(B_in[pp]));
#endif
    }
    ph.setValue(99);
    delete[] s;
    delete[] logL;

#ifdef TIMER_PROFILING
    stop_watch.stop_and_update();
    std::cout << std::endl;
    std::cout << "tmo_mai11 = " << stop_watch.get_time() << " msec"
              << std::endl;
#endif
}
}
