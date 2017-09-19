/**
 * This file is a part of LuminanceHDR package.
 * ----------------------------------------------------------------------
 * Copyright (C) 2006,2007 Giuseppe Rota
 * Copyright (C) 2010-2012 Davide Anastasia, Franco Comida, Daniel Kaneider
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

//! \author Davide Anastasia <davideanastasia@users.sourceforge.net>
//! \author Daniel Kaneider
//! \author Franco Comida
//! \author Giuseppe Rota <grota@users.sourceforge.net>

#ifndef GLOBAL_H
#define GLOBAL_H

#include <QString>
#include <QStringList>
#include <QUrl>

enum InterpolationMethod { LanczosInterp, BilinearInterp };

bool matchesLdrFilename(const QString &file);
bool matchesHdrFilename(const QString &file);
bool matchesValidHDRorLDRfilename(const QString &file);
QStringList convertUrlListToFilenameList(const QList<QUrl> &urls);
QStringList getAllHdrFileExtensions();

#endif
