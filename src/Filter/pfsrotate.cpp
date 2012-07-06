/**
 * @brief Resize images in PFS stream
 * 
 * This file is a part of PFSTOOLS package.
 * ---------------------------------------------------------------------- 
 * Copyright (C) 2003,2004 Rafal Mantiuk and Grzegorz Krawczyk,
 *  Alexander Efremov
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
 * @author Alexander Efremov, <aefremov@mpi-sb.mpg.de>
 *
 * $Id: pfsrotate.cpp,v 1.1 2005/06/15 13:36:54 rafm Exp $
 */

#include <assert.h>
#include <iostream>

#include "pfsrotate.h"
#include "Common/msec_timer.h"
#include "Libpfs/domio.h"

namespace pfs
{
  
  pfs::Frame* rotateFrame(pfs::Frame* frame, bool clock_wise)
  {
#ifdef TIMER_PROFILING
    msec_timer f_timer;
    f_timer.start();
#endif

    int xSize = frame->getHeight();
    int ySize = frame->getWidth();
    pfs::Frame *resizedFrame = pfs::DOMIO::createFrame( xSize, ySize );
    
    const ChannelMap& channels = frame->getChannels();

    for ( ChannelMap::const_iterator it = channels.begin();
          it != channels.end();
          ++it)
    {
        const pfs::Channel *originalCh = it->second;
        pfs::Channel *newCh = resizedFrame->createChannel(originalCh->getName());

        rotateArray(originalCh->getChannelData(), newCh->getChannelData(), clock_wise);
    }
    
    pfs::copyTags( frame, resizedFrame );
    
#ifdef TIMER_PROFILING
    f_timer.stop_and_update();
    std::cout << "rotateFrame() = " << f_timer.get_time() << " msec" << std::endl;
#endif 
    
    return resizedFrame;
  }
  
  void rotateArray(const pfs::Array2D *in, pfs::Array2D *out, bool clockwise)
  {
      const float* Vin  = in->getRawData();
      float* Vout       = out->getRawData();

      const int I_ROWS = in->getRows();
      const int I_COLS = in->getCols();

      //const int O_ROWS = out->getRows();
      const int O_COLS = out->getCols();

      if (clockwise)
      {
#pragma omp parallel for
          for (int j = 0; j < I_ROWS; j++)
          {
              for (int i = 0; i < I_COLS; i++)
              {
                  Vout[(i+1)*O_COLS - 1 - j] = Vin[j*I_COLS + i];
              }
          }
      }
      else
      {
#pragma omp parallel for
          for (int j = 0; j < I_ROWS; j++)
          {
              for (int i = 0; i < I_COLS; i++)
              {
                  Vout[(I_COLS - i - 1)*O_COLS + j] = Vin[j*I_COLS + i];
              }
          }
      }
  }
  
}
