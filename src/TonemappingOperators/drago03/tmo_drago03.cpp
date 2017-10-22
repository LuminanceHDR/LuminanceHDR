/**
 * @brief Frederic Drago logmapping operator
 *
 * Adaptive logarithmic mapping for displaying high contrast
 * scenes.
 * F. Drago, K. Myszkowski, T. Annen, and N. Chiba. In Eurographics 2003.
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
 * $Id: tmo_drago03.cpp,v 1.4 2008/11/04 23:43:08 rafm Exp $
 */

#include "tmo_drago03.h"

#include <cassert>
#include <cmath>

#include <boost/compute/core.hpp>
#include <boost/compute/algorithm/transform.hpp>
#include <boost/compute/container/vector.hpp>
#include <boost/compute/functional/math.hpp>
#include <boost/compute/functional/bind.hpp>
#include <boost/compute/lambda/functional.hpp>
#include <boost/compute/lambda/placeholders.hpp>
#include <boost/compute/types/struct.hpp>
#include <boost/math/special_functions/fpclassify.hpp>

#include "Libpfs/frame.h"
#include "Libpfs/progress.h"
#include "TonemappingOperators/pfstmo.h"
#include <Libpfs/utils/msec_timer.h>

namespace {
const float LOG05 = -0.693147f;  // log(0.5)
}

namespace compute = boost::compute;
using boost::compute::lambda::_1;

#define TIMER_PROFILING

void calculateLuminance(unsigned int width, unsigned int height, const float *Y,
                        float &avLum, float &maxLum) {
    avLum = 0.0f;
    maxLum = 0.0f;

    int size = width * height;

    for (int i = 0; i < size; i++) {
        avLum += log(Y[i] + 1e-4);
        maxLum = (Y[i] > maxLum) ? Y[i] : maxLum;
    }
    avLum = exp(avLum / size);
}

void tmo_drago03(const pfs::Array2Df &Y, pfs::Array2Df &L, float maxLum,
                 float avLum, float bias, pfs::Progress &ph) {
    assert(Y.getRows() == L.getRows());
    assert(Y.getCols() == L.getCols());

#ifdef TIMER_PROFILING
    msec_timer f_timer;
    f_timer.start();
#endif

    size_t size = Y.getRows() * Y.getCols();

    compute::device device = compute::system::default_device();
    compute::context context(device);
    compute::command_queue queue(context, device);

    std::cout << " (platform: " << device.platform().name() << ")" << std::endl;

    compute::vector<float> L_c(size, context);
    compute::vector<float> Y_c(size, context);
    compute::copy(Y.begin(), Y.end(), Y_c.begin(), queue);
    // normalize maximum luminance by average luminance
    maxLum /= avLum;

    float divider = std::log10(maxLum + 1.0f);
    float biasP = log(bias) / LOG05;

    compute::vector<float> Yw_c(size, context);
    compute::transform(Y_c.begin(), Y_c.end(), Yw_c.begin(), _1 / avLum, queue); //Yw
    compute::vector<float> interpol_c(size, context);
    compute::vector<float> biasP_c(size, context);
    compute::fill(biasP_c.begin(), biasP_c.end(), biasP, queue); // biasP
    compute::transform(Yw_c.begin(), Yw_c.end(), interpol_c.begin(), _1 / maxLum, queue); //Yw / maxLum
    compute::transform(interpol_c.begin(), interpol_c.end(), biasP_c.begin(), interpol_c.begin(), // biasFunc(biasp, Yw / maxLum)
            compute::pow<float>(), queue);
    compute::transform(interpol_c.begin(), interpol_c.end(), interpol_c.begin(), 2.f + _1 * 8.f, queue); // 2.f + biasFunc(biasp, Yw / maxLum) * 8.f
    compute::transform(interpol_c.begin(), interpol_c.end(), interpol_c.begin(), compute::log<float>(), queue); //log
    compute::transform(Yw_c.begin(), Yw_c.end(), L_c.begin(), compute::log1p<float>(), queue); //log1p(Yw)
    compute::transform(L_c.begin(), L_c.end(), interpol_c.begin(), L_c.begin(),
            compute::divides<float>(), queue); // log1p(Yp) / interp
    compute::transform(L_c.begin(), L_c.end(), L_c.begin(), _1 / divider, queue);
    compute::copy(L_c.begin(), L_c.end(), L.begin(), queue);
#ifdef TIMER_PROFILING
    f_timer.stop_and_update();
    cout << "drago03 = " << f_timer.get_time() << " msec"
              << endl;
#endif
}
