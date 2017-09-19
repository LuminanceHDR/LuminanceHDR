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
 * $Id: display_function.cpp,v 1.3 2008/06/16 18:42:58 rafm Exp $
 */

#include <boost/math/constants/constants.hpp>

#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "Libpfs/pfs.h"

#include "arch/string.h"
#include "display_function.h"

// ========== GGBA Display Function ==============

DisplayFunctionGGBA::DisplayFunctionGGBA(float gamma, float L_max,
                                         float L_black, float E_amb,
                                         float screen_refl) {
    init(gamma, L_max, L_black, E_amb, screen_refl);
}

void DisplayFunctionGGBA::init(float gamma, float L_max, float L_black,
                               float E_amb, float screen_refl) {
    this->gamma = gamma;
    this->L_max = L_max;
    this->L_black = L_black;
    this->E_amb = E_amb;
    this->screen_refl = screen_refl;
    this->L_offset =
        L_black + screen_refl / boost::math::double_constants::pi * E_amb;
}

DisplayFunctionGGBA::DisplayFunctionGGBA(const char *predefined) {
    if (!strcasecmp(predefined, "lcd_office")) {
        init(2.2f, 100, 0.8f, 400, 0.01f);
    } else if (!strcasecmp(predefined, "lcd")) {
        init(2.2f, 200, 0.8f, 60, 0.01f);
    } else if (!strcasecmp(predefined, "lcd_bright")) {
        init(2.6f, 500, 0.5f, 10, 0.01f);
    } else if (!strcasecmp(predefined, "crt")) {
        init(2.2f, 80, 1.0f, 60, 0.02f);
    } else {
        throw pfs::Exception(
            "Unknown display type. Recognized types: "
            "lcd_office, lcd, lcd_bright, "
            "crt.");
    }
}

void DisplayFunctionGGBA::print(FILE *fh) {
    fprintf(fh, "Display function: gamma-gain-black-ambient\n");
    fprintf(fh, "   gamma = %g\t L_max = %g\t L_black = %g\n", (double)gamma,
            (double)L_max, (double)L_black);
    fprintf(fh, "   E_amb = %g\t k     = %g\n", (double)E_amb,
            (double)screen_refl);
}

float DisplayFunctionGGBA::inv_display(float L) {
    if (L < L_offset) L = L_offset;
    if (L > (L_offset + L_max)) L = L_offset + L_max;
    return powf((L - L_offset) / (L_max - L_black), 1 / gamma);
}

float DisplayFunctionGGBA::display(float pix) {
    assert(pix >= 0 && pix <= 1);
    return pow(pix, gamma) * (L_max - L_black) + L_offset;
}

#ifdef LUMINANCE_USE_SSE

v4sf DisplayFunctionGGBA::inv_display(v4sf L) {
    const v4sf voffset = _mm_set1_ps(L_offset);
    const v4sf vmax = _mm_set1_ps(L_max);
    L = _mm_max_ps(L, voffset);
    L = _mm_min_ps(L, voffset + vmax);
    return _mm_pow_ps((L - voffset) / (vmax - _mm_set1_ps(L_black)),
                      _mm_set1_ps(1.0f / gamma));
}

v4sf DisplayFunctionGGBA::display(v4sf pix) {
    return _mm_pow_ps(pix, _mm_set1_ps(gamma)) *
               (_mm_set1_ps(L_max) - _mm_set1_ps(L_black)) +
           _mm_set1_ps(L_offset);
}

#endif

// ========== LUT Display Function ==============

static const int max_lut_size = 4096;

DisplayFunctionLUT::DisplayFunctionLUT(const char *file_name)
    : pix_lut(NULL), L_lut(NULL) {
    FILE *fh = fopen(file_name, "r");
    if (fh == NULL) throw pfs::Exception("Cannot open lookup-table file");

    L_lut = new float[max_lut_size];
    pix_lut = new float[max_lut_size];

    const size_t max_line = 100;
    char buf[max_line];

    int i = 0;
    while (fgets(buf, max_line, fh) != NULL) {
        float p_buf, L_buf;
        if (sscanf(buf, "%f%*[ ,;]%f\n", &p_buf, &L_buf) != 2) continue;
        if (p_buf < 0 || p_buf > 1) {
            fclose(fh);
            throw pfs::Exception(
                "Improper LUT: pixel values must be from 0 to 1");
        }

        if (L_buf <= 0) {
            fclose(fh);
            throw pfs::Exception(
                "Improper LUT: luminance must be greater than 0");
        }
        L_lut[i] = log10(L_buf);
        pix_lut[i] = p_buf;
        i++;
        if (i >= max_lut_size) {
            fclose(fh);
            throw pfs::Exception("LUT too large (more than 4096 entries)");
        }
    }
    lut_size = i;
    if (pix_lut[0] != 0 || pix_lut[lut_size - 1] != 1) {
        fclose(fh);
        throw pfs::Exception(
            "The first and last LUT entries for pixel value should be 0 and 1");
    }
    fclose(fh);
}

DisplayFunctionLUT::~DisplayFunctionLUT() {
    delete[] pix_lut;
    delete[] L_lut;
}

inline float bin_search_interp(float x, const float *lut_x, const float *lut_y,
                               const int lutSize) {
    if (x <= lut_x[0]) return lut_y[0];
    if (x >= lut_x[lutSize - 1]) return lut_y[lutSize - 1];

    size_t l = 0, r = lutSize;
    while (true) {
        size_t m = (l + r) / 2;
        if (m == l) break;
        if (x < lut_x[m])
            r = m;
        else
            l = m;
    }
    return lut_y[l] + (lut_y[l + 1] - lut_y[l]) * (x - lut_x[l]);
}

float DisplayFunctionLUT::inv_display(float L) {
    return bin_search_interp(log10(L), L_lut, pix_lut, lut_size);
}

float DisplayFunctionLUT::display(float pix) {
    return pow(10, bin_search_interp(pix, pix_lut, L_lut, lut_size));
}

void DisplayFunctionLUT::print(FILE *fh) {
    fprintf(fh, "Display function: lookup-table\n");
    fprintf(fh, "   L_min = %g \tL_max = %g\n", (double)pow(10, L_lut[0]),
            (double)pow(10, L_lut[lut_size - 1]));
}

// ========== Command line parsing ==============
/*
DisplayFunction *createDisplayFunctionFromArgs( int &argc, char* argv[] )
{
  DisplayFunction *df = 0;

    for( int i=1 ; i<argc; i++ )
    {
      if( !strcmp( argv[i], "--display-function" ) || !strcmp( argv[i], "-d" )
) {
        if( i+1 >= argc )
          throw pfs::Exception( "missing display function specification" );

        float gamma = 2.2f, L_max = 100.f, L_black = 1.f, k = 0.01, E_amb = 50;
        bool GGBA_model = true;
        char *token;
        token = strtok( argv[i+1], ":" );
        while( token != NULL ) {
          if( !strncmp( token, "pd=", 3 )  ) {
              df = new DisplayFunctionGGBA( token+3 );
              GGBA_model = false;
              break;
          } else if( !strncmp( token, "lut=", 4 )  ) {
              df = new DisplayFunctionLUT( token+4 );
              GGBA_model = false;
              break;
          } else if( !strncmp( token, "g=", 2 ) ) {
            gamma = strtod( token+2, NULL );
          } else if( !strncmp( token, "l=", 2 ) ) {
            L_max = strtod( token+2, NULL );
          } else if( !strncmp( token, "b=", 2 ) ) {
            L_black = strtod( token+2, NULL );
          } else if( !strncmp( token, "k=", 2 ) ) {
            k = strtod( token+2, NULL );
          } else if( !strncmp( token, "a=", 2 ) ) {
            E_amb = strtod( token+2, NULL );
          } else {
            throw pfs::Exception( "Bad display type specification" );
          }
          token = strtok( NULL, ":" );
        }
        if( GGBA_model )
          df = new DisplayFunctionGGBA( gamma, L_max, L_black, E_amb, k );

        removeCommandLineArg( argc, argv, i, 2 );
        break;
      }
    }
    return df;
}

static void removeCommandLineArg( int &argc, char* argv[],
  int firstArgToRemove, int numArgsToRemove )
{
  assert( firstArgToRemove+numArgsToRemove <= argc );
  if( argc-firstArgToRemove-numArgsToRemove > 0 ) {
    for( int i = firstArgToRemove; i < argc-numArgsToRemove; i++ )
      argv[i] = argv[i+numArgsToRemove];
  }

  argc -= numArgsToRemove;
}

*/
