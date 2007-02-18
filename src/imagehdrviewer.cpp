/**
 * This file is a part of Qtpfsgui package.
 * ---------------------------------------------------------------------- 
 * Copyright (C) 2006,2007 Giuseppe Rota
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
 * @author Giuseppe Rota <grota@users.sourceforge.net>
 * based on Rafal Mantiuk and Grzegorz Krawczyk 's pfsview code
 */

#include "imagehdrviewer.h"
#include <stdio.h>
#include <math.h>
#include <QApplication>
#include <QLabel>
#include <QFileInfo>

inline float clamp( float val, float min, float max )
{
    if( val < min ) return min;
    if( val > max ) return max;
    return val;
}


inline int clamp( int val, int min, int max )
{
    if( val < min ) return min;
    if( val > max ) return max;
    return val;
}


static float getInverseMapping( LumMappingMethod mappingMethod,
				float v, float minValue, float maxValue ) {
    switch( mappingMethod ) {
    case MAP_GAMMA1_4:
	return powf( v, 1.4 )*(maxValue-minValue) + minValue;
    case MAP_GAMMA1_8:
	return powf( v, 1.8 )*(maxValue-minValue) + minValue;
    case MAP_GAMMA2_2:
	return powf( v, 2.2 )*(maxValue-minValue) + minValue;
    case MAP_GAMMA2_6:
	return powf( v, 2.6 )*(maxValue-minValue) + minValue;
    case MAP_LINEAR:
	return v*(maxValue-minValue) + minValue;
    case MAP_LOGARITHMIC:
	return powf( 10, v * (log10f(maxValue) - log10f(minValue)) + log10f( minValue ) );
    default:
	assert(0);
	return 0;
    }
}


inline int binarySearchPixels( float x, const float *lut, const int lutSize ) {
    int l = 0, r = lutSize;
    while( true ) {
	int m = (l+r)/2;
	if( m == l ) break;
	if( x < lut[m] )
	    r = m;
	else
	    l = m;
    }
    return l;
}

// static void transformImageToWorkArea( pfs::Array2D **workArea,
// 				      pfs::Array2D *X, pfs::Array2D *Y, pfs::Array2D *Z ) {
// 
//     for( int c = 0; c < 3; c++ ) {
// 	if( workArea[c] == NULL ) {
// 	    workArea[c] = new pfs::Array2DImpl( X->getCols(), X->getRows() );
// 	}
// 	// workArea has different colums/rows when rotating
// 	if ( workArea[c]->getCols() != X->getCols()) {
// 	    delete workArea[c] ;
// 	    workArea[c]=new pfs::Array2DImpl( X->getCols(), X->getRows() );
// 	}
//     }
//     //Copy & tranform image to work area
//     copyArray( X, workArea[0] );
//     copyArray( Y, workArea[1] );
//     copyArray( Z, workArea[2] );
// 
//     pfs::transformColorSpace( pfs::CS_XYZ, workArea[0], workArea[1], workArea[2],
// 			         pfs::CS_RGB, workArea[0], workArea[1], workArea[2] );
// }


//==================================================================================
//**********************************************************************************
//==================================================================================

ImageMDIwindow::ImageMDIwindow( QWidget *parent, unsigned int neg, unsigned int naninf ) : QMainWindow(parent), pfsFrame(NULL), mappingMethod( MAP_GAMMA2_2 ), minValue( 1.0f ), maxValue( 1.0f ), fittingwin(false), naninfcol(naninf), negcol(neg) {

    setAttribute(Qt::WA_DeleteOnClose);
    image=NULL;
    workArea[0] = workArea[1] = workArea[2] = NULL;

    imageLabel = new QLabel;
    imageLabel->setBackgroundRole(QPalette::Base);
    imageLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    imageLabel->setScaledContents(true); ///true in qt4 example

    scrollArea = new QScrollArea;
    scrollArea->setBackgroundRole(QPalette::Dark);
    scrollArea->setWidget(imageLabel);
    setCentralWidget(scrollArea);

    toolBar = this->addToolBar("Viewing Settings Toolbar");
    QLabel *mappingMethodLabel = new QLabel( "&Mapping:", toolBar );
    mappingMethodLabel->setAlignment( Qt::AlignRight | Qt::AlignVCenter );
    mappingMethodCB = new QComboBox( toolBar );
    toolBar->addWidget(mappingMethodLabel);
    toolBar->addWidget(mappingMethodCB);
    toolBar->addSeparator();
    mappingMethodLabel->setBuddy( mappingMethodCB );
    QStringList methods;
    methods+="Linear";
    methods+="Gamma 1.4";
    methods+="Gamma 1.8";
    methods+="Gamma 2.2";
    methods+="Gamma 2.6";
    methods+="Logarithmic";
    mappingMethodCB->addItems(methods);
    mappingMethodCB->setCurrentIndex( 3 );
    connect( mappingMethodCB, SIGNAL( activated( int ) ), this, SLOT( setLumMappingMethod(int) ) );

    QLabel *histlabel = new QLabel( "Histogram:", toolBar );
    lumRange = new LuminanceRangeWidget( toolBar );
    toolBar->addWidget(histlabel);
    toolBar->addWidget(lumRange);
    toolBar->addSeparator();
    connect( lumRange, SIGNAL( updateRangeWindow() ), this, SLOT( updateRangeWindow() ) );
}


void ImageMDIwindow::updateHDR(pfs::Frame* inputframe) {
    assert(inputframe!=NULL);
    //delete previous pfs::Frame*.
    if (pfsFrame!=NULL)
       delete pfsFrame;
    pfsFrame = inputframe;
    lumRange->setHistogramImage(getPrimaryChannel());
    lumRange->fitToDynamicRange();
    //fitToDynamicRange() already takes care -indirectly- to call updateImage()
//     updateImage();
    //zoom at original size, 100%
    scaleFactor = 1.0;
    //make the label use the image dimensions
    imageLabel->adjustSize();
    update();
    imageLabel->update();
    repaint();
    imageLabel->repaint();
}


void ImageMDIwindow::updateImage() {
	assert( pfsFrame != NULL );
	QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );
	
	pfs::Channel *R, *G, *B;
	pfsFrame->getRGBChannels( R, G, B );
	assert(R!=NULL && G!=NULL && B!=NULL);
	
	//workarea  needed in updateMapping(...) called by mapFrameToImage(...)
// 	transformImageToWorkArea( workArea, X, Y, Z );
	workArea[0]=R;
	workArea[1]=G;
	workArea[2]=B;
	assert( workArea[0] != NULL && workArea[1] != NULL && workArea[2] != NULL );
	
	int zoomedWidth = workArea[0]->getCols();
	int zoomedHeight = workArea[0]->getRows();
	if( image != NULL )
		delete image;
	image = new QImage(zoomedWidth, zoomedHeight, QImage::Format_RGB32 );
	assert( image != NULL );
	
	assert( workArea[0] != NULL && workArea[1] != NULL && workArea[2] != NULL );
	mapFrameToImage( /*workArea[0], workArea[1], workArea[2], image,
			minValue, maxValue, mappingMethod*/ );
	
	//assign the mapped image to the label
	imageLabel->setPixmap(QPixmap::fromImage(*image));
	QApplication::restoreOverrideCursor();
}

void ImageMDIwindow::mapFrameToImage() {

    int rows = workArea[0]->getRows();
    int cols = workArea[0]->getCols();
    int index = 0;
    float imgRow = 0, imgCol;
    float lutPixFloor[257*2];
    QRgb lutPixel[257*2];
    int lutSize=258;

    for( int p = 1; p <= 257; p++ ) {
	float p_left = ((float)p - 1.f)/255.f; // Should be -1.5f, but we don't want negative nums
	lutPixFloor[p] = getInverseMapping( mappingMethod, p_left, minValue, maxValue );
    }
    lutPixel[0] = QColor( 0, 0, 0 ).rgb();
    lutPixel[257] = QColor( 255, 255, 255 ).rgb();

    for( int r = 0; r < rows; r++, imgRow++ ) {
	QRgb* line = (QRgb*)image->scanLine( (int)imgRow );
	imgCol = 0;
	for( int c = 0; c < cols; c++, index++, imgCol++ ) {
	    QRgb pixel;
	    // Color channels
	    int pr, pg, pb;
	    pr = binarySearchPixels( (*workArea[0])(index), lutPixFloor, lutSize );
	    pg = binarySearchPixels( (*workArea[1])(index), lutPixFloor, lutSize );
	    pb = binarySearchPixels( (*workArea[2])(index), lutPixFloor, lutSize );

	    // Clipping
	    pixel = QColor( clamp( pr-1, 0, 255 ),
			    clamp( pg-1, 0, 255 ),
			    clamp( pb-1, 0, 255 ) ).rgb();

	    if( !finite( (*workArea[0])(index) ) || !finite( (*workArea[1])(index) ) || !finite( (*workArea[2])(index) ) ) {   // x is NaN or Inf 
		pixel = naninfcol;
	    }

	    if( (*workArea[0])(index)<0 || (*workArea[1])(index)<0 || (*workArea[2])(index)<0 ) {   // x is negative
		pixel = negcol;
	  }

	    line[(int)imgCol] = pixel;
	}
    }
}

void ImageMDIwindow::updateRangeWindow() {
 setRangeWindow( pow( 10, lumRange->getRangeWindowMin() ), pow( 10, lumRange->getRangeWindowMax() ) );
}

void ImageMDIwindow::setRangeWindow( float min, float max ) {
	minValue = min;
	maxValue = max;
	updateImage();
}
void ImageMDIwindow::setLumMappingMethod( int method ) {
	mappingMethodCB->setCurrentIndex( method );
	mappingMethod = (LumMappingMethod)method;
	updateImage();
}
void ImageMDIwindow::zoomIn() {
	scaleImage(1.25);
}
void ImageMDIwindow::zoomOut() {
	scaleImage(0.8);
}
void ImageMDIwindow::fitToWindow(bool checked) {
// 	scrollArea->setWidgetResizable(checked);
	fittingwin=checked;
	if (checked)
		scaleImageToFit();
	else
		// restore to the previous zoom factor
		scaleImage(1);
}
void ImageMDIwindow::normalSize() {
// 	scrollArea->setWidgetResizable(false);
	imageLabel->adjustSize();
	scaleFactor = 1.0;
}
void ImageMDIwindow::scaleImageToFit() {
	int sa_width=scrollArea->size().width();
	int sa_height=scrollArea->size().height();
	float imageratio=float(imageLabel->pixmap()->size().width())/float(imageLabel->pixmap()->size().height());
	float factor=1;
	if (sa_width<imageratio*sa_height) {
		factor=float(sa_width)/float(imageLabel->pixmap()->size().width());
	} else {
		factor=float(sa_height)/float(imageLabel->pixmap()->size().height());
	}
	imageLabel->resize(factor * 0.99 * imageLabel->pixmap()->size());
}
void ImageMDIwindow::scaleImage(double factor) {
	scaleFactor *= factor;
	imageLabel->resize(scaleFactor * imageLabel->pixmap()->size());
	adjustScrollBar(scrollArea->horizontalScrollBar(), factor);
	adjustScrollBar(scrollArea->verticalScrollBar(), factor);
}
void ImageMDIwindow::adjustScrollBar(QScrollBar *scrollBar, double factor) {
	scrollBar->setValue(int(factor * scrollBar->value() + ((factor - 1) * scrollBar->pageStep()/2)));
}
double ImageMDIwindow::getScaleFactor() {
	return scaleFactor;
}
bool ImageMDIwindow::getFittingWin(){
	return fittingwin;
}
void ImageMDIwindow::resizeEvent ( QResizeEvent * ) {
	if (fittingwin) {
		scaleImageToFit();
	}
}
void ImageMDIwindow::update_colors( unsigned int neg, unsigned int naninf ) {
	naninfcol=naninf;
	negcol=neg;
	updateImage();
// 	qDebug("uc: %u and %u",naninfcol,negcol);
}

const pfs::Array2D *ImageMDIwindow::getPrimaryChannel() {
    assert( pfsFrame != NULL );
    pfs::Channel *R, *G, *B;
    pfsFrame->getRGBChannels( R, G, B );
    return G;
}

ImageMDIwindow::~ImageMDIwindow() {
	//do not delete workarea, it shares the same memory area of pfsFrame
	if (imageLabel) delete imageLabel;
	if (scrollArea) delete scrollArea;
	if (image) delete image;
	if (pfsFrame!=NULL)
		delete pfsFrame;
}

pfs::Frame* ImageMDIwindow::getHDRPfsFrame() {
    return pfsFrame;
}
