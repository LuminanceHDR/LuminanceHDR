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
 * Refactory of TMThread.h class to TonemapOperator in order to remove dependency from QObject and QThread
 * @author Davide Anastasia <davideanastasia@users.sourceforge.net>
 *
 *
 */

#include <QMutex>

#include <map>
#include <boost/assign.hpp>

#include "TonemappingEngine/TonemapOperator.h"
#include "TonemappingOperators/pfstmo.h"
#include "Libpfs/channel.h"
#include "Libpfs/colorspace.h"

using namespace boost::assign;

template <TMOperator Key, typename ConcreteClass>
struct TonemapOperatorRegister : public TonemapOperator
{
    static TonemapOperator* create()
    {
        return new ConcreteClass();
    }

    TMOperator getType() const
    {
        return Key;
    }
};

class TonemapOperatorMantiuk06
        : public TonemapOperatorRegister<mantiuk06, TonemapOperatorMantiuk06>
{
public:
    void tonemapFrame(pfs::Frame* workingFrame, TonemappingOptions* opts, ProgressHelper& ph)
    {
        ph.emitSetMaximum(100);

        // pfstmo_mantiuk06 not reentrant
        m_mutex.lock();
        try
        {
            pfstmo_mantiuk06(workingFrame,
                             opts->operator_options.mantiuk06options.contrastfactor,
                             opts->operator_options.mantiuk06options.saturationfactor,
                             opts->operator_options.mantiuk06options.detailfactor,
                             opts->operator_options.mantiuk06options.contrastequalization,
                             &ph);
        }
        catch (...)
        {
            m_mutex.unlock();
            throw std::runtime_error("Tonemap Failed");
        }
        m_mutex.unlock();
    }

private:
    // It may be removed, because the issue in the race condition of the
    // operator has been solved
    static QMutex m_mutex;
};

QMutex TonemapOperatorMantiuk06::m_mutex;   // static member

struct TonemapOperatorMantiuk08
        : public TonemapOperatorRegister<mantiuk08, TonemapOperatorMantiuk08>
{
    void tonemapFrame(pfs::Frame* workingframe, TonemappingOptions* opts, ProgressHelper& ph)
    {
        ph.emitSetMaximum(100);

        // Convert to CS_XYZ: tm operator now use this colorspace
        pfs::Channel *X, *Y, *Z;
        workingframe->getXYZChannels( X, Y, Z );
        pfs::transformColorSpace(pfs::CS_RGB, X->getChannelData(), Y->getChannelData(), Z->getChannelData(),
                                 pfs::CS_XYZ, X->getChannelData(), Y->getChannelData(), Z->getChannelData());

        pfstmo_mantiuk08(workingframe,
                         opts->operator_options.mantiuk08options.colorsaturation,
                         opts->operator_options.mantiuk08options.contrastenhancement,
                         opts->operator_options.mantiuk08options.luminancelevel,
                         opts->operator_options.mantiuk08options.setluminance,
                         &ph);

        pfs::transformColorSpace(pfs::CS_XYZ, X->getChannelData(), Y->getChannelData(), Z->getChannelData(),
                                 pfs::CS_RGB, X->getChannelData(), Y->getChannelData(), Z->getChannelData());
    }
};

struct TonemapOperatorFattal02
        : public TonemapOperatorRegister<fattal, TonemapOperatorFattal02>
{
    void tonemapFrame(pfs::Frame* workingframe, TonemappingOptions* opts, ProgressHelper& ph)
    {
        ph.emitSetMaximum(100);

        float ratio = opts->origxsize / opts->xsize;
        int detail_level = 0;
        if ( ratio < 2 )
            detail_level = 3;
        else if ( ratio < 4 )
            detail_level = 2;
        else if ( ratio < 8 )
            detail_level = 1;
        else
            detail_level = 0;

    //    std::cout << "RATIO = " << ratio << ", ";
    //    std::cout << "DETAIL_LEVEL = " << detail_level << std::endl;

        pfstmo_fattal02(workingframe,
                        opts->operator_options.fattaloptions.alpha,
                        opts->operator_options.fattaloptions.beta,
                        opts->operator_options.fattaloptions.color,
                        opts->operator_options.fattaloptions.noiseredux,
                        opts->operator_options.fattaloptions.newfattal,
                        opts->operator_options.fattaloptions.fftsolver,
                        detail_level,
                        &ph);
    }
};

struct TonemapOperatorDrago03
        : public TonemapOperatorRegister<drago, TonemapOperatorDrago03>
{
    void tonemapFrame(pfs::Frame* workingframe, TonemappingOptions* opts, ProgressHelper& ph)
    {
        ph.emitSetMaximum(100);         // this guy should not be here!

        pfstmo_drago03(workingframe, opts->operator_options.dragooptions.bias, &ph);
    }
};

class TonemapOperatorDurand02
        : public TonemapOperatorRegister<durand, TonemapOperatorDurand02>
{
    void tonemapFrame(pfs::Frame* workingframe, TonemappingOptions* opts, ProgressHelper& ph)
    {
        ph.emitSetMaximum(100);

        // pfstmo_durand02 not reentrant
        m_mutex.lock();
        try
        {
            pfstmo_durand02(workingframe,
                            opts->operator_options.durandoptions.spatial,
                            opts->operator_options.durandoptions.range,
                            opts->operator_options.durandoptions.base,
                            &ph);
        }
        catch (...)
        {
            m_mutex.unlock();
            throw std::runtime_error("Tonemap Failed");
        }
        m_mutex.unlock();
    }

private:
    static QMutex m_mutex;
};

QMutex TonemapOperatorDurand02::m_mutex;


struct TonemapOperatorReinhard02
        : public TonemapOperatorRegister<reinhard02, TonemapOperatorReinhard02>
{
    void tonemapFrame(pfs::Frame* workingframe, TonemappingOptions* opts, ProgressHelper& ph)
    {
        ph.emitSetMaximum(100);

        // Convert to CS_XYZ: tm operator now use this colorspace
        pfs::Channel *X, *Y, *Z;
        workingframe->getXYZChannels( X, Y, Z );
        pfs::transformColorSpace(pfs::CS_RGB, X->getChannelData(), Y->getChannelData(), Z->getChannelData(),
                                 pfs::CS_XYZ, X->getChannelData(), Y->getChannelData(), Z->getChannelData());

        m_mutex.lock();
        try {
            pfstmo_reinhard02(workingframe,
                              opts->operator_options.reinhard02options.key,
                              opts->operator_options.reinhard02options.phi,
                              opts->operator_options.reinhard02options.range,
                              opts->operator_options.reinhard02options.lower,
                              opts->operator_options.reinhard02options.upper,
                              opts->operator_options.reinhard02options.scales,
                              &ph);
        }
        catch (...) {
            m_mutex.unlock();
            throw std::runtime_error("Tonemap Failed");
        }
        m_mutex.unlock();

        pfs::transformColorSpace(pfs::CS_XYZ, X->getChannelData(), Y->getChannelData(), Z->getChannelData(),
                                 pfs::CS_SRGB, X->getChannelData(), Y->getChannelData(), Z->getChannelData());
    }

private:
    static QMutex m_mutex;
};

QMutex TonemapOperatorReinhard02::m_mutex;

struct TonemapOperatorReinhard05
        : public TonemapOperatorRegister<reinhard05, TonemapOperatorReinhard05>
{
    void tonemapFrame(pfs::Frame* workingframe,
                      TonemappingOptions* opts,
                      ProgressHelper& ph)
    {
        ph.emitSetMaximum(100);

        pfstmo_reinhard05( workingframe,
                           opts->operator_options.reinhard05options.brightness,
                           opts->operator_options.reinhard05options.chromaticAdaptation,
                           opts->operator_options.reinhard05options.lightAdaptation,
                           &ph);
    }
};

struct TonemapOperatorAshikhmin02
        : public TonemapOperatorRegister<ashikhmin, TonemapOperatorAshikhmin02>
{
    void tonemapFrame(pfs::Frame* workingframe, TonemappingOptions* opts, ProgressHelper& ph)
    {
        ph.emitSetMaximum(100);

        // Convert to CS_XYZ: tm operator now use this colorspace
        pfs::Channel *X, *Y, *Z;
        workingframe->getXYZChannels( X, Y, Z );
        pfs::transformColorSpace(pfs::CS_RGB, X->getChannelData(), Y->getChannelData(), Z->getChannelData(),
                                 pfs::CS_XYZ, X->getChannelData(), Y->getChannelData(), Z->getChannelData());

        pfstmo_ashikhmin02(workingframe,
                           opts->operator_options.ashikhminoptions.simple,
                           opts->operator_options.ashikhminoptions.lct,
                           (opts->operator_options.ashikhminoptions.eq2 ? 2 : 4),
                           &ph);

        pfs::transformColorSpace(pfs::CS_XYZ, X->getChannelData(), Y->getChannelData(), Z->getChannelData(),
                                 pfs::CS_RGB, X->getChannelData(), Y->getChannelData(), Z->getChannelData());
    }
};

struct TonemapOperatorPattanaik00
        : public TonemapOperatorRegister<pattanaik, TonemapOperatorPattanaik00>
{
    void tonemapFrame(pfs::Frame* workingframe, TonemappingOptions* opts, ProgressHelper& ph)
    {
        ph.emitSetMaximum(100);

        // Convert to CS_XYZ: tm operator now use this colorspace
        pfs::Channel *X, *Y, *Z;
        workingframe->getXYZChannels( X, Y, Z );
        pfs::transformColorSpace(pfs::CS_RGB, X->getChannelData(), Y->getChannelData(), Z->getChannelData(),
                                 pfs::CS_XYZ, X->getChannelData(), Y->getChannelData(), Z->getChannelData());

        pfstmo_pattanaik00(workingframe,
                           opts->operator_options.pattanaikoptions.local,
                           opts->operator_options.pattanaikoptions.multiplier,
                           opts->operator_options.pattanaikoptions.cone,
                           opts->operator_options.pattanaikoptions.rod,
                           opts->operator_options.pattanaikoptions.autolum,
                           &ph);


        pfs::transformColorSpace(pfs::CS_XYZ, X->getChannelData(), Y->getChannelData(), Z->getChannelData(),
                                 pfs::CS_SRGB, X->getChannelData(), Y->getChannelData(), Z->getChannelData());
    }
};

typedef TonemapOperator* (*TonemapOperatorCreator)();
typedef std::map<TMOperator, TonemapOperatorCreator> TonemapOperatorCreatorMap;

inline
const TonemapOperatorCreatorMap& registry()
{
    static TonemapOperatorCreatorMap reg =
            map_list_of
            // XYZ -> *
            (mantiuk06, TonemapOperatorRegister<mantiuk06, TonemapOperatorMantiuk06>::create)
            (mantiuk08, TonemapOperatorRegister<mantiuk08, TonemapOperatorMantiuk08>::create)
            (fattal, TonemapOperatorRegister<fattal, TonemapOperatorFattal02>::create)
            (drago, TonemapOperatorRegister<drago, TonemapOperatorDrago03>::create)
            (durand, TonemapOperatorRegister<durand, TonemapOperatorDurand02>::create)
            (reinhard02, TonemapOperatorRegister<reinhard02, TonemapOperatorReinhard02>::create)
            (reinhard05, TonemapOperatorRegister<reinhard05, TonemapOperatorReinhard05>::create)
            (ashikhmin, TonemapOperatorRegister<ashikhmin, TonemapOperatorAshikhmin02>::create)
            (pattanaik, TonemapOperatorRegister<pattanaik, TonemapOperatorPattanaik00>::create)
            ;
    return reg;
}

TonemapOperator::TonemapOperator()
{}

TonemapOperator::~TonemapOperator()
{}

TonemapOperator* TonemapOperator::getTonemapOperator(const TMOperator tmo)
{
    TonemapOperatorCreatorMap::const_iterator it = registry().find(tmo);
    if ( it != registry().end() )
    {
        return (it->second)();
    }
    throw std::runtime_error("Invalid TMOperator");
}
