/*
 * This file is a part of Luminance HDR package.
 * ----------------------------------------------------------------------
 * Copyright (C) 2012 Franco Comida
 * Copyright (C) 2012-2013 Davide Anastasia
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
 */

//! @author Franco Comida <fcomida@users.sourceforge.net>
//! Original work
//! @author Davide Anastasia <davideanastasia@users.sourceforge.net>
//! clean up memory management

#if defined(WIN32) || defined(__APPLE__) || defined(__FreeBSD__)
#define USE_TEMPORARY_FILE
#endif

#include "jpegwriter.h"

#include <QDebug>
#include <QFile>
#include <QSharedPointer>

#include <stdexcept>
#include <vector>
#include <algorithm>
#include <iostream>
#include <sstream>

#include <stdio.h>
#include <lcms2.h>
#include <jpeglib.h>

#include <Libpfs/frame.h>
#include <Common/FloatRgbToQRgb.h>
#include <Common/ResourceHandlerCommon.h>
#include <Common/ResourceHandlerLcms.h>
#include <Fileformat/pfsoutldrimage.h>

#ifdef USE_TEMPORARY_FILE
#include <QTemporaryFile>
#endif

using namespace std;
using namespace pfs;
using namespace boost;

///////////////////////////////////////////////////////////////////////////////
//
// This code is taken from iccjpeg.c from lcms distribution
//
// Since an ICC profile can be larger than the maximum size of a JPEG marker
// (64K), we need provisions to split it into multiple markers.  The format
// defined by the ICC specifies one or more APP2 markers containing the
// following data:
//	Identifying string	ASCII "ICC_PROFILE\0"  (12 bytes)
//	Marker sequence number	1 for first APP2, 2 for next, etc (1 byte)
//	Number of markers	Total number of APP2's used (1 byte)
//      Profile data		(remainder of APP2 data)
// Decoders should use the marker sequence numbers to reassemble the profile,
// rather than assuming that the APP2 markers appear in the correct sequence.
//

#define ICC_MARKER  (JPEG_APP0 + 2)	/* JPEG marker code for ICC */
#define ICC_OVERHEAD_LEN  14		/* size of non-profile data in APP2 */
#define MAX_BYTES_IN_MARKER  65533	/* maximum data len of a JPEG marker */
#define MAX_DATA_BYTES_IN_MARKER  (MAX_BYTES_IN_MARKER - ICC_OVERHEAD_LEN)


// This routine writes the given ICC profile data into a JPEG file.
// It *must* be called AFTER calling jpeg_start_compress() and BEFORE
// the first call to jpeg_write_scanlines().
// (This ordering ensures that the APP2 marker(s) will appear after the
// SOI and JFIF or Adobe markers, but before all else.)

void write_icc_profile (j_compress_ptr cinfo, const JOCTET *icc_data_ptr,
                        unsigned int icc_data_len)
{
    unsigned int num_markers;	// total number of markers we'll write
    int cur_marker = 1;         // per spec, counting starts at 1
    unsigned int length;		// number of bytes to write in this marker

    // Calculate the number of markers we'll need, rounding up of course
    num_markers = icc_data_len / MAX_DATA_BYTES_IN_MARKER;
    if (num_markers * MAX_DATA_BYTES_IN_MARKER != icc_data_len)
        num_markers++;

    while (icc_data_len > 0) {
        // length of profile to put in this marker
        length = icc_data_len;
        if (length > MAX_DATA_BYTES_IN_MARKER)
            length = MAX_DATA_BYTES_IN_MARKER;
        icc_data_len -= length;

        // Write the JPEG marker header (APP2 code and marker length)
        jpeg_write_m_header(cinfo, ICC_MARKER,
                            (unsigned int) (length + ICC_OVERHEAD_LEN));

        // Write the marker identifying string "ICC_PROFILE" (null-terminated).
        // We code it in this less-than-transparent way so that the code works
        // even if the local character set is not ASCII.
        jpeg_write_m_byte(cinfo, 0x49);
        jpeg_write_m_byte(cinfo, 0x43);
        jpeg_write_m_byte(cinfo, 0x43);
        jpeg_write_m_byte(cinfo, 0x5F);
        jpeg_write_m_byte(cinfo, 0x50);
        jpeg_write_m_byte(cinfo, 0x52);
        jpeg_write_m_byte(cinfo, 0x4F);
        jpeg_write_m_byte(cinfo, 0x46);
        jpeg_write_m_byte(cinfo, 0x49);
        jpeg_write_m_byte(cinfo, 0x4C);
        jpeg_write_m_byte(cinfo, 0x45);
        jpeg_write_m_byte(cinfo, 0x0);

        // Add the sequencing info
        jpeg_write_m_byte(cinfo, cur_marker);
        jpeg_write_m_byte(cinfo, (int) num_markers);

        // Add the profile data
        while (length--) {
            jpeg_write_m_byte(cinfo, *icc_data_ptr);
            icc_data_ptr++;
        }
        cur_marker++;
    }
}

//
// End of code from iccjpeg.c
//
///////////////////////////////////////////////////////////////////////////////

static struct my_error_mgr
{
    struct  jpeg_error_mgr pub;  // "public" fields
    //	LPVOID  Cargo;               // "private" fields
} ErrorHandler; 

void my_writer_error_handler (j_common_ptr cinfo)
{
    char buffer[JMSG_LENGTH_MAX];
    (*cinfo->err->format_message) (cinfo, buffer);
    throw std::runtime_error( std::string(buffer) );
}

void my_writer_output_message (j_common_ptr cinfo)
{
    char buffer[JMSG_LENGTH_MAX];
    (*cinfo->err->format_message) (cinfo, buffer);
    throw std::runtime_error( std::string(buffer) );
}

struct JpegWriterParams
{
    JpegWriterParams()
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
                luminanceMapping_ = it->second.as<LumMappingMethod>(luminanceMapping_);
                continue;
            }
        }
    }

    size_t quality_;
    float minLuminance_;
    float maxLuminance_;
    LumMappingMethod luminanceMapping_;
};

ostream& operator<<(ostream& out, const JpegWriterParams& params)
{
    stringstream ss;
    ss << "JpegWriterParams: [";
    ss << "quality: " << params.quality_ << ", ";
    ss << "min_luminance: " << params.minLuminance_ << ", ";
    ss << "max_luminance: " << params.maxLuminance_ << ", ";
    ss << "mapping_method: " << params.luminanceMapping_ << "]";

    return (out << ss.str());
}

class JpegWriterImpl
{
public:
    JpegWriterImpl()
        : m_filesize(0)
    {}

    virtual ~JpegWriterImpl()
    {}

    virtual bool open( size_t bufferSize ) = 0;
    virtual void close() = 0;

    // get an handle to the underlying FILE*
    virtual FILE* handle() = 0;

    // compute size (if functionality available)
    virtual void computeSize() = 0;

    bool write(const pfs::Frame &frame, const JpegWriterParams& params)
    {
        cmsUInt32Number cmsProfileSize = 0;
        ScopedCmsProfile hsRGB( cmsCreate_sRGBProfile() );

        cmsSaveProfileToMem(hsRGB.data(), NULL, &cmsProfileSize);           // get the size

        std::vector<JOCTET> cmsOutputProfile(cmsProfileSize);

        cmsSaveProfileToMem(hsRGB.data(), cmsOutputProfile.data(), &cmsProfileSize);    //

        qDebug() << "sRGB profile size: " << cmsProfileSize;

        struct jpeg_compress_struct cinfo;
        cinfo.err                        = jpeg_std_error(&ErrorHandler.pub);
        ErrorHandler.pub.error_exit      = my_writer_error_handler;
        ErrorHandler.pub.output_message  = my_writer_output_message;

        jpeg_create_compress(&cinfo);

        cinfo.image_width           = frame.getWidth();                 // image width and height, in pixels
        cinfo.image_height          = frame.getHeight();
        cinfo.input_components      = cinfo.num_components = 3;         // # of color components per pixel
        cinfo.in_color_space        = JCS_RGB;                          // colorspace of input image
        cinfo.jpeg_color_space      = JCS_YCbCr;
        cinfo.density_unit          = 1;                                // dots/inch
        cinfo.X_density             = cinfo.Y_density = 72;

        jpeg_set_defaults(&cinfo);
        jpeg_set_colorspace(&cinfo, JCS_YCbCr);

        // avoid subsampling on high quality factor
        jpeg_set_quality(&cinfo, params.quality_, 1);
        if ( params.quality_ >= 70 ) {
            for (int i = 0; i < cinfo.num_components; i++) {
                cinfo.comp_info[i].h_samp_factor = 1;
                cinfo.comp_info[i].v_samp_factor = 1;
            }
        }

        // open output file!
        if ( !open(cinfo.image_width*cinfo.image_height*cinfo.num_components) ) {
            std::cerr << "Cannot open the output stream";
            return false;
        }

        try
        {
            const Channel* rChannel;
            const Channel* gChannel;
            const Channel* bChannel;
            frame.getXYZChannels(rChannel, gChannel, bChannel);

            const float* redData = rChannel->getRawData();
            const float* greenData = gChannel->getRawData();
            const float* blueData = bChannel->getRawData();

            jpeg_stdio_dest(&cinfo, handle());
            jpeg_start_compress(&cinfo, true);

            write_icc_profile(&cinfo, cmsOutputProfile.data(), cmsProfileSize);

            // If an exception is raised, this buffer gets automatically destructed!
            std::vector<JSAMPLE> scanLineOut(cinfo.image_width * cinfo.num_components);
            JSAMPROW scanLineOutArray[1] = { scanLineOut.data() };

            while ( cinfo.next_scanline < cinfo.image_height )
            {
                // copy line from Frame into scanLineOut
                planarToInterleaved(redData + cinfo.next_scanline*cinfo.image_width,
                                    greenData + cinfo.next_scanline*cinfo.image_width,
                                    blueData + cinfo.next_scanline*cinfo.image_width,
                                    scanLineOut.data(), RGB_FORMAT,
                                    cinfo.image_width,
                                    FloatRgbToQRgb(params.minLuminance_,
                                                   params.maxLuminance_,
                                                   params.luminanceMapping_)
                                    );

                jpeg_write_scanlines(&cinfo, scanLineOutArray, 1);
            }
        }
        catch (const std::runtime_error& err)
        {
            qDebug() << err.what();
            jpeg_destroy_compress(&cinfo);

            return false;
        }

        jpeg_finish_compress(&cinfo);
        jpeg_destroy_compress(&cinfo);

        computeSize();
        close();

        return true;
    }

    size_t getFileSize()
    { return m_filesize; }

public:
    size_t m_filesize;
};

struct JpegWriterImplMemory : public JpegWriterImpl
{
    JpegWriterImplMemory()
        : JpegWriterImpl()
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
    ResouceHandlerFile m_handle;
#ifdef USE_TEMPORARY_FILE
    QTemporaryFile m_temporaryFile;
#else
    std::vector<char> m_temporaryBuffer;
#endif
};

struct JpegWriterImplFile : public JpegWriterImpl
{
    JpegWriterImplFile(const string& filename)
        : JpegWriterImpl()
        , m_handle()
        , m_filename(filename)
    {}

    virtual bool open(size_t /*bufferSize*/)
    {
        m_handle.reset( fopen(m_filename.c_str(), "wb") );
        if ( m_handle ) return true;
        return false;
    }

    virtual void close()
    { m_handle.reset(); }

    FILE* handle()
    { return m_handle.data(); }

    void computeSize()
    { m_filesize = 0; }

private:
    ResouceHandlerFile m_handle;
    std::string m_filename;
};


JpegWriter::JpegWriter()
    : m_impl(new JpegWriterImplMemory)
{}

JpegWriter::JpegWriter(const std::string &filename)
    : m_impl(new JpegWriterImplFile(filename))
{}

JpegWriter::~JpegWriter()
{}

bool JpegWriter::write(const pfs::Frame& frame, const Params& params)
{
    JpegWriterParams p;
    p.parse( params );

#ifndef NDEBUG
    cout << p << endl << flush;
#endif

    return m_impl->write(frame, p);
}

size_t JpegWriter::getFileSize()
{
    return m_impl->getFileSize();
}
