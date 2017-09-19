/*
 * This file is a part of Luminance HDR package.
 * ----------------------------------------------------------------------
 * Copyright (C) 2012 Franco Comida
 * Copyright (C) 2013 Davide Anastasia
 *
 *    This program is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * ----------------------------------------------------------------------
 */

#include "pngwriter.h"

#include <algorithm>
#include <iostream>
#include <sstream>
#include <vector>

#include <lcms2.h>
#include <png.h>
#include <stdio.h>

#include <Libpfs/colorspace/normalizer.h>
#include <Libpfs/colorspace/rgbremapper.h>
#include <Libpfs/fixedstrideiterator.h>
#include <Libpfs/frame.h>
#include <Libpfs/utils/chain.h>
#include <Libpfs/utils/clamp.h>
#include <Libpfs/utils/resourcehandlerlcms.h>
#include <Libpfs/utils/resourcehandlerstdio.h>
#include <Libpfs/utils/transform.h>

using namespace std;
using namespace pfs;

namespace pfs {
namespace io {

struct PngWriterParams {
    PngWriterParams()
        : quality_(100),
          minLuminance_(0.f),
          maxLuminance_(1.f),
          luminanceMapping_(MAP_LINEAR) {}

    void parse(const Params &params) {
        for (Params::const_iterator it = params.begin(), itEnd = params.end();
             it != itEnd; ++it) {
            if (it->first == "quality") {
                quality_ = it->second.as<size_t>(quality_);
                continue;
            }
            if (it->first == "min_luminance") {
                minLuminance_ = it->second.as<float>(minLuminance_);
                continue;
            }
            if (it->first == "max_luminance") {
                maxLuminance_ = it->second.as<float>(maxLuminance_);
                continue;
            }
            if (it->first == "mapping_method") {
                luminanceMapping_ =
                    it->second.as<RGBMappingType>(luminanceMapping_);
                continue;
            }
        }
    }

    int compressionLevel() const {
        int compLevel = (9 - (int)((float)quality_ / 11.11111f + 0.5f));

        assert(compLevel >= 0);
        assert(compLevel <= 9);

        return compLevel;
    }

    size_t quality_;
    float minLuminance_;
    float maxLuminance_;
    RGBMappingType luminanceMapping_;
};

ostream &operator<<(ostream &out, const PngWriterParams &params) {
    stringstream ss;
    ss << "PngWriterParams: [";
    ss << "compression_level: " << params.compressionLevel() << ", ";
    ss << "min_luminance: " << params.minLuminance_ << ", ";
    ss << "max_luminance: " << params.maxLuminance_ << ", ";
    ss << "mapping_method: " << params.luminanceMapping_ << "]";

    return (out << ss.str());
}

static void png_write_icc_profile(png_structp png_ptr, png_infop info_ptr) {
    cmsUInt32Number profileSize = 0;
    cmsHPROFILE hsRGB = cmsCreate_sRGBProfile();
    cmsSaveProfileToMem(hsRGB, NULL, &profileSize);  // get the size

#if PNG_LIBPNG_VER_MINOR < 5
    std::vector<char> profileBuffer(profileSize);
#else
    std::vector<unsigned char> profileBuffer(profileSize);
#endif

    cmsSaveProfileToMem(hsRGB, profileBuffer.data(), &profileSize);
#ifndef NDEBUG
    std::clog << "sRGB profile size: " << profileSize << "\n";
#endif

    // char profileName[5] = "sRGB";
    png_set_iCCP(png_ptr, info_ptr, "sRGB" /*profileName*/, 0,
                 profileBuffer.data(), (png_uint_32)profileSize);
}

class PngWriterImpl {
   public:
    PngWriterImpl() : m_filesize(0) {}
    virtual ~PngWriterImpl() {}

    virtual void setupPngDest(png_structp png_ptr,
                              const std::string &filename) = 0;

    virtual void close() = 0;
    virtual void computeSize() = 0;

    size_t getFileSize() { return m_filesize; }
    void setFileSize(size_t size) { m_filesize = size; }

    bool write(const pfs::Frame &frame, const PngWriterParams &params,
               const std::string &filename) {
        png_uint_32 width = frame.getWidth();
        png_uint_32 height = frame.getHeight();

        png_structp png_ptr =
            png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
        if (!png_ptr) {
            close();

            throw io::WriteException("PNG: Failed to create write struct");
            return false;
        }

        png_infop info_ptr = png_create_info_struct(png_ptr);
        if (!info_ptr) {
            png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
            close();

            throw io::WriteException("PNG: Failed to create info struct");
            return false;
        }

        if (setjmp(png_jmpbuf(png_ptr))) {
            png_destroy_write_struct(&png_ptr, &info_ptr);
            close();

            throw io::WriteException("PNG: Error writing file");
            return false;
        }

        setupPngDest(png_ptr, filename);

        png_set_IHDR(png_ptr, info_ptr, width, height, 8,
                     /*PNG_COLOR_TYPE_RGB_ALPHA*/ PNG_COLOR_TYPE_RGB,
                     PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT,
                     PNG_FILTER_TYPE_DEFAULT);

        png_set_compression_level(png_ptr, params.compressionLevel());
        png_set_bgr(png_ptr);
        png_write_icc_profile(png_ptr,
                              info_ptr);  // user defined function, see above
        png_write_info(png_ptr, info_ptr);

        const Channel *rChannel;
        const Channel *gChannel;
        const Channel *bChannel;
        frame.getXYZChannels(rChannel, gChannel, bChannel);

        std::vector<png_byte> scanLineOut(width * 3);
        for (png_uint_32 row = 0; row < height; ++row) {
            utils::transform(
                rChannel->row_begin(row), rChannel->row_end(row),
                gChannel->row_begin(row), bChannel->row_begin(row),
                FixedStrideIterator<png_byte *, 3>(scanLineOut.data() + 2),
                FixedStrideIterator<png_byte *, 3>(scanLineOut.data() + 1),
                FixedStrideIterator<png_byte *, 3>(scanLineOut.data()),
                utils::chain(colorspace::Normalizer(params.minLuminance_,
                                                    params.maxLuminance_),
                             utils::CLAMP_F32,
                             Remapper<png_byte>(params.luminanceMapping_)));
            png_write_row(png_ptr, scanLineOut.data());
        }

        png_write_end(png_ptr, info_ptr);
        png_destroy_write_struct(&png_ptr, &info_ptr);

        computeSize();
        close();

        return true;
    }

   protected:
    size_t m_filesize;
};

struct PngWriterImplFile : public PngWriterImpl {
    PngWriterImplFile() : PngWriterImpl(), m_handle() {}

    void setupPngDest(png_structp png_ptr, const std::string &filename) {
        open(filename);
        png_init_io(png_ptr, handle());
    }

    void close() { m_handle.reset(); }
    void computeSize() { m_filesize = 0; }

   private:
    FILE *handle() { return m_handle.data(); }

    void open(const std::string &filename) {
        m_handle.reset(fopen(filename.c_str(), "wb"));
        if (!m_handle) {
            throw io::InvalidFile("Cannot open file " + filename);
        }
    }

    utils::ScopedStdIoFile m_handle;
};

typedef std::vector<char> PngBuffer;

static void my_png_write_data(png_structp png_ptr, png_bytep data,
                              png_size_t length) {
    PngBuffer *buffer = (PngBuffer *)png_get_io_ptr(png_ptr);
    size_t newSize = buffer->size() + length;

    buffer->resize(newSize);
    // copy the data ... it's not really necessary, as I need only the size!
    std::copy(data, data + length, buffer->end() - length);
}

struct PngWriterImplMemory : public PngWriterImpl {
    PngWriterImplMemory() : PngWriterImpl(), m_buffer() {}

    void setupPngDest(png_structp png_ptr, const std::string &) {
        png_set_write_fn(png_ptr, &m_buffer, my_png_write_data, NULL);
    }

    void close() { m_buffer.clear(); }
    void computeSize() { setFileSize(m_buffer.size()); }

   private:
    PngBuffer m_buffer;
};

PngWriter::PngWriter() : FrameWriter(), m_impl(new PngWriterImplMemory) {}

PngWriter::PngWriter(const string &filename)
    : FrameWriter(filename), m_impl(new PngWriterImplFile()) {}

PngWriter::~PngWriter() { m_impl->close(); }

bool PngWriter::write(const pfs::Frame &frame, const Params &params) {
    PngWriterParams p;
    p.parse(params);

#ifndef NDEBUG
    cout << p << endl << flush;
#endif

    return m_impl->write(frame, p, filename());
}

size_t PngWriter::getFileSize() const { return m_impl->getFileSize(); }

}  // io
}  // pfs
