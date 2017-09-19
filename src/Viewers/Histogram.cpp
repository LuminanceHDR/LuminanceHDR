/**
 * @brief
 *
 * This file is a part of LuminanceHDR package.
 * ----------------------------------------------------------------------
 * Copyright (C) 2003,2004 Rafal Mantiuk and Grzegorz Krawczyk
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
 * @author Rafal Mantiuk, <mantiuk@mpi-sb.mpg.de>
 *
 * $Id: histogram.cpp,v 1.2 2005/09/02 13:10:35 rafm Exp $
 */

#include "Histogram.h"

#include <assert.h>
#include <math.h>

#include <Libpfs/array2d.h>

Histogram::Histogram(int bins, int accuracy)
    : bins(bins), accuracy(accuracy), P(new float[bins]) {}

Histogram::~Histogram() { delete[] P; }

void Histogram::computeLog(const pfs::Array2Df *image) {
    const int size = image->getRows() * image->getCols();

    float max, min;  // Find min, max
    {
        min = 999999999.0f;
        max = -999999999.0f;

        for (int i = 0; i < size; i += accuracy) {
            float v = (*image)(i);
            if (v > max)
                max = v;
            else if (v < min)
                min = v;
        }
    }
    computeLog(image, min, max);
}

void Histogram::computeLog(const pfs::Array2Df *image, float min, float max) {
    const int size = image->getRows() * image->getCols();

    // Empty all bins
    for (int i = 0; i < bins; i++) P[i] = 0;

    float count = 0;
    float binWidth = (max - min) / (float)bins;
    for (int i = 0; i < size; i += accuracy) {
        float v = (*image)(i);
        if (v <= 0) continue;
        v = log10(v);
        int bin = (int)((v - min) / binWidth);
        if (bin > bins || bin < 0) continue;
        if (bin == bins) bin = bins - 1;
        P[bin] += 1;
        count++;
    }

    // Normalize, to get probability
    for (int i = 0; i < bins; i++) P[i] /= (float)(count / accuracy);
}

float Histogram::getMaxP() const {
    float maxP = -1;
    for (int i = 0; i < bins; i++) {
        if (P[i] > maxP) {
            maxP = P[i];
        }
    }
    return maxP;
}
