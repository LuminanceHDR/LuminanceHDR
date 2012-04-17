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
#include <QSharedPointer>
#include <vector>
#include <algorithm>
#include <lcms.h>
#include <stdio.h>

#if defined(WIN32) || defined(__APPLE__)
#include <QTemporaryFile>
#endif

#include "pngwriter.h"

PngWriter::PngWriter(const QImage *out_qimage, QString filename, int quality) :
	m_out_qimage(out_qimage),
	m_fname(filename),
	m_quality(quality)
{
}

PngWriter::PngWriter(const QImage *out_qimage, int quality):
	m_out_qimage(out_qimage),
	m_quality(quality)
{}

bool PngWriter::writeQImageToPng()
{
	png_uint_32 width = m_out_qimage->width();
	png_uint_32 height = m_out_qimage->height();

	png_uint_32 profile_size = 0;	

    cmsHPROFILE hsRGB = cmsCreate_sRGBProfile();
    _cmsSaveProfileToMem(hsRGB, NULL, &profile_size);           // get the size

    std::vector<char> profile_buffer(profile_size);

    _cmsSaveProfileToMem(hsRGB, profile_buffer.data(), &profile_size);    //

	qDebug() << "sRGB profile size: " << profile_size;

    // I protecte the output file into a QSharedPointer with custom Deleter,
    // so I don't have to close it whenever it goes out of scope!
    QSharedPointer<FILE> outfile;

#if defined(WIN32) || defined(__APPLE__)
    QTemporaryFile output_temp_file;
#else
    std::vector<char> outbuf;
#endif
    if ( !m_fname.isEmpty() )         // we are writing to file
    {
        QByteArray ba( m_fname.toUtf8() );
		qDebug() << "writeQImageToPng: filename: " << ba.data();

        outfile = QSharedPointer<FILE>(fopen(ba.data(), "wb"), fclose);

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
        outfile = QSharedPointer<FILE>(fopen(output_temp_filename.constData(), "w+"), fclose);

        if ( outfile.data() == NULL ) return false;
#else
		std::vector<char> t(width * height * 4 + (width * height * 4) * 0.1);
		outbuf.swap( t );
        // reset all element of the vector to zero!
        std::fill(outbuf.begin(), outbuf.end(), 0);

		qDebug() << "outbuf size: " << outbuf.size();

        outfile = QSharedPointer<FILE>(fmemopen(outbuf.data(), outbuf.size(), "w+"), fclose);
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
		return false;
	}

	png_infop info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr)
	{
		qDebug() << "PNG: Failed to create info struct";
		png_destroy_write_struct(&png_ptr,
         (png_infopp)NULL);
		return false;
    }

	if (setjmp(png_jmpbuf(png_ptr)))
	{
		qDebug() << "PNG: Error writing file";
		png_destroy_write_struct(&png_ptr, &info_ptr);
		return false;
	}

	png_init_io(png_ptr, outfile.data());

	png_set_IHDR(png_ptr, info_ptr, width, height,
       8, PNG_COLOR_TYPE_RGB_ALPHA, PNG_INTERLACE_NONE,
       PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

	int compression_level = 9 - m_quality/11.11111;

	png_set_compression_level(png_ptr, compression_level);

	png_set_bgr(png_ptr);

	png_set_iCCP(png_ptr, info_ptr, "sRGB", NULL,
                      profile_buffer.data(), profile_size);

	png_write_info(png_ptr, info_ptr);

	png_byte *row_pointers[height];

	for (png_uint_32 row = 0; row < height; row++)
		row_pointers[row] = NULL;

	for (png_uint_32 row = 0; row < height; row++)
		row_pointers[row] = (png_byte *) png_malloc(png_ptr, png_get_rowbytes(png_ptr,
         info_ptr));

	for (png_uint_32 row = 0; row < height; row++)
		memcpy(row_pointers[row], m_out_qimage->scanLine( row ), png_get_rowbytes(png_ptr, info_ptr));

	png_write_image(png_ptr, row_pointers);

	png_write_end(png_ptr, info_ptr);

	png_destroy_write_struct(&png_ptr, &info_ptr);

    if ( m_fname.isEmpty() )
    {
#if defined(WIN32) || defined(__APPLE__)
        fflush(outfile.data());
        fseek(outfile.data(), 0, SEEK_END);
        m_filesize = ftell(outfile.data());
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
	
	return true;
}

int PngWriter::getFileSize()
{
	return m_filesize;
}
