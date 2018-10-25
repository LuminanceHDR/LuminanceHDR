/**
 * @file tmo_pattanaik00.cpp
 * @brief Time-dependent Visual Adaptation Model, [Pattanaik2000]
 *
 * Time-Dependent Visual Adaptation for Realistic Image Display
 * S.N. Pattanaik, J. Tumblin, H. Yee, and D.P. Greenberg
 * In Proceedings of ACM SIGGRAPH 2000
 *
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
 * $Id: tmo_pattanaik00.cpp,v 1.3 2008/11/04 23:43:08 rafm Exp $
 */

#include <cmath>

#include "tmo_pattanaik00.h"

#include "Libpfs/array2d.h"
#include "Libpfs/pfs.h"
#include "Libpfs/progress.h"
#include "Libpfs/utils/msec_timer.h"
#include "TonemappingOperators/pfstmo.h"
#include "../../sleef.c"
#include "../../opthelper.h"
#define pow_F(a,b) (xexpf(b*xlogf(a)))
/// sensitivity of human visual system
float n = 0.73f;

float sigma_response_rod(float I);
float sigma_response_cone(float I);
float model_response(float I, float sigma);

namespace {
const float LOG5 = std::log(5.f);

/**
 * @brief Calculate local adaptation for specified pixel coordinates
 *
 * Calculation based on article "Adaptive Gain Control" by Pattanaik
 * 2002.
 *
 * @param Y luminance map
 * @param x x-coordinate of pixel
 * @param y x-coordinate of pixel
 * @param Acone [out] calculated adaptation for cones
 * @param Arod [out] calculated adaptation for rods
 */


float pow6(float x) {
    return (x*x) * (x*x) * (x*x);
}

#ifdef __SSE2__
vfloat pow6(vfloat x) {
    return (x*x) * (x*x) * (x*x);
}
#endif

float pow4(float x) {
    return (x*x) * (x*x);
}

float pow2(float x) {
    return (x*x);
}

float calculateLocalAdaptation(const pfs::Array2Df &Y, int x, int y) {
    int width = Y.getCols();
    int height = Y.getRows();

    int kernel_size = 4;

    float logLc = xlogf(Y(x, y)) / LOG5;

    float pix_num = 0.0;
    float pix_sum = 0.0;

    // we have to calculate the expensive f(x) = exp( -pow( log(element) / c1 - c2, 6)) for the elements which are marked with a 1 in the following matrix
    // without SSE this are 49 f(x) calculations
    // the current SSE implementation needs only 14 f(x) calculations (13 is also possible, but I've been lazy) and has no branches
    //
    // 0 0 0 0 1 0 0 0 0
    // 0 0 1 1 1 1 1 0 0
    // 0 1 1 1 1 1 1 1 0
    // 0 1 1 1 1 1 1 1 0
    // 1 1 1 1 1 1 1 1 1
    // 0 1 1 1 1 1 1 1 0
    // 0 1 1 1 1 1 1 1 0
    // 0 0 1 1 1 1 1 0 0
    // 0 0 0 0 1 0 0 0 0

#ifdef __SSE2__
    if(x >= 4 && x < width -4 && y > 4 && y < height - 4) {
        vfloat pix_numv = ZEROV;
        vfloat pix_sumv = ZEROV;
        vfloat log5v = F2V(LOG5);
        vfloat logLcv = F2V(logLc);
        {
            vfloat Lv = _mm_set_ps(Y(x, y - 4), Y(x + 2, y - 3), Y(x + 2, y + 3),Y(x, y + 4));
            vfloat wv = xexpf(-pow6(xlogf(Lv) / log5v - logLcv));
            pix_sumv += wv * Lv;
            pix_numv += wv;
        }
        {
            vfloat Lv = LVFU(Y(x - 2, y - 3));
            vfloat wv = xexpf(-pow6(xlogf(Lv) / log5v - logLcv));
            pix_sumv += wv * Lv;
            pix_numv += wv;
        }
        {
            vfloat Lv = LVFU(Y(x - 3, y - 2));
            vfloat wv = xexpf(-pow6(xlogf(Lv) / log5v - logLcv));
            pix_sumv += wv * Lv;
            pix_numv += wv;
            Lv = LVFU(Y(x + 1, y - 2));
            wv = xexpf(-pow6(xlogf(Lv) / log5v - logLcv));
            wv = (vfloat)vandi(vint(wv), _mm_set_epi32(0,0xffffffff,0xffffffff,0xffffffff));
            pix_sumv += wv * Lv;
            pix_numv += wv;

        }
        {
            vfloat Lv = LVFU(Y(x - 3, y - 1));
            vfloat wv = xexpf(-pow6(xlogf(Lv) / log5v - logLcv));
            pix_sumv += wv * Lv;
            pix_numv += wv;
            Lv = LVFU(Y(x + 1, y - 1));
            wv = xexpf(-pow6(xlogf(Lv) / log5v - logLcv));
            wv = (vfloat)vandi(vint(wv), _mm_set_epi32(0,0xffffffff,0xffffffff,0xffffffff));
            pix_sumv += wv * Lv;
            pix_numv += wv;

        }
        {
            vfloat Lv = LVFU(Y(x - 4, y));
            vfloat wv = xexpf(-pow6(xlogf(Lv) / log5v - logLcv));
            pix_sumv += wv * Lv;
            pix_numv += wv;
            Lv = LVFU(Y(x, y));
            wv = xexpf(-pow6(xlogf(Lv) / log5v - logLcv));
            pix_sumv += wv * Lv;
            pix_numv += wv;
            float L = Y(x + 4, y);
            float w = xexpf(-pow6(xlogf(L) / LOG5 - logLc));
            pix_sum += w * L;
            pix_num += w;

        }
        {
            vfloat Lv = LVFU(Y(x - 3, y + 1));
            vfloat wv = xexpf(-pow6(xlogf(Lv) / log5v - logLcv));
            pix_sumv += wv * Lv;
            pix_numv += wv;
            Lv = LVFU(Y(x + 1, y + 1));
            wv = xexpf(-pow6(xlogf(Lv) / log5v - logLcv));
            wv = (vfloat)vandi(vint(wv), _mm_set_epi32(0,0xffffffff,0xffffffff,0xffffffff));
            pix_sumv += wv * Lv;
            pix_numv += wv;

        }
        {
            vfloat Lv = LVFU(Y(x - 3, y + 2));
            vfloat wv = xexpf(-pow6(xlogf(Lv) / log5v - logLcv));
            pix_sumv += wv * Lv;
            pix_numv += wv;
            Lv = LVFU(Y(x + 1, y + 2));
            wv = xexpf(-pow6(xlogf(Lv) / log5v - logLcv));
            wv = (vfloat)vandi(vint(wv), _mm_set_epi32(0,0xffffffff,0xffffffff,0xffffffff));
            pix_sumv += wv * Lv;
            pix_numv += wv;

        }
        {
            vfloat Lv = LVFU(Y(x - 2, y + 3));
            vfloat wv = xexpf(-pow6(xlogf(Lv) / log5v - logLcv));
            pix_sumv += wv * Lv;
            pix_numv += wv;
        }
        pix_sum += vhadd(pix_sumv);
        pix_num += vhadd(pix_numv);

    } else {
        for (int ky = -kernel_size; ky <= kernel_size; ky++) {
            for (int kx = -kernel_size; kx <= kernel_size; kx++) {
                if ((kx * kx + ky * ky) <= (kernel_size * kernel_size) &&
                    x + kx > 0 && x + kx < width && y + ky > 0 && y + ky < height) {
                    float L = Y(x + kx, y + ky);
                    float w = xexpf(-pow6(xlogf(L) / LOG5 - logLc));
                    pix_sum += w * L;
                    pix_num += w;
                }
            }
        }
    }

#else
    for (int ky = -kernel_size; ky <= kernel_size; ky++) {
        for (int kx = -kernel_size; kx <= kernel_size; kx++) {
            if ((kx * kx + ky * ky) <= (kernel_size * kernel_size) &&
                x + kx > 0 && x + kx < width && y + ky > 0 && y + ky < height) {
                float L = Y(x + kx, y + ky);
                float w = xexpf(-pow6(xlogf(L) / LOG5 - logLc));
                pix_sum += w * L;
                pix_num += w;
            }
        }
    }
#endif
    if (pix_num > 0.0) {
        return pix_sum / pix_num;
    } else {
        return Y(x, y);
    }
}
}

// tone mapping operator code
void tmo_pattanaik00(pfs::Array2Df &R, pfs::Array2Df &G, pfs::Array2Df &B,
                     const pfs::Array2Df &Y, VisualAdaptationModel *am,
                     bool local, pfs::Progress &ph) {
#ifdef TIMER_PROFILING
    msec_timer stop_watch;
    stop_watch.start();
#endif

    ///--- initialization of parameters
    /// cones level of adaptation
    float Acone = am->getAcone();
    /// rods level of adaptation
    float Arod = am->getArod();
    /// cones bleaching term
    float Bcone = am->getBcone();
    /// rods bleaching term
    float Brod = am->getBrod();

    /// fraction of adaptation luminance that represents white luminance
    float white_factor = 5.0f;
    /// fraction of adaptation luminance that represents dark luminance
    float dark_factor = 32.0f / 5.0f;

    /// goal adaptation luminance while watching display
    float G_display = 25.0f;
    /// display luminance of white
    float display_white = G_display * white_factor;
    /// display luminance of dark
    float display_dark = G_display / dark_factor;
    /// half-saturation constant for display
    float display_sigma = sigma_response_cone(G_display);

    /// reference white for display
    float REF_Wht_display = model_response(display_white, display_sigma);
    /// reference black for display
    float REF_Blk_display = model_response(display_dark, display_sigma);

    /// display color saturation
    float S_d = (REF_Wht_display - REF_Blk_display) /
                (log10(display_white) - log10(display_dark));

    ///--- precalculated parameters
    /// half-saturation constant for cones
    float sigma_cone = sigma_response_cone(Acone);
    /// half-saturation constant for rods
    float sigma_rod = sigma_response_rod(Arod);

    // appearance model
    float scene_white_cone = Acone * white_factor;
    float scene_dark_cone = Acone / dark_factor;
    float scene_white_rod = Arod * white_factor;
    float scene_dark_rod = Arod / dark_factor;

    float REF_Wht_scene = Bcone * model_response(scene_white_cone, sigma_cone) +
                          Brod * model_response(scene_white_rod, sigma_rod);

    float REF_Blk_scene = Bcone * model_response(scene_dark_cone, sigma_cone) +
                          Brod * model_response(scene_dark_rod, sigma_rod);

    /// scene luminance minus shift
    float disp_x = 0.0f;
    /// scene luminance to display luminance scale factor
    float disp_y = 1.0f;
    /// display luminance plus shift
    float disp_z = 0.0f;

    float scale =
        (REF_Wht_display - REF_Blk_display) / (REF_Wht_scene - REF_Blk_scene);

    if (scale < 1.0f) {
        if (REF_Wht_scene > REF_Wht_display) {
            disp_x = REF_Wht_scene;
            disp_y = scale;
            disp_z = REF_Wht_display;
        } else if (REF_Blk_scene < REF_Blk_display) {
            disp_x = REF_Blk_scene;
            disp_y = scale;
            disp_z = REF_Blk_display;
        }
    } else if (scale > 1.0f) {
        if (REF_Wht_scene > REF_Wht_display) {
            disp_x = REF_Wht_scene;
            disp_y = 1.0f;
            disp_z = REF_Wht_display;
        } else if (REF_Blk_scene < REF_Blk_display) {
            disp_x = REF_Blk_scene;
            disp_y = 1.0f;
            disp_z = REF_Blk_display;
        }
    }

    ///--- tone map image

    int im_width = Y.getCols();
    int im_height = Y.getRows();
    int progress = 0;
    const int progressSteps = std::max(im_height / 98, 1);
    int phVal = 0;
    ph.setValue(phVal);
    const float dsbydw = display_sigma / display_white;

#ifdef _OPENMP
    #pragma omp parallel for firstprivate(Bcone, Brod, sigma_cone, sigma_rod) schedule(dynamic,16)
#endif
    for (int y = 0; y < im_height; y++) {
        for (int x = 0; x < im_width; x++) {
            float l = Y(x, y);
            float r = R(x, y) / l;
            float g = G(x, y) / l;
            float b = B(x, y) / l;

            if (local) {
                float adapt = calculateLocalAdaptation(Y, x, y);
                Bcone = 2e6 / (2e6 + adapt);
                Brod = 0.04f / (0.04f + adapt);

                sigma_cone = sigma_response_cone(adapt);
                sigma_rod = sigma_response_rod(adapt);
            }

            // receptor responses
            float Rrod = Brod * model_response(l, sigma_rod);
            float Rcone = Bcone * model_response(l, sigma_cone);
            float Rlum = Rrod + Rcone;
            if (Rlum > 0.0f) {
                Rrod /= Rlum;
                Rcone /= Rlum;
            }

            float Scolor = (Bcone * pow_F(sigma_cone, n) * n * pow_F(l, n)) / pow2(pow_F(l, n) + pow_F(sigma_cone, n));
            Scolor /= S_d;

            // appearance model
            float Ra = (Rlum - disp_x) * disp_y + disp_z;
            Ra = (Ra < 1.0f) ? ((Ra > 0.0f) ? Ra : 0.0f) : 0.9999999f;

            // inverse display model
            float I = dsbydw * pow_F(Ra / (1.0f - Ra), 1.0f / n);

            // apply new luminance
            r = I * (pow_F(r, Scolor) * Rcone + Rrod);
            g = I * (pow_F(g, Scolor) * Rcone + Rrod);
            b = I * (pow_F(b, Scolor) * Rcone + Rrod);

            R(x, y) = (r < 1.0f) ? ((r > 0.0f) ? r : 0.0f) : 1.0f;
            G(x, y) = (g < 1.0f) ? ((g > 0.0f) ? g : 0.0f) : 1.0f;
            B(x, y) = (b < 1.0f) ? ((b > 0.0f) ? b : 0.0f) : 1.0f;
        }
#ifdef _OPENMP
    #pragma omp critical
#endif
    {
        progress += 1;
        if((progress % progressSteps) == 0) {
            phVal ++;
            ph.setValue(std::min(phVal,98));
        }
    }

    }
    ph.setValue(98);
#ifdef TIMER_PROFILING
    stop_watch.stop_and_update();
    cout << endl;
    cout << "tmo_pattanaik00 = " << stop_watch.get_time() << " msec" << endl;
#endif

}

///////////////////////////////////////////////////////////

float sigma_response_rod(float I) {
    //!! INFO: this is essentially the same as below (formulas come from
    // R.W.G.Hunt book)
    //  float j = 0.00001f / (5.0f*I + 0.00001f);
    //   float fls =
    //   3800*pow(j,2.0f)*5*I+0.2*pow(1-pow(j,2.0f),4.0f)*pow(5*I,1.0f/6.0f);
    //   float sigma_rod = pow(2,1.0f/n) / fls * I;

    float j2 = pow2(1.0f / (5e4f * I + 1));
    return (2.5874f * I) / (19000.0f * j2 * I + 0.2615f * pow4(1.0f - j2) * std::sqrt(xcbrtf(I)));
}

float sigma_response_cone(float I) {
    //!! INFO: this is essentially the same as below (formulas come from
    // R.W.G.Hunt book)
    //   float k = 1.0f/(5.0f*I+1);
    //   float fl = 0.2f*5.0f*pow(k,4.0f)*I
    //     + 0.1f*pow(1.0f-pow(k,4.0f),2.0f)*pow(5*I,1.0f/3.0f);
    //   float sigma_cone = pow(2.0f,1.0f/n) / fl * (5.0f*I);

    float k4 = pow4(1.0f / (5.0f * I + 1));
    return (12.9223f * I) / (k4 * I + 0.171f * pow2(1.0f - k4) * xcbrtf(I));
}

float model_response(float I, float sigma) {
    return pow_F(I, n) / (pow_F(I, n) + pow_F(sigma, n));
}

VisualAdaptationModel::VisualAdaptationModel() { setAdaptation(60.0f, 60.0f); }

void VisualAdaptationModel::calculateAdaptation(float Gcone, float Grod,
                                                float dt) {
    // Visual adaptation model for cones
    const float t0cone = 0.080f;
    const float t0rod = 0.150f;
    const float tau_cone = 110.0f;
    const float tau_rod = 400.0f;

    float in, out;
    float delta_out;
    float f, j, k;

    // Calculation of Acone & Arod
    // Acone
    in = Gcone;
    out = Acone;
    delta_out = in - out;
    f = (1.0f - xexpf(-dt / t0cone));
    Acone += f * delta_out;

    // Arod
    in = Grod;
    out = Arod;
    delta_out = in - out;
    f = (1.0f - xexpf(-dt / t0rod));
    Arod += f * delta_out;

    // Calculation of Bcone & Brod
    // Bcone
    in = Gcone;
    out = Bcone;
    j = dt * in * out / 2.2e8;
    k = dt * (1.0f - out) / tau_cone;
    f = k - j;
    Bcone += f;

    // Brod
    in = Grod;
    out = Brod;
    j = dt * in * out / 16;
    k = dt * (1.0f - out) / tau_rod;
    f = k - j;
    Brod += f;
}

void VisualAdaptationModel::calculateAdaptation(const pfs::Array2Df &Y,
                                                float dt) {
    float Acone = calculateLogAvgLuminance(Y);
    calculateAdaptation(Acone, Acone, dt);
}

void VisualAdaptationModel::setAdaptation(float Gcone, float Grod) {
    Acone = Gcone;
    Arod = Grod;
    Bcone = 2e6 / (2e6 + Acone);
    Brod = 0.04f / (0.04f + Arod);
}

void VisualAdaptationModel::setAdaptation(const pfs::Array2Df &Y) {
    float Acone = calculateLogAvgLuminance(Y) * 5.0f;
    setAdaptation(Acone, Acone);
}

float VisualAdaptationModel::calculateLogAvgLuminance(const pfs::Array2Df &Y) {

    float avLum = 0.0f;

    size_t size = Y.getCols() * Y.getRows();
#ifdef _OPENMP
    #pragma omp parallel
#endif
{
    float avLumThr = 0.f;
#ifdef __SSE2__
    vfloat avLumThrv = ZEROV;
    vfloat c1v = F2V(1e-4f);
#endif
#ifdef _OPENMP
    #pragma omp for nowait
#endif
    for (size_t y = 0; y < Y.getRows(); ++y) {
        size_t x = 0;
#ifdef __SSE2__
        for (; x < Y.getCols() - 3; x+=4) {
            avLumThrv += xlogf(LVFU(Y(x,y)) + c1v);
        }
#endif
        for (; x < Y.getCols(); ++x) {
            avLumThr += xlogf(Y(x,y) + 1e-4f);
        }
    }
#ifdef _OPENMP
#pragma omp critical
#endif
{
    avLum += avLumThr;
#ifdef __SSE2__
    avLum += vhadd(avLumThrv);
#endif
}
}
    return xexpf(avLum / size) - 1e-4;
}
