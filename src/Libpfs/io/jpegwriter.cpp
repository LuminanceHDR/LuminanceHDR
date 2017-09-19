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

#include <Libpfs/io/jpegwriter.h>

#include <algorithm>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <vector>

#include <jpeglib.h>
#include <lcms2.h>
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

///////////////////////////////////////////////////////////////////////////////
//
// This code is taken from iccjpeg.c from lcms distribution
//
// Since an ICC profile can be larger than the maximum size of a JPEG marker
// (64K), we need provisions to split it into multiple markers.  The format
// defined by the ICC specifies one or more APP2 markers containing the
// following data:
//    Identifying string    ASCII "ICC_PROFILE\0"  (12 bytes)
//    Marker sequence number    1 for first APP2, 2 for next, etc (1 byte)
//    Number of markers    Total number of APP2's used (1 byte)
//      Profile data        (remainder of APP2 data)
// Decoders should use the marker sequence numbers to reassemble the profile,
// rather than assuming that the APP2 markers appear in the correct sequence.
//

#define ICC_MARKER (JPEG_APP0 + 2) /* JPEG marker code for ICC */
#define ICC_OVERHEAD_LEN 14        /* size of non-profile data in APP2 */
#define MAX_BYTES_IN_MARKER 65533  /* maximum data len of a JPEG marker */
#define MAX_DATA_BYTES_IN_MARKER (MAX_BYTES_IN_MARKER - ICC_OVERHEAD_LEN)

// This routine writes the given ICC profile data into a JPEG file.
// It *must* be called AFTER calling jpeg_start_compress() and BEFORE
// the first call to jpeg_write_scanlines().
// (This ordering ensures that the APP2 marker(s) will appear after the
// SOI and JFIF or Adobe markers, but before all else.)

static void write_icc_profile(j_compress_ptr cinfo, const JOCTET *icc_data_ptr,
                              unsigned int icc_data_len) {
    unsigned int num_markers;  // total number of markers we'll write
    int cur_marker = 1;        // per spec, counting starts at 1
    unsigned int length;       // number of bytes to write in this marker

    // Calculate the number of markers we'll need, rounding up of course
    num_markers = icc_data_len / MAX_DATA_BYTES_IN_MARKER;
    if (num_markers * MAX_DATA_BYTES_IN_MARKER != icc_data_len) num_markers++;

    while (icc_data_len > 0) {
        // length of profile to put in this marker
        length = icc_data_len;
        if (length > MAX_DATA_BYTES_IN_MARKER)
            length = MAX_DATA_BYTES_IN_MARKER;
        icc_data_len -= length;

        // Write the JPEG marker header (APP2 code and marker length)
        jpeg_write_m_header(cinfo, ICC_MARKER,
                            (unsigned int)(length + ICC_OVERHEAD_LEN));

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
        jpeg_write_m_byte(cinfo, (int)num_markers);

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

static void my_writer_error_handler(j_common_ptr cinfo) {
    char buffer[JMSG_LENGTH_MAX];
    (*cinfo->err->format_message)(cinfo, buffer);
    throw std::runtime_error(std::string(buffer));
}

static void my_writer_output_message(j_common_ptr cinfo) {
    char buffer[JMSG_LENGTH_MAX];
    (*cinfo->err->format_message)(cinfo, buffer);
    throw std::runtime_error(std::string(buffer));
}

struct JpegWriterParams {
    JpegWriterParams()
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

    size_t quality_;
    float minLuminance_;
    float maxLuminance_;
    RGBMappingType luminanceMapping_;
};

ostream &operator<<(ostream &out, const JpegWriterParams &params) {
    stringstream ss;
    ss << "JpegWriterParams: [";
    ss << "quality: " << params.quality_ << ", ";
    ss << "min_luminance: " << params.minLuminance_ << ", ";
    ss << "max_luminance: " << params.maxLuminance_ << ", ";
    ss << "mapping_method: " << params.luminanceMapping_ << "]";

    return (out << ss.str());
}

class JpegWriterImpl {
   public:
    JpegWriterImpl() {}
    virtual ~JpegWriterImpl() {}

    virtual void setupJpegDest(j_compress_ptr cinfo,
                               const std::string &filename) = 0;
    virtual void close() = 0;
    virtual size_t getFileSize() const = 0;

    bool write(const pfs::Frame &frame, const JpegWriterParams &params,
               const std::string &filename) {
        cmsUInt32Number cmsProfileSize = 0;
        utils::ScopedCmsProfile hsRGB(cmsCreate_sRGBProfile());

        cmsSaveProfileToMem(hsRGB.data(), NULL,
                            &cmsProfileSize);  // get the size

        std::vector<JOCTET> cmsOutputProfile(cmsProfileSize);

        cmsSaveProfileToMem(hsRGB.data(), cmsOutputProfile.data(),
                            &cmsProfileSize);

        struct jpeg_compress_struct cinfo;
        struct jpeg_error_mgr errorHandler;

        jpeg_create_compress(&cinfo);

        cinfo.err = jpeg_std_error(&errorHandler);
        errorHandler.error_exit = my_writer_error_handler;
        errorHandler.output_message = my_writer_output_message;

        cinfo.image_width =
            frame.getWidth();  // image width and height, in pixels
        cinfo.image_height = frame.getHeight();
        cinfo.input_components = cinfo.num_components =
            3;                           // # of color components per pixel
        cinfo.in_color_space = JCS_RGB;  // colorspace of input image
        cinfo.jpeg_color_space = JCS_YCbCr;
        cinfo.density_unit = 1;  // dots/inch
        cinfo.X_density = cinfo.Y_density = 72;

        jpeg_set_defaults(&cinfo);
        jpeg_set_colorspace(&cinfo, JCS_YCbCr);

        // avoid subsampling on high quality factor
        jpeg_set_quality(&cinfo, params.quality_, 1);
        if (params.quality_ >= 70) {
            for (int i = 0; i < cinfo.num_components; i++) {
                cinfo.comp_info[i].h_samp_factor = 1;
                cinfo.comp_info[i].v_samp_factor = 1;
            }
        }

        try {
            setupJpegDest(&cinfo, filename);

            jpeg_start_compress(&cinfo, true);

            const Channel *rChannel;
            const Channel *gChannel;
            const Channel *bChannel;
            frame.getXYZChannels(rChannel, gChannel, bChannel);

            write_icc_profile(&cinfo, cmsOutputProfile.data(), cmsProfileSize);

            // If an exception is raised, this buffer gets automatically
            // destructed!
            std::vector<JSAMPLE> scanLineOut(cinfo.image_width *
                                             cinfo.num_components);
            JSAMPROW scanLineOutArray[1] = {scanLineOut.data()};

            while (cinfo.next_scanline < cinfo.image_height) {
                // copy line from Frame into scanLineOut
                utils::transform(
                    rChannel->row_begin(cinfo.next_scanline),
                    rChannel->row_end(cinfo.next_scanline),
                    gChannel->row_begin(cinfo.next_scanline),
                    bChannel->row_begin(cinfo.next_scanline),
                    FixedStrideIterator<JSAMPLE *, 3>(scanLineOut.data()),
                    FixedStrideIterator<JSAMPLE *, 3>(scanLineOut.data() + 1),
                    FixedStrideIterator<JSAMPLE *, 3>(scanLineOut.data() + 2),
                    utils::chain(colorspace::Normalizer(params.minLuminance_,
                                                        params.maxLuminance_),
                                 utils::CLAMP_F32,
                                 Remapper<JSAMPLE>(params.luminanceMapping_)));
                jpeg_write_scanlines(&cinfo, scanLineOutArray, 1);
            }
        } catch (const std::runtime_error &err) {
            std::clog << err.what() << std::endl;

            jpeg_destroy_compress(&cinfo);

            return false;
        }

        jpeg_finish_compress(&cinfo);
        jpeg_destroy_compress(&cinfo);

        close();

        return true;
    }
};

//! \ref
//! http://www.andrewewhite.net/wordpress/2010/04/07/simple-cc-jpeg-writer-part-2-write-to-buffer-in-memory/
typedef std::vector<JOCTET> JpegBuffer;

struct JpegWriterImplMemory : public JpegWriterImpl {
    typedef std::map<j_compress_ptr, JpegBuffer *> JpegRegistry;

    JpegWriterImplMemory() : JpegWriterImpl(), m_cinfo(NULL), m_buffer(0) {}

    ~JpegWriterImplMemory() { close(); }

    // implementation below!
    void setupJpegDest(j_compress_ptr cinfo, const std::string &filename);

    void close() {
        sm_registry.erase(m_cinfo);
        m_cinfo->dest = NULL;
    }
    size_t getFileSize() const { return (m_buffer.size() * sizeof(JOCTET)); }

    static JpegBuffer &getBuffer(j_compress_ptr cinfo) {
        JpegRegistry::iterator it = sm_registry.find(cinfo);
        if (it != sm_registry.end()) {
            return *it->second;
        }
        throw pfs::io::WriteException(
            "Cannot find a valid buffer for the current writer");
    }

   private:
    struct jpeg_destination_mgr m_dmgr;
    j_compress_ptr m_cinfo;
    JpegBuffer m_buffer;

    static JpegRegistry sm_registry;
};

JpegWriterImplMemory::JpegRegistry JpegWriterImplMemory::sm_registry;

#define BLOCK_SIZE 16384

static void my_init_destination(j_compress_ptr cinfo) {
    JpegBuffer &myBuffer = JpegWriterImplMemory::getBuffer(cinfo);

    myBuffer.resize(BLOCK_SIZE);
    cinfo->dest->next_output_byte = &myBuffer[0];
    cinfo->dest->free_in_buffer = myBuffer.size();
}

static boolean my_empty_output_buffer(j_compress_ptr cinfo) {
    JpegBuffer &myBuffer = JpegWriterImplMemory::getBuffer(cinfo);

    size_t oldsize = myBuffer.size();
    myBuffer.resize(oldsize + BLOCK_SIZE);
    cinfo->dest->next_output_byte = &myBuffer[oldsize];
    cinfo->dest->free_in_buffer = myBuffer.size() - oldsize;
    return true;
}

static void my_term_destination(j_compress_ptr cinfo) {
    JpegBuffer &myBuffer = JpegWriterImplMemory::getBuffer(cinfo);

    myBuffer.resize(myBuffer.size() - cinfo->dest->free_in_buffer);
}
#undef BLOCK_SIZE

void JpegWriterImplMemory::setupJpegDest(j_compress_ptr cinfo,
                                         const string & /*filename*/) {
    // remember the j_compress_struct ... it will be necessary in the dtor!
    m_cinfo = cinfo;
    // add entry into registry!
    sm_registry.insert(JpegRegistry::value_type(cinfo, &m_buffer));

    // update destinations...
    m_dmgr.init_destination = my_init_destination;
    m_dmgr.empty_output_buffer = my_empty_output_buffer;
    m_dmgr.term_destination = my_term_destination;

    cinfo->dest = &m_dmgr;
}

//! \brief Writer to file basic implementation
struct JpegWriterImplFile : public JpegWriterImpl {
    JpegWriterImplFile() : JpegWriterImpl(), m_handle() {}

    void setupJpegDest(j_compress_ptr cinfo, const std::string &filename) {
        open(filename);
        jpeg_stdio_dest(cinfo, handle());
    }

    void close() { m_handle.reset(); }
    size_t getFileSize() const { return 0; }

   private:
    void open(const std::string &filename) {
        m_handle.reset(fopen(filename.c_str(), "wb"));
        if (!m_handle) {
            throw pfs::io::InvalidFile("Cannot open the output file " +
                                       filename);
        }
    }

    FILE *handle() { return m_handle.data(); }

    utils::ScopedStdIoFile m_handle;
};

JpegWriter::JpegWriter() : m_impl(new JpegWriterImplMemory()) {}

JpegWriter::JpegWriter(const std::string &filename)
    : FrameWriter(filename), m_impl(new JpegWriterImplFile()) {}

JpegWriter::~JpegWriter() {}

bool JpegWriter::write(const pfs::Frame &frame, const Params &params) {
    JpegWriterParams p;
    p.parse(params);

#ifndef NDEBUG
    cout << p << endl << flush;
#endif

    return m_impl->write(frame, p, filename());
}

size_t JpegWriter::getFileSize() const { return m_impl->getFileSize(); }

}  // io
}  // pfs
