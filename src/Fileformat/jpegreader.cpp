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

#include <QByteArray>
#include <QDebug>

#ifdef USE_LCMS2
#	include <lcms2.h>
#else
#	include <lcms.h>
#endif

#include <stdio.h>
#include <setjmp.h>

jmp_buf place;


#include "jpegreader.h"
#include "Common/LuminanceOptions.h"

///////////////////////////////////////////////////////////////////////////////
//
// Code taken from iccjpeg.c from lcms distribution
//
//

#define ICC_MARKER  (JPEG_APP0 + 2)	/* JPEG marker code for ICC */
#define ICC_OVERHEAD_LEN  14		/* size of non-profile data in APP2 */
#define MAX_BYTES_IN_MARKER  65533	/* maximum data len of a JPEG marker */
#define MAX_DATA_BYTES_IN_MARKER  (MAX_BYTES_IN_MARKER - ICC_OVERHEAD_LEN)


/*
 * Prepare for reading an ICC profile
 */

void setup_read_icc_profile (j_decompress_ptr cinfo)
{
  /* Tell the library to keep any APP2 data it may find */
  jpeg_save_markers(cinfo, ICC_MARKER, 0xFFFF);
}

/*
 * Handy subroutine to test whether a saved marker is an ICC profile marker.
 */

static boolean
marker_is_icc (jpeg_saved_marker_ptr marker)
{
  return
    marker->marker == ICC_MARKER &&
    marker->data_length >= ICC_OVERHEAD_LEN &&
    /* verify the identifying string */
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

boolean read_icc_profile (j_decompress_ptr cinfo,
		  JOCTET **icc_data_ptr,
		  unsigned int *icc_data_len)
{
  jpeg_saved_marker_ptr marker;
  int num_markers = 0;
  int seq_no;
  JOCTET *icc_data;
  unsigned int total_length;
#define MAX_SEQ_NO  255		/* sufficient since marker numbers are bytes */
  char marker_present[MAX_SEQ_NO+1];	  /* 1 if marker found */
  unsigned int data_length[MAX_SEQ_NO+1]; /* size of profile data in marker */
  unsigned int data_offset[MAX_SEQ_NO+1]; /* offset for data in marker */

  *icc_data_ptr = NULL;		/* avoid confusion if false return */
  *icc_data_len = 0;

  /* This first pass over the saved markers discovers whether there are
   * any ICC markers and verifies the consistency of the marker numbering.
   */

  for (seq_no = 1; seq_no <= MAX_SEQ_NO; seq_no++)
    marker_present[seq_no] = 0;

  for (marker = cinfo->marker_list; marker != NULL; marker = marker->next) {
    if (marker_is_icc(marker)) {
      if (num_markers == 0)
	num_markers = GETJOCTET(marker->data[13]);
      else if (num_markers != GETJOCTET(marker->data[13])) {
	qDebug() << "inconsistent num_markers fields";
	return false;		/* inconsistent num_markers fields */
	}
      seq_no = GETJOCTET(marker->data[12]);
      if (seq_no <= 0 || seq_no > num_markers) {
	qDebug() << "bogus sequence number";
	return false;		/* bogus sequence number */
	}
      if (marker_present[seq_no]) {
	qDebug() << "duplicate sequence numbers";
	return false;		/* duplicate sequence numbers */
	}
      marker_present[seq_no] = 1;
      data_length[seq_no] = marker->data_length - ICC_OVERHEAD_LEN;
    }
  }

  if (num_markers == 0) {
	qDebug() << "num_markers = 0";
    return false;
  }

  /* Check for missing markers, count total space needed,
   * compute offset of each marker's part of the data.
   */

  total_length = 0;
  for (seq_no = 1; seq_no <= num_markers; seq_no++) {
    if (marker_present[seq_no] == 0) {
	  qDebug() << "missing sequence number";
      return false;		/* missing sequence number */
	}
    data_offset[seq_no] = total_length;
    total_length += data_length[seq_no];
  }

  if (total_length <= 0) {
	qDebug() << "found only empty markers?";
    return false;		/* found only empty markers? */
  }

  /* Allocate space for assembled data */
  icc_data = (JOCTET *) malloc(total_length * sizeof(JOCTET));
  if (icc_data == NULL)
    return false;		/* oops, out of memory */

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

static struct my_error_mgr { 

	struct  jpeg_error_mgr pub;  // "public" fields 
	LPVOID  Cargo;               // "private" fields 

} ErrorHandler; 

void my_error_handler (j_common_ptr cinfo)
{
	char buffer[JMSG_LENGTH_MAX];
	(*cinfo->err->format_message) (cinfo, buffer);
	throw buffer;
}
  
void my_output_message (j_common_ptr cinfo)
{
	char buffer[JMSG_LENGTH_MAX];
	(*cinfo->err->format_message) (cinfo, buffer);
	throw buffer;
}

void addAlphaValues(JSAMPROW ScanLineIn, unsigned char *ScanLineOut, int size)
{
	int h = 0;
	for (int i = 0; i < size; i += 3) {
		*(ScanLineOut + h)     = *(ScanLineIn + i + 2);
		*(ScanLineOut + h + 1) = *(ScanLineIn + i + 1);
		*(ScanLineOut + h + 2) = *(ScanLineIn + i);
		*(ScanLineOut + h + 3) = 255;
		h += 4;
	}
}

JpegReader::JpegReader(QString filename) :
	fname(filename)
{
}

QImage *JpegReader::readJpegIntoQImage() 
{
	bool doTransform = false;

	cmsHPROFILE hsRGB, hIn;
	cmsHTRANSFORM xform = NULL;
	
	unsigned int EmbedLen;
	JOCTET * EmbedBuffer;

	JSAMPROW ScanLineIn;
	JSAMPROW ScanLineTemp;
	unsigned char *ScanLineOut;

	cinfo.err = jpeg_std_error(&ErrorHandler.pub);
	ErrorHandler.pub.error_exit      = my_error_handler;	
	ErrorHandler.pub.output_message  = my_output_message;

	FILE * infile;

	QByteArray ba;
	ba = fname.toUtf8();
	qDebug() << "readJpegIntoQImage: filename: " << ba.data();

	if ((infile = fopen(ba.data(), "rb")) == NULL) {
		fprintf(stderr, "can't open %s\n", ba.data());
		return NULL;
	}

	LuminanceOptions luminance_opts;
	int camera_profile_opt = luminance_opts.getCameraProfile();

	if (camera_profile_opt == 2) { // from file
		
		QString profile_fname = luminance_opts.getCameraProfileFileName();
		qDebug() << "Camera profile: " << profile_fname;


		if (!profile_fname.isEmpty()) {
		
			ba = profile_fname.toUtf8();

			cmsErrorAction(LCMS_ERROR_SHOW);

			hsRGB = cmsCreate_sRGBProfile();
			hIn = cmsOpenProfileFromFile(ba.data(), "r");

			if (hIn == NULL) {
				return NULL;
			}
			
			if (cmsGetColorSpace(hIn) != icSigRgbData) {
				return NULL;
			}
				

			doTransform = true;
        	xform = cmsCreateTransform(hIn, TYPE_RGB_8, hsRGB, TYPE_RGB_8, INTENT_PERCEPTUAL, 0);
			qDebug() << "Created transform";

        	if (xform == NULL) {
            	cmsCloseProfile(hIn);
            	return NULL;
        	}

        	cmsCloseProfile(hIn);
		}
	}
	
	jpeg_create_decompress(&cinfo);
	qDebug() << "Created decompressor";
	jpeg_stdio_src(&cinfo, infile);
	qDebug() << "Assigned input source";
	setup_read_icc_profile(&cinfo);
	
	try {
		jpeg_read_header(&cinfo, true);
	}
	catch (char *error)
	{
		qDebug() << error;
		jpeg_destroy_decompress(&cinfo);
		fclose(infile);
		return NULL;
	}
	qDebug() << "Readed JPEG headers";

	if (camera_profile_opt == 1 && read_icc_profile(&cinfo, &EmbedBuffer, &EmbedLen) == true) {
		qDebug() << "Found embedded profile";
		hsRGB = cmsCreate_sRGBProfile();
		hIn = cmsOpenProfileFromMem(EmbedBuffer, EmbedLen);

		if (hIn == NULL) {
			free(EmbedBuffer);
			return NULL;
		}
		if (cmsGetColorSpace(hIn) != icSigRgbData) {
			free(EmbedBuffer);
			return NULL;
		}

		doTransform = true;

        xform = cmsCreateTransform(hIn, TYPE_RGB_8, hsRGB, TYPE_RGB_8, INTENT_PERCEPTUAL, 0);

		qDebug() << "Created transform";

		free(EmbedBuffer);
	}
	
	try {
		jpeg_start_decompress(&cinfo);
		qDebug() << "Start decompress";
	}
	catch (char *error) {
		qDebug() << error;
		jpeg_destroy_decompress(&cinfo);
		fclose(infile);
		return NULL;
	}		

	qDebug() << cinfo.output_width;
	qDebug() << cinfo.output_height;
	qDebug() << cinfo.num_components;

	QImage *out_qimage = new QImage(cinfo.output_width, cinfo.output_height, QImage::Format_RGB32);
	ScanLineIn  = (JSAMPROW) _cmsMalloc(cinfo.output_width * cinfo.num_components);
	ScanLineTemp  = (JSAMPROW) _cmsMalloc(cinfo.output_width * cinfo.num_components);
	ScanLineOut  = (unsigned char *) _cmsMalloc(cinfo.output_width * (cinfo.num_components + 1));
	
	for (int i = 0; cinfo.output_scanline < cinfo.output_height; i++) {
		
		try {
			jpeg_read_scanlines(&cinfo, &ScanLineIn, 1);
		}
		catch (char *error) {
			qDebug() << error;
			jpeg_destroy_decompress(&cinfo);
			fclose(infile);
			_cmsFree(ScanLineIn);
			_cmsFree(ScanLineTemp);
			_cmsFree(ScanLineOut);
			jpeg_destroy_decompress(&cinfo);
			return NULL;
		}
		
		if (doTransform) {
			cmsDoTransform(xform, ScanLineIn, ScanLineTemp, cinfo.output_width);
			addAlphaValues(ScanLineTemp, ScanLineOut, cinfo.output_width * cinfo.num_components);
		
		}
		else 
			addAlphaValues(ScanLineIn, ScanLineOut, cinfo.output_width * cinfo.num_components);
			
		memcpy(out_qimage->scanLine( i ), ScanLineOut, cinfo.output_width * (cinfo.num_components + 1));
	}

	if (doTransform)
		cmsDeleteTransform(xform);
	
	_cmsFree(ScanLineIn);
	_cmsFree(ScanLineTemp);
	_cmsFree(ScanLineOut);

	jpeg_finish_decompress(&cinfo);
	jpeg_destroy_decompress(&cinfo);

	fclose(infile);

	return out_qimage;
}

