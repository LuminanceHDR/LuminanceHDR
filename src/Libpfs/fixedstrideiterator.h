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

#ifndef PFS_FIXEDSTRIDEITERATOR_H
#define PFS_FIXEDSTRIDEITERATOR_H

#include <cassert>
#include <iterator>

namespace pfs {

//! \author Davide Anastasia <davideanastasia@users.sourceforge.net>
//! \brief stride iterator
//! \ref C++ Cookbook
//! \ref http://zotu.blogspot.co.uk/2010/01/creating-random-access-iterator.html
template <typename IterType, size_t StepSize>
class FixedStrideIterator
    : public std::iterator<std::random_access_iterator_tag, IterType> {
   public:
    // public typedefs
    typedef FixedStrideIterator<IterType, StepSize> self;
    typedef typename std::iterator<std::random_access_iterator_tag, IterType>
        iterator_base;
    typedef typename std::random_access_iterator_tag iterator_category;

    typedef typename std::iterator_traits<IterType>::value_type value_type;
    typedef typename std::iterator_traits<IterType>::reference reference;
    typedef typename std::iterator_traits<IterType>::difference_type
        difference_type;
    typedef typename std::iterator_traits<IterType>::pointer pointer;

    // ctors
    FixedStrideIterator() : m_data() {}
    FixedStrideIterator(const self &rhs) : m_data(rhs.m_data) {}
    FixedStrideIterator(IterType x) : m_data(x) {}

    // operators
    //! \brief Prefix increment
    self &operator++() {
        m_data += StepSize;
        return *this;
    }

    //! \brief Postfix increment
    self operator++(int) {
        self tmp(*this);
        m_data += StepSize;
        return tmp;
    }

    //! \brief Increment
    self &operator+=(const difference_type &n) {
        m_data += (n * StepSize);
        return *this;
    }

    //! \brief Sum
    self operator+(const difference_type &n) const {
        return self(m_data + n * StepSize);
    }

    //! \brief Prefix decrement
    self &operator--() {
        m_data -= StepSize;
        return *this;
    }

    //! \brief Postfix decrement
    self operator--(int) {
        self tmp(*this);
        m_data -= StepSize;
        return tmp;
    }

    //! \brief Decrement
    self &operator-=(const difference_type &n) {
        m_data -= (n * StepSize);
        return *this;
    }

    //! \brief Difference
    self operator-(const difference_type &n) const {
        return self(m_data - n * StepSize);
    }

    //! \brief dereferencing
    reference operator*() const { return *m_data; }

    //! \brief pointer
    pointer operator->() const { return m_data; }

    //! \brief subscription
    reference operator[](const difference_type &n) {
        return m_data[n * StepSize];
    }

    // friend operators
    template <typename IType, size_t SSize>
    friend bool operator==(const FixedStrideIterator<IType, SSize> &x,
                           const FixedStrideIterator<IType, SSize> &y);

    template <typename IType, size_t SSize>
    friend bool operator!=(const FixedStrideIterator<IType, SSize> &x,
                           const FixedStrideIterator<IType, SSize> &y);

    template <typename IType, size_t SSize>
    friend bool operator<(const FixedStrideIterator<IType, SSize> &x,
                          const FixedStrideIterator<IType, SSize> &y);

    template <typename IType, size_t SSize>
    friend bool operator<=(const FixedStrideIterator<IType, SSize> &x,
                           const FixedStrideIterator<IType, SSize> &y);

    template <typename IType, size_t SSize>
    friend bool operator>(const FixedStrideIterator<IType, SSize> &x,
                          const FixedStrideIterator<IType, SSize> &y);

    template <typename IType, size_t SSize>
    friend bool operator>=(const FixedStrideIterator<IType, SSize> &x,
                           const FixedStrideIterator<IType, SSize> &y);

    template <typename IType, size_t SSize>
    friend typename FixedStrideIterator<IType, SSize>::difference_type
    operator+(const FixedStrideIterator<IType, SSize> &x,
              const FixedStrideIterator<IType, SSize> &y);

    template <typename IType, size_t SSize>
    friend typename FixedStrideIterator<IType, SSize>::difference_type
    operator-(const FixedStrideIterator<IType, SSize> &x,
              const FixedStrideIterator<IType, SSize> &y);

   private:
    IterType m_data;
};

template <typename IType, size_t SSize>
inline bool operator==(const FixedStrideIterator<IType, SSize> &x,
                       const FixedStrideIterator<IType, SSize> &y) {
    return (x.m_data == y.m_data);
}

template <typename IType, size_t SSize>
inline bool operator!=(const FixedStrideIterator<IType, SSize> &x,
                       const FixedStrideIterator<IType, SSize> &y) {
    return (x.m_data != y.m_data);
}

template <typename IType, size_t SSize>
inline bool operator<(const FixedStrideIterator<IType, SSize> &x,
                      const FixedStrideIterator<IType, SSize> &y) {
    return (x.m_data < y.m_data);
}

template <typename IType, size_t SSize>
inline bool operator<=(const FixedStrideIterator<IType, SSize> &x,
                       const FixedStrideIterator<IType, SSize> &y) {
    return (x.m_data <= y.m_data);
}

template <typename IType, size_t SSize>
inline bool operator>(const FixedStrideIterator<IType, SSize> &x,
                      const FixedStrideIterator<IType, SSize> &y) {
    return (x.m_data > y.m_data);
}

template <typename IType, size_t SSize>
inline bool operator>=(const FixedStrideIterator<IType, SSize> &x,
                       const FixedStrideIterator<IType, SSize> &y) {
    return (x.m_data >= y.m_data);
}

template <typename IType, size_t SSize>
inline typename FixedStrideIterator<IType, SSize>::difference_type operator+(
    const FixedStrideIterator<IType, SSize> &x,
    const FixedStrideIterator<IType, SSize> &y) {
    return (x.m_data + y.m_data) / SSize;
}

template <typename IType, size_t SSize>
inline typename FixedStrideIterator<IType, SSize>::difference_type operator-(
    const FixedStrideIterator<IType, SSize> &x,
    const FixedStrideIterator<IType, SSize> &y) {
    return (x.m_data - y.m_data) / SSize;
}

}  // pfs

#endif  // PFS_FIXEDSTRIDEITERATOR_H
