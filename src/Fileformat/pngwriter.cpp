/*
 * This file is a part of Luminance HDR package.
 * ----------------------------------------------------------------------
 * Copyright (C) 2012 Franco Comida
 * Copyright (C) 2013 Davide Anastasia
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with this program; if not, write to the Free Software
 *	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * ----------------------------------------------------------------------
 */

//! \author Franco Comida <fcomida@users.sourceforge.net>
//! \author Davide Anastasia <davideanastasia@users.sourceforge.net>

#if defined(WIN32) || defined(__APPLE__) || defined(__FreeBSD__)
#define USE_TEMPORARY_FILE
#endif

#include "pngwriter.h"

#include <QDebug>
#include <QFile>
#ifdef USE_TEMPORARY_FILE
#include <QTemporaryFile>
#endif

#include <vector>
#include <iostream>
#include <sstream>
#include <algorithm>

#include <lcms2.h>
#include <stdio.h>
#include <png.h>

#include <Libpfs/frame.h>
#include <Libpfs/colorspace/rgbremapper.h>
#include <Libpfs/utils/resourcehandlerlcms.h>
#include <Libpfs/utils/resourcehandlerstdio.h>
#include <Libpfs/utils/transform.h>

using namespace std;
using namespace boost;
using namespace pfs;

struct PngWriterParams
{
    PngWriterParams()
        : quality_(100)
        , minLuminance_(0.f)
        , maxLuminance_(1.f)
        , luminanceMapping_(MAP_LINEAR)
    {}

    void parse(const Params& params)
    {
        for ( Params::const_iterator it = params.begin(), itEnd = params.end();
              it != itEnd; ++it )
        {
            if ( it->first == "quality" ) {
                quality_ = it->second.as<size_t>(quality_);
                continue;
            }
            if ( it->first == "min_luminance" ) {
                minLuminance_ = it->second.as<float>(minLuminance_);
                continue;
            }
            if ( it->first == "max_luminance" ) {
                maxLuminance_ = it->second.as<float>(maxLuminance_);
                continue;
            }
            if ( it->first == "mapping_method" ) {
                luminanceMapping_ = it->second.as<RGBMappingType>(luminanceMapping_);
                continue;
            }
        }
    }

    size_t quality_;
    float minLuminance_;
    float maxLuminance_;
    RGBMappingType luminanceMapping_;
};

ostream& operator<<(ostream& out, const PngWriterParams& params)
{
    stringstream ss;
    ss << "PngWriterParams: [";
    ss << "quality: " << params.quality_ << ", ";
    ss << "min_luminance: " << params.minLuminance_ << ", ";
    ss << "max_luminance: " << params.maxLuminance_ << ", ";
    ss << "mapping_method: " << params.luminanceMapping_ << "]";

    return (out << ss.str());
}

class PngWriterImpl
{
public:
    PngWriterImpl()
        : m_filesize(0)
    {}

    virtual ~PngWriterImpl()
    {}

    virtual bool open(size_t bufferSize) = 0;
    virtual void close() = 0;

    // get an handle to the underlying FILE*
    virtual FILE* handle() = 0;

    // compute size (if functionality available)
    virtual void computeSize() = 0;

    size_t getFileSize()
    { return m_filesize; }

    bool write(const pfs::Frame &frame, const PngWriterParams& params)
    {
        png_uint_32 width = frame.getWidth();
        png_uint_32 height = frame.getHeight();

        cmsUInt32Number profile_size = 0;

        cmsHPROFILE hsRGB = cmsCreate_sRGBProfile();
        cmsSaveProfileToMem(hsRGB, NULL, &profile_size);        // get the size

#if PNG_LIBPNG_VER_MINOR < 5
        std::vector<char> profile_buffer(profile_size);
#else
        std::vector<unsigned char> profile_buffer(profile_size);
#endif

        cmsSaveProfileToMem(hsRGB, profile_buffer.data(), &profile_size);

        qDebug() << "sRGB profile size: " << profile_size;

        if ( !open(width*height*4 + (width*height*4)*0.1) ) {
            std::cerr << "Cannot open the output stream";
            return false;
        }

        png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING,
                                                      NULL, NULL, NULL);
        if (!png_ptr)
        {
            qDebug() << "PNG: Failed to create write struct";
            close();
            return false;
        }

        png_infop info_ptr = png_create_info_struct(png_ptr);
        if (!info_ptr)
        {
            qDebug() << "PNG: Failed to create info struct";
            png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
            close();
            return false;
        }

        if (setjmp(png_jmpbuf(png_ptr)))
        {
            qDebug() << "PNG: Error writing file";
            png_destroy_write_struct(&png_ptr, &info_ptr);
            close();
            return false;
        }

        png_init_io(png_ptr, handle());

        png_set_IHDR(png_ptr, info_ptr, width, height,
                     8, /*PNG_COLOR_TYPE_RGB_ALPHA*/ PNG_COLOR_TYPE_RGB,
                     PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT,
                     PNG_FILTER_TYPE_DEFAULT);

        int compression_level = 9 - params.quality_/11.11111;

        png_set_compression_level(png_ptr, compression_level);

        png_set_bgr(png_ptr);

        char profileName[5] = "sRGB";
        png_set_iCCP(png_ptr, info_ptr, profileName, 0,
                     profile_buffer.data(), (png_uint_32)profile_size);

        png_write_info(png_ptr, info_ptr);

        const Channel* rChannel;
        const Channel* gChannel;
        const Channel* bChannel;
        frame.getXYZChannels(rChannel, gChannel, bChannel);

        std::vector<png_byte> scanLineOut( width * 3 );
        RGBRemapper rgbRemapper(params.minLuminance_, params.maxLuminance_,
                                params.luminanceMapping_);
        for (png_uint_32 row = 0; row < height; ++row)
        {
            utils::transform(rChannel->row_begin(row), rChannel->row_end(row),
                             gChannel->row_begin(row), bChannel->row_begin(row),
                             scanLineOut.data()+2,
                             scanLineOut.data()+1,
                             scanLineOut.data(),
                             rgbRemapper, 3);
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

struct PngWriterImplFile : public PngWriterImpl
{
    PngWriterImplFile(const std::string& filename)
        : PngWriterImpl()
        , m_handle()
        , m_filename(filename)
    {}

    bool open(size_t /*bufferSize*/)
    {
        m_handle.reset( fopen(m_filename.c_str(), "wb") );
        if ( m_handle ) return true;
        return false;
    }

    void close()
    { m_handle.reset(); }

    FILE* handle()
    { return m_handle.data(); }

    void computeSize()
    { m_filesize = 0; }

private:
    utils::ScopedStdIoFile m_handle;
    std::string m_filename;
};

struct PngWriterImplMemory : public PngWriterImpl
{
    PngWriterImplMemory()
        : PngWriterImpl()
    {}

    bool open(size_t bufferSize)
    {
#ifdef USE_TEMPORARY_FILE
        if ( !m_temporaryFile.open() ) {
            // could not open the temporary file!
            return false;
        }
        QByteArray temporaryFileName = QFile::encodeName( m_temporaryFile.fileName() );
        m_temporaryFile.close();

        m_handle.reset( fopen(temporaryFileName.constData(), "w+") );
#else
        m_temporaryBuffer.resize( bufferSize );
        // reset all element of the vector to zero!
        std::fill(m_temporaryBuffer.begin(), m_temporaryBuffer.end(), 0);

        m_handle.reset( fmemopen(m_temporaryBuffer.data(),
                                 m_temporaryBuffer.size(), "w+") );
#endif
        if ( !m_handle ) return false;
        return true;
    }

    void close()
    {  m_handle.reset(); }

    FILE* handle()
    { return m_handle.data(); }

    void computeSize()
    {
#ifdef USE_TEMPORARY_FILE
        fflush(handle());
        fseek(handle(), 0, SEEK_END);
        m_filesize = ftell(handle());
#else
        int size = m_temporaryBuffer.size() - 1;
        for (; size > 0; --size)
        {
            if ( m_temporaryBuffer[size] != 0 ) break;
        }
        m_filesize = size;
#endif
    }

private:
    utils::ScopedStdIoFile m_handle;
#ifdef USE_TEMPORARY_FILE
    QTemporaryFile m_temporaryFile;
#else
    std::vector<char> m_temporaryBuffer;
#endif
};

PngWriter::PngWriter()
    : m_impl(new PngWriterImplMemory)
{}

PngWriter::PngWriter(const string &filename)
    : m_impl(new PngWriterImplFile(filename))
{}

PngWriter::~PngWriter()
{}

bool PngWriter::write(const pfs::Frame& frame, const Params& params)
{
    PngWriterParams p;
    p.parse( params );

#ifndef NDEBUG
    cout << p << endl << flush;
#endif

    return m_impl->write(frame, p);
}

size_t PngWriter::getFileSize() const
{
    return m_impl->getFileSize();
}
