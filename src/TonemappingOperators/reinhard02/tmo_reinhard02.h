/**
 * @file tmo_reinhard02.h
 * @brief Tone map luminance channel using Reinhard02 model
 *
 * @author Grzegorz Krawczyk, <krawczyk@mpi-sb.mpg.de>
 *
 * $Id: tmo_reinhard02.h,v 1.2 2008/09/04 12:46:49 julians37 Exp $
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

#ifndef TMO_REINHARD02_H
#define TMO_REINHARD02_H

#include <fftw3.h>
#include <boost/thread/mutex.hpp>

#include <Libpfs/array2d_fwd.h>

namespace pfs {
class Progress;
}

//--- from defines.h
typedef struct { int xmax, ymax; /* image dimensions */ } CVTS;

typedef float COLOR[3]; /* red, green, blue (or X,Y,Z) */

//--- end of defines.h

/*
 * @brief Photographic tone-reproduction
 *
 * @param width image width
 * @param height image height
 * @param Y input luminance
 * @param L output tonemapped intensities
 * @param use_scales true: local version, false: global version of TMO
 * @param key maps log average luminance to this value (default: 0.18)
 * @param phi sharpening parameter (defaults to 1 - no sharpening)
 * @param num number of scales to use in computation (default: 8)
 * @param low size in pixels of smallest scale (should be kept at 1)
 * @param high size in pixels of largest scale (default 1.6^8 = 43)
 */
class Reinhard02 {
   public:
    Reinhard02(const pfs::Array2Df *Y, pfs::Array2Df *L, bool use_scales,
               float key, float phi, int num, int low, int high,
               bool temporal_coherent, pfs::Progress &ph);

    ~Reinhard02();

    void tmo_reinhard02();

   private:
    CVTS m_cvts;
    COLOR **m_image;
    float m_sigma_0, m_sigma_1;
    float **m_luminance;
    int m_width, m_height;
    const pfs::Array2Df *m_Y;
    pfs::Array2Df *m_L;
    bool m_use_scales;
    bool m_use_border;
    float m_key, m_phi, m_white;
    int m_range, m_scale_low, m_scale_high;
    const float m_alpha;
    float m_bbeta;
    float m_threshold;
    float m_k;
    pfs::Progress &m_ph;

    fftwf_complex **m_filter_fft;
    fftwf_complex *m_image_fft;
    fftwf_complex *m_convolution_fft;
    float ***m_convolved_image;

    float bessel(float);
    float kaiserbessel(float, float, float);
    float get_maxvalue();
    void tonemap_image();
    float log_average();
    void scale_to_midtone();
    void copy_luminance();
    void gaussian_filter(fftwf_complex *, float, float);
    void build_gaussian_fft();
    void build_image_fft();
    void convolve_filter(int, fftwf_complex *);
    void compute_fourier_convolution();
};
#endif // TMO_REINHARD02_H
