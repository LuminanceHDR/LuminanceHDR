/**
 * @file tmo_ferradans11.cpp
 * Implementation of the algorithm presented in :
 *
 * An Analysis of Visual Adaptation and Contrast Perception for Tone Mapping
 * S. Ferradans, M. Bertalmio, E. Provenzi, V. Caselles
 * In IEEE Trans. Pattern Analysis and Machine Intelligence
 *
*
 * @author Sira Ferradans Copyright (C) 2013
 *
 *
 * This file is a part of LuminanceHDR package, based on pfstmo.
 * ----------------------------------------------------------------------
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
 * Adapted to Luminance HDR
 * @author Franco Comida <francocomida@gmail.com>
 *
 */

#include <algorithm>
#include <cmath>

#include <assert.h>
#include <fftw3.h>
#include <math.h>

#include <cstring>

#include <stdlib.h>
#ifdef _OPENMP
#include <omp.h>
#endif

#include <boost/math/constants/constants.hpp>
#include <boost/thread/mutex.hpp>

#include <Common/init_fftw.h>
#include <Libpfs/array2d.h>
#include <Libpfs/progress.h>
#include <Libpfs/utils/msec_timer.h>
#include <Libpfs/utils/numeric.h>
#include <TonemappingOperators/pfstmo.h>
#include "tmo_ferradans11.h"
#define BENCHMARK
#include "../../StopWatch.h"
#include "../../sleef.c"
//#include "../../opthelper.h"
#define pow_F(a,b) (xexpf(b*xlogf(a)))

using namespace std;
using namespace pfs;
using namespace utils;

// for debugging purposes
#if 0
#define PFSEOL "\x0a"
static void dumpPFS( const char *fileName, const pfstmo::Array2D *data, const char *channelName )
{
   FILE *fh = fopen( fileName, "wb" );
   assert( fh != NULL );

   int width = data->getCols();
   int height = data->getRows();

   fprintf( fh, "PFS1" PFSEOL "%d %d" PFSEOL "1" PFSEOL "0" PFSEOL
     "%s" PFSEOL "0" PFSEOL "ENDH", width, height, channelName );

   for( int y = 0; y < height; y++ )
     for( int x = 0; x < width; x++ ) {
       float d = (*data)(x,y);
       fwrite( &d, sizeof( float ), 1, fh );
     }

   fclose( fh );
}
#endif

//--------------------------------------------------------------------

namespace {

static inline bool abs_compare(float a, float b) { return fabs(a) < fabs(b); }
/*Implementation, hardcoded of the R function */
inline float apply_arctg_slope10(float Ip, float I, float I2, float I3, float I4,
                          float I5, float I6, float I7) {

    // Arctan, slope 10
    float gr1, gr2, gr3, gr4, gr5, gr6, gr7;
    gr1 = Ip - I;
    gr2 = Ip * Ip - 2 * Ip * I + I2;
    gr3 = Ip * Ip * Ip - 3 * Ip * Ip * I + 3 * Ip * I2 - I3;
    gr4 = Ip * Ip * Ip * Ip + 4 * Ip * Ip * I2 + I4 - 4 * Ip * I3 +
          2 * Ip * Ip * (I2 - 2 * Ip * I);
    gr5 = Ip * gr4 - Ip * Ip * Ip * Ip * I - 4 * Ip * Ip * I3 - I5 +
          4 * Ip * I4 - 2 * Ip * Ip * (I3 - 2 * Ip * I2);
    gr6 = Ip * Ip * gr4 - Ip * Ip * Ip * Ip * Ip * I - 4 * Ip * Ip * Ip * I3 -
          Ip * I5 + 4 * Ip * Ip * I4 - 2 * Ip * Ip * Ip * (I3 - 2 * Ip * I2) -
          (I * Ip * gr4 - Ip * Ip * Ip * Ip * I2 - 4 * Ip * Ip * I4 - I6 +
           4 * Ip * I5 - 2 * Ip * Ip * (I4 - 2 * Ip * I3));
    gr7 = Ip * Ip * (Ip * (Ip * Ip * Ip * Ip + 4 * Ip * Ip * I2 + I4 -
                           4 * Ip * I3 + 2 * Ip * Ip * (I2 - 2 * Ip * I)) -
                     Ip * Ip * Ip * Ip * I - 4 * Ip * Ip * I3 - I5 +
                     4 * Ip * I4 - 2 * Ip * Ip * (I3 - 2 * Ip * I2)) -
          2 * Ip * (Ip * (Ip * Ip * Ip * Ip * I + 4 * Ip * Ip * I3 + I5 -
                          4 * Ip * I4 + 2 * Ip * Ip * (I3 - 2 * Ip * I2)) -
                    Ip * Ip * Ip * Ip * I2 - 4 * Ip * Ip * I4 - I6 +
                    4 * Ip * I5 - 2 * Ip * Ip * (I4 - 2 * Ip * I3)) +
          Ip * (Ip * Ip * Ip * Ip * I2 + 4 * Ip * Ip * I4 + I6 - 4 * Ip * I5 +
                2 * Ip * Ip * (I4 - 2 * Ip * I3)) -
          Ip * Ip * Ip * Ip * I3 - 4 * Ip * Ip * I5 - I7 + 4 * Ip * I6 -
          2 * Ip * Ip * (I5 - 2 * Ip * I4);

    // Hardcoded coefficients of the polynomial of degree 9 that allows to
    // approximate the neighborhood averaging as a sum of convolutions
    return ((-7.7456e+00) * gr7 + (3.1255e-16) * gr6 + (1.5836e+01) * gr5 +
            (-1.8371e-15) * gr4 + (-1.1013e+01) * gr3 + (4.4531e-16) * gr2 +
            (3.7891e+00) * gr1 + 1.2391e-15);
}

/*
 *  This Quickselect routine is based on the algorithm described in
 *  "Numerical recipes in C", Second Edition,
 *  Cambridge University Press, 1992, Section 8.5, ISBN 0-521-43108-5
 *  This code by Nicolas Devillard - 1998. Public domain.
 */

#define ELEM_SWAP(a, b) \
    {                   \
        float t = (a);  \
        (a) = (b);      \
        (b) = t;        \
    }
//#define ELEM_SWAP(a,b) { register float t=(a);(a)=(b);(b)=t; }
//#define ELEM_SWAP(a,b) { register float t=a;a=b;b=t; }

float quick_select(float arr[], int n) {

    int low, high;
    int median;
    int middle, ll, hh;

    low = 0;
    high = n - 1;
    median = (low + high) / 2;
    for (;;) {
        if (high <= low) /* One element only */
            return arr[median];

        if (high == low + 1) { /* Two elements only */
            if (arr[low] > arr[high]) ELEM_SWAP(arr[low], arr[high]);
            return arr[median];
        }

        /* Find median of low, middle and high items; swap into position low */
        middle = (low + high) / 2;
        if (arr[middle] > arr[high]) ELEM_SWAP(arr[middle], arr[high]);
        if (arr[low] > arr[high]) ELEM_SWAP(arr[low], arr[high]);
        if (arr[middle] > arr[low]) ELEM_SWAP(arr[middle], arr[low]);

        /* Swap low item (now in position middle) into position (low+1) */
        ELEM_SWAP(arr[middle], arr[low + 1]);

        /* Nibble from each end towards middle, swapping items when stuck */
        ll = low + 1;
        hh = high;
        for (;;) {
            do
                ll++;
            while (arr[low] > arr[ll]);
            do
                hh--;
            while (arr[hh] > arr[low]);

            if (hh < ll) break;

            ELEM_SWAP(arr[ll], arr[hh]);
        }

        /* Swap middle item (in position low) back into correct position */
        ELEM_SWAP(arr[low], arr[hh]);

        /* Re-set active partition */
        if (hh <= median) low = ll;
        if (hh >= median) high = hh - 1;
    }
}

#undef ELEM_SWAP

float medval(float a[], int length) {
    return accumulate(a, a + length, 0.f) / (float)length;
}

float MSE(float Im1[], float Im2[], int largo, float escala) {
    float res = 0.f;
    float v = 1.f / escala;
    float v2 = 1.f;
#pragma omp parallel for reduction(+ : res)
    for (int i = 0; i < largo; i++) {
        float tmp = (Im1[i]) * v - (Im2[i]) * v2;
        tmp = fabs(tmp);
        res += tmp;
    }
    return (res / largo);
}

void producto(fftwf_complex *A, const fftwf_complex *B, int fil, int col) {

    int length = fil * col;

#pragma omp parallel for
    for (int i = 0; i < length; i++) {
        float a1 = (A[i][0] * B[i][1] + A[i][1] * B[i][0]);
        A[i][0] = (A[i][0] * B[i][0] - A[i][1] * B[i][1]);
        A[i][1] = a1;
    }
}

void nucleo_gaussiano(float res[], int fil, int col, float sigma) {
    float normaliza =
        1.0 / (sqrt(2 * boost::math::double_constants::pi) * sigma + 1e-6);
    int mitfil = fil / 2;
    int mitcol = col / 2;
#pragma omp parallel for
    for (int i = 0; i < fil; i++)
        for (int j = 0; j < col; j++)
            res[i * col + j] = normaliza * xexpf(-((i - mitfil) * (i - mitfil) +
                                                 (j - mitcol) * (j - mitcol)) /
                                               (2 * sigma * sigma));
}

void escala(float a[], int largo, float maxv, float minv) {
    float M = a[0];
    float m = a[0];
#pragma omp parallel for reduction(max:M) reduction(min:m)
    for (int i = 1; i < largo; i++) {
        M = std::max(M, a[i]);
        m = std::min(m, a[i]);
    }

    float s = (maxv - minv) / (M - m);
#pragma omp parallel for
    for (int i = 0; i < largo; i++) {
        float R = a[i];
        a[i] = minv + s * (R - m);
    }
}

// Related to fft
void fftshift(float a[], int fil, int col) {
    float tmp;

#pragma omp parallel for private(tmp)
    for (int i = 0; i < fil / 2; i++)
        for (int j = 0; j < col / 2; j++) {
            tmp = a[i * col + j];
            a[i * col + j] = a[(i + fil / 2) * col + j + col / 2];
            a[(i + fil / 2) * col + j + col / 2] = tmp;
            tmp = a[(i + fil / 2) * col + j];
            a[(i + fil / 2) * col + j] = a[i * col + j + col / 2];
            a[i * col + j + col / 2] = tmp;
        }
}
}

void tmo_ferradans11(pfs::Array2Df &imR, pfs::Array2Df &imG, pfs::Array2Df &imB,
                     float rho, float invalpha, pfs::Progress &ph) {
BENCHFUN
#ifdef TIMER_PROFILING
    msec_timer stop_watch;
    stop_watch.start();
#endif

    init_fftw();

    ph.setValue(0);

    int fil = imR.getRows();
    int col = imR.getCols();
    int length = fil * col;
    int colors = 3;
    float dt = 0.2;                    // 1e-1;//
    float threshold_diff = dt / 20.0;  // 1e-5;//

    float *RGBorig[3];
    RGBorig[0] = new float[length];
    RGBorig[1] = new float[length];
    RGBorig[2] = new float[length];

#pragma omp parallel for
    for (int i = 0; i < length; i++) {
        RGBorig[0][i] = (float)max(imR(i), 0.f);
        RGBorig[1][i] = (float)max(imG(i), 0.f);
        RGBorig[2][i] = (float)max(imB(i), 0.f);
    }

    ///////////////////////////////////////////

    ph.setValue(10);

    float *RGB[3];
    RGB[0] = new float[length];
    RGB[1] = new float[length];
    RGB[2] = new float[length];

    int iteration = 0;
    float difference = 1000.0;

    float med[3];
    float *aux = new float[length];
    float median, mu[3];

    for (int k = 0; k < 3; k++) {
        vsadd(RGBorig[k], 1e-6, RGBorig[k], length);
        copy(RGBorig[k], RGBorig[k] + length, aux);
        median = quick_select(aux, length);
        float mdval = medval(aux, length);
        mu[k] = pow(mdval, 0.5) * pow(median, 0.5);
        copy(RGBorig[k], RGBorig[k] + length, RGB[k]);
    }

    delete[] aux;

    ph.setValue(15);
    if (ph.canceled()) {
        delete[] RGBorig[0];
        delete[] RGBorig[1];
        delete[] RGBorig[2];
        delete[] RGB[0];
        delete[] RGB[1];
        delete[] RGB[2];
        return;
    }

// SEMISATURATION CONSTANT SIGMA DEPENDS ON THE ILUMINATION
// OF THE BACKGROUND. DATA IN TABLA1 FROM VALETON+VAN NORREN.
// TAKE AS REFERENCE THE CHANNEL WITH MAXIMUM ILUMINATION.
// float muMax=max(max(mu[0],mu[1]),mu[2]);
    for (int k = 0; k < 3; k++) {
        // MOVE log(mu) EQUALLY FOR THE 3 COLOR CHANNELS: rho DOES NOT CHANGE
        // PARAMETER OF OUR ALGORITHM
        // float x=log10(muMax)-log10(mu[k]);
        float z = -0.37 * (log10(mu[k]) + 4 - rho) + 1.9;
        mu[k] *= pow(10, z);
    }
    // VALETON + VAN NORREN:
    // range = 4 orders; r is half the range
    // n=0.74 : EXPONENT in NAKA-RUSHTON formula
    float r = 2;
    float n = 0.74;

    for (int k = 0; k < 3; k++) {
        // find ctes. for WEBER-FECHNER from NAKA-RUSHTON
        float logs = log10(mu[k]);
        float I0 = mu[k] / pow(10, 1.2);
        float sigma_n = pow(mu[k], n);

        // WYSZECKI-STILES, PÃ�G. 530: FECHNER FRACTION
        float K_ = 100.0 / 1.85;
        if (k == 2) K_ = 100.0 / 8.7;
        float Ir = pow(10, logs + r);
        float mKlogc =
            pow(Ir, n) / (pow(Ir, n) + pow(mu[k], n)) - K_ * log10(Ir + I0);

// mix W-F and N-R

        float ln10 = log(10.f);
#pragma omp parallel for
        for (int i = 0; i < length; i++) {
            if (RGBorig[k][i] <= Ir) {
                RGB[k][i] = K_ * xlogf(RGBorig[k][i] + I0) / ln10 + mKlogc;
            } else {
                float In = pow_F(RGBorig[k][i], n);
                RGB[k][i] = In / (In + sigma_n);
            }
        }

        float minmez = *min_element(RGB[k], RGB[k] + length);
        vsadd(RGB[k], -minmez, RGB[k], length);

        float escalamez = 1.f / (*max_element(RGB[k], RGB[k] + length) + 1e-12);
        vsmul(RGB[k], escalamez, RGB[k], length);
    }

    ph.setValue(20);
    if (ph.canceled()) {
        delete[] RGBorig[0];
        delete[] RGBorig[1];
        delete[] RGBorig[2];
        delete[] RGB[0];
        delete[] RGB[1];
        delete[] RGB[2];
        return;
    }

#pragma omp parallel for
    for (int color = 0; color < colors; color++) {
        copy(RGB[color], RGB[color] + length, RGBorig[color]);
        med[color] = medval(RGB[color], length);
    }

    FFTW_MUTEX::fftw_mutex_alloc.lock();
    float *RGB0 = fftwf_alloc_real(length);
    float *u0 = fftwf_alloc_real(length);
    float *u2 = fftwf_alloc_real(length);
    float *u3 = fftwf_alloc_real(length);
    float *u4 = fftwf_alloc_real(length);
    float *u5 = fftwf_alloc_real(length);
    float *u6 = fftwf_alloc_real(length);
    float *u7 = fftwf_alloc_real(length);
    FFTW_MUTEX::fftw_mutex_alloc.unlock();

    FFTW_MUTEX::fftw_mutex_plan.lock();
    fftwf_complex *U = (fftwf_complex *)fftwf_malloc(sizeof(fftwf_complex) * length);

    // test for available wisdom
    fftwf_plan pU = fftwf_plan_dft_r2c_2d(fil, col, u0, U, FFTW_WISDOM_ONLY);
    if(!pU) {
        // no wisdom available, load wisdom from file
        fftwf_import_wisdom_from_filename("lhdrwisdom.fftw");
        // test again for wisdom
        pU = fftwf_plan_dft_r2c_2d(fil, col, u0, U, FFTW_WISDOM_ONLY);
        if(!pU) {
            // build plan with FFTW_MEASURE
            pU = fftwf_plan_dft_r2c_2d(fil, col, u0, U, FFTW_MEASURE);
            // save the wisdom
            fftwf_export_wisdom_to_filename("lhdrwisdom.fftw");
        }
    }

    fftwf_complex *U2 = (fftwf_complex *)fftwf_malloc(sizeof(fftwf_complex) * length);
    fftwf_plan pU2 = fftwf_plan_dft_r2c_2d(fil, col, u2, U2, FFTW_WISDOM_ONLY);

    fftwf_complex *U3 = (fftwf_complex *)fftwf_malloc(sizeof(fftwf_complex) * length);
    fftwf_plan pU3 = fftwf_plan_dft_r2c_2d(fil, col, u3, U3, FFTW_WISDOM_ONLY);

    fftwf_complex *U4 = (fftwf_complex *)fftwf_malloc(sizeof(fftwf_complex) * length);
    fftwf_plan pU4 = fftwf_plan_dft_r2c_2d(fil, col, u4, U4, FFTW_WISDOM_ONLY);

    fftwf_complex *U5 = (fftwf_complex *)fftwf_malloc(sizeof(fftwf_complex) * length);
    fftwf_plan pU5 = fftwf_plan_dft_r2c_2d(fil, col, u5, U5, FFTW_WISDOM_ONLY);

    fftwf_complex *U6 = (fftwf_complex *)fftwf_malloc(sizeof(fftwf_complex) * length);
    fftwf_plan pU6 = fftwf_plan_dft_r2c_2d(fil, col, u6, U6, FFTW_WISDOM_ONLY);

    fftwf_complex *U7 = (fftwf_complex *)fftwf_malloc(sizeof(fftwf_complex) * length);
    fftwf_plan pU7 = fftwf_plan_dft_r2c_2d(fil, col, u7, U7, FFTW_WISDOM_ONLY);


    float *iu = fftwf_alloc_real(length);
    fftwf_plan pinvU = fftwf_plan_dft_c2r_2d(fil, col, U, iu, FFTW_WISDOM_ONLY);
    if(!pinvU) {
        // no wisdom available. Makes no sense to load wisdom from file because we just did that a few lines above.
        // build plan with FFTW_MEASURE
        pinvU = fftwf_plan_dft_c2r_2d(fil, col, U, iu, FFTW_MEASURE);
        // save the wisdom
        fftwf_export_wisdom_to_filename("lhdrwisdom.fftw");
    }

    fftwf_plan pinvU2 = fftwf_plan_dft_c2r_2d(fil, col, U2, u2, FFTW_WISDOM_ONLY);

    fftwf_plan pinvU3 = fftwf_plan_dft_c2r_2d(fil, col, U3, u3, FFTW_WISDOM_ONLY);

    fftwf_plan pinvU4 = fftwf_plan_dft_c2r_2d(fil, col, U4, u4, FFTW_WISDOM_ONLY);

    fftwf_plan pinvU5 = fftwf_plan_dft_c2r_2d(fil, col, U5, u5, FFTW_WISDOM_ONLY);

    fftwf_plan pinvU6 = fftwf_plan_dft_c2r_2d(fil, col, U6, u6, FFTW_WISDOM_ONLY);

    fftwf_plan pinvU7 = fftwf_plan_dft_c2r_2d(fil, col, U7, u7, FFTW_WISDOM_ONLY);

    float alpha = min(col, fil) / invalpha;
    float *g = fftwf_alloc_real(length);
    FFTW_MUTEX::fftw_mutex_plan.unlock();

    nucleo_gaussiano(g, fil, col, alpha);
    escala(g, length, 1.f, 0.f);
    fftshift(g, fil, col);

    float suma, norm = 1.f / length;
    suma = accumulate(g, g + length, 0.f);

    float w = (1.0f / suma);
    vsmul(g, w, g, length);

    FFTW_MUTEX::fftw_mutex_plan.lock();
    fftwf_complex *G =
        (fftwf_complex *)fftwf_malloc(sizeof(fftwf_complex) * length);
    fftwf_plan pG = fftwf_plan_dft_r2c_2d(fil, col, g, G, FFTW_WISDOM_ONLY);
    FFTW_MUTEX::fftw_mutex_plan.unlock();
    fftwf_execute(pG);
    FFTW_MUTEX::fftw_mutex_destroy_plan.lock();
    fftwf_destroy_plan(pG);

    fftwf_free(g);
    FFTW_MUTEX::fftw_mutex_destroy_plan.unlock();

    ph.setValue(30);
    if (ph.canceled()) {
        delete[] RGBorig[0];
        delete[] RGBorig[1];
        delete[] RGBorig[2];
        delete[] RGB[0];
        delete[] RGB[1];
        delete[] RGB[2];
        FFTW_MUTEX::fftw_mutex_destroy_plan.lock();
        fftwf_destroy_plan(pU);
        fftwf_destroy_plan(pU2);
        fftwf_destroy_plan(pU3);
        fftwf_destroy_plan(pU4);
        fftwf_destroy_plan(pU5);
        fftwf_destroy_plan(pU6);
        fftwf_destroy_plan(pU7);

        fftwf_destroy_plan(pinvU);
        fftwf_destroy_plan(pinvU2);
        fftwf_destroy_plan(pinvU3);
        fftwf_destroy_plan(pinvU4);
        fftwf_destroy_plan(pinvU5);
        fftwf_destroy_plan(pinvU6);
        fftwf_destroy_plan(pinvU7);

        fftwf_free(RGB0);
        fftwf_free(u0);
        fftwf_free(u2);
        fftwf_free(u3);
        fftwf_free(u4);
        fftwf_free(u5);
        fftwf_free(u6);
        fftwf_free(u7);

        fftwf_free(iu);

        fftwf_free(U);
        fftwf_free(U2);
        fftwf_free(U3);
        fftwf_free(U4);
        fftwf_free(U5);
        fftwf_free(U6);
        fftwf_free(U7);

        FFTW_MUTEX::fftw_mutex_destroy_plan.unlock();
        return;
    }
    float delta = 0.f, oldDifference = 0.f;
    int steps;
    StopWatch StopW("main ferradans loop");
    while (difference > threshold_diff) {
        if (ph.canceled()) {
            break;
        }

        iteration++;
        difference = 0.0;

        for (int color = 0; color < colors; color++) {
            copy(RGB[color], RGB[color] + length, u0);
            copy(RGB[color], RGB[color] + length, RGB0);

            transform(u0, u0 + length, u0, u2, multiplies<float>());
            transform(u2, u2 + length, u0, u3, multiplies<float>());
            transform(u3, u3 + length, u0, u4, multiplies<float>());
            transform(u4, u4 + length, u0, u5, multiplies<float>());
            transform(u5, u5 + length, u0, u6, multiplies<float>());
            transform(u6, u6 + length, u0, u7, multiplies<float>());

            fftwf_execute(pU);
            producto(U, G, fil, col);
            fftwf_execute(pinvU);

            fftwf_execute(pU2);
            producto(U2, G, fil, col);
            fftwf_execute(pinvU2);

            fftwf_execute(pU3);
            producto(U3, G, fil, col);
            fftwf_execute(pinvU3);

            fftwf_execute(pU4);
            producto(U4, G, fil, col);
            fftwf_execute(pinvU4);

            fftwf_execute(pU5);
            producto(U5, G, fil, col);
            fftwf_execute(pinvU5);

            fftwf_execute(pU6);
            producto(U6, G, fil, col);
            fftwf_execute(pinvU6);

            fftwf_execute(pU7);
            producto(U7, G, fil, col);
            fftwf_execute(pinvU7);

#pragma omp parallel for
            for (int i = 0; i < length; i++) {
                // compute contrast component
                u0[i] = apply_arctg_slope10(u0[i], norm * iu[i], norm * u2[i], norm * u3[i],
                                            norm * u4[i], norm * u5[i], norm * u6[i], norm * u7[i]);

                // project onto the interval [-1,1]
                u0[i] = max(min(u0[i], 1.f), -1.f);
            }

            // normalizing R term to estandarize results
            //
            float mabsv = fabs(*max_element(u0, u0 + length, abs_compare));

            float multiplier = 0.5f / mabsv;

            float norm1 = (1.0 + dt * (1.0 + 255.0 / 253.0));  // assuming alpha=255/253,beta=1

#pragma omp parallel for
            for (int i = 0; i < length; i++) {
                RGB[color][i] = (RGB[color][i] + dt * (RGBorig[color][i] + multiplier * u0[i] + 255.f / 253.f * med[color])) / norm1;
                // project onto the interval [0,1]
                RGB[color][i] = max(min(RGB[color][i], 1.f), 0.f);
            }

            float mse = MSE(RGB0, RGB[color], length, 1.f);
            difference += mse;
        }
        delta = fabs(oldDifference - difference);
        steps = (difference - threshold_diff) / delta;
        oldDifference = difference;
        if (iteration > 1) ph.setValue(30 + 69 / (steps + 1));
    }
    StopW.stop();
    FFTW_MUTEX::fftw_mutex_destroy_plan.lock();
    fftwf_destroy_plan(pU);
    fftwf_destroy_plan(pU2);
    fftwf_destroy_plan(pU3);
    fftwf_destroy_plan(pU4);
    fftwf_destroy_plan(pU5);
    fftwf_destroy_plan(pU6);
    fftwf_destroy_plan(pU7);

    fftwf_destroy_plan(pinvU);
    fftwf_destroy_plan(pinvU2);
    fftwf_destroy_plan(pinvU3);
    fftwf_destroy_plan(pinvU4);
    fftwf_destroy_plan(pinvU5);
    fftwf_destroy_plan(pinvU6);
    fftwf_destroy_plan(pinvU7);

    fftwf_free(RGB0);
    fftwf_free(u0);
    fftwf_free(u2);
    fftwf_free(u3);
    fftwf_free(u4);
    fftwf_free(u5);
    fftwf_free(u6);
    fftwf_free(u7);

    fftwf_free(iu);

    fftwf_free(U);
    fftwf_free(U2);
    fftwf_free(U3);
    fftwf_free(U4);
    fftwf_free(U5);
    fftwf_free(U6);
    fftwf_free(U7);

    FFTW_MUTEX::fftw_mutex_destroy_plan.unlock();

    ph.setValue(90);

    for (int c = 0; c < 3; c++)
        escala(RGB[c], length, 1.f, 0.f);

    // range between (0,1)
    copy(RGB[0], RGB[0] + length, imR.begin());
    copy(RGB[1], RGB[1] + length, imG.begin());
    copy(RGB[2], RGB[2] + length, imB.begin());

    delete[] RGBorig[0];
    delete[] RGBorig[1];
    delete[] RGBorig[2];
    delete[] RGB[0];
    delete[] RGB[1];
    delete[] RGB[2];
    FFTW_MUTEX::fftw_mutex_free.lock();
    fftwf_free(G);
    FFTW_MUTEX::fftw_mutex_free.unlock();
#ifdef TIMER_PROFILING
    stop_watch.stop_and_update();
    cout << endl;
    cout << "tmo_ferradans11 = " << stop_watch.get_time() << " msec" << endl;
#endif
}
