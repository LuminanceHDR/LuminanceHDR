/*
 * This file is a part of Luminance HDR package
 * ----------------------------------------------------------------------
 * Copyright (C) 2011-2012 Davide Anastasia
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

//! \brief apply gamma and black/white point to the input frame
//! \author Davide Anastasia <davideanastasia@users.sourceforge.net>

#include <cmath>
#include <iostream>

#include "Libpfs/channel.h"
#include "Libpfs/frame.h"
#include "Libpfs/utils/msec_timer.h"

namespace {

template <typename T>
inline T clamp(const T &v, const T &lower_bound, const T &upper_bound) {
    if (v <= lower_bound) return lower_bound;
    if (v >= upper_bound) return upper_bound;
    return v;
}

////! \note I assume that *in* contains only value between [0,1]
// void gamma_levels_array(const pfs::Array2D* in, pfs::Array2D* out,
//                        float black_in, float white_in,
//                        float black_out, float white_out, float gamma)
//{
//    // same formula used inside GammaAndLevels::refreshLUT()
//    //float value = powf( ( ((float)(i)/255.0f) - bin ) / (win-bin),
//    expgamma);
//    //LUT[i] = clamp(blackout+value*(whiteout-blackout),0,255);

//    const float* in_vector = in->getRawData();
//    float* out_vector = out->getRawData();

//    const int ELEMS = in->getCols()*in->getRows();

//    if (gamma != 1.0f)
//    {
//#pragma omp parallel for
//        for (int idx = 0; idx < ELEMS; ++idx)
//        {
//            float tmp = (in_vector[idx] - black_in)/(white_in - black_in);
//            tmp = powf(tmp, gamma);

//            tmp = black_out + tmp*(white_out-black_out);

//            out_vector[idx] = clamp(tmp, 0.0f, 1.0f);
//        }
//    }
//    else
//    {
//#pragma omp parallel for
//        for (int idx = 0; idx < ELEMS; ++idx)
//        {
//            float tmp = (in_vector[idx] - black_in)/(white_in - black_in);
//            //tmp = powf(tmp, gamma);

//            tmp = black_out + tmp*(white_out-black_out);

//            out_vector[idx] = clamp(tmp, 0.0f, 1.0f);
//        }
//    }
//}
}

namespace pfs {

void gammaAndLevels(pfs::Frame *inFrame, float black_in, float white_in,
                    float black_out, float white_out, float gamma) {
#ifdef TIMER_PROFILING
    msec_timer f_timer;
    f_timer.start();
#endif

#ifndef NDEBUG
    std::cerr << "Black in = " << black_in << ", Black out = " << black_out
              << ", White in = " << white_in << ", White out = " << white_out
              << ", Gamma = " << gamma << std::endl;
#endif

    const int outWidth = inFrame->getWidth();
    const int outHeight = inFrame->getHeight();

    pfs::Channel *Xc, *Yc, *Zc;
    inFrame->getXYZChannels(Xc, Yc, Zc);
    assert(Xc != NULL && Yc != NULL && Zc != NULL);

    const float *R_i = Xc->data();
    const float *G_i = Yc->data();
    const float *B_i = Zc->data();

    float *R_o = Xc->data();
    float *G_o = Yc->data();
    float *B_o = Zc->data();

// float exp_gamma = 1.f/gamma;
#pragma omp parallel for
    for (int idx = 0; idx < outWidth * outHeight; ++idx) {
        float red = R_i[idx];
        float green = G_i[idx];
        float blue = B_i[idx];

        float L = 0.2126f * red + 0.7152f * green +
                  0.0722f * blue;  // number between [0..1]

        float c = powf(L, gamma - 1.0f);

        red = (red - black_in) / (white_in - black_in);
        red *= c;

        green = (green - black_in) / (white_in - black_in);
        green *= c;

        blue = (blue - black_in) / (white_in - black_in);
        blue *= c;

        R_o[idx] = clamp(black_out + red * (white_out - black_out), 0.f, 1.f);
        G_o[idx] = clamp(black_out + green * (white_out - black_out), 0.f, 1.f);
        B_o[idx] = clamp(black_out + blue * (white_out - black_out), 0.f, 1.f);
    }

#ifdef TIMER_PROFILING
    f_timer.stop_and_update();
    std::cout << "gamma_levels() = " << f_timer.get_time() << " msec"
              << std::endl;
#endif
}
}
