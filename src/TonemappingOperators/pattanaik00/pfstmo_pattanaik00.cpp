/**
 * @file pfstmo_pattanaik00.cpp
 * @brief Tone map XYZ channels using Pattanaik00 model
 *
 * Time-Dependent Visual Adaptation for Realistic Image Display
 * S.N. Pattanaik, J. Tumblin, H. Yee, and D.P. Greenberg
 * In Proceedings of ACM SIGGRAPH 2000
 *
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
 * $Id: pfstmo_pattanaik00.cpp,v 1.3 2008/09/04 12:46:49 julians37 Exp $
 */

#include "tmo_pattanaik00.h"

#include <algorithm>
#include <cmath>
#include <iostream>

#include "Libpfs/colorspace/colorspace.h"
#include "Libpfs/exception.h"
#include "Libpfs/frame.h"
#include "Libpfs/progress.h"

namespace {
void multiplyChannels(pfs::Array2Df &X, pfs::Array2Df &Y, pfs::Array2Df &Z,
                      float mult) {
    int size = Y.getCols() * Y.getRows();

    for (int i = 0; i < size; i++) {
        X(i) *= mult;
        Y(i) *= mult;
        Z(i) *= mult;
    }
}
}

void pfstmo_pattanaik00(pfs::Frame &frame, bool local, float multiplier,
                        float Acone, float Arod, bool autolum,
                        pfs::Progress &ph) {

    //--- default tone mapping parameters;
    const bool timedependence = false;
    // bool local = false;
    // float multiplier = 1.0f;
    // Acone = -1.0f;
    // Arod  = -1.0f;
    float fps = 16.0f;  // not used

#ifndef NDEBUG
    std::cout << "pfstmo_pattanaik00 (";
    std::cout << "local: " << local << ", ";
    std::cout << "multiplier: " << multiplier << ", ";
    std::cout << "Acone: " << Acone << ", ";
    std::cout << "Arod: " << Arod << ", ";
    std::cout << "autolum: " << autolum << ")" << std::endl;
#endif

    ph.setValue(0);

    std::unique_ptr<VisualAdaptationModel> am(new VisualAdaptationModel());

    pfs::Channel *X, *Y, *Z;
    frame.getXYZChannels(X, Y, Z);
    //---

    if (Y == NULL || X == NULL || Z == NULL) {
        throw pfs::Exception("Missing X, Y, Z channels in the PFS stream");
    }

    frame.getTags().setTag("LUMINANCE", "RELATIVE");
    // adaptation model
    if (multiplier != 1.0f) {
        multiplyChannels(*X, *Y, *Z, multiplier);
    }

    if (!local) {
        if (!timedependence) {
            if (!autolum)
                am->setAdaptation(Acone, Arod);
            else
                am->setAdaptation(*Y);
        } else
            am->calculateAdaptation(*Y, 1.0f / fps);
    }
    // tone mapping
    int w = Y->getWidth();
    int h = Y->getHeight();

    pfs::Array2Df R(w, h);
    pfs::Array2Df G(w, h);
    pfs::Array2Df B(w, h);

    pfs::transformColorSpace(pfs::CS_XYZ, X, Y, Z, pfs::CS_RGB, &R, &G, &B);

    try {
        tmo_pattanaik00(R, G, B, *Y, am.get(), local, ph);
    } catch (...) {
        throw pfs::Exception("Tonemapping Failed!");
    }
    pfs::transformColorSpace(pfs::CS_RGB, &R, &G, &B, pfs::CS_XYZ, X, Y, Z);

    if (!ph.canceled()) {
        ph.setValue(100);
    }
}
