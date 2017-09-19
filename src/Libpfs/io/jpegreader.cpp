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
 */

#include <Libpfs/io/jpegreader.h>

#include <Libpfs/colorspace/cmyk.h>
#include <Libpfs/colorspace/copy.h>
#include <Libpfs/colorspace/lcms.h>
#include <Libpfs/fixedstrideiterator.h>
#include <Libpfs/frame.h>
#include <Libpfs/utils/resourcehandlerlcms.h>
#include <Libpfs/utils/resourcehandlerstdio.h>
#include <Libpfs/utils/transform.h>

#include <jpeglib.h>
#include <cassert>
#include <iostream>

using namespace pfs;

#ifndef NDEBUG
#define PRINT_DEBUG(str) std::cerr << str << std::endl
#else
#define PRINT_DEBUG(str)
#endif

///////////////////////////////////////////////////////////////////////////////
//
// Code taken from iccjpeg.c from lcms distribution
//
//

#define ICC_MARKER (JPEG_APP0 + 2) /* JPEG marker code for ICC */
#define ICC_OVERHEAD_LEN 14        /* size of non-profile data in APP2 */
#define MAX_BYTES_IN_MARKER 65533  /* maximum data len of a JPEG marker */
#define MAX_DATA_BYTES_IN_MARKER (MAX_BYTES_IN_MARKER - ICC_OVERHEAD_LEN)

// Prepare for reading an ICC profile
static void setup_read_icc_profile(j_decompress_ptr cinfo) {
    // Tell the library to keep any APP2 data it may find
    jpeg_save_markers(cinfo, ICC_MARKER, 0xFFFF);
}

// Handy subroutine to test whether a saved marker is an ICC profile marker.
static boolean marker_is_icc(jpeg_saved_marker_ptr marker) {
    return marker->marker == ICC_MARKER &&
           marker->data_length >= ICC_OVERHEAD_LEN &&
           // verify the identifying string
           GETJOCTET(marker->data[0]) == 0x49 &&
           GETJOCTET(marker->data[1]) == 0x43 &&
           GETJOCTET(marker->data[2]) == 0x43 &&
           GETJOCTET(marker->data[3]) == 0x5F &&
           GETJOCTET(marker->data[4]) == 0x50 &&
           GETJOCTET(marker->data[5]) == 0x52 &&
           GETJOCTET(marker->data[6]) == 0x4F &&
           GETJOCTET(marker->data[7]) == 0x46 &&
           GETJOCTET(marker->data[8]) == 0x49 &&
           GETJOCTET(marker->data[9]) == 0x4C &&
           GETJOCTET(marker->data[10]) == 0x45 &&
           GETJOCTET(marker->data[11]) == 0x0;
}

static boolean read_icc_profile(j_decompress_ptr cinfo, JOCTET **icc_data_ptr,
                                unsigned int *icc_data_len) {
    jpeg_saved_marker_ptr marker;
    int num_markers = 0;
    int seq_no;
    JOCTET *icc_data;
    unsigned int total_length;
#define MAX_SEQ_NO 255 /* sufficient since marker numbers are bytes */
    char marker_present[MAX_SEQ_NO + 1]; /* 1 if marker found */
    unsigned int
        data_length[MAX_SEQ_NO + 1]; /* size of profile data in marker */
    unsigned int data_offset[MAX_SEQ_NO + 1]; /* offset for data in marker */

    *icc_data_ptr = NULL; /* avoid confusion if false return */
    *icc_data_len = 0;

    /* This first pass over the saved markers discovers whether there are
   * any ICC markers and verifies the consistency of the marker numbering.
   */

    for (seq_no = 1; seq_no <= MAX_SEQ_NO; seq_no++) marker_present[seq_no] = 0;

    for (marker = cinfo->marker_list; marker != NULL; marker = marker->next) {
        if (marker_is_icc(marker)) {
            if (num_markers == 0)
                num_markers = GETJOCTET(marker->data[13]);
            else if (num_markers != GETJOCTET(marker->data[13])) {
                PRINT_DEBUG("inconsistent num_markers fields");
                return false; /* inconsistent num_markers fields */
            }
            seq_no = GETJOCTET(marker->data[12]);
            if (seq_no <= 0 || seq_no > num_markers) {
                PRINT_DEBUG("bogus sequence number");
                return false; /* bogus sequence number */
            }
            if (marker_present[seq_no]) {
                PRINT_DEBUG("duplicate sequence numbers");
                return false; /* duplicate sequence numbers */
            }
            marker_present[seq_no] = 1;
            data_length[seq_no] = marker->data_length - ICC_OVERHEAD_LEN;
        }
    }

    if (num_markers == 0) {
        PRINT_DEBUG("num_markers = 0");
        return false;
    }

    /* Check for missing markers, count total space needed,
   * compute offset of each marker's part of the data.
   */

    total_length = 0;
    for (seq_no = 1; seq_no <= num_markers; seq_no++) {
        if (marker_present[seq_no] == 0) {
            PRINT_DEBUG("missing sequence number");
            return false; /* missing sequence number */
        }
        data_offset[seq_no] = total_length;
        total_length += data_length[seq_no];
    }

    // if (total_length <= 0) { // total_length is unsigned
    if (total_length == 0) {
        PRINT_DEBUG("found only empty markers?");
        return false; /* found only empty markers? */
    }

    /* Allocate space for assembled data */
    icc_data = (JOCTET *)malloc(total_length * sizeof(JOCTET));
    if (icc_data == NULL) return false; /* oops, out of memory */

    /* and fill it in */
    for (marker = cinfo->marker_list; marker != NULL; marker = marker->next) {
        if (marker_is_icc(marker)) {
            JOCTET FAR *src_ptr;
            JOCTET *dst_ptr;
            unsigned int length;
            seq_no = GETJOCTET(marker->data[12]);
            dst_ptr = icc_data + data_offset[seq_no];
            src_ptr = marker->data + ICC_OVERHEAD_LEN;
            length = data_length[seq_no];
            while (length--) {
                *dst_ptr++ = *src_ptr++;
            }
        }
    }

    *icc_data_ptr = icc_data;
    *icc_data_len = total_length;

    return true;
}
//
//
// End of code from iccjpeg.c
//
///////////////////////////////////////////////////////////////////////////////

static void my_error_handler(j_common_ptr cinfo) {
    char buffer[JMSG_LENGTH_MAX];
    (*cinfo->err->format_message)(cinfo, buffer);
    throw pfs::io::ReadException(std::string(buffer));
}

static void my_output_message(j_common_ptr cinfo) {
    char buffer[JMSG_LENGTH_MAX];
    (*cinfo->err->format_message)(cinfo, buffer);
    throw pfs::io::ReadException(std::string(buffer));
}

namespace pfs {
namespace io {

struct JpegReader::JpegReaderData {
    struct jpeg_decompress_struct cinfo_;
    struct jpeg_error_mgr err_;

    utils::ScopedStdIoFile file_;

    inline j_decompress_ptr cinfo() { return &cinfo_; }

    inline FILE *handle() { return file_.data(); }
};

JpegReader::JpegReader(const std::string &filename)
    : FrameReader(filename), m_data(new JpegReaderData) {
    JpegReader::open();
}

JpegReader::~JpegReader() { close(); }

bool JpegReader::isOpen() const { return m_data->file_; }

void JpegReader::close() {
    // destroy decompress structure
    jpeg_destroy_decompress(m_data->cinfo());
    // close open file
    m_data->file_.reset();
}

void JpegReader::open() {
    if (isOpen()) close();

    // setup error
    m_data->cinfo()->err = jpeg_std_error(&m_data->err_);
    m_data->cinfo()->err->error_exit = my_error_handler;
    m_data->cinfo()->err->output_message = my_output_message;

    // open the stream, read the header and setup all the rest :)
    m_data->file_.reset(fopen(filename().c_str(), "rb"));
    if (!(m_data->handle())) {
        throw pfs::io::InvalidFile("Cannot open file " + filename());
    }

    jpeg_create_decompress(m_data->cinfo());
    jpeg_stdio_src(m_data->cinfo(), m_data->handle());
    setup_read_icc_profile(m_data->cinfo());
    jpeg_read_header(m_data->cinfo(), true);

    PRINT_DEBUG("Readed JPEG headers");
    PRINT_DEBUG(
        "cinfo.jpeg_color_space: " << m_data->cinfo()->jpeg_color_space);

    if (m_data->cinfo()->jpeg_color_space == JCS_GRAYSCALE) {
        PRINT_DEBUG("Unsupported color space: grayscale");
        close();

        throw pfs::io::ReadException("Unsupported color space: grayscale");
    }

    if (m_data->cinfo()->jpeg_color_space == JCS_YCCK) {
        PRINT_DEBUG("Converting to CMYK");
        m_data->cinfo()->out_color_space = JCS_CMYK;
    }

    PRINT_DEBUG("cinfo.num_components: " << m_data->cinfo()->num_components);

    setWidth(m_data->cinfo()->image_width);
    setHeight(m_data->cinfo()->image_height);
}

static cmsHTRANSFORM getColorSpaceTransform(j_decompress_ptr cinfo) {
    unsigned int cmsProfileLength;
    JOCTET *cmsProfileBuffer;
    if (!read_icc_profile(cinfo, &cmsProfileBuffer, &cmsProfileLength)) {
        return NULL;
    }
    PRINT_DEBUG("Found embedded profile");

    utils::ScopedCmsProfile hsRGB(cmsCreate_sRGBProfile());
    utils::ScopedCmsProfile hIn(
        cmsOpenProfileFromMem(cmsProfileBuffer, cmsProfileLength));
    free(cmsProfileBuffer);

    if ((!hIn) || (!hsRGB)) {  // one of the two profile is not valid
        return NULL;
    }

#ifndef NDEBUG
    if (cmsGetColorSpace(hIn.data()) == cmsSigRgbData)
        PRINT_DEBUG("Image format = sRGB");
    else if (cmsGetColorSpace(hIn.data()) == cmsSigCmykData)
        PRINT_DEBUG("Image format = CMYK");
    else if (cmsGetColorSpace(hIn.data()) == cmsSigYCbCrData)
        PRINT_DEBUG("Image format = YCbCr");
    else if (cmsGetColorSpace(hIn.data()) == cmsSigLuvKData)
        PRINT_DEBUG("Image format = LuvK");
#endif

    switch (cinfo->jpeg_color_space) {
        case JCS_RGB:
        case JCS_YCbCr: {
            PRINT_DEBUG("Transform colorspace = sRGB");
            return cmsCreateTransform(hIn.data(), TYPE_RGB_8, hsRGB.data(),
                                      TYPE_RGB_FLT, /*TYPE_BGRA_8,*/
                                      INTENT_PERCEPTUAL, 0);

        } break;
        case JCS_CMYK:
        case JCS_YCCK: {
            PRINT_DEBUG("Transform colorspace = CMYK");
            return cmsCreateTransform(hIn.data(), TYPE_YUVK_8, hsRGB.data(),
                                      TYPE_RGB_FLT, /*TYPE_BGRA_8,*/
                                      INTENT_PERCEPTUAL, 0);
        } break;
        default:
            // This case should never happen, but at least the compiler
            // stops complaining!
            return NULL;
            break;
    }

    return NULL;
}

//! \brief read from a 3 components (RGB) input JPEG file
template <typename Converter>
static void read3Components(j_decompress_ptr cinfo, Frame &frame,
                            const Converter &conv) {
    Channel *red;
    Channel *green;
    Channel *blue;

    frame.createXYZChannels(red, green, blue);

    std::vector<JSAMPLE> scanLineBuffer(cinfo->image_width *
                                        cinfo->num_components);
    JSAMPROW scanLineBufferArray[1] = {scanLineBuffer.data()};

    for (int i = 0; cinfo->output_scanline < cinfo->output_height; ++i) {
        jpeg_read_scanlines(cinfo, scanLineBufferArray, 1);

        utils::transform(
            FixedStrideIterator<JSAMPLE *, 3>(scanLineBuffer.data()),
            FixedStrideIterator<JSAMPLE *, 3>(scanLineBuffer.data() +
                                              cinfo->image_width * 3),
            FixedStrideIterator<JSAMPLE *, 3>(scanLineBuffer.data() + 1),
            FixedStrideIterator<JSAMPLE *, 3>(scanLineBuffer.data() + 2),
            red->row_begin(i), green->row_begin(i), blue->row_begin(i), conv);
    }
}

//! \brief read from a 4 components (CMYK) input JPEG file
template <typename Converter>
static void read4Components(j_decompress_ptr cinfo, Frame &frame,
                            const Converter &conv) {
    Channel *red;
    Channel *green;
    Channel *blue;

    frame.createXYZChannels(red, green, blue);

    std::vector<JSAMPLE> scanLineBuffer(cinfo->image_width *
                                        cinfo->num_components);
    JSAMPROW scanLineBufferArray[1] = {scanLineBuffer.data()};

    for (int i = 0; cinfo->output_scanline < cinfo->output_height; ++i) {
        jpeg_read_scanlines(cinfo, scanLineBufferArray, 1);

        utils::transform(
            FixedStrideIterator<JSAMPLE *, 4>(scanLineBuffer.data()),  // C
            FixedStrideIterator<JSAMPLE *, 4>(scanLineBuffer.data() +
                                              cinfo->image_width * 4),  // end C
            FixedStrideIterator<JSAMPLE *, 4>(scanLineBuffer.data() + 1),  // M
            FixedStrideIterator<JSAMPLE *, 4>(scanLineBuffer.data() + 2),  // Y
            FixedStrideIterator<JSAMPLE *, 4>(scanLineBuffer.data() + 3),  // K
            red->row_begin(i), green->row_begin(i),
            blue->row_begin(i),  // R G B
            conv);
    }
}

void JpegReader::read(Frame &frame, const Params &params) {
    try {
        Frame tempFrame(width(), height());

        jpeg_start_decompress(m_data->cinfo());

        assert(m_data->cinfo()->image_height != 0);
        assert(m_data->cinfo()->image_width != 0);
        assert(m_data->cinfo()->output_height != 0);
        assert(m_data->cinfo()->output_width != 0);
        assert(m_data->cinfo()->image_height == m_data->cinfo()->output_height);
        assert(m_data->cinfo()->image_width == m_data->cinfo()->output_width);

        utils::ScopedCmsTransform xform(
            getColorSpaceTransform(m_data->cinfo()));

        switch (m_data->cinfo()->jpeg_color_space) {
            case JCS_RGB:
            case JCS_YCbCr: {
                if (xform) {
                    PRINT_DEBUG("Use LCMS RGB");
                    read3Components(m_data->cinfo(), tempFrame,
                                    colorspace::Convert3LCMS3(xform.data()));
                } else {
                    read3Components(m_data->cinfo(), tempFrame,
                                    colorspace::Copy());
                }
            } break;
            case JCS_CMYK:
            case JCS_YCCK: {
                if (xform) {
                    PRINT_DEBUG("Use LCMS CMYK");
                    read4Components(m_data->cinfo(), tempFrame,
                                    colorspace::Convert4LCMS3(xform.data()));
                } else {
                    read4Components(m_data->cinfo(), tempFrame,
                                    colorspace::ConvertInvertedCMYK2RGB());
                }
            } break;
            default:
                // This case should never happen, but at least the compiler
                // stops complaining!
                break;
        }

        jpeg_finish_decompress(m_data->cinfo());
        jpeg_destroy_decompress(m_data->cinfo());

        FrameReader::read(tempFrame, params);
        frame.swap(tempFrame);
    } catch (...) {
        close();
        throw;
    }
}

}  // io
}  // pfs
