/*
 * This file is a part of Luminance HDR package
 * ----------------------------------------------------------------------
 * Copyright (C) 2012 Davide Anastasia
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

#include "PrintArray2D.h"
#include <Libpfs/array2d.h>

void print(const pfs::Array2Df& a2d)
{
    using namespace pfs;

    for (int r = 0; r < a2d.getRows(); r++)
    {
        Array2Df::const_iterator currSample = a2d.row_begin(r);
        Array2Df::const_iterator endSample = a2d.row_end(r);

        std::cout << "[";
        while (currSample != endSample)
        {
            std::cout << " " << *currSample++ << " ";
        }
        std::cout << "]" << std::endl;
    }
}

void print(const std::vector<float>& vecF)
{
    std::vector<float>::const_iterator currSample = vecF.begin();
    std::vector<float>::const_iterator endSample = vecF.end();

    std::cout << "[";
    while (currSample != endSample)
    {
        std::cout << " " << *currSample++ << " ";
    }
    std::cout << "]" << std::endl;
}
