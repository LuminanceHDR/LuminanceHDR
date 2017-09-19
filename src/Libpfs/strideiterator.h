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

#ifndef PFS_STRIDE_ITERATOR_H
#define PFS_STRIDE_ITERATOR_H

#include <cassert>
#include <iterator>

namespace pfs {

//! \author Davide Anastasia <davideanastasia@users.sourceforge.net>
//! \brief stride iterator
//! \ref C++ Cookbook
//! \ref http://zotu.blogspot.co.uk/2010/01/creating-random-access-iterator.html
template <typename IterType>
class StrideIterator
    : public std::iterator<std::random_access_iterator_tag, IterType> {
   public:
    // public typedefs
    typedef StrideIterator<IterType> self;
    typedef typename std::iterator<std::random_access_iterator_tag, IterType>
        iterator_base;
    typedef typename std::random_access_iterator_tag iterator_category;

    typedef typename std::iterator_traits<IterType>::value_type value_type;
    typedef typename std::iterator_traits<IterType>::reference reference;
    typedef typename std::iterator_traits<IterType>::difference_type
        difference_type;
    typedef typename std::iterator_traits<IterType>::pointer pointer;

    // ctors
    StrideIterator() : m_data(), m_step() {}
    StrideIterator(const self &rhs) : m_data(rhs.m_data), m_step(rhs.m_step) {}
    StrideIterator(IterType x, difference_type step)
        : m_data(x), m_step(step) {}

    // operators
    //! \brief Prefix increment
    self &operator++() {
        m_data += m_step;
        return *this;
    }

    //! \brief Postfix increment
    self operator++(int) {
        self tmp(*this);
        m_data += m_step;
        return tmp;
    }

    //! \brief Increment
    self &operator+=(const difference_type &n) {
        m_data += (n * m_step);
        return *this;
    }

    //! \brief Sum
    self operator+(const difference_type &n) const {
        return self(m_data + n * m_step, m_step);
    }

    //! \brief Prefix decrement
    self &operator--() {
        m_data -= m_step;
        return *this;
    }

    //! \brief Postfix decrement
    self operator--(int) {
        self tmp(*this);
        m_data -= m_step;
        return tmp;
    }

    //! \brief Decrement
    self &operator-=(const difference_type &n) {
        m_data -= (n * m_step);
        return *this;
    }

    //! \brief Difference
    self operator-(const difference_type &n) const {
        return self(m_data - n * m_step, m_step);
    }

    //! \brief dereferencing
    reference operator*() const { return *m_data; }

    //! \brief pointer
    pointer operator->() const { return m_data; }

    //! \brief subscription
    reference operator[](const difference_type &n) {
        return m_data[n * m_step];
    }

    // friend operators
    template <typename Type>
    friend bool operator==(const StrideIterator<Type> &x,
                           const StrideIterator<Type> &y);

    template <typename Type>
    friend bool operator!=(const StrideIterator<Type> &x,
                           const StrideIterator<Type> &y);

    template <typename Type>
    friend bool operator<(const StrideIterator<Type> &x,
                          const StrideIterator<Type> &y);

    template <typename Type>
    friend bool operator<=(const StrideIterator<Type> &x,
                           const StrideIterator<Type> &y);

    template <typename Type>
    friend bool operator>(const StrideIterator<Type> &x,
                          const StrideIterator<Type> &y);

    template <typename Type>
    friend bool operator>=(const StrideIterator<Type> &x,
                           const StrideIterator<Type> &y);

    template <typename Type>
    friend typename StrideIterator<Type>::difference_type operator+(
        const StrideIterator<Type> &x, const StrideIterator<Type> &y);

    template <typename Type>
    friend typename StrideIterator<Type>::difference_type operator-(
        const StrideIterator<Type> &x, const StrideIterator<Type> &y);

   private:
    IterType m_data;
    difference_type m_step;
};

template <typename Type>
inline bool operator==(const StrideIterator<Type> &x,
                       const StrideIterator<Type> &y) {
    assert(x.m_step == y.m_step);
    return (x.m_data == y.m_data);
}

template <typename Type>
inline bool operator!=(const StrideIterator<Type> &x,
                       const StrideIterator<Type> &y) {
    assert(x.m_step == y.m_step);
    return (x.m_data != y.m_data);
}

template <typename Type>
inline bool operator<(const StrideIterator<Type> &x,
                      const StrideIterator<Type> &y) {
    assert(x.m_step == y.m_step);
    return (x.m_data < y.m_data);
}

template <typename Type>
inline bool operator<=(const StrideIterator<Type> &x,
                       const StrideIterator<Type> &y) {
    assert(x.m_step == y.m_step);
    return (x.m_data <= y.m_data);
}

template <typename Type>
inline bool operator>(const StrideIterator<Type> &x,
                      const StrideIterator<Type> &y) {
    assert(x.m_step == y.m_step);
    return (x.m_data > y.m_data);
}
template <typename Type>
inline bool operator>=(const StrideIterator<Type> &x,
                       const StrideIterator<Type> &y) {
    assert(x.m_step == y.m_step);
    return (x.m_data >= y.m_data);
}

template <typename Type>
inline typename StrideIterator<Type>::difference_type operator+(
    const StrideIterator<Type> &x, const StrideIterator<Type> &y) {
    assert(x.m_step == y.m_step);
    return (x.m_data + y.m_data) / x.m_step;
}

template <typename Type>
inline typename StrideIterator<Type>::difference_type operator-(
    const StrideIterator<Type> &x, const StrideIterator<Type> &y) {
    assert(x.m_step == y.m_step);
    return (x.m_data - y.m_data) / x.m_step;
}

}  // pfs

#endif  // PFS_STRIDE_ITERATOR_H
