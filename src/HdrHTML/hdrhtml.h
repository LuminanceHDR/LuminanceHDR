/**
 * @brief Create a web page with an HDR viewer
 *
 * This file is a part of LuminanceHDR package, based on pfstools.
 * ----------------------------------------------------------------------
 * Copyright (C) 2009 Rafal Mantiuk
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
 * @author Rafal Mantiuk, <mantiuk@mpi-sb.mpg.de>
 *
 * $Id: hdrhtml.h,v 1.3 2009/02/07 07:08:47 rafm Exp $
 */

#ifndef HDRHTML_H
#define HDRHTML_H

#include <list>
#include <string>

namespace hdrhtml {

class HDRHTMLImage {
   public:
    std::string base_name;
    int width, height;
    int f8_stops;
    int f_step_res;
    int basis;
    int shared_basis;
    int pix_per_fstop;
    float hist_start;
    int hist_width;
    float exposure;
    float best_exp;

    HDRHTMLImage(const char *base_name, int width, int height)
        : base_name(base_name),
          width(width),
          height(height),
          f8_stops(0),
          f_step_res(0),
          basis(0),
          shared_basis(0),
          pix_per_fstop(0),
          hist_start(0),
          hist_width(0),
          exposure(0),
          best_exp(0) {}
};

class HDRHTMLSet {
    const char *page_name;

   public:
    const char *image_dir;
    const char *image_template;
    std::list<HDRHTMLImage> image_list;

    HDRHTMLSet(const char *page_name, const char *image_dir = NULL)
        : page_name(page_name), image_dir(image_dir), image_template(NULL) {}

    void add_image(int width, int height, float *R, float *G, float *B,
                   float *Y, const char *base_name, const char *out_dir,
                   int quality, bool verbose);

    void generate_webpage(const char *page_template, const char *image_template,
                          const char *out_dir, const char *object_output = NULL,
                          const char *html_output = NULL, bool verbose = false);
};
}
#endif
