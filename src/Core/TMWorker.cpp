/**
 * This file is a part of Luminance HDR package.
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
 * Original Work
 * @author Davide Anastasia <davideanastasia@users.sourceforge.net>
 *
 */

#include "Core/TMWorker.h"

#ifdef QT_DEBUG
#include <QDebug>
#endif

#include "Libpfs/frame.h"
#include "Core/TonemappingOptions.h"
#include "Filter/pfscut.h"
#include "Filter/pfsgamma.h"
#include "Filter/pfssize.h"

TMWorker::TMWorker(QObject* parent):
    QObject(parent)
{}

TMWorker::~TMWorker()
{
#ifdef QT_DEBUG
    qDebug() << "TMWorker::~TMWorker()";
#endif
}

pfs::Frame* TMWorker::getTonemappedFrame(const pfs::Frame* in_frame, TonemappingOptions* tm_options)
{
    pfs::Frame* working_frame = preProcessFrame(in_frame, tm_options);
    try {
        tonemapFrame(working_frame, tm_options);
    }
    catch(...) {
        emit tonemappingFailed("Tonemap failed!");
        return NULL;
    }
    postProcessFrame(working_frame, tm_options);

    emit tonemappingSuccess(working_frame, tm_options);
    return working_frame;
}

void TMWorker::tonemapFrame(pfs::Frame* working_frame, TonemappingOptions* tm_options)
{
    // build object, pass new frame to it
}

pfs::Frame* TMWorker::preProcessFrame(pfs::Frame* input_frame, TonemappingOptions* tm_options)
{
    pfs::Frame* working_frame = NULL;

    if ( tm_options->tonemapSelection )
    {
        // workingframe = "crop"
        // std::cout << "crop:[" << opts.selection_x_up_left <<", " << opts.selection_y_up_left <<"],";
        // std::cout << "[" << opts.selection_x_bottom_right <<", " << opts.selection_y_bottom_right <<"]" << std::endl;
        working_frame = pfs::pfscut(input_frame,
                                   tm_options->selection_x_up_left,
                                   tm_options->selection_y_up_left,
                                   tm_options->selection_x_bottom_right,
                                   tm_options->selection_y_bottom_right);
    }
    else if ( tm_options->xsize != tm_options->origxsize )
    {
        // workingframe = "resize"
        working_frame = pfs::resizeFrame(input_frame, tm_options->xsize);
    }
    else
    {
        // workingframe = "full res"
        working_frame = pfs::pfscopy(input_frame);
    }

    if ( tm_options->pregamma != 1.0f )
    {
        pfs::applyGammaOnFrame( working_frame, tm_options->pregamma );
    }
}

void TMWorker::postProcessFrame(pfs::Frame*, TonemappingOptions*)
{

}

