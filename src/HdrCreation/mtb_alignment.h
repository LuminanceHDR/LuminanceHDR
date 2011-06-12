/**
 * This file is a part of Luminance HDR package.
 * ---------------------------------------------------------------------- 
 *  Copyright (C) 2007 by Nicholas Phillips
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
 * @author Nicholas Phillips <ngphillips@gmail.com>
 *         Giuseppe Rota (small modifications for Qt4)
 *
 */


#include <QList>
#include <QImage>
#include <vector>
using std::vector;

void mtb_alignment(QList<QImage*> &ImagePtrList, QList<bool> &ldr_tiff_input);

QImage* shiftQImage(const QImage *in, int dx, int dy);

///////// private functions
void mtbalign(const QImage *image1, const QImage *image2,
              const double quantile, const int noise, const int shift_bits,
              int &shift_x, int &shift_y);

void getExpShift(const QImage *img1, const int median1, 
		 const QImage *img2, const int median2, 
		 const int noise, const int shift_bits, 
		 int &shift_x, int &shift_y);

void shiftimage(const QImage *in, const int dx, const int dy, QImage *out);

long sumimage(const QImage *img);

void XORimages(const QImage *img1, const QImage *mask1, const QImage *img2, const QImage *mask2, QImage *diff);

/**
 * setThreshold gets the data from the input image and creates the threshold and mask images.
 * those should be bitmap (0,1 valued) with depth()=1 but instead they are like grayscale
 * indexed images, with depth()=8. This waste of space happens because the grayscale
 * images are easier to deal with (no bit shifts).
 *
 * \param *in source data image \n
 * \param threshold the threshold value \n
 * \param noise \n
 * \param *out the output image (8bit) \n
 * \param *mask the noise mask image \n
*/
void setThreshold(const QImage *in, const int threshold, const int noise,
			QImage *out, QImage *mask);

void getLum(const QImage *in, QImage *out, vector<double> &cdf);

/**
 * setbitmap allocates mem for a QImage pointer
 * it's up to the caller to "delete" the pointer to free the mem.
 *
 * \param size the image size \n
 * \return the pointer for which mem will be allocated
 */
QImage* setbitmap(const QSize size);
