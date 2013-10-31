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
#include <string>

#include "HdrWizard/WhiteBalance.h"

#include <boost/program_options.hpp>

#include <Libpfs/frame.h>
#include <Libpfs/channel.h>
#include <Libpfs/params.h>
#include <Libpfs/io/framewriterfactory.h>
#include <Libpfs/io/framereaderfactory.h>

using namespace std;
using namespace pfs;
using namespace pfs::io;

namespace po = boost::program_options;

void getParameters(int argc, char** argv, std::string& input, std::string& output, int& wbMode)
{
    po::options_description desc("Allowed options: ");
    desc.add_options()
            ("output,o", po::value<std::string>(&output)->required(), "output file")
            ("input,i", po::value<std::string>(&input)->required(), "input file")
            ("mode,m", po::value<int>(&wbMode)->default_value(0), "auto-scale")
            ;

    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv).
              options(desc).allow_unregistered().run(), vm);
    po::notify(vm);

    if (wbMode < 0) wbMode = 0;
    if (wbMode > 2) wbMode = 2;
}

int main( int argc, char ** argv )
{
    std::string inputFile;
    std::string outputFile;
    int wbMode;

    getParameters(argc, argv, inputFile, outputFile, wbMode);

    Frame frame;
    FrameReaderPtr reader = FrameReaderFactory::open(inputFile);
    reader->read(frame, Params());

    whiteBalance(frame, static_cast<WhiteBalanceType>(wbMode));

    FrameWriterPtr writer = FrameWriterFactory::open(outputFile);
    writer->write(frame, Params());

    return 0;
}
