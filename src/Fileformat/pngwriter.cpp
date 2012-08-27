/**
 * This file is a part of Luminance HDR package.
 * ----------------------------------------------------------------------
 * Copyright (C) 2012 Franco Comida
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
 *
 * @author Franco Comida <fcomida@users.sourceforge.net>
 *
 */

#include <QDebug>
#include <QFile>
#include <vector>
#include <algorithm>
#include <lcms2.h>
#include <stdio.h>

#if defined(WIN32) || defined(__APPLE__) || defined(__FreeBSD__)
#include <QTemporaryFile>
#endif

#include "pngwriter.h"

PngWriter::PngWriter(const QImage *out_qimage, QString filename, int quality) :
	m_out_qimage(out_qimage),
	m_fname(filename),
	m_quality(quality)
{}

PngWriter::PngWriter(const QImage *out_qimage, int quality):
	m_out_qimage(out_qimage),
	m_quality(quality)
{}

bool PngWriter::writeQImageToPng()
{
	png_uint_32 width = m_out_qimage->width();
	png_uint_32 height = m_out_qimage->height();

    cmsUInt32Number profile_size = 0;

	cmsHPROFILE hsRGB = cmsCreate_sRGBProfile();
    cmsSaveProfileToMem(hsRGB, NULL, &profile_size);			// get the size

#if PNG_LIBPNG_VER_MINOR < 5
    std::vector<char> profile_buffer(profile_size);
#else
    std::vector<unsigned char> profile_buffer(profile_size);
#endif

    cmsSaveProfileToMem(hsRGB, profile_buffer.data(), &profile_size);	  //

	qDebug() << "sRGB profile size: " << profile_size;

	FILE *outfile;

#if defined(WIN32) || defined(__APPLE__) || defined(__FreeBSD__)
	QTemporaryFile output_temp_file;
#else
	std::vector<char> outbuf;
#endif
	if ( !m_fname.isEmpty() )		  // we are writing to file
	{
        QByteArray ba( QFile::encodeName(m_fname) );
		qDebug() << "writeQImageToPng: filename: " << ba.data();

		outfile = fopen(ba.data(), "wb");

		if (outfile == NULL)
		{
			qDebug() << "can't open " << m_fname;
			return false;
		}
	} 
	else							// we are writing to memory buffer
	{
#if defined(WIN32) || defined(__APPLE__) || defined(__FreeBSD__)
		if ( !output_temp_file.open() ) return false; // could not open the temporary file!

		QByteArray output_temp_filename = QFile::encodeName( output_temp_file.fileName() );
		output_temp_file.close();
		outfile = fopen(output_temp_filename.constData(), "w+");
		if ( outfile == NULL ) return false;
#else
		std::vector<char> t(width * height * 4 + (width * height * 4) * 0.1);
		outbuf.swap( t );
		// reset all element of the vector to zero!
		std::fill(outbuf.begin(), outbuf.end(), 0);

		qDebug() << "outbuf size: " << outbuf.size();

		outfile = fmemopen(outbuf.data(), outbuf.size(), "w+");
		if (outfile == NULL) {
			qDebug() << "Failed opening file on memory";
			return false;
		}
#endif
	}

	png_structp png_ptr = png_create_write_struct
	   (PNG_LIBPNG_VER_STRING, NULL,
		NULL, NULL);
	if (!png_ptr)
	{
		qDebug() << "PNG: Failed to create write struct";
		fclose(outfile);
		return false;
	}

	png_infop info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr)
	{
		qDebug() << "PNG: Failed to create info struct";
		png_destroy_write_struct(&png_ptr,
		 (png_infopp)NULL);
		fclose(outfile);
		return false;
	}

	if (setjmp(png_jmpbuf(png_ptr)))
	{
		qDebug() << "PNG: Error writing file";
		png_destroy_write_struct(&png_ptr, &info_ptr);
		fclose(outfile);
		return false;
	}

	png_init_io(png_ptr, outfile);

	png_set_IHDR(png_ptr, info_ptr, width, height,
	   8, PNG_COLOR_TYPE_RGB_ALPHA, PNG_INTERLACE_NONE,
	   PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

	int compression_level = 9 - m_quality/11.11111;

	png_set_compression_level(png_ptr, compression_level);

	png_set_bgr(png_ptr);

	char profileName[5] = "sRGB";
	png_set_iCCP(png_ptr, info_ptr, profileName, 0,
				profile_buffer.data(), (png_uint_32)profile_size);

	png_write_info(png_ptr, info_ptr);

	std::vector<png_bytep> row_pointers(height);

	for (png_uint_32 row = 0; row < height; row++)
		row_pointers[row] = NULL;

	for (png_uint_32 row = 0; row < height; row++)
		row_pointers[row] = (png_bytep) png_malloc(png_ptr, png_get_rowbytes(png_ptr,
		 info_ptr));

	for (png_uint_32 row = 0; row < height; row++) {
		memcpy(row_pointers[row], m_out_qimage->scanLine( row ), png_get_rowbytes(png_ptr, info_ptr));
		png_write_row(png_ptr, row_pointers[row]);
	}

	png_write_end(png_ptr, info_ptr);

	for (png_uint_32 row = 0; row < height; row++)
		png_free(png_ptr, row_pointers[row]);
	
	png_destroy_write_struct(&png_ptr, &info_ptr);

	if ( m_fname.isEmpty() )
	{
#if defined(WIN32) || defined(__APPLE__) || defined(__FreeBSD__)
		fflush(outfile);
		fseek(outfile, 0, SEEK_END);
		m_filesize = ftell(outfile);
#else
		png_uint_32 size = outbuf.size() - 1;
		for (; size > 0; --size)
		{
			if (outbuf[size] != 0)
			break;
		}
		m_filesize = size;
		qDebug() << "File size: " << m_filesize;
#endif
	}
	fclose(outfile);
	return true;
}

int PngWriter::getFileSize()
{
	return m_filesize;
}
