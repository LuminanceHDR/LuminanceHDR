/*
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
 */

#include <gtest/gtest.h>

#include <Libpfs/array2d.h>
#include <TonemappingOperators/fattal02/pde.h>
#include <HdrWizard/AutoAntighosting.h>

#include <cmath>

TEST(solve_pde_dct, Test1)
{
    Array2Df U(100,100);
    Array2Df divergence(100,100);

    for (int j = 0; j < 100; j++)
    {
        for (int i = 0; i < 100; i++)
        {
            divergence(i, j) = std::exp(
                        -std::pow(-(i-50.f), 2.f)/0.2f -
                        std::pow(-(j-50.f), 2.f)/0.2f);
        }
    }

    solve_pde_dct(&divergence, &U);
    float residual = residual_pde(&U, &divergence);

    ASSERT_LE(residual, 1e-2);
}

