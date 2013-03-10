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

//! \brief Type safe variant container for generic function parameters
//! \author Davide Anastasia <davideanastasia@users.sourceforge.net>

#ifndef LIBPFS_PARAMS_H
#define LIBPFS_PARAMS_H

#include <map>
#include <iostream>
#include <string>
#include <boost/any.hpp>

namespace pfs {

struct StringUnsensitiveComp {
    bool operator()(const std::string& str1, const std::string& str2) const;
};

struct Param
{
    Param()
        : value_() {}
    Param(const boost::any& value)
        : value_(value) {}

    template <typename Type>
    Param(const Type& value)
        : value_(value) {}

    template <typename Type>
    const Type& as(const Type& defaultValue) const {
        try {
            return boost::any_cast<const Type&>(value_);
        }
        catch (const boost::bad_any_cast & ex)
        {
#ifndef NDEBUG
            std::cerr << ex.what() << std::endl;
#endif
            return defaultValue;
        }
    }

    //! \throws boost::bad_any_cast
    template <typename Type>
    const Type& as() const {
        return boost::any_cast<const Type&>(value_);
    }

    operator const boost::any& () const {
        return value_;
    }

private:
    boost::any value_;
};


class Params
{
public:
    typedef std::map< std::string, Param, StringUnsensitiveComp > ParamsHolder;
    typedef ParamsHolder::iterator iterator;
    typedef ParamsHolder::const_iterator const_iterator;

    Params()
        : holder_() {}
    Params(const ParamsHolder& params)
        : holder_(params) {}

    Params& insert(const std::string& key, const Param& value)
    {
        holder_[key] = value;

        return *this;
        // holder_.insert( ParamsHolder::value_type(key, value) );
    }

    operator const ParamsHolder& () {
        return holder_;
    }

    iterator begin() { return holder_.begin(); }
    iterator end() { return holder_.end(); }

    const_iterator begin() const { return holder_.begin(); }
    const_iterator end() const { return holder_.end(); }

private:
    ParamsHolder holder_;
};



} // pfs

#endif // LIBPFS_PARAMS_H
