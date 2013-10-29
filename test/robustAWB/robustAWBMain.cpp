/**
 * This file is a part of Luminance HDR package.
 * ----------------------------------------------------------------------
 * Copyright (C) 2013 Franco Comida
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

//! \author Franco Comida <fcomida@users.sourceforge.net>
//! \date July 8th, 2013

#include <iostream>

#include <QCoreApplication>
#include <QStringList>
#include <QString>

#include "Common/LuminanceOptions.h"
#include "Common/TranslatorManager.h"
#include "HdrWizard/WhiteBalance.h"

#include <Libpfs/frame.h>
#include <Libpfs/channel.h>
#include <Libpfs/params.h>
#include <Libpfs/io/jpegreader.h>
#include <Libpfs/io/jpegwriter.h>

using namespace std;

int main( int argc, char ** argv )
{
    QCoreApplication application( argc, argv );
    LuminanceOptions lumOpts;

    TranslatorManager::setLanguage( lumOpts.getGuiLang(), false );

    QStringList arguments = QCoreApplication::arguments();
    if ( arguments.size() <= 2 )
    {
        std::cout << "Usage: " << arguments[0].toLocal8Bit().constData()
                  << " <infilename> " << " <outfilename> " << std::endl;

        application.exit( -1 );
    }
    else
    {
        QString infilename = arguments[1];
        QString outfilename = arguments[2];
        using namespace pfs;
        using namespace io;

        if ( !QFile::exists( infilename ) )
        {
            cout << "File " << infilename.toLocal8Bit().constData()
                << " does not exist" << endl;

            application.exit(-1);
        }

        cout << "Start reading..." << std::flush;

        Params params;
        Frame* frame = new Frame();
        JpegReader reader(infilename.toLocal8Bit().constData());
        reader.read(*frame, params);

        cout << " done!" << std::endl;

        if ( !frame )
        {
            cout << "I cannot read " << infilename.toLocal8Bit().constData()
                << endl;

            application.exit(-1);
        }
        Channel *R, *G, *B;
        frame->getXYZChannels(R, G, B);
        
        robustAWB(R, G, B);

        JpegWriter writer(outfilename.toLocal8Bit().constData());
        writer.write(*frame, params);
        
        application.exit(0);
    }
}
