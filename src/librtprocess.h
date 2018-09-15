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

using ColorFilterArray = std::array<std::array<unsigned, 2>, 2>;
using CaFitParams = std::array<std::array<std::array<double, 16>, 2>, 2>;

namespace librtprocess
{
    void bayerborder_demosaic(int winw, int winh, int lborders, float **rawData, float **red, float **green, float **blue, const ColorFilterArray &cfarray);
    void xtransborder_demosaic(int winw, int winh, int border, float **rawData, float **red, float **green, float **blue, int xtrans[6][6]);
    void amaze_demosaic(int winx, int winy, int winw, int winh, float **rawData, float **red, float **green, float **blue, const ColorFilterArray &cfarray, const std::function<bool(double)> &setProgCancel, double initGain, int border, float inputScale = 65535.f, float outputScale = 65535.f);
    bool CA_correct(int W, int H, const bool autoCA, size_t autoIterations, const double cared, const double cablue, bool avoidColourshift, float **rawDataIn, float **rawDataOut, const ColorFilterArray &cfarray, const std::function<bool(double)> &setProgCancel, CaFitParams &fitParams, bool fitParamsIn, float inputScale = 65535.f, float outputScale = 65535.f);

    enum eGaussType {GAUSS_STANDARD, GAUSS_MULT, GAUSS_DIV};
    void gaussianBlur(float** src, float** dst, const int W, const int H, const double sigma, float *buffer = nullptr, eGaussType gausstype = GAUSS_STANDARD, float** buffer2 = nullptr);

}//namespace


#endif
