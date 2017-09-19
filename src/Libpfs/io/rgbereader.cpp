/*
 * This file is a part of Luminance HDR package.
 * ----------------------------------------------------------------------
 * Copyright (C) 2003-2007 Rafal Mantiuk and Grzegorz Krawczyk
 * Copyright (C) 2006-2007 Giuseppe Rota
 * Copyright (C) 2013 Davide Anastasia
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

#include <cassert>
#include <cmath>
#include <cstring>
#include <iostream>

#include <Libpfs/array2d.h>
#include <Libpfs/colorspace/colorspace.h>
#include <Libpfs/frame.h>
#include <Libpfs/io/rgbecommon.h>
#include <Libpfs/io/rgbereader.h>

using namespace std;

namespace pfs {
namespace io {

void rgbe2rgb(const Trgbe_pixel &rgbe, float exposure, float &r, float &g,
              float &b) {
    if (rgbe.e != 0)  // a non-zero pixel
    {
        int e = rgbe.e - int(128 + 8);
        double f = ldexp(1.0, e) * WHITE_EFFICACY / exposure;

        r = (float)(rgbe.r * f);
        g = (float)(rgbe.g * f);
        b = (float)(rgbe.b * f);
    } else
        r = g = b = 0.f;
}

// Reading RGBE files
void readRadianceHeader(FILE *file, int &width, int &height, float &exposure,
                        Colorspace &colorspace) {
    // DEBUG_STR << "RGBE: reading header..." << endl;

    // read header information
    char head[255];
    float fval;
    int format = 0;
    exposure = 1.0f;

    while (!feof(file)) {
        if (fgets(head, 200, file) == NULL) {
            throw pfs::io::InvalidHeader("RGBE: invalid header");
        }
        if (strcmp(head, "\n") == 0) break;
        if (strcmp(head, "#?RADIANCE\n") == 0) {
            // format specifier found
            format = 1;
        }
        if (strcmp(head, "#?RGBE\n") == 0) {
            // format specifier found
            format = 1;
        }
        if (strcmp(head, "#?AUTOPANO\n") == 0) {
            // format specifier found
            format = 1;
        }
        if (head[0] == '#') {
            // comment found - skip
            continue;
        }
        if (strcmp(head, "FORMAT=32-bit_rle_rgbe\n") == 0) {
            // header found
            colorspace = RGB;
            continue;
        }
        if (strcmp(head, "FORMAT=32-bit_rle_xyze\n") == 0) {
            // header found
            colorspace = XYZ;
            continue;
        }
        if (sscanf(head, "EXPOSURE=%f", &fval) == 1) {
            // exposure value
            exposure *= fval;
        }
    }

    // ignore wierd exposure adjustments
    if (exposure > 1e12 || exposure < 1e-12) {
        exposure = 1.0f;
    }

    if (!format) {
        throw pfs::Exception("RGBE: no format specifier found");
    }

    // image size
    char xbuf[4], ybuf[4];
    if (fgets(head, sizeof(head) / sizeof(head[0]), file) == NULL ||
        sscanf(head, "%3s %d %3s %d", ybuf, &height, xbuf, &width) != 4) {
        throw pfs::Exception("RGBE: unknown image size");
    }

    assert(height > 0);
    assert(width > 0);

    /*
    if( ybuf[1]=='x' || ybuf[1]=='X' ) {
        height += width;
        width = height - width;
        height = height - width;
    }
  */
    // DEBUG_STR << "RGBE: image size " << width << "x" << height << endl;
}

void RLERead(FILE *file, Trgbe *scanline, int size) {
    int peek = 0;
    while (peek < size) {
        Trgbe p[2];
        if (fread(p, sizeof(p), 1, file) == 0) {
            throw pfs::io::ReadException("RGBE: Invalid data size");
        }
        if (p[0] > 128) {
            // a run
            int run_len = p[0] - 128;

            while (run_len > 0) {
                scanline[peek++] = p[1];
                run_len--;
            }
        } else {
            // a non-run
            scanline[peek++] = p[1];

            int nonrun_len = p[0] - 1;
            if (nonrun_len > 0) {
                if (fread(scanline + peek, sizeof(*scanline), nonrun_len,
                          file) == 0) {
                    throw pfs::io::ReadException("RGBE: Invalid data size");
                } else {
                    peek += nonrun_len;
                }
            }
        }
    }
    if (peek != size) {
        throw pfs::io::ReadException(
            "RGBE: difference in size while reading RLE scanline");
    }
}

void readRadiance(FILE *file, int width, int height, float exposure,
                  pfs::Array2Df &X, pfs::Array2Df &Y, pfs::Array2Df &Z) {
    // read image
    // depending on format read either rle or normal (note: only rle supported)
    std::vector<Trgbe> scanline(width * 4);

    for (int y = 0; y < height; ++y) {
        // read rle header
        Trgbe header[4];
        if (fread(header, sizeof(header), 1, file) == sizeof(header)) {
            throw pfs::io::ReadException("RGBE: invalid data size");
        }
        if (header[0] != 2 || header[1] != 2 ||
            (header[2] << 8) + header[3] != width) {
            //--- simple scanline (not rle)
            size_t rez =
                fread(scanline.data() + 4, sizeof(Trgbe), 4 * width - 4, file);
            if (rez != (size_t)4 * width - 4) {
                //     DEBUG_STR << "RGBE: scanline " << y
                //           << "(" << (int)rez << "/" << width << ")" <<endl;
                throw pfs::Exception(
                    "RGBE: not enough data to read "
                    "in the simple format.");
            }
            //--- yes, we've read one pixel as a header
            scanline[0] = header[0];
            scanline[1] = header[1];
            scanline[2] = header[2];
            scanline[3] = header[3];

            //--- write scanline to the image
            for (int x = 0; x < width; ++x) {
                Trgbe_pixel rgbe;
                rgbe.r = scanline[4 * x + 0];
                rgbe.g = scanline[4 * x + 1];
                rgbe.b = scanline[4 * x + 2];
                rgbe.e = scanline[4 * x + 3];

                rgbe2rgb(rgbe, exposure, X(x, y), Y(x, y), Z(x, y));
            }
        } else {
            //--- rle scanline
            //--- each channel is encoded separately
            for (int ch = 0; ch < 4; ++ch) {
                RLERead(file, scanline.data() + width * ch, width);
            }

            //--- write scanline to the image
            for (int x = 0; x < width; ++x) {
                Trgbe_pixel rgbe;
                rgbe.r = scanline[x + width * 0];
                rgbe.g = scanline[x + width * 1];
                rgbe.b = scanline[x + width * 2];
                rgbe.e = scanline[x + width * 3];

                rgbe2rgb(rgbe, exposure, X(x, y), Y(x, y), Z(x, y));
            }
        }
    }
}

RGBEReader::RGBEReader(const string &filename)
    : FrameReader(filename), m_exposure(0.0) {
    RGBEReader::open();
}

void RGBEReader::open() {
    m_file.reset(fopen(filename().c_str(), "rb"));
    if (!m_file) {
        throw InvalidFile("Cannot open file " + filename());
    }

    // Can we do it in a better way?!
    int width = 0;
    int height = 0;
    float exposure = 0.f;
    Colorspace colorspace;

    readRadianceHeader(m_file.data(), width, height, exposure, colorspace);

    m_colorspace = colorspace;

    setWidth(width);
    setHeight(height);
    m_exposure = exposure;
}

void RGBEReader::close() {
    m_file.reset();
    m_exposure = 0.f;

    setWidth(0);
    setHeight(0);
}

void RGBEReader::read(Frame &frame, const Params & /*params*/) {
    if (!isOpen()) open();

    Frame tempFrame(width(), height());

    pfs::Channel *X, *Y, *Z;
    tempFrame.createXYZChannels(X, Y, Z);

    readRadiance(m_file.data(), width(), height(), m_exposure, *X, *Y, *Z);

    if (m_colorspace == XYZ) pfs::transformXYZ2RGB(X, Y, Z, X, Y, Z);

    tempFrame.getTags().setTag("LUMINANCE", "RELATIVE");
    tempFrame.getTags().setTag("FILE_NAME", filename());

    frame.swap(tempFrame);
}

}  // io
}  // pfs
