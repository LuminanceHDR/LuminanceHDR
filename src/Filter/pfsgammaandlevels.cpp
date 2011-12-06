/**
 * @brief apply gamma and level filtering to a frame
 *
 * ----------------------------------------------------------------------
 * Copyright (C) 2011 Davide Anastasia
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
 * @author Davide Anastasia
 * builds a copy of the input frame and applies gamma and in/out black/white points
 *
 */

#include <cmath>
#include <iostream>

#include <QDebug>

#include "Libpfs/frame.h"
#include "Libpfs/channel.h"
#include "Libpfs/domio.h"
#include "Common/msec_timer.h"

namespace
{

template <typename T>
inline T clamp(const T& v, const T& lower_bound, const T& upper_bound)
{
    if ( v <= lower_bound ) return lower_bound;
    if ( v >= upper_bound ) return upper_bound;
    return v;
}

//! \note I assume that *in* contains only value between [0,1]
void gamma_levels_array(const pfs::Array2D* in, pfs::Array2D* out,
                        float black_in, float white_in,
                        float black_out, float white_out, float gamma)
{
    // same formula used inside GammaAndLevels::refreshLUT()
    //float value = powf( ( ((float)(i)/255.0f) - bin ) / (win-bin), expgamma);
    //LUT[i] = clamp(blackout+value*(whiteout-blackout),0,255);

    const float* in_vector = in->getRawData();
    float* out_vector = out->getRawData();

    const int ELEMS = in->getCols()*in->getRows();

    if (gamma != 1.0f)
    {
#pragma omp parallel for
        for (int idx = 0; idx < ELEMS; ++idx)
        {
            float tmp = (in_vector[idx] - black_in)/(white_in - black_in);
            tmp = powf(tmp, gamma);

            tmp = black_out + tmp*(white_out-black_out);

            out_vector[idx] = clamp(tmp, 0.0f, 1.0f);
        }
    }
    else
    {
#pragma omp parallel for
        for (int idx = 0; idx < ELEMS; ++idx)
        {
            float tmp = (in_vector[idx] - black_in)/(white_in - black_in);
            //tmp = powf(tmp, gamma);

            tmp = black_out + tmp*(white_out-black_out);

            out_vector[idx] = clamp(tmp, 0.0f, 1.0f);
        }
    }
}

}

namespace pfs
{

pfs::Frame* gamma_levels(pfs::Frame* inFrame, float black_in, float white_in,
                         float black_out, float white_out, float gamma)
{
#ifdef TIMER_PROFILING
    msec_timer f_timer;
    f_timer.start();
#endif

    qDebug() << "Black in =" << black_in << "black out =" << black_out
             << "White in =" << white_in << "white out =" << white_out
             << "Gamma =" << gamma;

    pfs::DOMIO pfsio;

    const int outWidth   = inFrame->getWidth();
    const int outHeight  = inFrame->getHeight();

    pfs::Frame *outFrame = pfsio.createFrame(outWidth, outHeight);

    pfs::ChannelIterator *it = inFrame->getChannels();

    while (it->hasNext())
    {
      pfs::Channel *inCh  = it->getNext();
      pfs::Channel *outCh = outFrame->createChannel(inCh->getName());

      pfs::Array2D* inArray2D   = inCh->getChannelData();
      pfs::Array2D* outArray2D  = outCh->getChannelData();

      gamma_levels_array(inArray2D, outArray2D, black_in, white_in,
                         black_out, white_out, gamma);
    }

    pfs::copyTags(inFrame, outFrame);

#ifdef TIMER_PROFILING
    f_timer.stop_and_update();
    std::cout << "gamma_levels() = " << f_timer.get_time() << " msec" << std::endl;
#endif

    return outFrame;
}

}
