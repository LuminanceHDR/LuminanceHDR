#include "WhiteBalance.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <iomanip>
#include <numeric>

#include <boost/algorithm/minmax_element.hpp>

#include <Libpfs/colorspace/colorspace.h>
#include <Libpfs/colorspace/normalizer.h>
#include <Libpfs/manip/copy.h>
#include <Libpfs/utils/chain.h>
#include <Libpfs/utils/clamp.h>
#include <Libpfs/utils/numeric.h>
#include <Libpfs/utils/transform.h>
#include "Libpfs/utils/msec_timer.h"

using namespace pfs;
using namespace pfs::colorspace;
using namespace pfs::utils;

void computeHistogram(const pfs::Array2Df &data, std::vector<size_t> &histogram,
                      float min, float max) {
    Normalizer norm(min, max);
    for (pfs::Array2Df::const_iterator it = data.begin(), itEnd = data.end();
         it != itEnd; ++it) {
        // 1.0f -> histogram.size() - 1

        size_t bin =
            static_cast<size_t>(norm(*it) * (histogram.size() - 1) + 0.5f);
        ++histogram[bin];
    }
}

std::pair<float, float> quantiles(const pfs::Array2Df &data, float nb_min,
                                  float nb_max, float min, float max) {
    // compute histogram (less expensive than sorting the entire sequence...
    std::vector<size_t> hist(65535, 0);
    computeHistogram(data, hist, min, max);

    // normalize percentiles to image size...
    size_t lb_percentile = static_cast<size_t>(nb_min * data.size() + 0.5f);
    size_t ub_percentile = static_cast<size_t>(nb_max * data.size() + 0.5f);

    size_t counter = 0;
    std::pair<float, float> minmax;
    for (size_t idx = 0; idx < hist.size(); ++idx) {
        counter += hist[idx];
        if (counter >= lb_percentile) {
            minmax.first =
                static_cast<float>(idx) / (hist.size() - 1) * (max - min) + min;
            break;
        }
    }

    counter = 0;
    for (size_t idx = 0; idx < hist.size(); ++idx) {
        counter += hist[idx];
        if (counter >= ub_percentile) {
            minmax.second =
                static_cast<float>(idx) / (hist.size() - 1) * (max - min) + min;
            break;
        }
    }

#ifndef NDEBUG
    std::cout << "([" << nb_min << ", " << min << ", " << lb_percentile << "]"
                                                                           ", ["
              << nb_max << ", " << max << ", " << ub_percentile << "])"
              << std::endl;
#endif

    return minmax;
}

std::pair<float, float> getMinMax(const pfs::Array2Df &data) {
    std::pair<pfs::Array2Df::const_iterator, pfs::Array2Df::const_iterator>
        minmax = boost::minmax_element(data.begin(), data.end());

    return std::pair<float, float>(*minmax.first, *minmax.second);
}

void balance(pfs::Array2Df &data, float nb_min, float nb_max) {
    std::pair<float, float> minmax = getMinMax(data);
    if (nb_min > 0.f || nb_max < 1.f) {
        minmax = quantiles(data, nb_min, nb_max, minmax.first, minmax.second);
    }
    std::transform(data.begin(), data.end(), data.begin(),
                   utils::chain(utils::ClampF32(minmax.first, minmax.second),
                                Normalizer(minmax.first, minmax.second)));
}

void checkParameterValidity(float &nb_min, float &nb_max) {
    if (nb_min < 0.f) {
        nb_min = 0.f;
    }
    if (nb_max > 1.f) {
        nb_max = 1.f;
    }
    if (nb_min > nb_max) {
        std::swap(nb_min, nb_max);
    }
}

void colorBalanceRGB(Array2Df &R, Array2Df &G, Array2Df &B, float nb_min,
                     float nb_max) {
    checkParameterValidity(nb_min, nb_max);

#pragma omp parallel sections
    {
#pragma omp section
        {
            /* Executes in thread 1 */
            balance(R, nb_min, nb_max);
        }
#pragma omp section
        {
            /* Executes in thread 2 */
            balance(G, nb_min, nb_max);
        }
#pragma omp section
        {
            /* Executes in thread 3 */
            balance(B, nb_min, nb_max);
        }
    }
}

void robustAWB(Array2Df *R_orig, Array2Df *G_orig, Array2Df *B_orig) {
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
        for (int i = 0; i < width * height; i++)
            F(i) = (abs(U(i)) + abs(V(i))) / Y(i);
        int sum = 0;
        //#pragma omp parallel for reduction(+:sum)
        for (int i = 0; i < width * height; i++) {
            if (F(i) < T) {
                sum = sum + 1;
                gray_r.push_back(U(i));
                gray_b.push_back(V(i));
            }
        }
        if (sum == 0) break;
        float U_bar =
            accumulate(gray_r.begin(), gray_r.end(), 0.0f) / gray_r.size();
        float V_bar =
            accumulate(gray_b.begin(), gray_b.end(), 0.0f) / gray_b.size();
        float err;
        float delta;
        int ch;
        gray_r.clear();
        gray_b.clear();
        if (abs(U_bar) > abs(V_bar)) {
            err = U_bar;
            ch = 2;
        } else {
            err = V_bar;
            ch = 0;
        }
        if (abs(err) >= a && abs(err) < c) {
            delta = 2.0f * (err / abs(err)) * u;
        } else if (abs(err) >= c) {
            break;
        } else if (abs(err) < b) {
            delta = 0.0f;
            break;
        } else {
            delta = err * u;
        }
        gain[ch] -= delta;
#pragma omp parallel for
        for (int i = 0; i < width * height; i++) {
            R(i) = (*R_orig)(i)*gain[0];
            B(i) = (*B_orig)(i)*gain[2];
        }
        // qDebug() << it << " : " << err;
    }
    copy(&R, R_orig);
    copy(&B, B_orig);
#ifdef TIMER_PROFILING
    stop_watch.stop_and_update();
    std::cout << "robustAWB = " << stop_watch.get_time() << " msec"
              << std::endl;
#endif
}

float computeAccumulation(const pfs::Array2Df &matrix) {
    float acc = 0.f;
    for (size_t i = 0; i < matrix.size(); i++) {
        acc += std::pow(matrix(i), 6.0f);
    }
    return std::pow(acc / matrix.size(), 1.f / 6.f);
}

void shadesOfGrayAWB(Array2Df &R, Array2Df &G, Array2Df &B) {
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
            /* Executes in thread 3 */
            eG = computeAccumulation(G);
        }
#pragma omp section
        {
            /* Executes in thread 2 */
            eB = computeAccumulation(B);
        }
    }
    float norm = std::sqrt(eR * eR + eG * eG + eB * eB);
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
    std::cout << "shadesOfGrayAWB = " << stop_watch.get_time() << " msec"
              << std::endl;
#endif
}

void whiteBalance(Frame &frame, WhiteBalanceType type) {
    Channel *r;
    Channel *g;
    Channel *b;
    frame.getXYZChannels(r, g, b);

    whiteBalance(*r, *g, *b, type);
}

void whiteBalance(pfs::Array2Df &R, pfs::Array2Df &G, pfs::Array2Df &B,
                  WhiteBalanceType type) {
    switch (type) {
        case WB_COLORBALANCE: {
            colorBalanceRGB(R, G, B, 0.005, 0.995);
        } break;
        case WB_ROBUST: {
            robustAWB(&R, &G, &B);
        } break;
        case WB_SHADESOFGRAY: {
            shadesOfGrayAWB(R, G, B);
        } break;
    }
}
