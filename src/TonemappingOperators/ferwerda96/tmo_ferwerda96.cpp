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
            y = powf(2.7f, (0.249f * t + 0.65f)) - 0.72f;

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

struct Clamp {
    Clamp() {}
    float operator()(float in) const {
        if (in < 0.f) return 0.f;
        if (in > 1.f) return 1.f;
        return in;
    }
} clampfunc;
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

    float Ld_Max = *max_element(L->begin(), L->end());
    float coeff = 1.f / Ld_Max;

    float L_wa = mul1 * Ld_Max * .5f;
    float L_da = 2.0f * mul2 * Ld_Max * .5f;

    float maxX = *max_element(X->begin(), X->end());
    float maxY = *max_element(X->begin(), X->end());
    float maxZ = *max_element(X->begin(), X->end());

    cout << "Ld_Max " << Ld_Max << endl;
    cout << "L_wa " << L_wa << endl;
    cout << "L_da " << L_da << endl;
    cout << "MaxX " << *max_element(X->begin(), X->end()) << endl;
    cout << "MaxY " << *max_element(Y->begin(), Y->end()) << endl;
    cout << "MaxZ " << *max_element(Z->begin(), Z->end()) << endl;
    cout << "MinX " << *min_element(X->begin(), X->end()) << endl;
    cout << "MinY " << *min_element(Y->begin(), Y->end()) << endl;
    cout << "MinZ " << *min_element(Z->begin(), Z->end()) << endl;

    //compute the scaling factors
    float mC = TpFerwerda(L_da) / TpFerwerda(L_wa); //cones
    float mR = TsFerwerda(L_da) / TsFerwerda(L_wa); //rods
    float k = WalravenValeton_k(L_wa, L_da);

    cout << "mC " << mC << endl;
    cout << "mR " << mR << endl;
    cout << "k  " << k << endl;

    float vec[3] = {1.05f, 0.97f, 1.27f};
    float maxC[3] = {maxX, maxY, maxZ};

    const int channels = 3;
    Array2Df *Ch[channels] = {X, Y, Z};
    for (int c = 0; c < channels; c++) {
            float v = vec[c];
            float coeff = 1.f / maxC[c];
            transform(Ch[c]->begin(), Ch[c]->end(), L->begin(), Ch[c]->begin(),
                      [mC, mR, k, v](float a, float L) { return (mC * a + v * mR * k * L); } );
            transform(Ch[c]->begin(), Ch[c]->end(), Ch[c]->begin(),
                      [coeff](float a){ return a*coeff; } );
    }

#ifdef TIMER_PROFILING
    stop_watch.stop_and_update();
    cout << endl;
    cout << "tmo_ferwerda96 = " << stop_watch.get_time() << " msec" << endl;
#endif

    return 0;
}
