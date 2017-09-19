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
 * $Id: display_size.h,v 1.2 2008/06/16 18:42:58 rafm Exp $
 */

#ifndef DISPLAY_SIZE_H
#define DISPLAY_SIZE_H

#include <Libpfs/exception.h>
#include <stdio.h>

class DisplaySize {
    float view_d;
    float ppd;

   public:
    /**
     * @param vres vertical screen resolution in pixels
     * @param vd_screen_h viewing distance as the multiplies of screen height
     * (e.g. 2)
     * @param vd_meters viewing distance in meters
     */
    DisplaySize(int vres, float vd_screen_h, float vd_meters = 0.5);

    /**
     * @param ppd number of pixels per one visual degree (e.g. 30)
     * @param vd_meters viewing distance in meters
     */
    DisplaySize(float ppd, float vd_meters = 0.5);

    void print(FILE *fh);

    float getPixPerDeg();
    float getViewD();
};

DisplaySize *createDisplaySizeFromArgs(int &argc, char *argv[]);

#endif
