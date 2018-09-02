/*
 * This file is part of librtprocess.
 *
 * Copyright (c) 2018 Carlo Vaccari
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

#ifndef _LIBRTPROCESS_
#define _LIBRTPROCESS_

#include <array>
#include <functional>

#include "array2D.h"

using ColorFilterArray = std::array<std::array<unsigned, 2>, 2>;

namespace librtprocess
{

    void amaze_demosaic(int winx, int winy, int winw, int winh, const array2D<float> &rawData, array2D<float> &red, array2D<float> &green, array2D<float> &blue, const ColorFilterArray &cfarray, const std::function<bool(double)> &setProgCancel, double initGain, int border, float inputScale = 65535.f, float outputScale = 65535.f);
    float *CA_correct(int W, int H, const bool autoCA, const double cared, const double cablue, const double caautostrength, array2D<float> &rawData, const ColorFilterArray &cfarray, const std::function<bool(double)> &setProgCancel, double *fitParamsTransfer, bool fitParamsIn, bool fitParamsOut, float *buffer, bool freeBuffer);

}//namespace


#endif
