/**
 * @file tmo_reinhard02.cpp
 * @brief Tone map luminance channel using Reinhard02 model
 *
 * Implementation courtesy of Erik Reinhard.
 *
 * Original source code note:
 * Tonemap.c  University of Utah / Erik Reinhard / October 2001
 *
 * File taken from:
 * http://www.cs.utah.edu/~reinhard/cdrom/source.html
 *
 * Port to PFS tools library by Grzegorz Krawczyk <krawczyk@mpi-sb.mpg.de>
 *
 * $Id: tmo_reinhard02.cpp,v 1.4 2009/04/15 11:49:32 julians37 Exp $
 */

#include <boost/math/constants/constants.hpp>

#include <stdio.h>
#include <stdlib.h>
#include "arch/math.h"

#include "tmo_reinhard02.h"

#include <Common/init_fftw.h>
#include "Libpfs/array2d.h"
#include "Libpfs/array2d_fwd.h"
#include "Libpfs/progress.h"
#include "TonemappingOperators/pfstmo.h"

#define HAVE_ZFFT
// if no zfft library compile approximate sollution
#undef APPROXIMATE

/*
static int       width, height, scale;
static COLOR   **image;
static float ***convolved_image;
static float    sigma_0, sigma_1;
float         **luminance;

static float    k                = 1. / (2. * 1.4142136);
static float    key              = 0.18;
static float    threshold        = 0.05;
static float    phi              = 8.;
static float    white            = 1e20;
static int       scale_low        = 1;
static int       scale_high       = 43;  // 1.6^8 = 43
static int       range            = 8;
static int       use_scales       = 0;
static int       use_border       = 0;
static CVTS      cvts             = {0, 0};

static bool temporal_coherent;
*/

#define V1(x, y, i) (m_convolved_image[i][x][y])

#define SIGMA_I(i) \
    (m_sigma_0 + ((float)i / (float)m_range) * (m_sigma_1 - m_sigma_0))
#define S_I(i) (exp(SIGMA_I(i)))
#define V2(x, y, i) (V1(x, y, i + 1))
#define ACTIVITY(x, y, i)          \
    ((V1(x, y, i) - V2(x, y, i)) / \
     (((m_key * pow(2., m_phi)) / (S_I(i) * S_I(i))) + V1(x, y, i)))

/*
 * Kaiser-Bessel stuff

static float   alpha = 2.;         // Kaiser-Bessel window parameter
static float   bbeta;              // Will contain bessel (PI * alpha)
*/

/*
 * Modified zeroeth order bessel function of the first kind
 */

float Reinhard02::bessel(float x) {
    const float f = 1e-9;
    int n = 1;
    float s = 1.;
    float d = 1.;

    float t;

    while (d > f * s) {
        t = x / (2. * n);
        n++;
        d *= t * t;
        s += d;
    }
    return s;
}

/*
 * Initialiser for Kaiser-Bessel computations
 */

/*
 * Kaiser-Bessel function with window length M and parameter beta = 2.
 * Window length M = min (width, height) / 2
 */

float Reinhard02::kaiserbessel(float x, float y, float M) {
    float d = 1. - ((x * x + y * y) / (M * M));
    if (d <= 0.) return 0.;
    return bessel(boost::math::float_constants::pi * m_alpha * sqrt(d)) /
           m_bbeta;
}

/*
 * FFT functions
 */

void Reinhard02::compute_fft(fftwf_complex *array, int width, int height) {
    fftwf_plan p;
    p = fftwf_plan_dft_2d(height, width, array, array, -1,
        FFTW_ESTIMATE);

    fftwf_execute(p);

    fftwf_destroy_plan(p);
}

void Reinhard02::compute_inverse_fft(fftwf_complex *array, int width, int height) {
    fftwf_plan p;
    p = fftwf_plan_dft_2d(height, width, array, array, 1,
        FFTW_ESTIMATE);

    fftwf_execute(p);

    fftwf_destroy_plan(p);
}

// Compute Gaussian blurred images
void Reinhard02::gaussian_filter(fftwf_complex *filter, float scale, float k) {
    int x, y;
    float x1, y1, s;
    float a = 1. / (k * scale);
    float c = 1. / 4.;

    for (y = 0; y < m_cvts.ymax; y++) {
        y1 = (y >= m_cvts.ymax / 2) ? y - m_cvts.ymax : y;
        s = erf(a * (y1 - .5)) - erf(a * (y1 + .5));
        for (x = 0; x < m_cvts.xmax; x++) {
            x1 = (x >= m_cvts.xmax / 2) ? x - m_cvts.xmax : x;
            filter[y * m_cvts.xmax + x][0] =
                s * (erf(a * (x1 - .5)) - erf(a * (x1 + .5))) * c;
            filter[y * m_cvts.xmax + x][1] = 0.;
        }
    }
}

void Reinhard02::build_gaussian_fft() {
    int i;
    int length = m_cvts.xmax * m_cvts.ymax;
    float fft_scale = 1. / sqrt((float)length);
    m_filter_fft = (fftwf_complex **)fftwf_alloc_complex(m_range);

    for (int scale = 0; scale < m_range; scale++) {
#ifndef NDEBUG
        fprintf(stderr,
                "Computing FFT of Gaussian at scale %i (size %i x %i)%c", scale,
                m_cvts.xmax, m_cvts.ymax, (char)13);
#endif
        m_filter_fft[scale] = (fftwf_complex *)fftwf_alloc_complex(length);

        gaussian_filter(m_filter_fft[scale], S_I(scale), m_k);

        //compute_fft(m_filter_fft[scale], m_cvts.xmax, m_cvts.ymax);
        fftwf_plan p;
        p = fftwf_plan_dft_2d(m_cvts.ymax, m_cvts.xmax, m_filter_fft[scale], m_filter_fft[scale], -1,
        FFTW_ESTIMATE);

        fftwf_execute(p);

        fftwf_destroy_plan(p);

        for (i = 0; i < length; i++) {
            m_filter_fft[scale][i][0] *= fft_scale;
            m_filter_fft[scale][i][1] *= fft_scale;
        }
    }
#ifndef NDEBUG
    fprintf(stderr, "\n");
#endif
}

void Reinhard02::build_image_fft() {
    int i, x, y;
    int length = m_cvts.xmax * m_cvts.ymax;
    float fft_scale = 1. / sqrt((float)length);

#ifndef NDEBUG
    fprintf(stderr, "Computing image FFT\n");
#endif
    m_image_fft = (fftwf_complex *)fftwf_alloc_complex(length);

    for (y = 0; y < m_cvts.ymax; y++)
        for (x = 0; x < m_cvts.xmax; x++)
            m_image_fft[y * m_cvts.xmax + x][0] = m_luminance[y][x];

    //compute_fft(m_image_fft, m_cvts.xmax, m_cvts.ymax);
    fftwf_plan p;
    p = fftwf_plan_dft_2d(m_cvts.ymax, m_cvts.xmax, m_image_fft, m_image_fft, -1,
        FFTW_ESTIMATE);

    fftwf_execute(p);

    fftwf_destroy_plan(p);
    for (i = 0; i < length; i++) {
        m_image_fft[i][0] *= fft_scale;
        m_image_fft[i][1] *= fft_scale;
    }
}

void Reinhard02::convolve_filter(int scale, fftwf_complex *convolution_fft) {
    int i, x, y;

    for (i = 0; i < m_cvts.xmax * m_cvts.ymax; i++) {
        convolution_fft[i][0] = m_image_fft[i][0] * m_filter_fft[scale][i][0] +
                                m_image_fft[i][1] * m_filter_fft[scale][i][1];
        convolution_fft[i][1] = m_image_fft[i][0] * m_filter_fft[scale][i][1] +
                                m_image_fft[i][1] * m_filter_fft[scale][i][0];
    }
    //compute_inverse_fft(convolution_fft, m_cvts.xmax, m_cvts.ymax);
    fftwf_plan p;
    p = fftwf_plan_dft_2d(m_cvts.ymax, m_cvts.xmax, convolution_fft, convolution_fft, 1,
        FFTW_ESTIMATE);

    fftwf_execute(p);

    fftwf_destroy_plan(p);
    i = 0;
    for (y = 0; y < m_cvts.ymax; y++)
        for (x = 0; x < m_cvts.xmax; x++)
            m_convolved_image[scale][x][y] = convolution_fft[i++][0];
}

void Reinhard02::compute_fourier_convolution() {
    // activate parallel execution of fft routines
    init_fftw();
    int x;
    fftwf_complex *convolution_fft;

    //initialise_fft(m_cvts.xmax, m_cvts.ymax);
    build_image_fft();
    build_gaussian_fft();
    m_convolved_image = (float ***)malloc(m_range * sizeof(float **));

    convolution_fft = (fftwf_complex *)fftwf_alloc_complex(m_cvts.xmax * m_cvts.ymax);
    for (int scale = 0; scale < m_range; scale++) {
#ifndef NDEBUG
        fprintf(stderr, "Computing convolved image at scale %i%c", scale,
                (char)13);
#endif
        m_convolved_image[scale] =
            (float **)malloc(m_cvts.xmax * sizeof(float *));
        for (x = 0; x < m_cvts.xmax; x++)
            m_convolved_image[scale][x] =
                (float *)malloc(m_cvts.ymax * sizeof(float));
        convolve_filter(scale, convolution_fft);
        fftwf_free(m_filter_fft[scale]);
    }
#ifndef NDEBUG
    fprintf(stderr, "\n");
#endif
    fftwf_free(convolution_fft);
    fftwf_free(m_image_fft);
}

/*
 * Tonemapping routines
 */

float Reinhard02::get_maxvalue() {
    float max = 0.;
    int x, y;

    for (y = 0; y < m_cvts.ymax; y++)
        for (x = 0; x < m_cvts.xmax; x++)
            max = (max < m_image[y][x][0]) ? m_image[y][x][0] : max;
    return max;
}

void Reinhard02::tonemap_image() {
    float Lmax2;
    int x, y;
    int scale, prefscale;

    if (m_white < 1e20)
        Lmax2 = m_white * m_white;
    else {
        if (m_temporal_coherent) {
            max_luminance.set(get_maxvalue());
            Lmax2 = max_luminance.get();
        } else
            Lmax2 = get_maxvalue();
        Lmax2 *= Lmax2;
    }

    for (y = 0; y < m_cvts.ymax; y++)
        for (x = 0; x < m_cvts.xmax; x++) {
            if (m_use_scales) {
                prefscale = m_range - 1;
                for (scale = 0; scale < m_range - 1; scale++)
                    if (scale >= PyramidHeight ||
                        fabs(ACTIVITY(x, y, scale)) > m_threshold) {
                        prefscale = scale;
                        break;
                    }
                m_image[y][x][0] /= 1. + V1(x, y, prefscale);
            } else
                m_image[y][x][0] = m_image[y][x][0] *
                                   (1. + (m_image[y][x][0] / Lmax2)) /
                                   (1. + m_image[y][x][0]);
            // image[y][x][0] /= (1. + image[y][x][0]);
        }
}

/*
 * Miscellaneous functions
 */

void Reinhard02::clamp_image() {
    int x, y;

    for (y = 0; y < m_cvts.ymax; y++)
        for (x = 0; x < m_cvts.xmax; x++) {
            m_image[y][x][0] = (m_image[y][x][0] > 1.) ? 1. : m_image[y][x][0];
            m_image[y][x][1] = (m_image[y][x][1] > 1.) ? 1. : m_image[y][x][1];
            m_image[y][x][2] = (m_image[y][x][2] > 1.) ? 1. : m_image[y][x][2];
        }
}

float Reinhard02::log_average() {
    int x, y;
    float sum = 0.;

    for (y = 0; y < m_cvts.ymax; y++)
        for (x = 0; x < m_cvts.xmax; x++)
            sum += log(0.00001 + m_luminance[y][x]);
    return exp(sum / (float)(m_cvts.xmax * m_cvts.ymax));
}

void Reinhard02::scale_to_midtone() {
    int x, y, u, v, d;
    float factor;
    float scale_factor;
    float low_tone = m_key / 3.;
    int border_size = (m_cvts.xmax < m_cvts.ymax) ? int(m_cvts.xmax / 5.)
                                                  : int(m_cvts.ymax / 5.);
    int hw = m_cvts.xmax >> 1;
    int hh = m_cvts.ymax >> 1;

    float avg;
    if (m_temporal_coherent) {
        avg_luminance.set(log_average());
        avg = avg_luminance.get();
    } else
        avg = log_average();

    scale_factor = 1.0 / avg;
    for (y = 0; y < m_cvts.ymax; y++)
        for (x = 0; x < m_cvts.xmax; x++) {
            if (m_use_border) {
                u = (x > hw) ? m_cvts.xmax - x : x;
                v = (y > hh) ? m_cvts.ymax - y : y;
                d = (u < v) ? u : v;
                factor =
                    (d < border_size)
                        ? (m_key - low_tone) * kaiserbessel(border_size - d, 0,
                                                            border_size) +
                              low_tone
                        : m_key;
            } else
                factor = m_key;
            m_image[y][x][0] *= scale_factor * factor;
            m_luminance[y][x] *= scale_factor * factor;
        }
}

void Reinhard02::copy_luminance() {
    int x, y;

    for (x = 0; x < m_cvts.xmax; x++)
        for (y = 0; y < m_cvts.ymax; y++) m_luminance[y][x] = m_image[y][x][0];
}

/*
 * Memory allocation
 */

void Reinhard02::dynamic_range() {
    int x, y;
    float minval = 1e20;
    float maxval = -1e20;

    for (x = 0; x < m_cvts.xmax; x++)
        for (y = 0; y < m_cvts.ymax; y++) {
            if ((m_luminance[y][x] < minval) && (m_luminance[y][x] > 0.0))
                minval = m_luminance[y][x];
            if (m_luminance[y][x] > maxval) maxval = m_luminance[y][x];
        }
}

/*
 * @brief Photographic tone-reproduction
 *
 * @param Y input luminance
 * @param L output tonemapped intensities
 * @param use_scales true: local version, false: global version of TMO
 * @param key maps log average luminance to this value (default: 0.18)
 * @param phi sharpening parameter (defaults to 1 - no sharpening)
 * @param num number of scales to use in computation (default: 8)
 * @param low size in pixels of smallest scale (should be kept at 1)
 * @param high size in pixels of largest scale (default 1.6^8 = 43)
 */

boost::mutex Reinhard02::sm_mutex;

Reinhard02::Reinhard02(const pfs::Array2Df *Y, pfs::Array2Df *L,
                       bool use_scales, float key, float phi, int num, int low,
                       int high, bool temporal_coherent, pfs::Progress &ph)
    : m_cvts(CVTS()),
      m_sigma_0(0),
      m_sigma_1(0),
      m_luminance(0),
      m_width(Y->getCols()),
      m_height(Y->getRows()),
      m_Y(Y),
      m_L(L),
      m_use_scales(use_scales),
      m_use_border(false),
      m_key(key),
      m_phi(phi),
      m_white(1e20),
      m_range(num),
      m_scale_low(low),
      m_scale_high(high),
      m_temporal_coherent(temporal_coherent),
      m_alpha(2.),
      m_bbeta(0),
      m_threshold(0.05),
      m_ph(ph),
      Pyramid(0),
      PyramidHeight(0),
      PyramidWidth0(0),
      m_k(1. / (2. * 1.4142136))
{

    m_cvts.xmax = m_Y->getCols();
    m_cvts.ymax = m_Y->getRows();

    m_sigma_0 = log(m_scale_low);
    m_sigma_1 = log(m_scale_high);

    m_bbeta = bessel(boost::math::float_constants::pi * m_alpha);

    int y;
    m_luminance = (float **)malloc(m_cvts.ymax * sizeof(float *));
    m_image = (COLOR **)malloc(m_cvts.ymax * sizeof(COLOR *));
    for (y = 0; y < m_cvts.ymax; y++) {
        m_luminance[y] = (float *)malloc(m_cvts.xmax * sizeof(float));
        m_image[y] = (COLOR *)malloc(m_cvts.xmax * sizeof(COLOR));
    }

}

Reinhard02::~Reinhard02() {
    int y;

    for (y = 0; y < m_cvts.ymax; y++) {
        free(m_luminance[y]);
        free(m_image[y]);
    }
    free(m_luminance);
    free(m_image);
}

void Reinhard02::tmo_reinhard02() {
    m_ph.setValue(0);

    int x, y;
    // reading image
    for (y = 0; y < m_cvts.ymax; y++)
        for (x = 0; x < m_cvts.xmax; x++) m_image[y][x][0] = (*m_Y)(x, y);

    m_ph.setValue(10);
    if (m_ph.canceled()) goto end;

    copy_luminance();

    m_ph.setValue(20);
    if (m_ph.canceled()) goto end;

    scale_to_midtone();

    m_ph.setValue(30);
    if (m_ph.canceled()) goto end;

    if (m_use_scales) {
        sm_mutex.lock();
        compute_fourier_convolution();
        sm_mutex.unlock();
    }

    m_ph.setValue(50);
    if (m_ph.canceled()) goto end;

    tonemap_image();

    m_ph.setValue(85);
    if (m_ph.canceled()) goto end;

    // saving image
    for (y = 0; y < m_cvts.ymax; y++)
        for (x = 0; x < m_cvts.xmax; x++) (*m_L)(x, y) = m_image[y][x][0];

    if (!m_ph.canceled()) m_ph.setValue(100);

end:;
}
