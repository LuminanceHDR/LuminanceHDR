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

#include <boost/any.hpp>
#include <cassert>
#include <iostream>
#include <map>
#include <string>

#include <Libpfs/utils/string.h>

namespace pfs {

struct Param {
    Param() : value_() {}
    Param(const boost::any &value) : value_(value) {}

    template <typename Type>
    Param(const Type &value) : value_(value) {}

    template <typename Type>
    const Type &as(const Type &defaultValue) const {
        try {
            return boost::any_cast<const Type &>(value_);
        } catch (const boost::bad_any_cast &ex) {
#ifndef NDEBUG
            std::cerr << ex.what() << std::endl;
#endif
            return defaultValue;
        }
    }

    //! \throws boost::::bad_any_cast
    template <typename Type>
    const Type &as() const {
        return boost::any_cast<const Type &>(value_);
    }

    operator const boost::any &() const { return value_; }

   private:
    boost::any value_;
};

class Params {
   public:
    typedef std::map<std::string, Param, utils::StringUnsensitiveComp>
        ParamsHolder;
    typedef ParamsHolder::iterator iterator;
    typedef ParamsHolder::const_iterator const_iterator;

    //! \brief empty parameters holder ctor
    Params() : holder_() {}
    //! \brief copy ctor
    Params(const ParamsHolder &params) : holder_(params) {}
    //! \brief single key,value pair ctor
    Params(const std::string &key, const Param &value) : holder_() {
        set(key, value);
    }

    // pfs::Params()(key, value)(key, value)( ... )
    // or
    // pfs::Params(key, value)(key, value)( ... )
    Params &operator()(const std::string &key, const Param &value) {
        return set(key, value);
    }

    Params &set(const std::string &key, const Param &value) {
        holder_[key] = value;
        return *this;
    }

    ParamsHolder::size_type count(const std::string &key) const {
        return holder_.count(key);
    }

    template <typename Type>
    bool get(const std::string &key, Type &value) const {
        ParamsHolder::const_iterator it = holder_.find(key);
        if (it == holder_.end()) {
            return false;
        }

        try {
            value = it->second.as<Type>();
            return true;
        } catch (boost::bad_any_cast &ex) {
            return false;
        }
    }

    operator const ParamsHolder &() { return holder_; }

    iterator begin() { return holder_.begin(); }
    iterator end() { return holder_.end(); }

    const_iterator begin() const { return holder_.begin(); }
    const_iterator end() const { return holder_.end(); }

   private:
    ParamsHolder holder_;
};

}  // pfs

#endif  // LIBPFS_PARAMS_H
