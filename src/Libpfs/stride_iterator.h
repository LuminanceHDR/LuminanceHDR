/*
 * This file is a part of Luminance HDR package
 * ----------------------------------------------------------------------
 * Copyright (C) 2012 Davide Anastasia
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * ----------------------------------------------------------------------
 */

#ifndef STRIDE_ITERATOR_H
#define STRIDE_ITERATOR_H

#include <iterator>
#include <cassert>

namespace pfs
{

//! \brief stride iterator
//! \ref C++ Cookbook
//! \ref http://zotu.blogspot.co.uk/2010/01/creating-random-access-iterator.html
template <typename IterType>
class stride_iterator
        : public std::iterator< std::random_access_iterator_tag, IterType>
{
public:
    // public typedefs
    typedef stride_iterator<IterType> self;
    typedef typename std::iterator< std::random_access_iterator_tag, IterType> iterator_base;
    typedef typename std::random_access_iterator_tag iterator_category;

    typedef typename std::iterator_traits<IterType>::value_type value_type;
    typedef typename std::iterator_traits<IterType>::reference reference;
    typedef typename std::iterator_traits<IterType>::difference_type difference_type;
    typedef typename std::iterator_traits<IterType>::pointer pointer;

    // ctors
    stride_iterator()
        : m_data()
        , m_step()
    {}
    stride_iterator(const self& rhs)
        : m_data(rhs.m_data)
        , m_step(rhs.m_step)
    {}
    stride_iterator(IterType x, difference_type step)
        : m_data(x)
        , m_step(step)
    {}

    // operators
    //! \brief Prefix increment
    self& operator++()
    { m_data += m_step; return *this; }
    //! \brief Postfix increment
    self operator++(int)
    {
        self tmp(*this);
        m_data += m_step;
        return tmp;
    }
    //! \brief Increment
    self& operator+=(const difference_type& n)
    {
        m_data += (n * m_step);
        return *this;
    }
    //! \brief Sum
    self operator+(const difference_type& n) const
    { return self(m_data + n*m_step, m_step); }

    //! \brief Prefix decrement
    self& operator--()
    { m_data -= m_step; return *this; }
    //! \brief Postfix decrement
    self operator--(int)
    {
        self tmp(*this);
        m_data -= m_step;
        return tmp;
    }
    //! \brief Decrement
    self& operator-=(const difference_type& n)
    {
        m_data -= (n * m_step);
        return *this;
    }
    //! \brief Difference
    self operator-(const difference_type& n) const
    { return self(m_data - n*m_step, m_step); }

    //! \brief dereferencing
    reference operator*() const
    { return *m_data; }
    //! \brief pointer
    pointer operator->() const
    { return m_data; }
    //! \brief subscription
    reference operator[](const difference_type& n)
    { return m_data[n * m_step]; }

    // friend operators
    template <typename Type>
    friend bool operator==(const stride_iterator<Type>& x, const stride_iterator<Type>& y);
    template <typename Type>
    friend bool operator!=(const stride_iterator<Type>& x, const stride_iterator<Type>& y);

    template <typename Type>
    friend bool operator<(const stride_iterator<Type>& x, const stride_iterator<Type>& y);

    template <typename Type>
    friend bool operator<=(const stride_iterator<Type>& x, const stride_iterator<Type>& y);

    template <typename Type>
    friend bool operator>(const stride_iterator<Type>& x, const stride_iterator<Type>& y);
    template <typename Type>
    friend bool operator>=(const stride_iterator<Type>& x, const stride_iterator<Type>& y);

    template <typename Type>
    friend typename stride_iterator<Type>::difference_type operator+(
            const stride_iterator<Type>& x, const stride_iterator<Type>& y);
    template <typename Type>
    friend typename stride_iterator<Type>::difference_type operator-(
            const stride_iterator<Type>& x, const stride_iterator<Type>& y);

private:
    IterType m_data;
    difference_type m_step;
};

// friend operators
template <typename IterType>
bool operator==(const stride_iterator<IterType>& x,
                const stride_iterator<IterType>& y)
{
    assert(x.m_step == y.m_step);
    return (x.m_data == y.m_data);
}

template <typename IterType>
bool operator!=(const stride_iterator<IterType>& x,
                const stride_iterator<IterType>& y)
{
    assert(x.m_step == y.m_step);
    return (x.m_data != y.m_data);
}

template <typename IterType>
bool operator<(const stride_iterator<IterType>& x,
               const stride_iterator<IterType>& y)
{
    assert(x.m_step == y.m_step);
    return (x.m_data < y.m_data);
}

template <typename IterType>
bool operator<=(const stride_iterator<IterType>& x,
                const stride_iterator<IterType>& y)
{
    assert(x.m_step == y.m_step);
    return (x.m_data <= y.m_data);
}

template <typename IterType>
bool operator>(const stride_iterator<IterType>& x,
               const stride_iterator<IterType>& y)
{
    assert(x.m_step == y.m_step);
    return (x.m_data > y.m_data);
}

template <typename IterType>
bool operator>=(const stride_iterator<IterType>& x,
                const stride_iterator<IterType>& y)
{
    assert(x.m_step == y.m_step);
    return (x.m_data >= y.m_data);
}

template <typename IterType>
typename stride_iterator<IterType>::difference_type operator+(
        const stride_iterator<IterType>& x,
        const stride_iterator<IterType>& y)
{
    assert(x.m_step == y.m_step);
    return (x.m_data + y.m_data) / x.m_step;
}

template <typename IterType>
typename stride_iterator<IterType>::difference_type operator-(
        const stride_iterator<IterType>& x,
        const stride_iterator<IterType>& y)
{
    assert(x.m_step == y.m_step);
    return (x.m_data - y.m_data) / x.m_step;
}

} // namespace pfs

#endif // STRIDE_ITERATOR_H
