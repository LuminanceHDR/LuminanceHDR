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
 * $Id: display_function.h,v 1.3 2008/06/16 18:42:58 rafm Exp $
 */

#ifndef DISPLAY_FUNCTION_H
#define DISPLAY_FUNCTION_H

#include <cstdio>
#include "../../opthelper.h"

#include <Libpfs/exception.h>
#include "Libpfs/utils/sse.h"
#include "arch/math.h"

class DisplayFunction {
   public:
    /** Convert input luminance (cd/m^2) to pixel value (0-1)
     */
    virtual float inv_display(float L) = 0;

    /** Convert pixel value (0-1) to input luminance (cd/m^2)
     */
    virtual float display(float pix) = 0;

#ifdef __SSE2__
    virtual vfloat inv_display(vfloat L) = 0;
    virtual vfloat display(vfloat L) = 0;
#endif

    virtual void print(FILE *fh) = 0;

    virtual ~DisplayFunction() {}
};

/**
 * Gamma Gain Black and Ambient display model
 */
class DisplayFunctionGGBA : public DisplayFunction {
    float gamma, L_max, L_offset, L_black, E_amb, screen_refl;

   public:
    DisplayFunctionGGBA(float gamma, float L_max, float L_black, float E_amb,
                        float screen_refl);
    DisplayFunctionGGBA(const char *predefined);

    float inv_display(float L);
    float display(float pix);

#ifdef __SSE2__
    virtual vfloat inv_display(vfloat L);
    virtual vfloat display(vfloat L);
#endif

    void print(FILE *fh);

   private:
    void init(float gamma, float L_max, float L_black, float E_amb,
              float screen_refl);
};

class DisplayFunctionLUT : public DisplayFunction {
    float *pix_lut, *L_lut;
    size_t lut_size;

   public:
    DisplayFunctionLUT(const char *file_name);
    ~DisplayFunctionLUT();

    float inv_display(float L);
    float display(float pix);
    void print(FILE *fh);
};

// DisplayFunction *createDisplayFunctionFromArgs( int &argc, char* argv[] );

#endif
