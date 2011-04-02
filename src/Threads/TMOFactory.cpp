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

#include "TMOFactory.h"
#include "Ashikhmin02Thread.h"
#include "Drago03Thread.h"
#include "Durand02Thread.h"
#include "Fattal02Thread.h"
#include "Mantiuk06Thread.h"
#include "Mantiuk08Thread.h"
#include "Pattanaik00Thread.h"
#include "Reinhard02Thread.h"
#include "Reinhard05Thread.h"

TMOThread * TMOFactory::getTMOThread(const TMOperator &tmoOperator,
                                     pfs::Frame *workingPfsFrame, 
                                     const TonemappingOptions &opts)
{
	switch (tmoOperator)
  {
		case ashikhmin:
			return new Ashikhmin02Thread(workingPfsFrame, opts); 
		case drago:
			return new Drago03Thread(workingPfsFrame, opts);
		case durand:
			return new Durand02Thread(workingPfsFrame, opts);
		case fattal:
			return new Fattal02Thread(workingPfsFrame, opts);
		case mantiuk06:
			return new Mantiuk06Thread(workingPfsFrame, opts);
		case mantiuk08:
			return new Mantiuk08Thread(workingPfsFrame, opts);
		case pattanaik:
			return new Pattanaik00Thread(workingPfsFrame, opts);
		case reinhard02:
			return new Reinhard02Thread(workingPfsFrame, opts);
		case reinhard05:
		default:		// just to be sure
			return new Reinhard05Thread(workingPfsFrame, opts);
	}
}

