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
#include <stdexcept>

#ifdef USE_LCMS2
#include <lcms2.h>
#else
#include <lcms.h>
#endif

#include <stdio.h>

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

int cms_error_handler(int ErrorCode, const char *ErrorText)
{
	throw std::runtime_error(ErrorText);
}

static struct my_error_mgr { 

	struct  jpeg_error_mgr pub;  // "public" fields 
	LPVOID  Cargo;               // "private" fields 

} ErrorHandler; 

void my_error_handler (j_common_ptr cinfo)
{
	char buffer[JMSG_LENGTH_MAX];
	(*cinfo->err->format_message) (cinfo, buffer);
    throw std::runtime_error( std::string(buffer) );
}
  
void my_output_message (j_common_ptr cinfo)
{
	char buffer[JMSG_LENGTH_MAX];
	(*cinfo->err->format_message) (cinfo, buffer);
    throw std::runtime_error( std::string(buffer) );
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

void transform_to_rgb(JSAMPROW ScanLineIn, unsigned char *ScanLineOut, int size)
{
	for (int i = 0; i < size; i += 4) {
		unsigned char C = *(ScanLineIn + i + 0);
		unsigned char M = *(ScanLineIn + i + 1);
		unsigned char Y = *(ScanLineIn + i + 2);
		unsigned char K = *(ScanLineIn + i + 3);
		*(ScanLineOut + i + 2) = C*K/255;
		*(ScanLineOut + i + 1) = M*K/255;
		*(ScanLineOut + i + 0) = Y*K/255;
		*(ScanLineOut + i + 3) = 255;
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
	cmsErrorAction(LCMS_ERROR_SHOW);
	cmsSetErrorHandler(cms_error_handler);
	
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

	jpeg_create_decompress(&cinfo);
	qDebug() << "Created decompressor";
	jpeg_stdio_src(&cinfo, infile);
	qDebug() << "Assigned input source";
	setup_read_icc_profile(&cinfo);
	
	try {
		jpeg_read_header(&cinfo, true);
	}
	catch (const std::runtime_error& err)
	{
		qDebug() << err.what();
		jpeg_destroy_decompress(&cinfo);
		fclose(infile);
		throw err;
	}

	qDebug() << "Readed JPEG headers";
	qDebug() << cinfo.jpeg_color_space;

	if (cinfo.jpeg_color_space == JCS_GRAYSCALE) {
		qDebug() << "Unsuported color space: grayscale";
		jpeg_destroy_decompress(&cinfo);
		fclose(infile);
		throw std::runtime_error("Unsuported color space: grayscale");
	}		

	if (cinfo.jpeg_color_space == JCS_YCCK) {
		qDebug() << "Converting to CMYK";
		cinfo.out_color_space = JCS_CMYK;
	}

	qDebug() << cinfo.num_components;
	
	//if (cinfo.jpeg_color_space == JCS_RGB || cinfo.jpeg_color_space == JCS_YCbCr)
	//	cinfo.out_color_space = JCS_RGB;
	//else
	//	cinfo.out_color_space = JCS_CMYK;

	try {
		jpeg_start_decompress(&cinfo);
	}
	catch (const std::runtime_error& err)
	{
		qDebug() << err.what();
		jpeg_destroy_decompress(&cinfo);
		fclose(infile);
		throw err;
	}
	
	LuminanceOptions luminance_opts;
	int camera_profile_opt = luminance_opts.getCameraProfile();
	
	cmsSetErrorHandler(cms_error_handler);
	cmsErrorAction(LCMS_ERROR_SHOW);

	if (camera_profile_opt == 2) { // from file
		
		QString profile_fname = luminance_opts.getCameraProfileFileName();
		qDebug() << "Camera profile: " << profile_fname;

		if (!profile_fname.isEmpty()) {
		
			ba = profile_fname.toUtf8();

			hsRGB = cmsCreate_sRGBProfile();
			try {
				hIn = cmsOpenProfileFromFile(ba.data(), "r");
			}
			catch (const std::runtime_error& err) {
				qDebug() << err.what();
				jpeg_destroy_decompress(&cinfo);
				fclose(infile);
				throw err;
			}
			
			if (!(cmsGetColorSpace(hIn) == icSigRgbData || cmsGetColorSpace(hIn) == icSigCmykData)) {
				jpeg_destroy_decompress(&cinfo);
				fclose(infile);
				throw std::runtime_error("ERROR: Wrong colorspace");
			}	
			
			doTransform = true;
			
			try {
				if (cmsGetColorSpace(hIn) == icSigRgbData)
	        		xform = cmsCreateTransform(hIn, TYPE_RGB_8, hsRGB, TYPE_RGB_8, INTENT_PERCEPTUAL, 0);
				else
	        		xform = cmsCreateTransform(hIn, TYPE_YUVK_8, hsRGB, TYPE_RGB_8, INTENT_PERCEPTUAL, 0);
			}
			catch (const std::runtime_error& err) {
				qDebug() << err.what();
				jpeg_destroy_decompress(&cinfo);
				fclose(infile);
				cmsCloseProfile(hIn);
				throw err;
			}
			qDebug() << "Created transform";

        	cmsCloseProfile(hIn);
		}
	}
	
	if (camera_profile_opt == 1 && read_icc_profile(&cinfo, &EmbedBuffer, &EmbedLen) == true) {
		qDebug() << "Found embedded profile";
		try {
			hsRGB = cmsCreate_sRGBProfile();
			hIn = cmsOpenProfileFromMem(EmbedBuffer, EmbedLen);
		}
		catch (const std::runtime_error& err)
		{
			qDebug() << err.what();
			jpeg_destroy_decompress(&cinfo);
			fclose(infile);
			free(EmbedBuffer);
			throw err;
		}

		if (cmsGetColorSpace(hIn) == icSigRgbData)
			qDebug() << "Embedded colorspace = sRGB";
		else if (cmsGetColorSpace(hIn) == icSigCmykData)
			qDebug() << "Embedded colorspace = CMYK";
		else if (cmsGetColorSpace(hIn) == icSigYCbCrData)
			qDebug() << "Embedded colorspace = YCbCr";
		else if (cmsGetColorSpace(hIn) == icSigLuvKData)
			qDebug() << "Embedded colorspace = LuvK";					

		doTransform = true;
		try {
			switch (cinfo.jpeg_color_space)
			{
				case JCS_RGB:	
				case JCS_YCbCr:
					qDebug() << "Transform colorspace = sRGB";
					xform = cmsCreateTransform(hIn, TYPE_RGB_8, hsRGB, TYPE_RGB_8, INTENT_PERCEPTUAL, 0);
					ScanLineIn  = (JSAMPROW) _cmsMalloc(cinfo.output_width * cinfo.num_components);
					ScanLineTemp  = (JSAMPROW) _cmsMalloc(cinfo.output_width * cinfo.num_components);
					ScanLineOut  = (unsigned char *) _cmsMalloc(cinfo.output_width * (cinfo.num_components + 1));
					break;
				case JCS_CMYK:		
				case JCS_YCCK:
					qDebug() << "Transform colorspace = CMYK";
					xform = cmsCreateTransform(hIn, TYPE_YUVK_8, hsRGB, TYPE_RGB_8, INTENT_PERCEPTUAL, 0);
					ScanLineIn  = (JSAMPROW) _cmsMalloc(cinfo.output_width * cinfo.num_components);
					ScanLineTemp  = (JSAMPROW) _cmsMalloc(cinfo.output_width * cinfo.num_components);
					ScanLineOut  = (unsigned char *) _cmsMalloc(cinfo.output_width * cinfo.num_components);
					break;
			}
		}
		catch(const std::runtime_error& err)
		{
			qDebug() << err.what();
			jpeg_destroy_decompress(&cinfo);
			fclose(infile);
			free(EmbedBuffer);
			throw err;
		}
		qDebug() << "Created transform";

		free(EmbedBuffer);
	}
	else { 
		ScanLineIn  = (JSAMPROW) _cmsMalloc(cinfo.output_width * cinfo.num_components);
		ScanLineTemp  = (JSAMPROW) _cmsMalloc(cinfo.output_width * cinfo.num_components);
		if (cinfo.jpeg_color_space == JCS_RGB || cinfo.jpeg_color_space == JCS_YCbCr)
			ScanLineOut  = (unsigned char *) _cmsMalloc(cinfo.output_width * (cinfo.num_components + 1));
		else
			ScanLineOut  = (unsigned char *) _cmsMalloc(cinfo.output_width * cinfo.num_components);
	}
	
	QImage *out_qimage = new QImage(cinfo.output_width, cinfo.output_height, QImage::Format_RGB32);
    try
    {
        for (int i = 0; cinfo.output_scanline < cinfo.output_height; ++i)
        {
            jpeg_read_scanlines(&cinfo, &ScanLineIn, 1);

            if (doTransform) {
				cmsDoTransform(xform, ScanLineIn, ScanLineTemp, cinfo.output_width);
				switch (cinfo.jpeg_color_space)
				{
					case JCS_RGB:
					case JCS_YCbCr:
						addAlphaValues(ScanLineTemp, ScanLineOut, cinfo.output_width * cinfo.num_components);
						memcpy(out_qimage->scanLine( i ), ScanLineOut, cinfo.output_width * (cinfo.num_components + 1));
						break;
					case JCS_CMYK:
					case JCS_YCCK:
						addAlphaValues(ScanLineTemp, ScanLineOut, cinfo.output_width * (cinfo.num_components - 1));
						memcpy(out_qimage->scanLine( i ), ScanLineOut, cinfo.output_width * cinfo.num_components);
						break;
				}			
            }
            else {
				switch (cinfo.jpeg_color_space)
				{
					case JCS_RGB:
					case JCS_YCbCr:
						addAlphaValues(ScanLineIn, ScanLineOut, cinfo.output_width * cinfo.num_components);
						memcpy(out_qimage->scanLine( i ), ScanLineOut, cinfo.output_width * (cinfo.num_components + 1));
						break;
					case JCS_CMYK:
					case JCS_YCCK:
						transform_to_rgb(ScanLineIn, ScanLineOut, cinfo.output_width * cinfo.num_components);
						memcpy(out_qimage->scanLine( i ), ScanLineOut, cinfo.output_width * cinfo.num_components);
						break;
				}			
			}
        }
    }
    catch (const std::runtime_error& err)
    {
        qDebug() << err.what();
        jpeg_destroy_decompress(&cinfo);
        fclose(infile);
        _cmsFree(ScanLineIn);
        _cmsFree(ScanLineTemp);
        _cmsFree(ScanLineOut);
		cmsDeleteTransform(xform);
		throw err;
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

