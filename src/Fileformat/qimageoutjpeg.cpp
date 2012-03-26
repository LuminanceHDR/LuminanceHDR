/**
 * This file is a part of Luminance HDR package.
 * ----------------------------------------------------------------------
 * Copyright (C) 2012 Franco Comida
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
 *
 */

#include <QDebug>

#include <lcms.h>
#include <stdio.h>
#include <jpeglib.h>

#include "qimageoutjpeg.h"

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


void removeAlphaValues(unsigned char *in, JSAMPROW out, int size)
{
	int h = 0;
	for (int i = 0; i < size; i += 3) {
		*(out + i)     = *(in + h + 2);
		*(out + i + 1) = *(in + h + 1);
		*(out + i + 2) = *(in + h);
		h += 4;
	}
}

bool writeQImageToJpeg(QImage *out_qimage, QString fname, int quality) {
	QByteArray ba;

	cmsHPROFILE hsRGB;
	JOCTET *EmbedBuffer;
	size_t profile_size = 0;	

	hsRGB = cmsCreate_sRGBProfile();
	_cmsSaveProfileToMem(hsRGB, NULL, &profile_size); // get the size

	EmbedBuffer = (JOCTET *) _cmsMalloc(profile_size);

	_cmsSaveProfileToMem(hsRGB, EmbedBuffer, &profile_size);

	qDebug() << "sRGB profile size: " << profile_size;

	ba = fname.toUtf8();
	qDebug() << "writeQImageToJpeg: filename: " << ba.data();

	JSAMPROW ScanLineOut;
	struct jpeg_compress_struct cinfo;
	struct jpeg_error_mgr jerr;

	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_compress(&cinfo);

	FILE * outfile;
        
	if ((outfile = fopen(ba.data(), "wb")) == NULL) {
		qDebug() << "can't open " << fname;
        return false;
	}
	
	jpeg_stdio_dest(&cinfo, outfile);

	cinfo.image_width = out_qimage->width();      /* image width and height, in pixels */
	cinfo.image_height = out_qimage->height();
	cinfo.input_components = cinfo.num_components = 3;     /* # of color components per pixel */
	cinfo.in_color_space = JCS_RGB; /* colorspace of input image */
	cinfo.jpeg_color_space = JCS_RGB;

	jpeg_set_defaults(&cinfo);
	jpeg_set_colorspace(&cinfo, JCS_RGB);

	//avoid subsampling on high quality factor
	jpeg_set_quality(&cinfo, quality, 1);
	if (quality >= 70) {
		for(int i = 0; i < cinfo.num_components; i++) {
			cinfo.comp_info[i].h_samp_factor = 1;
			cinfo.comp_info[i].v_samp_factor = 1;
		}
	}

	jpeg_start_compress(&cinfo, true);

	write_icc_profile (&cinfo, EmbedBuffer, profile_size);

	_cmsFree(EmbedBuffer);

	ScanLineOut = (JSAMPROW) _cmsMalloc(cinfo.image_width * cinfo.num_components); 
       
	for (int i = 0; cinfo.next_scanline < cinfo.image_height; i++) {
		
		removeAlphaValues(out_qimage->scanLine( i ), ScanLineOut, cinfo.image_width * cinfo.num_components);
		jpeg_write_scanlines(&cinfo, &ScanLineOut, 1);
	}

	_cmsFree(ScanLineOut);

	jpeg_finish_compress(&cinfo);
	jpeg_destroy_compress(&cinfo);

	return true;
}
