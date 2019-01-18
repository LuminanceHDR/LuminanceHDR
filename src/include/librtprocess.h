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

#include <functional>


enum rpError {RP_NO_ERROR, RP_MEMORY_ERROR, RP_WRONG_CFA, RP_CACORRECT_ERROR};
rpError bayerborder_demosaic(int winw, int winh, int lborders, const float * const *rawData, float **red, float **green, float **blue, const unsigned cfarray[2][2]);
void xtransborder_demosaic(int winw, int winh, int border, const float * const *rawData, float **red, float **green, float **blue, const unsigned xtrans[6][6]);
rpError amaze_demosaic(int raw_width, int raw_height, int winx, int winy, int winw, int winh, const float * const *rawData, float **red, float **green, float **blue, const unsigned cfarray[2][2], const std::function<bool(double)> &setProgCancel, double initGain, int border, float inputScale = 65535.f, float outputScale = 65535.f);
rpError dcb_demosaic(int width, int height, const float * const *rawData, float **red, float **green, float **blue, const unsigned cfarray[2][2], const std::function<bool(double)> &setProgCancel, int iterations, bool dcb_enhance);
rpError rcd_demosaic(int width, int height, const float * const *rawData, float **red, float **green, float **blue, const unsigned cfarray[2][2], const std::function<bool(double)> &setProgCancel);
rpError markesteijn_demosaic(int width, int height, const float * const *rawdata, float **red, float **green, float **blue, const unsigned xtrans[6][6], const float rgb_cam[3][4], const std::function<bool(double)> &setProgCancel, const int passes, const bool useCieLab);
rpError xtransfast_demosaic(int width, int height, const float * const *rawData, float **red, float **green, float **blue, const unsigned xtrans[6][6], const std::function<bool(double)> &setProgCancel);
rpError vng4_demosaic (int width, int height, const float * const *rawData, float **red, float **green, float **blue, const unsigned cfarray[2][2], const std::function<bool(double)> &setProgCancel);
rpError igv_demosaic(int winw, int winh, const float * const *rawData, float **red, float **green, float **blue, const unsigned cfarray[2][2], const std::function<bool(double)> &setProgCancel);
rpError lmmse_demosaic(int width, int height, const float * const *rawData, float **red, float **green, float **blue, const unsigned cfarray[2][2], const std::function<bool(double)> &setProgCancel, int iterations);
rpError CA_correct(int winx, int winy, int winw, int winh, const bool autoCA, size_t autoIterations, const double cared, const double cablue, bool avoidColourshift, const float * const *rawDataIn, float **rawDataOut, const unsigned cfarray[2][2], const std::function<bool(double)> &setProgCancel, double fitParams[2][2][16], bool fitParamsIn, float inputScale = 65535.f, float outputScale = 65535.f);

enum eGaussType {GAUSS_STANDARD, GAUSS_MULT, GAUSS_DIV};
void gaussianBlur(float** src, float** dst, const int W, const int H, const double sigma, float *buffer = nullptr, eGaussType gausstype = GAUSS_STANDARD, float** buffer2 = nullptr);

#endif
