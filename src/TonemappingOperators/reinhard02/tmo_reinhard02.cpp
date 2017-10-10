/**
 * @file tmo_reinhard02.cpp
 * @brief Tone map luminance channel using Reinhard02 model
 *
 * This file is a part of LuminanceHDR package.
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
 * $Id: tmo_reinhard02.cpp,v 1.3 2008/11/04 23:43:08 rafm Exp $
 */

#include <boost/math/constants/constants.hpp>

#include <stdio.h>
#include <stdlib.h>
#include "arch/math.h"

#include "tmo_reinhard02.h"

#include "Libpfs/array2d.h"
#include "Libpfs/progress.h"
#include "TonemappingOperators/pfstmo.h"

#define SIGMA_I(i) \
    (m_sigma_0 + ((double)i / (double)m_range) * (m_sigma_1 - m_sigma_0))
#define S_I(i) (exp(SIGMA_I(i)))
#define V2(x, y, i) (V1(x, y, i + 1))
#define ACTIVITY(x, y, i)          \
    ((V1(x, y, i) - V2(x, y, i)) / \
     (((m_key * pow(2., m_phi)) / (S_I(i) * S_I(i))) + V1(x, y, i)))

/*
 * Kaiser-Bessel stuff
 */

/*
 * Modified zeroeth order bessel function of the first kind
 */

double Reinhard02::bessel(double x) {
    const double f = 1e-9;
    int n = 1;
    double s = 1.;
    double d = 1.;

    double t;

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

void Reinhard02::compute_bessel() {
    m_bbeta = bessel(boost::math::double_constants::pi * m_alpha);
}

/*
 * Kaiser-Bessel function with window length M and parameter beta = 2.
 * Window length M = min (width, height) / 2
 */

double Reinhard02::kaiserbessel(double x, double y, double M) {
    double d = 1. - ((x * x + y * y) / (M * M));
    if (d <= 0.) return 0.;
    return bessel(boost::math::double_constants::pi * m_alpha * sqrt(d)) /
           m_bbeta;
}

/*
 * Tonemapping routines
 */

double Reinhard02::get_maxvalue() {
    double max = 0.;
    int x, y;

    for (y = 0; y < m_cvts.ymax; y++)
        for (x = 0; x < m_cvts.xmax; x++)
            max = (max < m_image[y][x][0]) ? m_image[y][x][0] : max;
    return max;
}

void Reinhard02::tonemap_image() {
    double Lmax2;
    int x, y;
    int scale, prefscale;

    if (m_white < 1e20)
        Lmax2 = m_white * m_white;
    else {
        if (m_temporal_coherent) {
            m_max_luminance.set(get_maxvalue());
            Lmax2 = m_max_luminance.get();
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

double Reinhard02::log_average() {
    int x, y;
    double sum = 0.;

    for (x = 0; x < m_cvts.xmax; x++)
        for (y = 0; y < m_cvts.ymax; y++)
            sum += log(0.00001 + m_luminance[y][x]);
    return exp(sum / (double)(m_cvts.xmax * m_cvts.ymax));
}

void Reinhard02::scale_to_midtone() {
    int x, y, u, v, d;
    double factor;
    double scale_factor;
    double low_tone = m_key / 3.;
    int border_size = (m_cvts.xmax < m_cvts.ymax) ? int(m_cvts.xmax / 5.)
                                                  : int(m_cvts.ymax / 5.);
    int hw = m_cvts.xmax >> 1;
    int hh = m_cvts.ymax >> 1;

    double avg;
    if (m_temporal_coherent) {
        m_avg_luminance.set(log_average());
        avg = m_avg_luminance.get();
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
void Reinhard02::allocate_memory() {
    int y;

    m_luminance = (double **)malloc(m_cvts.ymax * sizeof(double *));
    m_image = (COLOR **)malloc(m_cvts.ymax * sizeof(COLOR *));
    for (y = 0; y < m_cvts.ymax; y++) {
        m_luminance[y] = (double *)malloc(m_cvts.xmax * sizeof(double));
        m_image[y] = (COLOR *)malloc(m_cvts.xmax * sizeof(COLOR));
    }
}

void Reinhard02::deallocate_memory() {
    int y;

    for (y = 0; y < m_cvts.ymax; y++) {
        free(m_luminance[y]);
        free(m_image[y]);
    }
    free(m_luminance);
    free(m_image);
}

void Reinhard02::dynamic_range() {
    int x, y;
    double minval = 1e20;
    double maxval = -1e20;

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
Reinhard02::Reinhard02(const pfs::Array2Df *Y, pfs::Array2Df *L,
                       bool use_scales, float key, float phi, int num, int low,
                       int high, bool temporal_coherent, pfs::Progress &ph)
    : m_width(Y->getCols()),
      m_height(Y->getRows()),
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
      m_threshold(0.05),
      m_ph(ph) {
    m_Y = Y;
    m_L = L;
}

void Reinhard02::tmo_reinhard02() {
    m_ph.setValue(0);

    int x, y;

    m_cvts.xmax = m_Y->getCols();
    m_cvts.ymax = m_Y->getRows();

    m_sigma_0 = log((double)m_scale_low);
    m_sigma_1 = log((double)m_scale_high);

    compute_bessel();
    allocate_memory();

    m_ph.setValue(10);
    if (m_ph.canceled()) goto end;

    // reading image
    for (y = 0; y < m_cvts.ymax; y++)
        for (x = 0; x < m_cvts.xmax; x++) m_image[y][x][0] = (*m_Y)(x, y);

    copy_luminance();
    m_ph.setValue(20);
    if (m_ph.canceled()) goto end;
    scale_to_midtone();
    m_ph.setValue(30);
    if (m_ph.canceled()) goto end;

    if (m_use_scales) {
        build_pyramid(m_luminance, m_cvts.xmax, m_cvts.ymax);
    }
    m_ph.setValue(50);
    if (m_ph.canceled()) goto end;

    tonemap_image();

    m_ph.setValue(85);
    if (m_ph.canceled()) goto end;
    // saving image
    for (y = 0; y < m_cvts.ymax; y++)
        for (x = 0; x < m_cvts.xmax; x++) (*m_L)(x, y) = m_image[y][x][0];

    //  print_parameter_settings();

    m_ph.setValue(95);
    if (m_ph.canceled()) goto end;
    deallocate_memory();
    if (m_use_scales) {
        clean_pyramid();
    }

    if (!m_ph.canceled()) m_ph.setValue(100);

end:;
}

double Reinhard02::V1(int x, int y, int level)
/* PRE:  */
{
    int x0, y0;
    int l, size;
    double s, t;

    /* Level 0 is a special case, the value is just the image */
    if (level == 0) return (m_luminance[y][x]);

    /* Compute the size of the slice */
    l = 1 << level;
    x0 = x >> level;
    y0 = y >> level;
    size = PyramidWidth0 >> (level - 1);

    x0 = (x0 >= size ? size - 1 : x0);
    y0 = (y0 >= size ? size - 1 : y0);

    s = (double)(x - x0 * l) / (double)l;
    t = (double)(y - y0 * l) / (double)l;

    level--;

    //!! FIX: a quick fix for boundary conditions
    int x01, y01;
    x01 = (x0 == size - 1 ? x0 : x0 + 1);
    y01 = (y0 == size - 1 ? y0 : y0 + 1);

    return ((1 - s) * (1 - t) * Pyramid[level][y0][x0] +
            s * (1 - t) * Pyramid[level][y0][x01] +
            (1 - s) * t * Pyramid[level][y01][x0] +
            s * t * Pyramid[level][y01][x01]);
}

/* PRE:  */
double Reinhard02::pyramid_lookup(unsigned int x, unsigned int y, int level) {
    // int n, s;

    /* Level 0 is a special case, the value is just the image */
    if (level == 0) {
        if ((x >= m_width) || (y >= m_height))
            return (0.0);
        else
            return (m_luminance[y][x]);
    }

    /* Compute the size of the slice */
    level--;
    // n = 1 << level;
    unsigned int s = PyramidWidth0 >> level;

    // x = x >> level;
    // y = y >> level;

    if ((x >= s) || (y >= s))
        return (0.0);
    else
        return (Pyramid[level][y][x]);
}

void Reinhard02::build_pyramid(double ** /*luminance*/, int image_width,
                               int image_height) {
    int k;
    int x, y;
    int i, j;
    int width;
    int max_dim;
    //    int height, pyramid_height;

    double sum = 0;

    double a = 0.4;
    double b = 0.25;
    double c = b - a / 2;
    double w[5];

    /* Compute the "filter kernel" */
    w[0] = c;
    w[1] = b;
    w[2] = a;
    w[3] = w[1];
    w[4] = w[0];

    /* Build the pyramid slices.  The bottom of the pyramid is the luminace  */
    /* image, and is not in the Pyramid array.                               */
    /* For simplicity, the first level is padded to a square whose side is a */
    /* power of two.                                                         */

    /* Compute the size of the Pyramid array */
    max_dim = (m_height > m_width ? m_height : m_width);
    PyramidHeight = (int)floor(log(max_dim - 0.5) / log(2.0f)) + 1;

    /* Compute the dimensions of the first level */
    width = 1 << (PyramidHeight - 1);
    PyramidWidth0 = width;

    //  fprintf(stderr, "max_dim %d   height %d\n", max_dim, PyramidHeight);

    /* Allocate the outer Pyramid array */
    Pyramid = (double ***)calloc(PyramidHeight, sizeof(double **));
    if (!Pyramid) {
        fprintf(stderr, "Unable to allocate pyramid array.\n");
        exit(1);
    }

    /* Allocate and assign the Pyramid slices */
    k = 0;

    while (width) {
        //    fprintf(stderr, "level %d, width = %d\n", k, width);

        /* Allocate the slice */
        Pyramid[k] = (double **)calloc(width, sizeof(double *));
        if (!Pyramid[k]) {
            fprintf(stderr, "Unable to allocate pyramid array.\n");
            exit(1);
        }
        for (y = 0; y < width; y++) {
            Pyramid[k][y] = (double *)calloc(width, sizeof(double));
            if (!Pyramid[k][y]) {
                fprintf(stderr, "Unable to allocate pyramid array.\n");
                exit(1);
            }
        }

        /* Compute the values in the slice */
        for (y = 0; y < width; y++) {
            for (x = 0; x < width; x++) {
                sum = 0;
                for (i = 0; i < 5; i++) {
                    for (j = 0; j < 5; j++) {
                        sum += w[i] * w[j] *
                               pyramid_lookup(2 * x + i - 2, 2 * y + j - 2, k);
                    }
                }
                Pyramid[k][y][x] = sum;
            }
        }

        /* compute the width of the next slice */
        width /= 2;
        k++;
    }
}

void Reinhard02::clean_pyramid() {
    int k = 0;
    int width = PyramidWidth0;
    while (width) {
        for (int y = 0; y < width; y++) free(Pyramid[k][y]);
        free(Pyramid[k]);
        k++;
        width /= 2;
    }
    free(Pyramid);
}
