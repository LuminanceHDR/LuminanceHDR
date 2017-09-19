/**
 * This file is a part of Luminance HDR package
 * ----------------------------------------------------------------------
 * Copyright (C) 2013 Franco Comida
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
 * Manual and auto anti-ghosting functions
 * @author Franco Comida <fcomida@users.sourceforge.net>
 *
 */

#include "HdrCreationItem.h"

#define agGridSize 40

using namespace std;
using namespace pfs;

float max(const Array2Df &u);
float min(const Array2Df &u);
void solve_pde_dct(Array2Df &F, Array2Df &U);
void clampToZero(Array2Df &R, Array2Df &G, Array2Df &B, float m);
int findIndex(const float *data, int size);
void hueSquaredMean(const HdrCreationItemContainer &data, vector<float> &HE);
void sdv(const HdrCreationItem &item1, const HdrCreationItem &item2,
         const float deltaEV, const int dx, const int dy, float &sR, float &sG,
         float &sB);

bool comparePatches(const HdrCreationItem &item1, const HdrCreationItem &item2,
                    const int i, const int j, const int gridX, const int gridY,
                    const float threshold, const float sR, const float sG,
                    const float sB, const float deltaEV, const int dx,
                    const int dy);

void computeIrradiance(Array2Df &irradiance, const Array2Df &in);
void computeLogIrradiance(Array2Df &logIrradiance, const Array2Df &u);

void computeGradient(Array2Df &gradientX, Array2Df &gradientY,
                     const Array2Df &in);
void computeDivergence(Array2Df &divergence, const Array2Df &gradientX,
                       const Array2Df &gradientY);

void blendGradients(Array2Df &gradientXBlended, Array2Df &gradientYBlended,
                    const Array2Df &gradientX, const Array2Df &gradientY,
                    const Array2Df &gradientXGood,
                    const Array2Df &gradientYGood,
                    bool patches[agGridSize][agGridSize], int gridX, int gridY);

void blendGradients(Array2Df &gradientXBlended, Array2Df &gradientYBlended,
                    const Array2Df &gradientX, const Array2Df &gradientY,
                    const Array2Df &gradientXGood,
                    const Array2Df &gradientYGood, const QImage &agMask);

void colorBalance(pfs::Array2Df &U, const pfs::Array2Df &F, int x, int y);
qreal averageLightness(const Array2Df &R, const Array2Df &G, const Array2Df &B,
                       const int i, const int j, const int gridX,
                       const int gridY);
qreal averageLightness(const Array2Df &R, const Array2Df &G, const Array2Df &B);
qreal averageLightness(const HdrCreationItem &item);
void blend(pfs::Array2Df &R1, pfs::Array2Df &G1, pfs::Array2Df &B1,
           const pfs::Array2Df &R2, const pfs::Array2Df &G2,
           const pfs::Array2Df &B2, const QImage &mask,
           const QImage &maskGoodImage);
