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

//using ColorFilterArray = std::array<std::array<unsigned, 3>, 3>;
using CaFitParams = std::array<std::array<std::array<double, 16>, 2>, 2>;
namespace librtprocess
{
  class ColorFilterArray: public std::array<std::array<unsigned, 3>, 3>
  {
    void init(int filters)
    {
      for(int row = 0; row < 2; row++) {
        for(int col = 0; col < 2; col++) {
          (*this)[row][col] = (filters >> ((((row) << 1 & 14) + ((col) & 1)) << 1) & 3);
        }
      }
    }

  public:
    ColorFilterArray(int filters)
    {
      init(filters);
    }

    void set_cfa(int filters)
    {
      init(filters);
    }
  };

    void amaze_demosaic(int winx, int winy, int winw, int winh, const array2D<float> &rawData, array2D<float> &red, array2D<float> &green, array2D<float> &blue, const ColorFilterArray &cfarray, const std::function<bool(double)> &setProgCancel, double initGain, int border, float inputScale = 65535.f, float outputScale = 65535.f);
    bool CA_correct(int winx, int winy, int winw, int winh, const bool autoCA, const double cared, const double cablue, array2D<float> &rawData, array2D<float> &rawDataOut, const ColorFilterArray &cfarray, const std::function<bool(double)> &setProgCancel, CaFitParams &fitParams, bool fitParamsIn, float inputScale = 65535.f, float outputScale = 65535.f);

}//namespace


#endif
