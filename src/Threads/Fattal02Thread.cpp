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

#include <QReadWriteLock>

#include "Common/config.h"
#include "Fattal02Thread.h"
#include "TonemappingOperators/tm_operators.h"

static QReadWriteLock lock;	

Fattal02Thread::Fattal02Thread(pfs::Frame *frame, const TonemappingOptions &opts):
TMOThread(frame, opts)
{
}

void Fattal02Thread::run()
{
	connect(ph, SIGNAL(valueChanged(int)), this, SIGNAL(setValue(int)));
	emit setMaximumSteps(100);
	try
  {
		// pfstmo_fattal02 not reentrant
		lock.lockForWrite();
		pfstmo_fattal02(workingframe,
                    opts.operator_options.fattaloptions.alpha,
                    opts.operator_options.fattaloptions.beta,
                    opts.operator_options.fattaloptions.color,
                    opts.operator_options.fattaloptions.noiseredux,
                    opts.operator_options.fattaloptions.newfattal,
                    ph);
		lock.unlock();
	}
	catch(...)
  {
		lock.unlock();
		emit tmo_error("Failed to tonemap image");
		emit deleteMe(this);
		return;
	}
	
	finalize();
}
//
// run()
//

