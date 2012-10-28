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
#include "Libpfs/manip/copy.h"
#include "Libpfs/manip/cut.h"
#include "Libpfs/manip/resize.h"
#include "Libpfs/manip/gamma.h"

#include "Core/TonemappingOptions.h"
#include "Common/ProgressHelper.h"
#include "TonemappingEngine/TonemapOperator.h"

TMWorker::TMWorker(QObject* parent):
    QObject(parent),
    m_Callback(new ProgressHelper)
{
#ifdef QT_DEBUG
    qDebug() << "TMWorker::TMWorker() ctor";
#endif

    connect(this, SIGNAL(destroyed()), m_Callback, SLOT(deleteLater()));

    connect(this, SIGNAL(tonemapRequestTermination()), m_Callback, SLOT(terminate()), Qt::DirectConnection);
    connect(m_Callback, SIGNAL(setValue(int)), this, SIGNAL(tonemapSetValue(int)), Qt::DirectConnection);
    connect(m_Callback, SIGNAL(setMinimum(int)), this, SIGNAL(tonemapSetMinimum(int)), Qt::DirectConnection);
    connect(m_Callback, SIGNAL(setMaximum(int)), this, SIGNAL(tonemapSetMaximum(int)), Qt::DirectConnection);
}

TMWorker::~TMWorker()
{
#ifdef QT_DEBUG
    qDebug() << "TMWorker::~TMWorker() dtor";
#endif
}

pfs::Frame* TMWorker::computeTonemap(/* const */ pfs::Frame* in_frame, TonemappingOptions* tm_options)
{
#ifdef QT_DEBUG
    qDebug() << "TMWorker::getTonemappedFrame()";
#endif

    pfs::Frame* working_frame = preprocessFrame(in_frame, tm_options);
    if (working_frame == NULL) return NULL;
    try {
        tonemapFrame(working_frame, tm_options);
    }
    catch(...) {
        emit tonemapFailed("Tonemap failed!");
        delete working_frame;
        return NULL;
    }

    if ( m_Callback->isTerminationRequested() )
    {
        m_Callback->terminate(false);
        delete working_frame;
        return NULL;
    }

    postprocessFrame(working_frame, tm_options);

    emit tonemapSuccess(working_frame, tm_options);
    return working_frame;
}

void TMWorker::tonemapFrame(pfs::Frame* working_frame, TonemappingOptions* tm_options)
{
    m_Callback->terminate(false);

    emit tonemapBegin();
    // build tonemap object
    TonemapOperator* tmEngine = TonemapOperator::getTonemapOperator(tm_options->tmoperator);

    // build object, pass new frame to it and collect the result
    tmEngine->tonemapFrame(working_frame, tm_options, *m_Callback);

    emit tonemapEnd();
    delete tmEngine;
}

pfs::Frame* TMWorker::preprocessFrame(pfs::Frame* input_frame, TonemappingOptions* tm_options)
{
    pfs::Frame* working_frame = NULL;

    if ( tm_options->tonemapSelection )
    {
        // workingframe = "crop"
        // std::cout << "crop:[" << opts.selection_x_up_left <<", " << opts.selection_y_up_left <<"],";
        // std::cout << "[" << opts.selection_x_bottom_right <<", " << opts.selection_y_bottom_right <<"]" << std::endl;
        working_frame = pfs::cut(input_frame,
                                 tm_options->selection_x_up_left,
                                 tm_options->selection_y_up_left,
                                 tm_options->selection_x_bottom_right,
                                 tm_options->selection_y_bottom_right);
    }
    else if ( tm_options->xsize != tm_options->origxsize )
    {
        // workingframe = "resize"
        working_frame = pfs::resize(input_frame, tm_options->xsize);
    }
    else
    {
        // workingframe = "full res"
        working_frame = pfs::copy(input_frame);
    }

    if ( tm_options->pregamma != 1.0f )
    {
        pfs::applyGamma( working_frame, tm_options->pregamma );
    }

    return working_frame;
}

void TMWorker::postprocessFrame(pfs::Frame*, TonemappingOptions*)
{
    // auto-level?
    // black-point?
    // white-point?
}

