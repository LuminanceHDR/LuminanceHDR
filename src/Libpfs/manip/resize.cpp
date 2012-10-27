/*
 * This file is a part of Luminance HDR package.
 * ---------------------------------------------------------------------- 
 * Copyright (C) 2003,2004 Rafal Mantiuk and Grzegorz Krawczyk
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

//! \brief Resize images in PFS stream
//! \author Rafal Mantiuk, <mantiuk@mpi-sb.mpg.de>
//! \author Davide Anastasia <davideanastasia@users.sourceforge.net>

#include <cmath>
#include <cassert>
#include <iostream>
#include <algorithm>
#include <numeric>
#include <cmath>

#include "resize.h"

#include "Common/msec_timer.h"
#include "Libpfs/array2d.h"
#include "Libpfs/frame.h"
#include "Libpfs/domio.h"
#include "Libpfs/manip/copy.h"

namespace pfs
{
namespace
{
const size_t BLOCK_FACTOR = 96;

//! \author Davide Anastasia <davideanastasia@users.sourceforge.net>
//! \note Code derived from
//! http://tech-algorithm.com/articles/bilinear-image-scaling/
//! with added OpenMP support and block based resampling
template <typename _Type>
void resizeBilinearGray(const _Type* pixels, _Type* output,
                        size_t w, size_t h, size_t w2, size_t h2)
{
    const float x_ratio = static_cast<float>(w - 1)/w2;
    const float y_ratio = static_cast<float>(h - 1)/h2;

    _Type A, B, C, D;
    _Type outputPixel;
    size_t x = 0;
    size_t y = 0;
    size_t index = 0;

    float x_diff = 0.0f;
    float y_diff = 0.0f;

#pragma omp parallel \
    shared(pixels, output, w, h, w2, h2) \
    private(x_diff, y_diff, x, y, index, outputPixel, A, B, C, D)
    {
#pragma omp for schedule(static, 1)
        for ( int iO = 0; iO < static_cast<int>(h2); iO += BLOCK_FACTOR )
        {
            for ( int jO = 0; jO < static_cast<int>(w2); jO += BLOCK_FACTOR )
            {
                for (size_t i = iO, iEnd = std::min(iO + BLOCK_FACTOR, h2);
                     i < iEnd;
                     i++)
                {
                    y = static_cast<size_t>(y_ratio * i);
                    y_diff = (y_ratio * i) - y;

                    for (size_t j = jO, jEnd = std::min(jO + BLOCK_FACTOR, w2);
                         j < jEnd;
                         j++)
                    {
                        x = static_cast<size_t>(x_ratio * j);
                        x_diff = (x_ratio * j) - x;

                        index = y*w + x;

                        A = pixels[index];
                        B = pixels[index + 1];
                        C = pixels[index + w];
                        D = pixels[index + w + 1];

                        // Y = A(1-w)(1-h) + B(w)(1-h) + C(h)(1-w) + D(w)(h)
                        outputPixel =
                                static_cast<_Type>(
                                    A*(1-x_diff)*(1-y_diff) +
                                    B*(x_diff)*(1-y_diff) +
                                    C*(y_diff)*(1-x_diff) +
                                    D*(x_diff*y_diff) );

                        output[i*w2 + j] = outputPixel;
                    }
                }
            }
        }
    } // end parallel region
}

void resample(const pfs::Array2D *in, pfs::Array2D *out)
{
    resizeBilinearGray(in->getRawData(), out->getRawData(),
                       in->getCols(), in->getRows(),
                       out->getCols(), out->getRows());
}

} // anonymous

void resize(const pfs::Array2D *in, pfs::Array2D *out )
{
    if ( in->getCols() == out->getCols() && in->getRows() == out->getRows() )
    {
        pfs::copy(in, out);
    }
    else
    {
        resample(in, out);
    }
}

Frame* resize(Frame* frame, int xSize)
{
#ifdef TIMER_PROFILING
    msec_timer f_timer;
    f_timer.start();
#endif

    int new_x = xSize;
    int new_y = (int)((float)frame->getHeight() * (float)xSize / (float)frame->getWidth());

    pfs::Frame *resizedFrame = pfs::DOMIO::createFrame( new_x, new_y );

    const ChannelContainer& channels = frame->getChannels();
    for ( ChannelContainer::const_iterator it = channels.begin();
          it != channels.end();
          ++it)
    {
        const pfs::Channel* originalCh = *it;
        pfs::Channel* newCh = resizedFrame->createChannel( originalCh->getName() );

        resize( originalCh->getChannelData(), newCh->getChannelData() );
    }
    pfs::copyTags( frame, resizedFrame );

#ifdef TIMER_PROFILING
    f_timer.stop_and_update();
    std::cout << "resizeFrame() = " << f_timer.get_time() << " msec" << std::endl;
#endif 

    return resizedFrame;
}

} // pfs
