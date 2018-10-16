/*
 * @brief Ferwerda Tone Mapping Operator:
 *    "A Model of Visual Adaptation for Realistic Image Synthesis"
 *     by James A. Ferwerda, Sumanta N. Pattanaik, Peter Shirley, Donald P. Greenberg
 *
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
#include "tmo_ferwerda96.h"

namespace {
    float TpFerwerda(float x)
    {
        float t = log10(x);

        float y;

        if(t <= -2.6f)
            y = -0.72f;
        else
            if(t >= 1.9f)
                y = t - 1.255f;
            else
                y = powf((0.249f * t + 0.65f), 2.7f) - 0.72f;

        return powf(10.f, y);
    }

    float TsFerwerda(float x)
    {
        float t = log10(x);

        float y;

        if(t <= -3.94f)
            y = -2.86f;
        else
            if(t >= -1.44f)
                y = t - 0.395f;
            else
                y = powf(2.18f, (0.405f * t + 1.6f)) - 2.86f;

        return powf(10.f, y);
    }

    float WalravenValeton_k(float Lwa, float wv_sigma)
    {
        float k = (wv_sigma - Lwa / 4.f) / (wv_sigma + Lwa);

        return (k < 0.0f) ? 0.0f : k;
    }
}

using namespace pfs;
using namespace std;

int tmo_ferwerda96(Array2Df *X, Array2Df *Y, Array2Df *Z, Array2Df *L,
                    float mul1, float mul2,
                    Progress &ph) {
#ifdef TIMER_PROFILING
    msec_timer stop_watch;
    stop_watch.start();
#endif
    assert(X != NULL);
    assert(Y != NULL);
    assert(Z != NULL);
    assert(L != NULL);

    mul1 *= 100.f;
    mul2 /= 1000.f;

    float Ld_Max = 100.f * mul1;
    float L_wa = *max_element(L->begin(), L->end()) * 0.5f * mul1;

    float L_da = mul2 * Ld_Max;

    //compute the scaling factors
    float mC = TpFerwerda(L_da) / TpFerwerda(L_wa); //cones
    float mR = TsFerwerda(L_da) / TsFerwerda(L_wa); //rods
    float k = WalravenValeton_k(L_wa, L_da);

    ph.setValue(2);
    if (ph.canceled()) return 0;

    float vec[3] = {1.05f, 0.97f, 1.27f};
    float maxX = *max_element(X->begin(), X->end());
    float maxY = *max_element(Y->begin(), Y->end());
    float maxZ = *max_element(Z->begin(), Z->end());
    float maxC = max(maxX, max(maxY, maxZ));
    float scale = 1.0f / maxC;

    ph.setValue(10);
    if (ph.canceled()) return 0;

    const int channels = 3;
    Array2Df *Ch[channels] = {X, Y, Z};
    for (int c = 0; c < channels; c++) {
        ph.setValue(10+(c+1)*30);
        if (ph.canceled()) return 0;

        transform(Ch[c]->begin(), Ch[c]->end(), L->begin(), Ch[c]->begin(),
                  [mC, mR, k, vec, c, scale](float a, float L) { return (mC * a + vec[c] * mR * k * L) * scale; } );
    }

#ifdef TIMER_PROFILING
    stop_watch.stop_and_update();
    cout << endl;
    cout << "tmo_ferwerda96 = " << stop_watch.get_time() << " msec" << endl;
#endif

    return 0;
}
