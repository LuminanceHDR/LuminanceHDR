/**
 * @brief Display Adaptive TMO
 *
 * From:
 * Rafal Mantiuk, Scott Daly, Louis Kerofsky.
 * Display Adaptive Tone Mapping.
 * To appear in: ACM Transactions on Graphics (Proc. of SIGGRAPH'08) 27 (3)
 * http://www.mpi-inf.mpg.de/resources/hdr/datmo/
 *
 * This file is a part of LuminanceHDR package, based on pfstmo.
 * ----------------------------------------------------------------------
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
 * @author Rafal Mantiuk, <mantiuk@gmail.com>
 *
 * $Id: display_size.cpp,v 1.3 2008/06/16 18:42:58 rafm Exp $
 */

#include <boost/math/constants/constants.hpp>

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "Libpfs/pfs.h"
#include "arch/math.h"
#include "display_size.h"

static void removeCommandLineArg(int &argc, char *argv[], int firstArgToRemove,
                                 int numArgsToRemove);

DisplaySize::DisplaySize(int vres, float vd_screen_h, float vd_meters)
    : view_d(vd_meters) {
    ppd = vres * boost::math::double_constants::pi /
          (360 * atan(0.5f / vd_screen_h));
}

DisplaySize::DisplaySize(float ppd, float vd_meters)
    : view_d(vd_meters), ppd(ppd) {}

void DisplaySize::print(FILE *fh) {
    fprintf(fh, "Display size paramaters:\n");
    fprintf(fh, "   pixels per visual degree = %g\n", (double)getPixPerDeg());
    fprintf(fh, "   viewing distance = %g [meters]\n", (double)getViewD());
}

float DisplaySize::getPixPerDeg() { return ppd; }

float DisplaySize::getViewD() { return view_d; }

// ========== Command line parsing ==============

DisplaySize *createDisplaySizeFromArgs(int &argc, char *argv[]) {
    DisplaySize *ds = 0;

    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--display-size") || !strcmp(argv[i], "-s")) {
            if (i + 1 >= argc)
                throw pfs::Exception("missing display size specification");

            float vres = 1024, vd = 2, d = 0.5, ppd = -1;
            char *token;
            token = strtok(argv[i + 1], ":");
            while (token != NULL) {
                if (!strncmp(token, "vres=", 5)) {
                    vres = strtod(token + 5, NULL);
                } else if (!strncmp(token, "vd=", 3)) {
                    vd = strtod(token + 3, NULL);
                } else if (!strncmp(token, "d=", 2)) {
                    d = strtod(token + 2, NULL);
                } else if (!strncmp(token, "ppd=", 4)) {
                    ppd = strtod(token + 4, NULL);
                } else {
                    throw pfs::Exception("Bad display size specification");
                }
                token = strtok(NULL, ":");
            }
            if (ppd != -1)
                ds = new DisplaySize(ppd, d);
            else
                ds = new DisplaySize(vres, vd, d);

            removeCommandLineArg(argc, argv, i, 2);
            break;
        }
    }
    return ds;
}

static void removeCommandLineArg(int &argc, char *argv[], int firstArgToRemove,
                                 int numArgsToRemove) {
    assert(firstArgToRemove + numArgsToRemove <= argc);
    if (argc - firstArgToRemove - numArgsToRemove > 0) {
        for (int i = firstArgToRemove; i < argc - numArgsToRemove; i++)
            argv[i] = argv[i + numArgsToRemove];
    }

    argc -= numArgsToRemove;
}
