/**
 * This file is a part of Luminance HDR package
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
 * @author Giuseppe Rota <grota@users.sourceforge.net>
 */

#ifndef CREATEHDR_H
#define CREATEHDR_H

#include <QString>
#include <QList>
#include <QImage>

enum TWeight
{
    TRIANGULAR,
    GAUSSIAN,
    PLATEAU
};

enum TResponse
{
    FROM_FILE,
    LINEAR,
    GAMMA,
    LOG10,
    FROM_ROBERTSON
};

enum TModel
{
    ROBERTSON,
    DEBEVEC
};

struct config_triple
{
    TWeight weights;
    TResponse response_curve;
    TModel model;
    QString LoadCurveFromFilename;
    QString SaveCurveToFilename;
};

// pfs::Frame* createHDR(const float* const arrayofexptime, const config_triple* const chosen_config, bool antighosting, int iterations, bool ldrinput, ...);

#endif
