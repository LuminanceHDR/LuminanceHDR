/*
 * @brief VanHateren Tone Mapping Operator:
 *    "Encoding of High Dynamic Range Video with a Model of Human Cones"
 * 	  by J. Hans Van Hateren
 *    in ACM Transaction on Graphics 2006
 *
 * This file is a part of LuminanceHDR package
 * ----------------------------------------------------------------------
 * Copyright (C) 2018 Franco Comida
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
 * @author Franco Comida, <fcomida@users.sourceforge.net>
 *
 */

#include <assert.h>
#include <math.h>
#include <iostream>

#include "Libpfs/array2d.h"
#include "Libpfs/frame.h"
#include "Libpfs/progress.h"
#include "Libpfs/utils/msec_timer.h"
#include "tmo_vanhateren06.h"

namespace {
}

using namespace pfs;
using namespace std;

int tmo_vanhateren06(Array2Df *X, Array2Df *Y, Array2Df *Z, Array2Df *L,
                    float pupil_area,
                    Progress &ph) {
#ifdef TIMER_PROFILING
    msec_timer stop_watch;
    stop_watch.start();
#endif
    assert(X != NULL);
    assert(Y != NULL);
    assert(Z != NULL);
    assert(L != NULL);


#ifdef TIMER_PROFILING
    stop_watch.stop_and_update();
    cout << endl;
    cout << "tmo_vanhateren06 = " << stop_watch.get_time() << " msec" << endl;
#endif

    return 0;
}
