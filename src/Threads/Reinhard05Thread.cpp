/*
 * This file is a part of LuminanceHDR package.
 * ---------------------------------------------------------------------- 
 * Copyright (C) 2006,2007 Giuseppe Rota
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
 *
 */

#include "Threads/Reinhard05Thread.h"
#include "TonemappingOperators/pfstmo.h"
#include "Core/TonemappingOptions.h"
#include "Libpfs/channel.h"
#include "Libpfs/colorspace.h"

TonemapOperatorReinhard05::TonemapOperatorReinhard05():
    TonemapOperator()
{}

void TonemapOperatorReinhard05::tonemapFrame(pfs::Frame* workingframe, TonemappingOptions* opts, ProgressHelper& ph)
{
    ph.emitSetMaximum(100);

    // Convert to CS_XYZ: tm operator now use this colorspace
    pfs::Channel *X, *Y, *Z;
    workingframe->getXYZChannels( X, Y, Z );
    pfs::transformColorSpace(pfs::CS_RGB, X->getChannelData(), Y->getChannelData(), Z->getChannelData(),
                             pfs::CS_XYZ, X->getChannelData(), Y->getChannelData(), Z->getChannelData());

    pfstmo_reinhard05(workingframe,
                      opts->operator_options.reinhard05options.brightness,
                      opts->operator_options.reinhard05options.chromaticAdaptation,
                      opts->operator_options.reinhard05options.lightAdaptation,
                      &ph);

    pfs::transformColorSpace(pfs::CS_XYZ, X->getChannelData(), Y->getChannelData(), Z->getChannelData(),
                             pfs::CS_RGB, X->getChannelData(), Y->getChannelData(), Z->getChannelData());
}

TMOperator TonemapOperatorReinhard05::getType()
{
    return reinhard05;
}
