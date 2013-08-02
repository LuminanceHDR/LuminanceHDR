/**
 * This file is a part of Luminance HDR package
 * ---------------------------------------------------------------------- 
 * Copyright (C) 2013 Franco Comida
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
 * Manual and auto antighosting functions
 * @author Franco Comida <fcomida@users.sourceforge.net>
 *
 */

#include <QDebug>

#include <Libpfs/frame.h>
#include <Libpfs/colorspace/colorspace.h>
#include <Libpfs/manip/copy.h>
#include <Libpfs/utils/minmax.h>

#include <fftw3.h>

#include "AutoAntighosting.h"
// --- LEGACY CODE ---

inline
void rgb2hsl(float r, float g, float b, float& h, float& s, float& l)
{
    float v, m, vm, r2, g2, b2;
    h = 0.0f;
    s = 0.0f;
    l = 0.0f;

    pfs::utils::minmax(r, g, b, m, v);

    l = (m + v) / 2.0f;
    if (l <= 0.0f)
        return;
    vm = v - m;
    s = vm;
    //if (s >= 0.0f)
    if (s > 0.0f)
        s /= (l <= 0.5f) ? (v + m) : (2.0f - v - m);
    else return;
    r2 = (v - r) / vm;
    g2 = (v - g) / vm;
    b2 = (v - b) / vm;
    if (r == v)
        h = (g == m ? 5.0f + b2 : 1.0f - g2);
    else if (g == v)
        h = (b == m ? 1.0f + r2 : 3.0f - b2);
    else
        h = (r == m ? 3.0f + g2 : 5.0f - r2);
    h /= 6.0f;
}

inline
void hsl2rgb(float h, float sl, float l, float& r, float& g, float& b)
{
    float v;
    r = l;
    g = l;
    b = l;
    v = (l <= 0.5f) ? (l * (1.0f + sl)) : (l + sl - l * sl);
    if (v > 0.0f)
    {
        float m;
        float sv;
        int sextant;
        float fract, vsf, mid1, mid2;
        m = l + l - v;
        sv = (v - m ) / v;
        h *= 6.0f;
        sextant = (int)h;
        fract = h - sextant;
        vsf = v * sv * fract;
        mid1 = m + vsf;
        mid2 = v - vsf;
        switch (sextant)
        {
        case 0:
            r = v;
            g = mid1;
            b = m;
            break;
        case 1:
            r = mid2;
            g = v;
            b = m;
            break;
        case 2:
            r = m;
            g = v;
            b = mid1;
            break;
        case 3:
            r = m;
            g = mid2;
            b = v;
            break;
        case 4:
            r = mid1;
            g = m;
            b = v;
            break;
        case 5:
            r = v;
            g = m;
            b = mid2;
            break;
        }
    }
}

float max(const Array2Df *u)
{
  const int nx = u->getCols();
  const int ny = u->getRows();

  return *std::max_element(u->data(), u->data() + nx*ny);
}

float min(const Array2Df *u)
{
  const int nx = u->getCols();
  const int ny = u->getRows();

  return *std::min_element(u->data(), u->data() + nx*ny);
}

void solve_pde_dct(Array2Df &F, Array2Df &U)
{
#ifdef TIMER_PROFILING
    msec_timer stop_watch;
    stop_watch.start();
#endif
    const int width = U.getCols();
    const int height = U.getRows();
    assert((int)F.getCols()==width && (int)F.getRows()==height);

    Array2Df Ftr(width, height);

    fftwf_plan p;
    #pragma omp parallel for private(p) schedule(static)
    for ( int j = 0; j < height; j++ ) {
        #pragma omp critical (make_plan)
        p = fftwf_plan_r2r_1d(width, F.data()+width*j, Ftr.data()+width*j, FFTW_REDFT00, FFTW_ESTIMATE);
        fftwf_execute(p); 
    }
    
  #pragma omp parallel 
  {
    vector<float> c(height);
    #pragma omp for
    for ( int i = 0; i < width; i++ ) {
        for (int j = 0; j < height; j++) {
            c[j] = 1.0f; 
        }
        float b = 2.0f*(cos(M_PI*i/width) - 2.0f); 
        c[0] /= b;
        Ftr(i, 0) /= b;
        for (int j = 1; j < height - 1; j++ ) {
            float m = (b - c[j-1]);
            c[j] /= m;
            Ftr(i, j) = (Ftr(i, j) - Ftr(i, j-1))/m;   
        }
        Ftr(i, height - 1) = (Ftr(i, height - 1) - Ftr(i, height - 2))/(b - c[height - 2]);
        U(i, height - 1) = Ftr(i, height - 1);
        for (int j = height - 2; j >= 0; j--) {
            U(i, j) = Ftr(i, j) - c[j]*U(i, j+1);
        }
    }
  }

    #pragma omp parallel for private(p) schedule(static)
    for ( int j = 0; j < height; j++ ) {
        #pragma omp critical (make_plan)
        p = fftwf_plan_r2r_1d(width, U.data()+width*j, U.data()+width*j, FFTW_REDFT00, FFTW_ESTIMATE);
        fftwf_execute(p); 
        for ( int i = 0; i < width; i++ ) {
            U(i, j) /= (2.0f*(width-1));
        }
    }

    fftwf_destroy_plan(p);
#ifdef TIMER_PROFILING
    stop_watch.stop_and_update();
    std::cout << "solve_pde_dct = " << stop_watch.get_time() << " msec" << std::endl;
#endif
}

void clampToZero(Array2Df &R, Array2Df &G, Array2Df &B, float m)
{
  const int width = R.getCols();
  const int height = R.getRows();
  
  #pragma omp parallel for
  for ( int i = 0; i < width*height; i++ )
  {
    R(i) -= m;
    G(i) -= m;
    B(i) -= m;
  }
}

int findIndex(const float* data, int size)
{
    assert(size > 0);

    int idx = 0;
    float currentMax = data[0];
    for (int i = 0; i < size; i++) {
        if ( data[i] > currentMax ) {
            currentMax = data[i];
            idx = i;
        }
    }
    return idx;
}

typedef vector<float> BufferF;

inline
float hueMean(const BufferF& data)
{
    return std::accumulate(data.begin(), data.end(), 0.0f)/data.size();
}

void hueSquaredMean(const HdrCreationItemContainer& data,
                    vector<float>& HE)
{
    size_t width = data[0].frame()->getWidth();
    size_t height = data[0].frame()->getHeight();
    size_t numItems = data.size();

    float r, g, b, h, s, l;

    BufferF hues(numItems, 0.f);
    BufferF HS(numItems, 0.f);

    const Channel *X, *Y, *Z;

    for (size_t j = 0; j < height; j++) {
        for (size_t i = 0; i < width; i++) {
            for (size_t w = 0; w < numItems; w++) {
                data[w].frame()->getXYZChannels( X, Y, Z );

                r = (*X)(i, j);
                g = (*Y)(i, j);
                b = (*Z)(i, j);
                rgb2hsl(r, g, b, h, s, l);
                hues[w] = h;
            }

            float hueMean_ = hueMean(hues);

            for (size_t w = 0; w < numItems; w++) {
                float H = hueMean_ - hues[w];
                HS[w] += H*H;
            }
        }
    }

    for (size_t w = 0; w < numItems; w++) {
        HE[w] = HS[w] / (width*height);

        qDebug() << "HE[" << w << "]: " << HE[w];
    }
}

bool comparePatches(const HdrCreationItem& item1,
                    const HdrCreationItem& item2,
                    int i, int j, int gridX, int gridY, float threshold, float deltaEV, int dx, int dy)
{
    vector<float> logRed(gridX*gridY);
    vector<float> logGreen(gridX*gridY);
    vector<float> logBlue(gridX*gridY);

    Channel *X1, *Y1, *Z1, *X2, *Y2, *Z2;
    item1.frame()->getXYZChannels( X1, Y1, Z1 );
    item2.frame()->getXYZChannels( X2, Y2, Z2 );
    Array2Df& R1 = *X1;
    Array2Df& G1 = *Y1;
    Array2Df& B1 = *Z1;
    Array2Df& R2 = *X2;
    Array2Df& G2 = *Y2;
    Array2Df& B2 = *Z2;
    
    int width = gridX*agGridSize;
    int height = gridY*agGridSize;
    int count = 0;
    for (int y = j * gridY; y < (j+1) * gridY; y++) {
        if (y+dy < 0 || y+dy > height-1)
            continue;
        for (int x = i * gridX; x < (i+1) * gridX; x++) {
            if (x+dx < 0 || x+dx > width-1)
                continue;
            if (deltaEV < 0) {
                logRed[count] = logf(R1(x, y)) - logf(R2(x+dx, y+dy)) - deltaEV;
                logGreen[count] = logf(G1(x, y)) - logf(G2(x+dx, y+dy)) - deltaEV;
                logBlue[count++] = logf(B1(x, y)) - logf(B2(x+dx, y+dy)) - deltaEV;
            }
            else {
                logRed[count] = logf(R2(x, y)) - logf(R1(x+dx, y+dy)) + deltaEV;
                logGreen[count] = logf(G2(x, y)) - logf(G1(x+dx, y+dy)) + deltaEV;
                logBlue[count++] = logf(B2(x, y)) - logf(B1(x+dx, y+dy)) + deltaEV;
            }
        }
    }
  
    float threshold1 = 0.7f * std::abs(deltaEV);
    count = 0;
    for (int h = 0; h < gridX*gridY; h++) {
        if (std::abs(logRed[h]) > threshold1 && std::abs(logGreen[h]) > threshold1 && std::abs(logBlue[h]) > threshold1)
            count++;
    }

    if ((static_cast<float>(count) / static_cast<float>(gridX*gridY)) > threshold)
        return true;
    else
        return false;

}

void computeIrradiance(Array2Df& irradiance, const Array2Df& in)
{
#ifdef TIMER_PROFILING
    msec_timer stop_watch;
    stop_watch.start();
#endif

    const int width = in.getCols();
    const int height = in.getRows();

#pragma omp parallel for schedule(static)
    for (int i = 0; i < width*height; ++i) {
        irradiance(i) = std::exp( in(i) );
    }

#ifdef TIMER_PROFILING
    stop_watch.stop_and_update();
    std::cout << "computeIrradiance = " << stop_watch.get_time() << " msec" << std::endl;
#endif
}

void computeLogIrradiance(Array2Df &logIrradiance, const Array2Df &u)
{
#ifdef TIMER_PROFILING
    msec_timer stop_watch;
    stop_watch.start();
#endif
    const int width = u.getCols();
    const int height = u.getRows();
    
    float ir, logIr;
    #pragma omp parallel for private (ir, logIr) schedule(static)
    for (int i = 0; i < width*height; i++) {
            ir = u(i);
            if (ir == 0.0f)
                logIr = -11.09f;
            else
                logIr = std::log(ir);

            logIrradiance(i) = logIr;
    }
 
#ifdef TIMER_PROFILING
    stop_watch.stop_and_update();
    std::cout << "computeLogIrradiance = " << stop_watch.get_time() << " msec" << std::endl;
#endif
}

void computeGradient(Array2Df &gradientX, Array2Df &gradientY, const Array2Df& in)
{
#ifdef TIMER_PROFILING
    msec_timer stop_watch;
    stop_watch.start();
#endif

    const int width = in.getCols();
    const int height = in.getRows();

    #pragma omp parallel for schedule(static)
    for (int j = 1; j < height-1; j++) {
        for (int i = 1; i < width-1; i++) {
            gradientX(i, j) = 0.5f*(in(i+1, j) - in(i-1, j));
            gradientY(i, j) = 0.5f*(in(i, j+1) - in(i, j-1));
        }
    }
    #pragma omp parallel for schedule(static)
    for (int i = 1; i < width-1; i++) {
        gradientX(i, 0) = 0.5f*(in(i+1, 0) - in(i-1, 0));
        gradientX(i, height-1) = 0.5f*(in(i+1, height-1) - in(i-1, height-1));
        gradientY(i, 0) = 0.0f;
        gradientY(i, height-1) = 0.0f;
    }
    #pragma omp parallel for schedule(static)
    for (int j = 1; j < height-1; j++) {
        gradientX(0, j) = 0.0f;
        gradientX(width-1, j) = 0.0f;
        gradientY(0, j) = 0.5f*(in(0, j+1) - in(0, j-1));
        gradientY(width-1, j) = 0.5f*(in(width-1, j+1) - in(width-1, j-1));
    }
    gradientX(0, 0) = gradientX(0, height-1) = gradientX(width-1, 0) = gradientX(width-1, height-1) = 0.0f;
    gradientY(0, 0) = gradientY(0, height-1) = gradientY(width-1, 0) = gradientY(width-1, height-1) = 0.0f;
#ifdef TIMER_PROFILING
    stop_watch.stop_and_update();
    std::cout << "computeGradient = " << stop_watch.get_time() << " msec" << std::endl;
#endif
}

void computeDivergence(Array2Df &divergence, const Array2Df& gradientX, const Array2Df& gradientY)
{
#ifdef TIMER_PROFILING
    msec_timer stop_watch;
    stop_watch.start();
#endif
    const int width = gradientX.getCols();
    const int height = gradientX.getRows();

    divergence(0, 0) = gradientX(0, 0) + gradientY(0, 0);
    #pragma omp parallel for schedule(static)
    for (int j = 1; j < height-1; j++) {
        for (int i = 1; i < width-1; i++) {
            divergence(i, j) = 0.5f*(gradientX(i+1, j) - gradientX(i-1, j)) +
                                  0.5f*(gradientY(i, j+1) - gradientY(i, j-1));
        }
    }
    #pragma omp parallel for schedule(static)
    for (int j = 1; j < height-1; j++) {
        divergence(0, j) = gradientX(1, j) - gradientX(0, j) + 0.5f*(gradientY(0, j+1) - gradientY(0, j-1));
        divergence(width-1, j) = gradientX(width-1, j) - gradientX(width-2, j) +
                                    0.5f*(gradientY(width-1, j) - gradientY(width-1, j-1));
    }
    #pragma omp parallel for schedule(static)
    for (int i = 1; i < width-1; i++) {
        divergence(i, 0) = 0.5f*(gradientX(i, 0) - gradientX(i-1, 0)) + gradientY(i, 0);
        divergence(i, height-1) = 0.5f*(gradientX(i+1, height-1) - gradientX(i-1, height-1)) +
                                     gradientY(i, height-1) - gradientY(i, height-2);
    }
#ifdef TIMER_PROFILING
    stop_watch.stop_and_update();
    std::cout << "computeDivergence = " << stop_watch.get_time() << " msec" << std::endl;
#endif
}

void blendGradients(Array2Df &gradientXBlended, Array2Df &gradientYBlended,
                    const Array2Df &gradientX, const Array2Df &gradientY,
                    const Array2Df &gradientXGood, const Array2Df &gradientYGood,
                    bool patches[agGridSize][agGridSize], const int gridX, const int gridY)
{
#ifdef TIMER_PROFILING
    msec_timer stop_watch;
    stop_watch.start();
#endif
    int width = gradientX.getCols();
    int height = gradientY.getRows();

    int x, y;
    #pragma omp parallel for private(x, y) schedule(static)
    for (int j = 0; j < height; j++) {
        y = floor(static_cast<float>(j)/gridY);
        for (int i = 0; i < width; i++) {
            x = floor(static_cast<float>(i)/gridX);
            if (patches[x][y] == true) {
                gradientXBlended(i, j) = gradientXGood(i, j);
                gradientYBlended(i, j) = gradientYGood(i, j);
                if (i % gridX == 0) {
                    gradientXBlended(i+1, j) = gradientXGood(i+1, j);
                    gradientYBlended(i+1, j) = gradientYGood(i+1, j);
                }
                if (j % gridY == 0) {
                    gradientXBlended(i, j+1) = gradientXGood(i, j+1);
                    gradientYBlended(i, j+1) = gradientYGood(i, j+1);
                }
            }
            else {
                gradientXBlended(i, j) = gradientX(i, j);
                gradientYBlended(i, j) = gradientY(i, j);
            }
        }
    }
#ifdef TIMER_PROFILING
    stop_watch.stop_and_update();
    std::cout << "blendGradients = " << stop_watch.get_time() << " msec" << std::endl;
#endif
}

void blendGradients(Array2Df &gradientXBlended, Array2Df &gradientYBlended,
                    const Array2Df &gradientX, const Array2Df &gradientY,
                    const Array2Df &gradientXGood, const Array2Df &gradientYGood,
                    const QImage& agMask)
{
#ifdef TIMER_PROFILING
    msec_timer stop_watch;
    stop_watch.start();
#endif
    int width = gradientX.getCols();
    int height = gradientY.getRows();

    #pragma omp parallel for schedule(static)
    for (int j = 0; j < height; j++) {
        for (int i = 0; i < width; i++) {
            if (qAlpha(agMask.pixel(i,j))  != 0) {
                gradientXBlended(i, j) = gradientXGood(i, j);
                gradientYBlended(i, j) = gradientYGood(i, j);
            }
            else {
                gradientXBlended(i, j) = gradientX(i, j);
                gradientYBlended(i, j) = gradientY(i, j);
            }
        }
    }
#ifdef TIMER_PROFILING
    stop_watch.stop_and_update();
    std::cout << "blendGradients = " << stop_watch.get_time() << " msec" << std::endl;
#endif
}

void colorBalance(pfs::Array2Df& U, const pfs::Array2Df& F, const int x, const int y)
{
    const int width = U.getCols();
    const int height = U.getRows();
    
    float sf = F(x, y) / U(x, y);
    #pragma omp parallel for schedule(static)
    for (int i = 0; i < width*height; i++)
        U(i) *= sf;
} 

/*
QImage* shiftQImage(const QImage *in, int dx, int dy)
{
    QImage *out = new QImage(in->size(),QImage::Format_ARGB32);
    assert(out!=NULL);
    out->fill(qRgba(0,0,0,0)); //transparent black
    for(int i = 0; i < in->height(); i++)
    {
        if( (i+dy) < 0 ) continue;
        if( (i+dy) >= in->height()) break;
        QRgb *inp = (QRgb*)in->scanLine(i);
        QRgb *outp = (QRgb*)out->scanLine(i+dy);
        for(int j = 0; j < in->width(); j++)
        {
            if( (j+dx) >= in->width()) break;
            if( (j+dx) >= 0 ) outp[j+dx] = *inp;
            inp++;
        }
    }
    return out;
}

void shiftItem(HdrCreationItem& item, int dx, int dy)
{
    FramePtr shiftedFrame( pfs::shift(*item.frame(), dx, dy) );
    item.frame().swap(shiftedFrame);
    shiftedFrame.reset();       // release memory

    QScopedPointer<QImage> img(shiftQImage(item.qimage(), dx, dy));
    item.qimage()->swap( *img );
    img.reset();    // release memory
}
*/
//////////////////////////////////////////////////////////////////////////////////////

/**
 * @brief get the min/max of a float array
 *
 * @param data input array
 * @param size array size
 * @param ptr_min, ptr_max pointers to the returned values, ignored if NULL
 */
static void minmax_f32(const float *data, size_t size,
                       float *ptr_min, float *ptr_max)
{
    float min, max;
    size_t i;

    /* compute min and max */
    min = data[0];
    max = data[0];
    for (i = 1; i < size; i++) {
        if (data[i] < min)
            min = data[i];
        if (data[i] > max)
            max = data[i];
    }

    /* save min and max to the returned pointers if available */
    if (NULL != ptr_min)
        *ptr_min = min;
    if (NULL != ptr_max)
        *ptr_max = max;
    return;
}

/**
 * @brief float comparison
 *
 * IEEE754 floats can be compared as integers. Not *converted* to
 * integers, but *read* as integers while maintaining an order.
 * cf. http://www.cygnus-software.com/papers/comparingfloats/Comparing%20floating%20point%20numbers.htm#_Toc135149455
 */
static int cmp_f32(const void *a, const void *b)
{
    if (*(const int *) a > *(const int *) b)
        return 1;
    if (*(const int *) a < *(const int *) b)
        return -1;
    return 0;
}

/**
 * @brief get quantiles from a float array such that a given
 * number of pixels is out of this interval
 *
 * This function computes min (resp. max) such that the number of
 * pixels < min (resp. > max) is inferior or equal to nb_min
 * (resp. nb_max). It uses a sorting algorithm.
 *
 * @param data input/output
 * @param size data array size
 * @param nb_min, nb_max number of pixels to flatten
 * @param ptr_min, ptr_max computed min/max output, ignored if NULL
 *
 * @todo instead of sorting the whole array (expensive), select
 * pertinent values with a 128 bins histogram then sort the bins
 * around the good bin
 */
static void quantiles_f32(const float *data, size_t size,
                          size_t nb_min, size_t nb_max,
                          float *ptr_min, float *ptr_max)
{
    float *data_tmp;

    data_tmp = (float *) malloc(size * sizeof(float));

    /* copy the data and sort */
    memcpy(data_tmp, data, size * sizeof(float));
    qsort(data_tmp, size, sizeof(float), &cmp_f32);

    /* get the min/max */
    if (NULL != ptr_min)
        *ptr_min = data_tmp[nb_min];
    if (NULL != ptr_max)
        *ptr_max = data_tmp[size - 1 - nb_max];

    free(data_tmp);
    return;
}

/**
 * @brief rescale a float array
 *
 * This function operates in-place. It rescales the data by a bounded
 * affine function such that min becomes 0 and max becomes 1.
 * Warnings similar to the ones mentioned in rescale_u8() apply about
 * the risks of rounding errors.
 *
 * @param data input/output array
 * @param size array size
 * @param min, max the minimum and maximum of the input array
 *
 * @return data
 */
static float *rescale_f32(float *data, size_t size, float min, float max)
{
    size_t i;

    if (max <= min)
        for (i = 0; i < size; i++)
            data[i] = .5;
    else
        for (i = 0; i < size; i++)
            data[i] = (min > data[i] ? 0. :
                       (max < data[i] ? 1. : (data[i] - min) / (max - min)));

    return data;
}
/**
 * @brief normalize a float array
 *
 * This function operates in-place. It computes the minimum and
 * maximum values of the data, and rescales the data to
 * [0-1], with optionally flattening some extremal pixels.
 *
 * @param data input/output array
 * @param size array size
 * @param nb_min, nb_max number extremal pixels flattened
 *
 * @return data
 */
float *balance_f32(float *data, size_t size, size_t nb_min, size_t nb_max)
{
    float min, max;

    /* sanity checks */
    if (NULL == data) {
        fprintf(stderr, "a pointer is NULL and should not be so\n");
        abort();
    }
    if (nb_min + nb_max >= size) {
        nb_min = (size - 1) / 2;
        nb_max = (size - 1) / 2;
        fprintf(stderr, "the number of pixels to flatten is too large\n");
        fprintf(stderr, "using (size - 1) / 2\n");
    }

    /* get the min/max */
    if (0 != nb_min || 0 != nb_max)
        quantiles_f32(data, size, nb_min, nb_max, &min, &max);
    else
        minmax_f32(data, size, &min, &max);

    /* rescale */
    (void) rescale_f32(data, size, min, max);

    return data;
}

/**
 ** @brief simplest color balance on RGB channels
 **
 ** The input image is normalized by affine transformation on each RGB
 ** channel, saturating a percentage of the pixels at the beginning and
 ** end of the color space on each channel.
 **/
void colorbalance_rgb_f32(Array2Df& R, Array2Df& G, Array2Df& B, size_t size,
                                   size_t nb_min, size_t nb_max)
{
    (void) balance_f32(R.data(), size, nb_min, nb_max);
    (void) balance_f32(G.data(), size, nb_min, nb_max);
    (void) balance_f32(B.data(), size, nb_min, nb_max);
}

/////////////////////////////////////////////////////////////////////////////

void robustAWB(Array2Df* R_orig, Array2Df* G_orig, Array2Df* B_orig)
{
#ifdef TIMER_PROFILING
    msec_timer stop_watch;
    stop_watch.start();
#endif
    const int width = R_orig->getCols();
    const int height = R_orig->getRows();
    float u = 0.3f;
    float a = 0.8f;
    float b = 0.001f;
    float c = 1.0f;
    float T = 0.3f;
    int iterMax = 1000;
    float gain[3] = {1.0f, 1.0f, 1.0f};

    Array2Df R(width, height);
    Array2Df G(width, height);
    Array2Df B(width, height);
    Array2Df Y(width, height);
    Array2Df U(width, height);
    Array2Df V(width, height);
    Array2Df F(width, height);

    vector<float> gray_r;
    vector<float> gray_b;

    copy(R_orig, &R);
    copy(G_orig, &G);
    copy(B_orig, &B);

    for (int it = 0; it < iterMax; it++) {
        transformRGB2Yuv(&R, &G, &B, &Y, &U, &V); 
        #pragma omp parallel for 
        for (int i = 0; i < width*height; i++)
            F(i) = (abs(U(i)) + abs(V(i)))/Y(i); 
        int sum = 0;
        //#pragma omp parallel for reduction(+:sum)
        for (int i = 0; i < width*height; i++) {
            if (F(i) < T) {
                sum = sum + 1; 
                gray_r.push_back(U(i));
                gray_b.push_back(V(i));
            }
        }
        if (sum == 0)
            break;
        float U_bar = accumulate(gray_r.begin(), gray_r.end(), 0.0f)/gray_r.size();
        float V_bar = accumulate(gray_b.begin(), gray_b.end(), 0.0f)/gray_b.size();
        float err;
        float delta;
        int ch;
        gray_r.clear();
        gray_b.clear();
        if (abs(U_bar) > abs(V_bar)) {
            err = U_bar;
            ch = 2;
        }
        else {
            err = V_bar;
            ch = 0;
        }
        if (abs(err) >= a && abs(err) < c) {
            delta = 2.0f*(err/abs(err))*u;
        }
        else if (abs(err) >= c) {
            break;
        }
        else if (abs(err) < b) {
            delta = 0.0f;
            break;
        }
        else {
            delta = err*u;
        }
        gain[ch] -= delta;
        #pragma omp parallel for
        for (int i = 0; i < width*height; i++) {
            R(i) = (*R_orig)(i) * gain[0];
            B(i) = (*B_orig)(i) * gain[2];
        }
        qDebug() << it << " : " << err;
    }
    copy(&R, R_orig);
    copy(&B, B_orig);
#ifdef TIMER_PROFILING
    stop_watch.stop_and_update();
    std::cout << "robustAWB = " << stop_watch.get_time() << " msec" << std::endl;
#endif
}

