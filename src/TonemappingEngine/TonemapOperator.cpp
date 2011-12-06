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

#include "TonemappingEngine/TonemapOperator.h"
#include "TonemappingEngine/TonemapOperatorAshikhmin02.h"
#include "TonemappingEngine/TonemapOperatorDrago03.h"
#include "TonemappingEngine/TonemapOperatorDurand02.h"
#include "TonemappingEngine/TonemapOperatorFattal02.h"
#include "TonemappingEngine/TonemapOperatorMantiuk06.h"
#include "TonemappingEngine/TonemapOperatorMantiuk08.h"
#include "TonemappingEngine/TonemapOperatorPattanaik00.h"
#include "TonemappingEngine/TonemapOperatorReinhard02.h"
#include "TonemappingEngine/TonemapOperatorReinhard05.h"

TonemapOperator::TonemapOperator()
{}

TonemapOperator::~TonemapOperator()
{}

TonemapOperator* TonemapOperator::getTonemapOperator(const TMOperator tmo)
{
    switch (tmo)
    {
    case ashikhmin:
        return new TonemapOperatorAshikhmin02();
    case drago:
        return new TonemapOperatorDrago03();
    case durand:
        return new TonemapOperatorDurand02();
    case fattal:
        return new TonemapOperatorFattal02();
    case mantiuk08:
        return new TonemapOperatorMantiuk08();
    case pattanaik:
        return new TonemapOperatorPattanaik00();
    case reinhard02:
        return new TonemapOperatorReinhard02();
    case reinhard05:
        return new TonemapOperatorReinhard05();
    // just to be sure
    case mantiuk06:
    default:
        return new TonemapOperatorMantiuk06();

    }
}
