/*
 * This file is a part of LuminanceHDR package.
 * ---------------------------------------------------------------------- 
 * Copyright (C) 2008,2007 Giuseppe Rota
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

#include "Common/config.h"
#include "Fileformat/pfsoutldrimage.h"
#include "Mantiuk08Thread.h"

void pfstmo_mantiuk08 (pfs::Frame*,float,float,float,bool,ProgressHelper *);

Mantiuk08Thread::Mantiuk08Thread(pfs::Frame *frame, const TonemappingOptions &opts) : 
	TMOThread(frame, opts) {
}

void Mantiuk08Thread::run() {
	connect(ph, SIGNAL(valueChanged(int)), this, SIGNAL(setValue(int)));
	emit setMaximumSteps(100);
	try {
		pfstmo_mantiuk08(workingframe,
		opts.operator_options.mantiuk08options.colorsaturation,
		opts.operator_options.mantiuk08options.contrastenhancement,
		opts.operator_options.mantiuk08options.luminancelevel,
		opts.operator_options.mantiuk08options.setluminance,ph);
	}
	catch(pfs::Exception e) {
		if (strcmp("failed to analyse the image", e.getMessage())) 
				return;
		emit tmo_error("Failed to tonemap image");
		emit deleteMe(this);
		return;
	}
	catch(...) {
		emit tmo_error("Failed to tonemap image");
		emit deleteMe(this);
		return;
	}
	
	finalize();
}
//
// run()
//

