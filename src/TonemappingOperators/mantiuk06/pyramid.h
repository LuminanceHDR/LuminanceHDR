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
 *
 */

//! \brief Pyramid implementation for Mantiuk06 tonemapping operator
//! \author Davide Anastasia <davideanastasia@users.sourceforge.net>
//! \date 09 Sept 2012
//! \note Implementation of this class is based on the original implementation
//! of the Mantiuk06 operator found in PFSTMO. However, the actual
//! implementation found in this file (and its .cpp file) is based on STL
//! containers

#ifndef MANTIUK06_PYRAMID_H
#define MANTIUK06_PYRAMID_H

#include <cstddef>
#include <vector>

#include "Libpfs/array2d.h"

class XYGradient {
   public:
    XYGradient() : m_Gx(), m_Gy() {}

    XYGradient(float gx, float gy) : m_Gx(gx), m_Gy(gy) {}

    XYGradient(float grad) : m_Gx(grad), m_Gy(grad) {}

    const float &gX() const { return m_Gx; }
    float &gX() { return m_Gx; }

    const float &gY() const { return m_Gy; }
    float &gY() { return m_Gy; }

    inline XYGradient &operator*=(float multiplier) {
        m_Gx *= multiplier;
        m_Gy *= multiplier;

        return *this;
    }

    inline XYGradient &operator*=(const XYGradient &multiplier) {
        m_Gx *= multiplier.m_Gx;
        m_Gy *= multiplier.m_Gy;

        return *this;
    }

   private:
    float m_Gx;
    float m_Gy;
};

inline XYGradient operator*(const XYGradient &x, const XYGradient &y) {
    return XYGradient(x.gX() * y.gX(), x.gY() * y.gY());
}

typedef ::pfs::Array2D<XYGradient> PyramidS;

class PyramidT {
   public:
    typedef std::vector<PyramidS> PyramidContainer;

    // iterator
    typedef PyramidContainer::iterator iterator;
    typedef PyramidContainer::const_iterator const_iterator;

    iterator begin() { return m_pyramid.begin(); }
    iterator end() { return m_pyramid.end(); }

    const_iterator begin() const { return m_pyramid.begin(); }
    const_iterator end() const { return m_pyramid.end(); }

    // builds a Pyramid
    PyramidT(size_t rows, size_t cols);

    inline size_t getRows() const { return m_rows; }
    inline size_t getCols() const { return m_cols; }
    inline size_t getElems() const { return m_rows * m_cols; }

    inline size_t numLevels() const { return m_pyramid.size(); }

    //! \brief fill all the levels of the pyramid based on the data inside
    //! the supplied vector (same size of the first level of the \c PyramidT)
    //! \param[in] data input vector of data
    void computeGradients(const pfs::Array2Df &inputData);

    //! \param[out] data input vector of data
    void computeSumOfDivergence(pfs::Array2Df &sumOfDivG);

    //! \param[out] result PyramidT structure that contains the scaling factors!
    void computeScaleFactors(PyramidT &result) const;

    //! \brief transform every level of the Pyramid in the R space
    //! \note Please refer to the original paper for the meaning of R and G
    void transformToR(float detailFactor);

    //! \brief transform every level of the Pyramid in the G space
    //! \note Please refer to the original paper for the meaning of R and G
    void transformToG(float detailFactor);

    void scale(float multiplier);
    void multiply(const PyramidT &multiplier);

   private:
    //! \brief number of rows for the higher level of the pyramid
    size_t m_rows;
    //! \brief number of cols for the higher level of the pyramid
    size_t m_cols;
    //! \brief container of PyramidS
    PyramidContainer m_pyramid;
};

// free functions (mostly in the header file to improve testability)
//! \brief downsample the image contained in \a inputData and stores the result
//! inside \a outputData
void matrixDownsample(size_t inCols, size_t inRows, const float *inputData,
                      float *outputData);

//! \brief upsample the image contained in \a inputData and stores the result
//! inside \a outputData
void matrixUpsample(size_t outCols, size_t outRows, const float *inputData,
                    float *outputData);

//! \brief compute X and Y gradients from \a inputData into \a gradient
void calculateGradients(const float *inputData, PyramidS &gradient);
//
void calculateAndAddDivergence(const PyramidS &G, float *divG);

//! \brief compute a scale factor based on the input \a g value
float calculateScaleFactor(float g);

//! \brief transform gradient \a G to R
struct TransformToR {
    TransformToR(float detailFactor);
    float operator()(float currG) const;

   private:
    float m_detailFactor;
};

//! \brief transform from \a R to G
struct TransformToG {
    TransformToG(float detailFactor);
    float operator()(float currR) const;

   private:
    float m_detailFactor;
};

#endif  // MANTIUK06_PYRAMID_H
