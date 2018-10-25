/*
 * This file is a part of LuminanceHDR package.
 * ----------------------------------------------------------------------
 * Copyright (C) 2006,2007 Giuseppe Rota
 * Copyright (C) 2011 Davide Anastasia
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
 * Original Work
 * @author Giuseppe Rota <grota@users.sourceforge.net>
 * Improvements, bugfixing
 * @author Franco Comida <fcomida@users.sourceforge.net>
 * Refactory of TMThread.h class to TonemapOperator in order to remove
 * dependency from QObject and QThread
 * @author Davide Anastasia <davideanastasia@users.sourceforge.net>
 *
 *
 */

#include <boost/assign.hpp>
#include <boost/thread/mutex.hpp>
#include <map>

#include "TonemappingOperators/pfstmo.h"

#include "Libpfs/channel.h"
#include "Libpfs/colorspace/colorspace.h"
#include "Libpfs/frame.h"
#include "Libpfs/progress.h"
#include "Libpfs/tm/TonemapOperator.h"

using namespace boost::assign;

template <TMOperator Key, typename ConcreteClass>
struct TonemapOperatorRegister : public TonemapOperator {
    static TonemapOperator *create() { return new ConcreteClass(); }

    TMOperator getType() const { return Key; }
};

class TonemapOperatorMantiuk06
    : public TonemapOperatorRegister<mantiuk06, TonemapOperatorMantiuk06> {
   public:
    void tonemapFrame(pfs::Frame &workingFrame, TonemappingOptions *opts,
                      pfs::Progress &ph) {
        ph.setMaximum(100);

        try {
            pfstmo_mantiuk06(
                workingFrame,
                opts->operator_options.mantiuk06options.contrastfactor,
                opts->operator_options.mantiuk06options.saturationfactor,
                opts->operator_options.mantiuk06options.detailfactor,
                opts->operator_options.mantiuk06options.contrastequalization,
                ph);
        } catch (...) {
            throw std::runtime_error("Mantiuk06: Tonemap Failed");
        }
    }
};

struct TonemapOperatorMantiuk08
    : public TonemapOperatorRegister<mantiuk08, TonemapOperatorMantiuk08> {
    void tonemapFrame(pfs::Frame &workingframe, TonemappingOptions *opts,
                      pfs::Progress &ph) {
        ph.setMaximum(100);

        // Convert to CS_XYZ: tm operator now use this colorspace
        pfs::Channel *X, *Y, *Z;
        workingframe.getXYZChannels(X, Y, Z);
        pfs::transformColorSpace(pfs::CS_RGB, X, Y, Z, pfs::CS_XYZ, X, Y, Z);

        try {
            pfstmo_mantiuk08(
                workingframe,
                opts->operator_options.mantiuk08options.colorsaturation,
                opts->operator_options.mantiuk08options.contrastenhancement,
                opts->operator_options.mantiuk08options.luminancelevel,
                opts->operator_options.mantiuk08options.setluminance, ph);
        } catch (...) {
            throw std::runtime_error("Mantiuk08: Tonemap Failed");
        }

        pfs::transformColorSpace(pfs::CS_XYZ, X, Y, Z, pfs::CS_RGB, X, Y, Z);
    }
};

struct TonemapOperatorFattal02
    : public TonemapOperatorRegister<fattal, TonemapOperatorFattal02> {
    void tonemapFrame(pfs::Frame &workingframe, TonemappingOptions *opts,
                      pfs::Progress &ph) {
        ph.setMaximum(100);

        int detail_level = 0;
        if (opts->xsize > 0) {
            float ratio = (float)opts->origxsize / (float)opts->xsize;
            if (ratio < 2)
                detail_level = 3;
            else if (ratio < 4)
                detail_level = 2;
            else if (ratio < 8)
                detail_level = 1;
            else
                detail_level = 0;
        } else
            detail_level = 3;

        try {
            pfstmo_fattal02(workingframe,
                            opts->operator_options.fattaloptions.alpha,
                            opts->operator_options.fattaloptions.beta,
                            opts->operator_options.fattaloptions.color,
                            opts->operator_options.fattaloptions.noiseredux,
                            opts->operator_options.fattaloptions.newfattal,
                            opts->operator_options.fattaloptions.fftsolver,
                            detail_level, ph);
        } catch (...) {
            throw std::runtime_error("Fattal: Tonemap Failed");
        }
    }
};

struct TonemapOperatorFerradans11
    : public TonemapOperatorRegister<ferradans, TonemapOperatorFerradans11> {
    void tonemapFrame(pfs::Frame &workingframe, TonemappingOptions *opts,
                      pfs::Progress &ph) {
        ph.setMaximum(100);

        try {
            pfstmo_ferradans11(
                workingframe, opts->operator_options.ferradansoptions.rho,
                opts->operator_options.ferradansoptions.inv_alpha, ph);
        } catch (...) {
            throw std::runtime_error("Ferradans: Tonemap Failed");
        }
    }
};

struct TonemapOperatorMai11
    : public TonemapOperatorRegister<mai, TonemapOperatorMai11> {
    void tonemapFrame(pfs::Frame &workingframe, TonemappingOptions *opts,
                      pfs::Progress &ph) {
        ph.setMaximum(100);

        try {
            pfstmo_mai11(workingframe, ph);
        } catch (...) {
            throw std::runtime_error("Mai: Tonemap Failed");
        }
    }
};

struct TonemapOperatorDrago03
    : public TonemapOperatorRegister<drago, TonemapOperatorDrago03> {
    void tonemapFrame(pfs::Frame &workingframe, TonemappingOptions *opts,
                      pfs::Progress &ph) {
        ph.setMaximum(100);  // this guy should not be here!

        try {
            pfstmo_drago03(workingframe,
                           opts->operator_options.dragooptions.bias, ph);
        } catch (...) {
            throw std::runtime_error("Drago: Tonemap Failed");
        }
    }
};

class TonemapOperatorDurand02
    : public TonemapOperatorRegister<durand, TonemapOperatorDurand02> {
    void tonemapFrame(pfs::Frame &workingframe, TonemappingOptions *opts,
                      pfs::Progress &ph) {
        ph.setMaximum(100);

        try {
            pfstmo_durand02(workingframe,
                            opts->operator_options.durandoptions.spatial,
                            opts->operator_options.durandoptions.range,
                            opts->operator_options.durandoptions.base, ph);
        } catch (...) {
            throw std::runtime_error("Durand: Tonemap Failed");
        }
    }
};

struct TonemapOperatorReinhard02
    : public TonemapOperatorRegister<reinhard02, TonemapOperatorReinhard02> {
    void tonemapFrame(pfs::Frame &workingframe, TonemappingOptions *opts,
                      pfs::Progress &ph) {
        ph.setMaximum(100);

        // Convert to CS_XYZ: tm operator now use this colorspace
        pfs::Channel *X, *Y, *Z;
        workingframe.getXYZChannels(X, Y, Z);
        pfs::transformColorSpace(pfs::CS_RGB, X, Y, Z, pfs::CS_XYZ, X, Y, Z);

        try {
            pfstmo_reinhard02(
                workingframe, opts->operator_options.reinhard02options.key,
                opts->operator_options.reinhard02options.phi,
                opts->operator_options.reinhard02options.range,
                opts->operator_options.reinhard02options.lower,
                opts->operator_options.reinhard02options.upper,
                opts->operator_options.reinhard02options.scales, ph);
        } catch (...) {
            throw std::runtime_error("Reinhard02: Tonemap Failed");
        }

        pfs::transformColorSpace(pfs::CS_XYZ, X, Y, Z, pfs::CS_RGB, X, Y, Z);
    }
};

struct TonemapOperatorReinhard05
    : public TonemapOperatorRegister<reinhard05, TonemapOperatorReinhard05> {
    void tonemapFrame(pfs::Frame &workingframe, TonemappingOptions *opts,
                      pfs::Progress &ph) {
        ph.setMaximum(100);

        try {
            pfstmo_reinhard05(
                workingframe,
                opts->operator_options.reinhard05options.brightness,
                opts->operator_options.reinhard05options.chromaticAdaptation,
                opts->operator_options.reinhard05options.lightAdaptation, ph);
        } catch (...) {
            throw std::runtime_error("Reinhard05: Tonemap Failed");
        }
    }
};

struct TonemapOperatorAshikhmin02
    : public TonemapOperatorRegister<ashikhmin, TonemapOperatorAshikhmin02> {
    void tonemapFrame(pfs::Frame &workingframe, TonemappingOptions *opts,
                      pfs::Progress &ph) {
        ph.setMaximum(100);

        try {
            pfstmo_ashikhmin02(
                workingframe, opts->operator_options.ashikhminoptions.simple,
                opts->operator_options.ashikhminoptions.lct,
                (opts->operator_options.ashikhminoptions.eq2 ? 2 : 4), ph);
        } catch (...) {
            throw std::runtime_error("Ashikhmin: Tonemap Failed");
        }
    }
};

struct TonemapOperatorPattanaik00
    : public TonemapOperatorRegister<pattanaik, TonemapOperatorPattanaik00> {
    void tonemapFrame(pfs::Frame &workingframe, TonemappingOptions *opts,
                      pfs::Progress &ph) {
        ph.setMaximum(100);

        // Convert to CS_XYZ: tm operator now use this colorspace
        pfs::Channel *X, *Y, *Z;
        workingframe.getXYZChannels(X, Y, Z);
        pfs::transformColorSpace(pfs::CS_RGB, X, Y, Z, pfs::CS_XYZ, X, Y, Z);

        try {
            pfstmo_pattanaik00(
                workingframe, opts->operator_options.pattanaikoptions.local,
                opts->operator_options.pattanaikoptions.multiplier,
                opts->operator_options.pattanaikoptions.cone, // * 1000,
                opts->operator_options.pattanaikoptions.rod, // * 1000,
                opts->operator_options.pattanaikoptions.autolum, ph);
        } catch (...) {
            throw std::runtime_error("Pattanaik: Tonemap Failed");
        }

        pfs::transformColorSpace(pfs::CS_XYZ, X, Y, Z, pfs::CS_RGB, X, Y, Z);
    }
};

struct TonemapOperatorFerwerda96
    : public TonemapOperatorRegister<ferwerda, TonemapOperatorFerwerda96> {
    void tonemapFrame(pfs::Frame &workingframe, TonemappingOptions *opts,
                      pfs::Progress &ph) {
        ph.setMaximum(100);

        try {
            pfstmo_ferwerda96(
                workingframe, opts->operator_options.ferwerdaoptions.multiplier,
                opts->operator_options.ferwerdaoptions.adaptationluminance,
                ph);
        } catch (...) {
            throw std::runtime_error("Ferwerda: Tonemap Failed");
        }
    }
};

struct TonemapOperatorKimKautz08
    : public TonemapOperatorRegister<kimkautz, TonemapOperatorKimKautz08> {
    void tonemapFrame(pfs::Frame &workingframe, TonemappingOptions *opts,
                      pfs::Progress &ph) {
        ph.setMaximum(100);

        try {
            pfstmo_kimkautz08(
                workingframe,
                opts->operator_options.kimkautzoptions.c1,
                opts->operator_options.kimkautzoptions.c2,
                ph);
        } catch (...) {
            throw std::runtime_error("KimKautz: Tonemap Failed");
        }
    }
};

struct TonemapOperatorVanHateren06
    : public TonemapOperatorRegister<vanhateren, TonemapOperatorVanHateren06> {
    void tonemapFrame(pfs::Frame &workingframe, TonemappingOptions *opts,
                      pfs::Progress &ph) {
        ph.setMaximum(100);

        try {
            pfstmo_vanhateren06(
                workingframe,
                opts->operator_options.vanhaterenoptions.pupil_area,
                ph);
        } catch (...) {
            throw std::runtime_error("VanHateren: Tonemap Failed");
        }
    }
};

typedef TonemapOperator *(*TonemapOperatorCreator)();
typedef std::map<TMOperator, TonemapOperatorCreator> TonemapOperatorCreatorMap;

inline const TonemapOperatorCreatorMap &registry() {
    static TonemapOperatorCreatorMap reg = map_list_of
        // XYZ -> *
        (mantiuk06,  TonemapOperatorRegister<mantiuk06,  TonemapOperatorMantiuk06>::create)
        (mantiuk08,  TonemapOperatorRegister<mantiuk08,  TonemapOperatorMantiuk08>::create)
        (fattal,     TonemapOperatorRegister<fattal,     TonemapOperatorFattal02>::create)
        (ferradans,  TonemapOperatorRegister<ferradans,  TonemapOperatorFerradans11>::create)
        (mai,        TonemapOperatorRegister<mai,        TonemapOperatorMai11>::create)
        (drago,      TonemapOperatorRegister<drago,      TonemapOperatorDrago03>::create)
        (durand,     TonemapOperatorRegister<durand,     TonemapOperatorDurand02>::create)
        (reinhard02, TonemapOperatorRegister<reinhard02, TonemapOperatorReinhard02>::create)
        (reinhard05, TonemapOperatorRegister<reinhard05, TonemapOperatorReinhard05>::create)
        (ashikhmin,  TonemapOperatorRegister<ashikhmin,  TonemapOperatorAshikhmin02>::create)
        (pattanaik,  TonemapOperatorRegister<pattanaik,  TonemapOperatorPattanaik00>::create)
        (ferwerda,   TonemapOperatorRegister<ferwerda,   TonemapOperatorFerwerda96>::create)
        (kimkautz,   TonemapOperatorRegister<kimkautz,   TonemapOperatorKimKautz08>::create)
        (vanhateren, TonemapOperatorRegister<vanhateren, TonemapOperatorVanHateren06>::create);
    return reg;
}

TonemapOperator::TonemapOperator() {}

TonemapOperator::~TonemapOperator() {}

TonemapOperator *TonemapOperator::getTonemapOperator(const TMOperator tmo) {
    TonemapOperatorCreatorMap::const_iterator it = registry().find(tmo);
    if (it != registry().end()) {
        return (it->second)();
    }
    throw std::runtime_error("Invalid TMOperator");
}
