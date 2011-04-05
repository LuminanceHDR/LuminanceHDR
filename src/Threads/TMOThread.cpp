/*
 * This file is a part of LuminanceHDR package.
 * ---------------------------------------------------------------------- 
 * Copyright (C) 2006,2007 Giuseppe Rota
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
 * Original Work
 * @author Giuseppe Rota <grota@users.sourceforge.net>
 * Improvements, bugfixing 
 * @author Franco Comida <fcomida@users.sourceforge.net>
 *
 */

#include <iostream>

#include "TMOThread.h"
#include "Libpfs/colorspace.h"
#include "Common/config.h"
#include "Filter/pfscut.h"
#include "Filter/pfsgamma.h"
#include "Filter/pfssize.h"
#include "Fileformat/pfsoutldrimage.h"

TMOThread::TMOThread(pfs::Frame *frame, const TonemappingOptions &opts) :
QThread(0), opts(opts), out_CS(pfs::CS_RGB)
{  
	ph = new ProgressHelper(0);
  
  if ( opts.tonemapSelection )
  {
    // workingframe = "crop"
    // std::cout << "crop:[" << opts.selection_x_up_left <<", " << opts.selection_y_up_left <<"],";
    // std::cout << "[" << opts.selection_x_bottom_right <<", " << opts.selection_y_bottom_right <<"]" << std::endl;
    workingframe = pfs::pfscut(frame, opts.selection_x_up_left, opts.selection_y_up_left, opts.selection_x_bottom_right, opts.selection_y_bottom_right);
  }
	else if ( opts.xsize != opts.origxsize )
  {
    // workingframe = "resize"
    workingframe = pfs::resizeFrame(frame, opts.xsize);
	}
  else
  {
    // workingframe = "full res"
    workingframe = pfs::pfscopy(frame); 
  }
  
	// Convert to CS_XYZ: tm operator now use this colorspace
	pfs::Channel *X, *Y, *Z;
	workingframe->getXYZChannels( X, Y, Z );
	pfs::transformColorSpace(pfs::CS_RGB, X->getChannelData(), Y->getChannelData(), Z->getChannelData(),
                           pfs::CS_XYZ, X->getChannelData(), Y->getChannelData(), Z->getChannelData());	

  if (opts.pregamma != 1.0f)
  {
    pfs::applyGammaOnFrame( workingframe, opts.pregamma );
	}
  
}

TMOThread::~TMOThread()
{

        this->wait();   // waits that all the signals have been served
	delete ph;

        std::cout << "TMOThread::~TMOThread()" << std::endl;
}

void TMOThread::terminateRequested()
{
	//std::cout << "TMOThread::terminateRequested()" << std::endl;
	ph->terminate(true);
}

void TMOThread::startTonemapping()
{
  this->start();
}

void TMOThread::finalize()
{
    LuminanceOptions *luminance_options = LuminanceOptions::getInstance();

    if (!(ph->isTerminationRequested()))
    {
        QImage* res = fromLDRPFStoQImage(workingframe, out_CS);

        switch (m_tmo_thread_mode) {
        case TMO_BATCH:
            {
                // different emit signal
                // I let the parent of this thread to delete working_frame
                pfs::DOMIO pfsio;
                pfsio.freeFrame(workingframe);
                emit imageComputed(res, &opts);
            }
            break;
        case TMO_INTERACTIVE:
        default:
            {
                emit imageComputed(res);
                if ( luminance_options->tmowindow_showprocessed )
                {
                    emit processedFrame(workingframe);
                }
                else
                {
                    //clean up workingframe in order not to waste much memory!
                    //delete workingframe;
                    pfs::DOMIO pfsio;
                    pfsio.freeFrame(workingframe);
                }

            }
            break;
        }
    }
    emit finished();
    emit deleteMe(this);
}

