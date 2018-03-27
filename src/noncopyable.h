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
 *  This file was copied from RawTherapee on 23 Nov 2017, commit d61df9d
 *
 *  Copyright (c) 2016 Flössie <floessie.mail@gmail.com>
*/

#pragma once

namespace lhdrengine
{

class NonCopyable
{
public:
    NonCopyable() = default;

    explicit NonCopyable(const NonCopyable&) = delete;
    NonCopyable& operator =(const NonCopyable&) = delete;
};

}
