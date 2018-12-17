/*
 *  This file is part of Luminance HDR.
 *
 *  Luminance HDR is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  Luminance HDR is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Luminance HDR. If not, see <http://www.gnu.org/licenses/>.
 *
 *  This file was copied from RawTherapee on 29 March 2018
 *
 *  Copyright (c) Ingo Weyrich <heckflosse67@gmx.de>
*/

#pragma once

#include <cstddef>

namespace lhdrengine
{

void findMinMaxPercentile(const float* data, size_t size, float minPrct, float& minOut, float maxPrct, float& maxOut, bool multiThread = true);
float accumulate(const float *array, size_t size, bool multithread = true);

}
