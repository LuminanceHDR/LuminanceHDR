/**
 * This file is a part of Luminance HDR package.
 * ----------------------------------------------------------------------
 * Copyright (C) 2012 Franco Comida, Davide Anastasia
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
 * @author Franco Comida <fcomida@users.sourceforge.net>
 * Original work
 * @author Davide Anastasia <davideanastasia@users.sourceforge.net>
 * clean up memory management
 *
 */

#include <QDebug>
#include <QFile>
#include <QSharedPointer>
#include <stdexcept>
#include <vector>
#include <algorithm>

#include <stdio.h>
#include <lcms2.h>
#include <jpeglib.h>

#if defined(WIN32) || defined(__APPLE__)
#include <QTemporaryFile>
#endif

#include "jpegwriter.h"
#include "Common/ResourceHandlerCommon.h"
#include "Common/ResourceHandlerLcms.h"

///////////////////////////////////////////////////////////////////////////////
//
// This code is taken from iccjpeg.c from lcms distribution
/*
 * Since an ICC profile can be larger than the maximum size of a JPEG marker
 * (64K), we need provisions to split it into multiple markers.  The format
 * defined by the ICC specifies one or more APP2 markers containing the
 * following data:
 *	Identifying string	ASCII "ICC_PROFILE\0"  (12 bytes)
 *	Marker sequence number	1 for first APP2, 2 for next, etc (1 byte)
 *	Number of markers	Total number of APP2's used (1 byte)
 *      Profile data		(remainder of APP2 data)
 * Decoders should use the marker sequence numbers to reassemble the profile,
 * rather than assuming that the APP2 markers appear in the correct sequence.
 */

#define ICC_MARKER  (JPEG_APP0 + 2)	/* JPEG marker code for ICC */
#define ICC_OVERHEAD_LEN  14		/* size of non-profile data in APP2 */
#define MAX_BYTES_IN_MARKER  65533	/* maximum data len of a JPEG marker */
#define MAX_DATA_BYTES_IN_MARKER  (MAX_BYTES_IN_MARKER - ICC_OVERHEAD_LEN)

/*
 * This routine writes the given ICC profile data into a JPEG file.
 * It *must* be called AFTER calling jpeg_start_compress() and BEFORE
 * the first call to jpeg_write_scanlines().
 * (This ordering ensures that the APP2 marker(s) will appear after the
 * SOI and JFIF or Adobe markers, but before all else.)
 */

void
write_icc_profile (j_compress_ptr cinfo,
		   const JOCTET *icc_data_ptr,
		   unsigned int icc_data_len)
{
  unsigned int num_markers;	/* total number of markers we'll write */
  int cur_marker = 1;		/* per spec, counting starts at 1 */
  unsigned int length;		/* number of bytes to write in this marker */

  /* Calculate the number of markers we'll need, rounding up of course */
  num_markers = icc_data_len / MAX_DATA_BYTES_IN_MARKER;
  if (num_markers * MAX_DATA_BYTES_IN_MARKER != icc_data_len)
    num_markers++;

  while (icc_data_len > 0) {
    /* length of profile to put in this marker */
    length = icc_data_len;
    if (length > MAX_DATA_BYTES_IN_MARKER)
      length = MAX_DATA_BYTES_IN_MARKER;
    icc_data_len -= length;

    /* Write the JPEG marker header (APP2 code and marker length) */
    jpeg_write_m_header(cinfo, ICC_MARKER,
			(unsigned int) (length + ICC_OVERHEAD_LEN));

    /* Write the marker identifying string "ICC_PROFILE" (null-terminated).
     * We code it in this less-than-transparent way so that the code works
     * even if the local character set is not ASCII.
     */
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

    /* Add the sequencing info */
    jpeg_write_m_byte(cinfo, cur_marker);
    jpeg_write_m_byte(cinfo, (int) num_markers);

    /* Add the profile data */
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

void removeAlphaValues(const unsigned char *in, JSAMPROW out, int size)
{
	int h = 0;
	for (int i = 0; i < size; i += 3) {
		*(out + i)     = *(in + h + 2);
		*(out + i + 1) = *(in + h + 1);
		*(out + i + 2) = *(in + h);
		h += 4;
	}
}

JpegWriter::JpegWriter(const QImage *out_qimage, QString fname, int quality):
	m_out_qimage(out_qimage),
	m_fname(fname),
	m_quality(quality)
{}

JpegWriter::JpegWriter(const QImage *out_qimage, int quality):
	m_out_qimage(out_qimage),
//	m_fname(""),	empty anyway!
	m_quality(quality)
{}

bool JpegWriter::writeQImageToJpeg()
{
    cmsUInt32Number cmsProfileSize = 0;
    ScopedCmsProfile hsRGB( cmsCreate_sRGBProfile() );

    cmsSaveProfileToMem(hsRGB.data(), NULL, &cmsProfileSize);           // get the size

    std::vector<JOCTET> cmsOutputProfile(cmsProfileSize);

    cmsSaveProfileToMem(hsRGB.data(), cmsOutputProfile.data(), &cmsProfileSize);    //

    qDebug() << "sRGB profile size: " << cmsProfileSize;

	struct jpeg_compress_struct cinfo;
	cinfo.err = jpeg_std_error(&ErrorHandler.pub);
	ErrorHandler.pub.error_exit      = my_writer_error_handler;	
	ErrorHandler.pub.output_message  = my_writer_output_message;

	jpeg_create_compress(&cinfo);

    cinfo.image_width = m_out_qimage->width();              // image width and height, in pixels
	cinfo.image_height = m_out_qimage->height();
    cinfo.input_components = cinfo.num_components = 3;      // # of color components per pixel
    cinfo.in_color_space = JCS_RGB;                         // colorspace of input image
	cinfo.jpeg_color_space = JCS_RGB;

	jpeg_set_defaults(&cinfo);
	jpeg_set_colorspace(&cinfo, JCS_RGB);

	//avoid subsampling on high quality factor
	jpeg_set_quality(&cinfo, m_quality, 1);
    if (m_quality >= 70)
    {
        for(int i = 0; i < cinfo.num_components; i++)
        {
			cinfo.comp_info[i].h_samp_factor = 1;
			cinfo.comp_info[i].v_samp_factor = 1;
		}
	}

    ResouceHandlerFile outfile;

#if defined(WIN32) || defined(__APPLE__)
    QTemporaryFile output_temp_file;
#else
    std::vector<char> outbuf;
#endif
    if ( !m_fname.isEmpty() )         // we are writing to file
    {
        QByteArray ba( QFile::encodeName(m_fname) );
		qDebug() << "writeQImageToJpeg: filename: " << ba.data();

        outfile.reset( fopen(ba.data(), "wb") );

        if (outfile.data() == NULL)
        {
			qDebug() << "can't open " << m_fname;
    	    return false;
		}
	} 
    else                            // we are writing to memory buffer
    {
#if defined(WIN32) || defined(__APPLE__)
        if ( !output_temp_file.open() ) return false; // could not open the temporary file!

        QByteArray output_temp_filename = QFile::encodeName( output_temp_file.fileName() );
        output_temp_file.close();
        outfile.reset(fopen(output_temp_filename.constData(), "w+"));

        if ( outfile.data() == NULL ) return false;
#else
		std::vector<char> t(cinfo.image_width * cinfo.image_height * cinfo.num_components);
		outbuf.swap( t );
        // reset all element of the vector to zero!
        std::fill(outbuf.begin(), outbuf.end(), 0);

        outfile.reset( fmemopen(outbuf.data(), outbuf.size(), "w+") );
#endif
	}

    try
    {
        jpeg_stdio_dest(&cinfo, outfile.data());
        jpeg_start_compress(&cinfo, true);

        write_icc_profile(&cinfo, cmsOutputProfile.data(), cmsProfileSize);

        // If an exception is raised, this buffer gets automatically destructed!
        std::vector<JSAMPLE> ScanLineOut(cinfo.image_width * cinfo.num_components);
        JSAMPROW ScanLineOutArray[1] = { ScanLineOut.data() };

        for (int i = 0; cinfo.next_scanline < cinfo.image_height; i++)
        {
            removeAlphaValues(m_out_qimage->scanLine( i ),
                              ScanLineOut.data(),
                              cinfo.image_width * cinfo.num_components);
            jpeg_write_scanlines(&cinfo, ScanLineOutArray, 1);
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

    if ( m_fname.isEmpty() )
    {
#if defined(WIN32) || defined(__APPLE__)
        fflush(outfile.data());
        fseek(outfile.data(), 0, SEEK_END);
        m_filesize = ftell(outfile.data());
#else
        int size = outbuf.size() - 1;
        for (; size > 0; --size)
        {
            if (outbuf[size] != 0)
                break;
        }
        m_filesize = size;
#endif
    }
	return true;
}

int JpegWriter::getFileSize()
{
	return m_filesize;
}
