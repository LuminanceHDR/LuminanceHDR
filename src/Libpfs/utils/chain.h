/*
 * This file is a part of Luminance HDR package.
 * ----------------------------------------------------------------------
 * Copyright (C) 2013 Davide Anastasia
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

#ifndef PFS_UTILS_CHAIN_H
#define PFS_UTILS_CHAIN_H

namespace pfs {
namespace utils {

template <typename Func1, typename Func2>
struct Chain {
    Chain(const Func1& func1 = Func1(), const Func2& func2 = Func2())
        : func1_(func1), func2_(func2)
    {}

    template <typename TypeIn, typename TypeOut>
    void operator()(TypeIn v1, TypeIn v2, TypeIn v3,
                    TypeOut& o1, TypeOut& o2, TypeOut& o3) const
    {
        func1_(v1, v2, v3, v1, v2, v3);
        func2_(v1, v2, v3, o1, o2, o3);
    }

    template <typename Type>
    Type operator()(Type v1) const
    {
        return func2_(func1_(v1));
    }

private:
    Func1 func1_;
    Func2 func2_;
};

}   // utils
}   // pfs


#endif // PFS_UTILS_CHAIN_H
