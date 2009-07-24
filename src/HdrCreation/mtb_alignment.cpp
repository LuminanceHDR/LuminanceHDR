/**
 * This file is a part of Luminance package.
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

#include <cmath>
#include <cassert>
#include <QVector>
#include "mtb_alignment.h"

QImage* shiftQImage(const QImage *in, int dx, int dy)
{
	QImage *out=new QImage(in->size(),QImage::Format_ARGB32);
	assert(out!=NULL);
	out->fill(qRgba(0,0,0,0)); //transparent black
	for(int i = 0; i < in->height(); i++) {
		if( (i+dy) < 0 ) continue;
		if( (i+dy) >= in->height()) break;
		QRgb *inp = (QRgb*)in->scanLine(i);
		QRgb *outp = (QRgb*)out->scanLine(i+dy);
		for(int j = 0; j < in->width(); j++) {
			if( (j+dx) >= in->width()) break;
			if( (j+dx) >= 0 ) outp[j+dx] = *inp;
			inp++;
		}
	}
	return out;
}

void mtb_alignment(QList<QImage*> &ImagePtrList, QList<bool> &ldr_tiff_input) {
	assert(ImagePtrList.size()>=2);
	int width=ImagePtrList.at(0)->width();
	int height=ImagePtrList.at(0)->height();
	double quantile = 0.5;
	int noise = 4;
	int shift_bits = qMax((int)floor(log2(qMin(width,height)))-6 , 0);
	//qDebug("::mtb_alignment: width=%d, height=%d, shift_bits=%d",width,height,shift_bits);

	//these arrays contain the shifts of each image (except the 0-th) wrt the previous one
	int *shiftsX=new int[ImagePtrList.size()-1];
	int *shiftsY=new int[ImagePtrList.size()-1];

	//find the shitfs
	for (int i=0; i<ImagePtrList.size()-1; i++) {
		mtbalign(ImagePtrList.at(i),ImagePtrList.at(i+1), quantile, noise, shift_bits, shiftsX[i], shiftsY[i]);
	}
	
	//qDebug("::mtb_alignment: now shifting the images");
	int originalsize=ImagePtrList.size();
	//shift the images (apply the shifts starting from the second (index=1))
	for (int i=1; i<originalsize; i++) {
		int cumulativeX=0;
		int cumulativeY=0;
		//gather all the x,y shifts until you reach the first image
		for (int j=i-1; j>=0; j--) {
			cumulativeX+=shiftsX[j];
			cumulativeY+=shiftsY[j];
// 			qDebug("::mtb_alignment: partial cumulativeX=%d, cumulativeY=%d",cumulativeX,cumulativeY);
		}
		//qDebug("::mtb_alignment: Cumulative shift for image %d = (%d,%d)",i,cumulativeX,cumulativeY);
		QImage *shifted=shiftQImage(ImagePtrList[1], cumulativeX, cumulativeY);
		if (ldr_tiff_input[1]) {
			delete [] ImagePtrList[1]->bits();
		}
		delete ImagePtrList[1];
		ImagePtrList.removeAt(1);
		ImagePtrList.append(shifted);
		ldr_tiff_input.removeAt(1);
		ldr_tiff_input.append(false);
	}
	delete shiftsX; delete shiftsY;
}


void mtbalign(const QImage *image1, const QImage *image2,
              const double quantile, const int noise, const int shift_bits,
              int &shift_x, int &shift_y)
{
	QImage *img1lum=new QImage(image1->size(),QImage::Format_Indexed8);
	QImage *img2lum=new QImage(image2->size(),QImage::Format_Indexed8);
	vector<double> cdf1,cdf2;
	int median1,median2;

	getLum(image1, img1lum, cdf1);
	for(median1 = 0; median1 < 256; median1++) if( cdf1[median1] >= quantile ) break;
	getLum(image2, img2lum, cdf2);
	for(median2 = 0; median2 < 256; median2++) if( cdf2[median2] >= quantile ) break;
	
// 	qDebug("::mtb_alignment: align::medians, image 1: %d, image 2: %d",median1,median2);
	getExpShift(img1lum, median1, img2lum, median2, noise, shift_bits, shift_x, shift_y);
	delete img1lum; delete img2lum;
	//qDebug("::mtb_alignment: align::done, final shift is (%d,%d)",shift_x, shift_y);
}

void getExpShift(const QImage *img1, const int median1, 
		 const QImage *img2, const int median2, 
		 const int noise, const int shift_bits, 
		 int &shift_x, int &shift_y)
{
	int curr_x = 0;
	int curr_y = 0;
	if( shift_bits > 0) {
		QImage img1small = img1->scaled(img1->size()/2);
		QImage img2small = img2->scaled(img2->size()/2);
		getExpShift(&img1small,median1, &img2small, median2, noise, shift_bits-1, curr_x, curr_y);
		curr_x *= 2;
		curr_y *= 2;
	}
	
	QImage *img1threshold = setbitmap(img1->size());
	QImage *img1mask      = setbitmap(img1->size());
	QImage *img2threshold = setbitmap(img1->size());
	QImage *img2mask      = setbitmap(img1->size());
	setThreshold(img1, median1, noise, img1threshold, img1mask);
	setThreshold(img2, median2, noise, img2threshold, img2mask);
	
	QImage *img2th_shifted   = setbitmap(img2->size());
	QImage *img2mask_shifted = setbitmap(img2->size());
	QImage *diff             = setbitmap(img2->size());

	int minerr = img1->width()*img2->height();
	for(int i = -1; i <= 1; i++) {
		for(int j = -1; j <= 1; j++) {
			int dx = curr_x + i;
			int dy = curr_y + j;
			shiftimage(img2threshold, dx, dy, img2th_shifted);
			shiftimage(img2mask, dx, dy, img2mask_shifted);
			XORimages(img1threshold, img1mask, img2th_shifted, img2mask_shifted, diff);
			long err = sumimage(diff);
			if( err < minerr ) {
				minerr = err;
				shift_x = dx;
				shift_y = dy;
			}
		}
	}

	delete img1threshold; delete img1mask; delete img2threshold; delete img2mask;
	delete img2th_shifted; delete img2mask_shifted; delete diff;
// 	qDebug("::mtb_alignment: getExpShift::Level %d  shift (%d,%d)", shift_bits,shift_x, shift_y);
	return;
}

void shiftimage(const QImage *in, const int dx, const int dy, QImage *out)
{
	out->fill(0);
	for(int i = 0; i < in->height(); i++) {
		if( (i+dy) < 0 ) continue;
		if( (i+dy) >= in->height()) break;
		const uchar *inp = in->scanLine(i);
		uchar *outp = out->scanLine(i+dy);
		for(int j = 0; j < in->width(); j++) {
			if( (j+dx) >= in->width()) break;
			if( (j+dx) >= 0 ) outp[j+dx] = *inp;
			inp++;
		}
	}
	return;
}

void setThreshold(const QImage *in, const int threshold, const int noise,
			QImage *threshold_out, QImage *mask_out)
{
	for(int i = 0; i < in->height(); i++) {
		const uchar *inp = in->scanLine(i);
		uchar *outp = threshold_out->scanLine(i);
		uchar *maskp = mask_out->scanLine(i);
		for(int j = 0; j < in->width(); j++) {
			*outp++ = *inp < threshold ? 0 : 1;
			*maskp++ = (*inp > (threshold-noise)) && (*inp < (threshold+noise)) ? 0 : 1;
			inp++;
		}
	}
	return;
}

void XORimages(const QImage *img1, const QImage *mask1, const QImage *img2, const QImage *mask2, QImage *diff)
{
	diff->fill(0);
	for(int i = 0; i < img1->height(); i++) {
		const uchar *p1 = img1->scanLine(i);
		const uchar *p2 = img2->scanLine(i);
		const uchar *m1 = mask1->scanLine(i);
		const uchar *m2 = mask2->scanLine(i);
		uchar *dp = diff->scanLine(i);
		for(int j = 0; j < img1->width(); j++) {
			//*dp++ = xor_t[*p1++][*p2++]*(*m1++)*(*m2++);
			*dp++ = (*p1++ xor *p2++) and *m1++ and *m2++;
		}
	}
	return;
}

long sumimage(const QImage *img)
{
	long ttl  = 0;
	for(int i = 0; i < img->height(); i++) {
		const uchar *p = img->scanLine(i);
		for(int j = 0; j < img->width(); j++) 
			ttl += (long)(*p++);
	}
	return ttl;
}

void getLum(const QImage *in, QImage *out, vector<double> &cdf)
{
	vector<long> hist;
	hist.resize(256);
	for(uint i = 0; i < 256; i++) hist[i] = 0;

	cdf.resize(256);
	
	
	QVector<QRgb> graycolortable;
	for(uint i = 0; i < 256; i++)
		graycolortable.append(qRgb(i,i,i));
	out->setColorTable(graycolortable);
	
	for(int i = 0; i < in->height(); i++) {
		QRgb *inl = (QRgb *)in->scanLine(i);
		uchar *outl = out->scanLine(i);
		for(int j = 0; j < in->width(); j++) {
			uint v = qGray(*inl);
			hist[v] = hist[v] + 1;
			inl++;
			*outl++ = v;
		}
	}
	double w = 1/((double)(in->height()*in->width()));
	cdf[0] = w*hist[0];
	for(int i = 1; i < 256; i++) cdf[i] = cdf[i-1] + w*hist[i];
	return;
}

//it's up to the caller to "delete" the pointer to free the mem!
QImage* setbitmap(const QSize size)
{
	QImage *img = new QImage(size,QImage::Format_Indexed8);
	assert(img!=NULL);
	
	QVector<QRgb> binaryColorTable;
	binaryColorTable.append(qRgb(0,0,0));
	binaryColorTable.append(qRgb(255,255,255));
	img->setColorTable(binaryColorTable);
	return img;
}

