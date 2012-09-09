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

#include <vector>
#include <list>

//! \brief Pyramid item, containing its sizes and X and Y gradients
class PyramidS
{
public:
    typedef std::vector<float> Vector;

    //! \brief Allocate a properly sized Pyramid level
    PyramidS(size_t rows, size_t cols);

    size_t getRows() const { return m_rows; }
    size_t getCols() const { return m_cols; }
    size_t size() const { return m_rows*m_cols; }

    const float* gX() const { return m_Gx.data(); }
    float* gX() { return m_Gx.data(); }

    const float* gY() const { return m_Gy.data(); }
    float* gY() { return m_Gy.data(); }

    //! \brief transform both X and Y gradients in the R space
    //! \note Please refer to the original paper for the meaning of R and G
    void transformToR(float detailFactor);

    //! \brief transform both X and Y gradients in the G space
    //! \note Please refer to the original paper for the meaning of R and G
    void transformToG(float detailFactor);

    void scale(float multiplier);
    void multiply(const PyramidS& multiplier);

private:
    size_t m_rows;
    size_t m_cols;
    Vector m_Gx;
    Vector m_Gy;
};

class PyramidT
{
public:
    typedef std::vector< PyramidS > PyramidContainer;

    // iterator
    typedef PyramidContainer::iterator iterator;

    iterator begin() { return m_pyramid.begin(); }
    iterator end() { return m_pyramid.end(); }

    // const iterator
    typedef PyramidContainer::const_iterator const_iterator;

    const_iterator begin() const { return m_pyramid.begin(); }
    const_iterator end() const { return m_pyramid.end(); }

    // builds a
    PyramidT(size_t rows, size_t cols);

    size_t getRows() const { return m_rows; }
    size_t getCols() const { return m_cols; }
    size_t getElems() const { return m_rows*m_cols; }

    inline
    size_t numLevels() const { return m_pyramid.size(); }

    //! \brief fill all the levels of the pyramid based on the data inside
    //! the supplied vector (same size of the first level of the \c PyramidT)
    //! \param[in] data input vector of data
    void computeGradients(const float* data);

    //! \param[out] data input vector of data
    void computeSumOfDivergence(float* data);

    //! \param[out] result PyramidT structure that contains the scaling factors!
    void computeScaleFactors( PyramidT& result ) const;

    //! \brief transform every level of the Pyramid in the R space
    //! \note Please refer to the original paper for the meaning of R and G
    void transformToR(float detailFactor);

    //! \brief transform every level of the Pyramid in the G space
    //! \note Please refer to the original paper for the meaning of R and G
    void transformToG(float detailFactor);

    void scale(float multiplier);
    void multiply(const PyramidT& multiplier);

private:
    //! \brief number of rows for the higher level of the pyramid
    size_t m_rows;
    //! \brief number of cols for the higher level of the pyramid
    size_t m_cols;
    //! \brief container of PyramidS
    PyramidContainer m_pyramid;
};

// free functions (mostly in the header file to improve testability)

//! \brief downsample the frame contained in inputData and stores the result
//! inside outputData
void matrixDownsample(size_t inCols, size_t inRows,
                      const float* inputData,
                      float* outputData);

void matrixUpsample(size_t outCols, size_t outRows,
                    const float* inputData, float* outputData);

//! \brief calculate X and Y gradients and stores inside the \c gxData and
//! \c gyData
void calculateGradients(size_t cols, size_t rows,
                        const float* inputData,
                        float* gxData, float* gyData);

void calculateAndAddDivergence(size_t cols, size_t rows,
                               const float* Gx, const float* Gy, float* divG);

void calculateScaleFactor(const float* G, float* C, size_t size);

//! \brief transform gradient (Gx,Gy) to R
void transformToR(float* G, float detailFactor, size_t size);

//! \brief transform from R to G
void transformToG(float* R, float detailFactor, size_t size);

#endif // MANTIUK06_PYRAMID_H
