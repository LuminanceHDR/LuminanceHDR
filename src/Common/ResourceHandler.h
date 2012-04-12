#ifndef RESOURCEHANDLER_H
#define RESOURCEHANDLER_H

/*
 * This file is a part of LuminanceHDR package.
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

template<typename T>
struct ResourceHandlerTraits
{
    static
    void cleanup(T* p)
    {
        delete p;
    }
};

//! \brief This class resemble QScopedPointer or boost::scoped_ptr
//! however, it doesn't provide and operator*(), which allow to store
//! a pointer to void
template<typename T, typename Traits = ResourceHandlerTraits<T> >
class ResourceHandler
{
public:
    ResourceHandler(T* p = 0):
        p_(p)
    {}

    inline
    void reset(T* p = 0)
    {
        if (p_ != 0)
        {
            Traits::cleanup(p_);
        }
        p_ = p;
    }

    ~ResourceHandler()
    {
        if (p_ != NULL)
        {
            Traits::cleanup(p_);
        }
    }

    inline
    T* data()
    {
        return p_;
    }

    inline
    T* take()
    {
        T* old_p = p_;
        p_ = 0;
        return old_p;
    }

    inline
    operator bool()
    {
        return p_;
    }

    inline
    bool operator!()
    {
        return !p_;
    }


private:
    ResourceHandler(const ResourceHandler&);
    ResourceHandler& operator=(const ResourceHandler&);

    T* p_;
};

#endif // RESOURCEHANDLER_H
