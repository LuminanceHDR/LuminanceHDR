#include "WhiteBalance.h"

#include <cmath>

#include <Libpfs/manip/copy.h>
#include <Libpfs/colorspace/colorspace.h>
#include <Libpfs/utils/numeric.h>

using namespace pfs;

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

void colorbalance_rgb_f32(Array2Df& R, Array2Df& G, Array2Df& B, size_t size,
                                   size_t nb_min, size_t nb_max)
{
    (void) balance_f32(R.data(), size, nb_min, nb_max);
    (void) balance_f32(G.data(), size, nb_min, nb_max);
    (void) balance_f32(B.data(), size, nb_min, nb_max);
}


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
    float c = 10.0f;
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
        // qDebug() << it << " : " << err;
    }
    copy(&R, R_orig);
    copy(&B, B_orig);
#ifdef TIMER_PROFILING
    stop_watch.stop_and_update();
    std::cout << "robustAWB = " << stop_watch.get_time() << " msec" << std::endl;
#endif
}


float computeAccumulation(pfs::Array2Df& matrix)
{
    float acc = 0.f;
    for (int i = 0; i < matrix.size(); i++)
    {
        acc += std::pow(matrix(i), 6.0f);
    }
    return std::pow(acc/matrix.size(), 1.f/6.f);
}

void shadesOfGrayAWB(Array2Df& R, Array2Df& G, Array2Df& B)
{
#ifdef TIMER_PROFILING
    msec_timer stop_watch;
    stop_watch.start();
#endif

    float eR = 0.f;
    float eG = 0.f;
    float eB = 0.f;

#pragma omp parallel sections
    {
#pragma omp section
        {
            /* Executes in thread 1 */
            eR = computeAccumulation(R);
        }
#pragma omp section
        {
            /* Executes in thread 2 */
            eB = computeAccumulation(B);
        }
#pragma omp section
        {
            /* Executes in thread 3 */
            eG = computeAccumulation(G);
        }
    }
    float norm = std::sqrt(eR*eR + eG*eG + eB*eB);

    eR /= norm;
    eG /= norm;
    eB /= norm;
    float maximum = std::max(eR, std::max(eG, eB));
    float gainR = maximum / eR;
    float gainG = maximum / eG;
    float gainB = maximum / eB;

#pragma omp parallel sections
    {
#pragma omp section
        {
            /* Executes in thread 1 */
            pfs::utils::vsmul(R.data(), gainR, R.data(), R.size());
        }
#pragma omp section
        {
            /* Executes in thread 2 */
            pfs::utils::vsmul(G.data(), gainG, G.data(), G.size());
        }
#pragma omp section
        {
            /* Executes in thread 3 */
            pfs::utils::vsmul(B.data(), gainB, B.data(), B.size());
        }
    }

#ifdef TIMER_PROFILING
    stop_watch.stop_and_update();
    std::cout << "shadesOfGrayAWB = " << stop_watch.get_time() << " msec" << std::endl;
#endif
}

void whiteBalance(Frame& frame, WhiteBalanceType type)
{
    Channel* r;
    Channel* g;
    Channel* b;
    frame.getXYZChannels(r, g, b);

    switch (type)
    {
    case WB_COLORBALANCE:
    {
        colorbalance_rgb_f32(*r, *g, *b, frame.size(), 3, 97);
    } break;
    case WB_ROBUST:
    {
        robustAWB(r, g, b);
    } break;
    case WB_SHADESOFGRAY:
    {
        shadesOfGrayAWB(*r, *g, *b);
    } break;
    }
}
