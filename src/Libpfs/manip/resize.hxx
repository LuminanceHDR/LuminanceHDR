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

#ifndef PFS_RESIZE_HXX
#define PFS_RESIZE_HXX

#include <boost/math/constants/constants.hpp>
#include "copy.h"
#include "resize.h"

#define PI4_Af 0.78515625f
#define PI4_Bf 0.00024127960205078125f
#define PI4_Cf 6.3329935073852539062e-07f
#define PI4_Df 4.9604681473525147339e-10f

namespace pfs {
namespace detail {

//! \author Franco Comida <fcomida@users.sourceforge.net>
//! \note Code derived from RawTherapee
//! https://github.com/Beep6581/RawTherapee/blob/dev/rtengine/ipresize.cc
const float RT_1_PI =
    2.0f * (float)boost::math::double_constants::one_div_two_pi;

inline float xrintf(float x) {
    return x < 0 ? (int)(x - 0.5f) : (int)(x + 0.5f);
}

inline float mlaf(float x, float y, float z) { return x * y + z; }

inline float xsinf(float d) {
    int q;
    float u, s;

    q = (int)xrintf(d * RT_1_PI);

    d = mlaf(q, -PI4_Af * 4, d);
    d = mlaf(q, -PI4_Bf * 4, d);
    d = mlaf(q, -PI4_Cf * 4, d);
    d = mlaf(q, -PI4_Df * 4, d);

    s = d * d;

    if ((q & 1) != 0) d = -d;

    u = 2.6083159809786593541503e-06f;
    u = mlaf(u, s, -0.0001981069071916863322258f);
    u = mlaf(u, s, 0.00833307858556509017944336f);
    u = mlaf(u, s, -0.166666597127914428710938f);

    u = mlaf(s, u * d, d);

    return u;
}

static inline float Lanc(float x, float a) {
    if (x * x < 1e-6f) {
        return 1.0f;
    } else if (x * x > a * a) {
        return 0.0f;
    } else {
        x = static_cast<float>(boost::math::double_constants::pi) * x;
        return a * xsinf(x) * xsinf(x / a) / (x * x);
    }
}

template <typename Type>
void Lanczos(const Type *src, Type *dst, int W, int H, int W2, int H2)

{
    const float scale = static_cast<float>(W2) / static_cast<float>(W);
    const float delta = 1.0f / scale;
    const float a = 3.0f;
    const float sc = std::min(scale, 1.0f);
    const int support = static_cast<int>(2.0f * a / sc) + 1;

    const Type zero = static_cast<Type>(0);

#pragma omp parallel
    {
        // storage for precomputed parameters for horisontal interpolation
        float *wwh = new float[support * W2];
        int *jj0 = new int[W2];
        int *jj1 = new int[W2];

        // temporal storage for vertically-interpolated row of pixels
        float *l = new float[W];

        // Phase 1: precompute coefficients for horisontal interpolation

        for (int j = 0; j < W2; j++) {
            // x coord of the center of pixel on src image
            float x0 = (static_cast<float>(j) + 0.5f) * delta - 0.5f;

            // weights for interpolation in horisontal direction
            float *w = wwh + j * support;

            // sum of weights used for normalization
            float ws = 0.0f;

            jj0[j] = std::max(0, static_cast<int>(floorf(x0 - a / sc)) + 1);
            jj1[j] = std::min(W, static_cast<int>(floorf(x0 + a / sc)) + 1);

            // calculate weights
            for (int jj = jj0[j]; jj < jj1[j]; jj++) {
                int k = jj - jj0[j];
                float z = sc * (x0 - static_cast<float>(jj));
                w[k] = Lanc(z, a);
                ws += w[k];
            }

            // normalize weights
            for (int k = 0; k < support; k++) {
                w[k] /= ws;
            }
        }

        // Phase 2: do actual interpolation
        // weights for interpolation in y direction
        float *w = new float[support];
#pragma omp for
        for (int i = 0; i < H2; i++) {
            // y coord of the center of pixel on src image
            float y0 = (static_cast<float>(i) + 0.5f) * delta - 0.5f;

            // sum of weights used for normalization
            float ws = 0.0f;

            int ii0 = std::max(0, static_cast<int>(floorf(y0 - a / sc)) + 1);
            int ii1 = std::min(H, static_cast<int>(floorf(y0 + a / sc)) + 1);

            // calculate weights for vertical interpolation
            for (int ii = ii0; ii < ii1; ii++) {
                int k = ii - ii0;
                float z = sc * (y0 - static_cast<float>(ii));
                w[k] = Lanc(z, a);
                ws += w[k];
            }

            // normalize weights
            for (int k = 0; k < support; k++) {
                w[k] /= ws;
            }

            // Do vertical interpolation. Store results.
            for (int j = 0; j < W; j++) {
                float o = 0.0f;

                for (int ii = ii0; ii < ii1; ii++) {
                    int k = ii - ii0;

                    o += w[k] * static_cast<float>(src[ii * W + j]);
                }

                l[j] = o;
            }

            // Do horizontal interpolation
            for (int j = 0; j < W2; j++) {
                float *wh = wwh + support * j;

                float o = 0.0f;

                for (int jj = jj0[j]; jj < jj1[j]; jj++) {
                    int k = jj - jj0[j];

                    o += wh[k] * l[jj];
                }

                dst[i * W2 + j] =
                    max(zero, min(static_cast<Type>(o),
                                  boost::numeric::bounds<Type>::highest()));
            }
        }

        delete[] w;
        delete[] wwh;
        delete[] jj0;
        delete[] jj1;
        delete[] l;
    }
}

const size_t BLOCK_FACTOR = 96;

//! \author Davide Anastasia <davideanastasia@users.sourceforge.net>
//! \note Code derived from
//! http://tech-algorithm.com/articles/bilinear-image-scaling/
//! with added OpenMP support and block based resampling
template <typename Type>
void resizeBilinearGray(const Type *pixels, Type *output, size_t w, size_t h,
                        size_t w2, size_t h2) {
    const float x_ratio = static_cast<float>(w - 1) / w2;
    const float y_ratio = static_cast<float>(h - 1) / h2;

    Type A, B, C, D;
    Type outputPixel;
    size_t x = 0;
    size_t y = 0;
    size_t index = 0;

    float x_diff = 0.0f;
    float y_diff = 0.0f;

#pragma omp parallel shared(pixels, output, w, h, w2, h2) private( \
    x_diff, y_diff, x, y, index, outputPixel, A, B, C, D)
    {
#pragma omp for schedule(static, 1)
        for (int iO = 0; iO < static_cast<int>(h2); iO += BLOCK_FACTOR) {
            for (int jO = 0; jO < static_cast<int>(w2); jO += BLOCK_FACTOR) {
                for (size_t i = iO, iEnd = std::min(iO + BLOCK_FACTOR, h2);
                     i < iEnd; i++) {
                    y = static_cast<size_t>(y_ratio * i);
                    y_diff = (y_ratio * i) - y;

                    for (size_t j = jO, jEnd = std::min(jO + BLOCK_FACTOR, w2);
                         j < jEnd; j++) {
                        x = static_cast<size_t>(x_ratio * j);
                        x_diff = (x_ratio * j) - x;

                        index = y * w + x;

                        A = pixels[index];
                        B = pixels[index + 1];
                        C = pixels[index + w];
                        D = pixels[index + w + 1];

                        // Y = A(1-w)(1-h) + B(w)(1-h) + C(h)(1-w) + D(w)(h)
                        outputPixel =
                            static_cast<Type>(A * (1 - x_diff) * (1 - y_diff) +
                                              B * (x_diff) * (1 - y_diff) +
                                              C * (y_diff) * (1 - x_diff) +
                                              D * (x_diff * y_diff));

                        output[i * w2 + j] = outputPixel;
                    }
                }
            }
        }
    }  // end parallel region
}

template <typename Type>
void resample(const ::pfs::Array2D<Type> *in, ::pfs::Array2D<Type> *out,
              InterpolationMethod m) {
    switch (m) {
        case LanczosInterp:
            Lanczos(in->data(), out->data(), in->getCols(), in->getRows(),
                    out->getCols(), out->getRows());
            break;
        case BilinearInterp:
            resizeBilinearGray(in->data(), out->data(), in->getCols(),
                               in->getRows(), out->getCols(), out->getRows());
            break;
    }
}

}  // anonymous

template <typename Type>
void resize(const Array2D<Type> *in, Array2D<Type> *out,
            InterpolationMethod m) {
    if (in->getCols() == out->getCols() && in->getRows() == out->getRows()) {
        pfs::copy(in, out);
    } else {
        detail::resample(in, out, m);
    }
}

}  // pfs

#endif  // PFS_RESIZE_HXX
