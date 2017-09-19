/*
 * This file is a part of Luminance HDR package.
 * ----------------------------------------------------------------------
 *    Copyright (C) 2007 by Nicholas Phillips
 *  Copyright (C) 2013 Davide Anastasia
 *
 *    This program is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * ----------------------------------------------------------------------
 *
 */

#include "mtb_alignment.h"

#include <iso646.h>
#include <boost/lexical_cast.hpp>
#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>

#include <Common/global.h>
#include <Libpfs/array2d.h>
#include <Libpfs/colorspace/convert.h>
#include <Libpfs/colorspace/xyz.h>
#include <Libpfs/frame.h>
#include <Libpfs/manip/resize.h>
#include <Libpfs/manip/shift.h>
#include <Libpfs/utils/transform.h>

#include <Libpfs/io/jpegwriter.h>
#include "arch/math.h"

using namespace std;
using namespace pfs;

#ifndef NDEBUG
#define PRINT_DEBUG(str) std::cerr << "MTB: " << str << std::endl
#else
#define PRINT_DEBUG(str)
#endif

typedef Array2D<uint8_t> Array2D8u;
typedef Array2D<bool> Array2Db;

namespace libhdr {

long XORimages(const Array2Db &img1, const Array2Db &mask1,
               const Array2Db &img2, const Array2Db &mask2) {
    long err = 0;
    for (size_t i = 0; i < img1.getRows(); i++) {
        Array2Db::const_iterator p1 = img1.row_begin(i);
        Array2Db::const_iterator p2 = img2.row_begin(i);
        Array2Db::const_iterator m1 = mask1.row_begin(i);
        Array2Db::const_iterator m2 = mask2.row_begin(i);

        for (size_t j = 0; j < img1.getCols(); j++) {
            err += (long)((*p1++ xor *p2++) and *m1++ and *m2++);
        }
    }
    return err;
}

// setThreshold gets the data from the input image and creates the threshold
// and mask images.
// Those are bitmap (0,1 valued) with depth()=1
void setThreshold(const Array2D8u &in, const int threshold, const int noise,
                  Array2Db &threshold_out, Array2Db &mask_out) {
    assert(in.getCols() == threshold_out.getCols());
    assert(in.getRows() == threshold_out.getRows());
    assert(in.getCols() == mask_out.getCols());
    assert(in.getRows() == mask_out.getRows());

    for (size_t i = 0; i < in.getRows(); i++) {
        Array2D8u::const_iterator inp = in.row_begin(i);

        Array2Db::iterator outp = threshold_out.row_begin(i);
        Array2Db::iterator maskp = mask_out.row_begin(i);

        for (size_t j = 0; j < in.getCols(); j++) {
            *outp++ = *inp < threshold ? 0 : 1;
            *maskp++ =
                (*inp > (threshold - noise)) && (*inp < (threshold + noise))
                    ? 0
                    : 1;
            ++inp;
        }
    }
}

void getExpShift(const Array2D8u &img1, const int median1,
                 const Array2D8u &img2, const int median2, const int noise,
                 const int shift_bits, int &shift_x, int &shift_y) {
    assert(img1.getCols() == img2.getCols());
    assert(img1.getRows() == img2.getRows());

    int curr_x = 0;
    int curr_y = 0;

    if (shift_bits > 0) {
        Array2D8u img1small(img1.getCols() / 2, img1.getRows() / 2);
        Array2D8u img2small(img2.getCols() / 2, img2.getRows() / 2);

        pfs::resize(img1, img1small, BilinearInterp);
        pfs::resize(img2, img2small, BilinearInterp);

        getExpShift(img1small, median1, img2small, median2, noise,
                    shift_bits - 1, curr_x, curr_y);
        curr_x *= 2;
        curr_y *= 2;
    }

    Array2Db img1threshold(img1.getCols(), img1.getRows());
    Array2Db img1mask(img1.getCols(), img1.getRows());
    Array2Db img2threshold(img2.getCols(), img2.getRows());
    Array2Db img2mask(img2.getCols(), img2.getRows());

    setThreshold(img1, median1, noise, img1threshold, img1mask);
    setThreshold(img2, median2, noise, img2threshold, img2mask);

    Array2Db img2_shifted(img2.getCols(), img2.getRows());
    Array2Db img2mask_shifted(img2.getCols(), img2.getRows());

    int minerr = img1.size();
    for (int i = -1; i <= 1; i++) {
        for (int j = -1; j <= 1; j++) {
            int dx = curr_x + i;
            int dy = curr_y + j;

            pfs::shift(img2threshold, dx, dy, img2_shifted);
            pfs::shift(img2mask, dx, dy, img2mask_shifted);

            long err = XORimages(img1threshold, img1mask, img2_shifted,
                                 img2mask_shifted);

            if (err < minerr) {
                minerr = err;
                shift_x = dx;
                shift_y = dy;
            }
        }
    }

    PRINT_DEBUG("getExpShift::Level " << shift_bits << " shift (" << shift_x
                                      << "," << shift_y << ")");
}

int getLum(const Frame &in, Array2D8u &out, double quantile) {
    assert(quantile >= 0.0);
    assert(quantile <= 1.0);

    Array2D8u tempOut(in.getWidth(), in.getHeight());

    const Channel *R;
    const Channel *G;
    const Channel *B;

    in.getXYZChannels(R, G, B);
    // convert to grayscale...
    utils::transform(R->begin(), R->end(), G->begin(), B->begin(),
                     tempOut.begin(), colorspace::ConvertRGB2Y());

    // build histogram
    vector<long> hist(256, 0);
    assert(hist.size() == 256u);

    for (Array2D8u::iterator it = tempOut.begin(); it != tempOut.end(); ++it) {
        ++hist[*it];
    }

    // find the quantile...
    size_t relativeQuantile = in.size() * quantile;
    size_t idx = 0;
    size_t cdf = 0;
    for (; idx < hist.size(); ++idx) {
        cdf += hist[idx];

        if (cdf >= relativeQuantile) break;
    }

    // return values...
    out.swap(tempOut);
    return idx;
}

void mtbalign(const pfs::Frame &image1, const pfs::Frame &image2,
              const double quantile, const int noise, const int shift_bits,
              int &shift_x, int &shift_y) {
    Array2D8u img1lum;
    Array2D8u img2lum;

    int median1 = getLum(image1, img1lum, quantile);
    int median2 = getLum(image2, img2lum, quantile);

    PRINT_DEBUG("align::medians, image 1: " << median1
                                            << ", image 2: " << median2);
    getExpShift(img1lum, median1, img2lum, median2, noise, shift_bits, shift_x,
                shift_y);

    PRINT_DEBUG("align::done, final shift is (" << shift_x << "," << shift_y
                                                << ")");
}

static const double quantile = 0.5;
static const int noise = 4;

void mtb_alignment(std::vector<pfs::FramePtr> &framePtrList) {
    if (framePtrList.size() <= 1) return;

    int width = framePtrList[0]->getWidth();
    int height = framePtrList[0]->getHeight();

    int shift_bits =
        std::max((int)floor(log2((double)std::min(width, height))) - 6, 0);
    PRINT_DEBUG("width=" << width << ", height=" << height
                         << ", shift_bits=" << shift_bits);

    // these arrays contain the shifts of each image (except the 0-th) wrt the
    // previous one
    vector<int> shiftsX(framePtrList.size() - 1);
    vector<int> shiftsY(framePtrList.size() - 1);

    // find the shitfs
    for (size_t i = 0; i < framePtrList.size() - 1; i++) {
        mtbalign(*framePtrList[i], *framePtrList[i + 1], quantile, noise,
                 shift_bits, shiftsX[i], shiftsY[i]);
    }

    PRINT_DEBUG("shifting the images");
    int originalsize = framePtrList.size();

    int cumulativeX = 0;
    int cumulativeY = 0;
    // shift the images (apply the shifts starting from the second (index=1))
    for (int i = 1; i < originalsize; i++) {
        cumulativeX += shiftsX[i - 1];
        cumulativeY += shiftsY[i - 1];

        // avoid shifting if cumulativeX and cumulativeY are zero
        if (cumulativeX || cumulativeY) {
            PRINT_DEBUG("Cumulative shift for image "
                        << i << " = (" << cumulativeX << "," << cumulativeY
                        << ")");

            FramePtr shiftedFrame(
                pfs::shift(*framePtrList[i], cumulativeX, cumulativeY));

            framePtrList[i]->swap(*shiftedFrame);
        }
    }
}
}
