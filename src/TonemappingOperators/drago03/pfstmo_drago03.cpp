/**
 * @brief Adaptive logarithmic tone mapping
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
 * @author Grzegorz Krawczyk, <krawczyk@mpi-inf.mpg.de>
 *
 * $Id: pfstmo_drago03.cpp,v 1.3 2008/09/04 12:46:48 julians37 Exp $
 */

#include <cmath>
#include <iostream>

#include <boost/math/special_functions/fpclassify.hpp>
#include <boost/compute/core.hpp>
#include <boost/compute/algorithm/transform.hpp>
#include <boost/compute/algorithm/replace.hpp>
#include <boost/compute/container/vector.hpp>
#include <boost/compute/functional/math.hpp>
#include <boost/compute/lambda/functional.hpp>
#include <boost/compute/lambda/placeholders.hpp>
#include <boost/math/special_functions/fpclassify.hpp>

#include "Libpfs/exception.h"
#include "Libpfs/frame.h"
#include "Libpfs/progress.h"
#include "tmo_drago03.h"

namespace compute = boost::compute;
using boost::compute::lambda::_1;
using boost::compute::lambda::_2;

void pfstmo_drago03(pfs::Frame &frame, float opt_biasValue, pfs::Progress &ph) {
#ifndef NDEBUG
    std::stringstream ss;
    ss << "pfstmo_drago03 (";
    ss << "bias: " << opt_biasValue;
    ss << ")";
    std::cout << ss.str() << std::endl;
#endif

    compute::device device = compute::system::default_device();
    compute::context context(device);
    compute::command_queue queue(context, device);

    pfs::Channel *X, *Y, *Z;
    frame.getXYZChannels(X, Y, Z);

    if (!X || !Y || !Z) {
        throw pfs::Exception("Missing X, Y, Z channels in the PFS stream");
    }

    frame.getTags().setTag("LUMINANCE", "RELATIVE");
    //---

    pfs::Array2Df &Xr = *X;
    pfs::Array2Df &Yr = *Y;
    pfs::Array2Df &Zr = *Z;

    int w = Yr.getCols();
    int h = Yr.getRows();

    size_t size = w*h;

    float maxLum;
    float avLum;
    calculateLuminance(w, h, Yr.data(), avLum, maxLum);

    pfs::Array2Df L(w, h);
    try {
        tmo_drago03(Yr, L, maxLum, avLum, opt_biasValue, ph);
    } catch (...) {
        throw pfs::Exception("Tonemapping Failed!");
    }

    compute::vector<float> L_c(size, context);
    compute::copy(L.begin(), L.end(), L_c.begin(), queue);
    compute::vector<float> Xr_c(size, context);
    compute::copy(Xr.begin(), Xr.end(), Xr_c.begin(), queue);
    compute::vector<float> Yr_c(size, context);
    compute::copy(Yr.begin(), Yr.end(), Yr_c.begin(), queue);
    compute::vector<float> Zr_c(size, context);
    compute::copy(Zr.begin(), Zr.end(), Zr_c.begin(), queue);

    compute::replace(Yr_c.begin(), Yr_c.end(), 0.f, 1.f, queue);
    compute::transform(L_c.begin(), L_c.end(), Yr_c.begin(), L_c.begin(), compute::divides<float>(), queue);
    compute::transform(Xr_c.begin(), Xr_c.end(), L_c.begin(), Xr_c.begin(), compute::multiplies<float>(), queue);
    compute::transform(Yr_c.begin(), Yr_c.end(), L_c.begin(), Yr_c.begin(), compute::multiplies<float>(), queue);
    compute::transform(Zr_c.begin(), Zr_c.end(), L_c.begin(), Zr_c.begin(), compute::multiplies<float>(), queue);

    compute::copy(Xr_c.begin(), Xr_c.end(), Xr.begin(), queue);
    compute::copy(Yr_c.begin(), Yr_c.end(), Yr.begin(), queue);
    compute::copy(Zr_c.begin(), Zr_c.end(), Zr.begin(), queue);

    if (!ph.canceled()) {
        ph.setValue(100);
    }
}
