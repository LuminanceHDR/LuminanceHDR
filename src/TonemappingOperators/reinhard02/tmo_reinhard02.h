/**
 * @file tmo_reinhard02.h
 * @brief Tone map luminance channel using Reinhard02 model
 *
 * @author Grzegorz Krawczyk, <krawczyk@mpi-sb.mpg.de>
 *
 * $Id: tmo_reinhard02.h,v 1.2 2008/09/04 12:46:49 julians37 Exp $
 */

#ifndef _tmo_reinhard02_h_
#define _tmo_reinhard02_h_

#include <fftw3.h>
#include <boost/thread/mutex.hpp>

#include <Libpfs/array2d_fwd.h>

namespace pfs {
class Progress;
}

/**
 * Used to achieve temporal coherence
 */
template <class T>
class TemporalSmoothVariable {
    //  const int hist_length = 100;
    T value;

    T getThreshold(T luminance) { return 0.01 * luminance; }

   public:
    TemporalSmoothVariable() : value(-1) {}

    void set(T new_value) {
        if (value == -1)
            value = new_value;
        else {
            T delta = new_value - value;
            const T threshold = getThreshold((new_value + value) / 2);
            if (delta > threshold)
                delta = threshold;
            else if (delta < -threshold)
                delta = -threshold;
            value += delta;
        }
    }

    T get() const { return value; }
};

static TemporalSmoothVariable<float> avg_luminance, max_luminance;

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
    TemporalSmoothVariable<float> m_avg_luminance, m_max_luminance;
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
    bool m_temporal_coherent;
    const float m_alpha;
    float m_bbeta;
    float m_threshold;
    pfs::Progress &m_ph;

    float ***Pyramid;
    int PyramidHeight;
    int PyramidWidth0;

    float m_k;
    fftwf_complex **m_filter_fft;
    fftwf_complex *m_image_fft;
    float ***m_convolved_image;

    static boost::mutex sm_mutex;

    float bessel(float);
    float kaiserbessel(float, float, float);
    float get_maxvalue();
    void tonemap_image();
    void clamp_image();
    float log_average();
    void scale_to_midtone();
    void copy_luminance();
    void dynamic_range();
    void compute_fft(fftwf_complex *, int, int);
    void compute_inverse_fft(fftwf_complex *, int, int);
    void gaussian_filter(fftwf_complex *, float, float);
    void build_gaussian_fft();
    void build_image_fft();
    void convolve_filter(int, fftwf_complex *);
    void compute_fourier_convolution();
};

#endif /* _tmo_reinhard02_h_ */
