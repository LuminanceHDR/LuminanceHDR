/**
 * This file is a part of Luminance HDR package.
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
 */

//! \author Davide Anastasia <davideanastasia@users.sourceforge.net>
//! \date October 20th, 2012

#include <iostream>

#include <QCoreApplication>
#include <QStringList>
#include <QString>

#include "Common/LuminanceOptions.h"
#include "Common/TranslatorManager.h"

#include "ImageInspector.h"

using namespace std;

int main( int argc, char ** argv )
{
    QCoreApplication application( argc, argv );
    LuminanceOptions lumOpts;

    TranslatorManager::setLanguage( lumOpts.getGuiLang(), false );

    QStringList arguments = QCoreApplication::arguments();
    if ( arguments.size() <= 1 )
    {
        std::cout << "Usage: " << arguments[0].toLocal8Bit().constData()
                  << " <filename> " << std::endl;

        application.exit( -1 );
    }
    else
    {
        QString filename = arguments[1];

        ImageInspector imageInspector;

        application.exit( imageInspector.inspect( filename, std::cout ) );
    }
}
