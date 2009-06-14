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

#include <math.h>
#include <QApplication>
#include <QFileInfo>
#include "../Common/global.h"
#include "hdrviewer.h"

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


static float getInverseMapping( LumMappingMethod mappingMethod, float v, float minValue, float maxValue ) {
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

//==================================================================================
//**********************************************************************************
//==================================================================================

HdrViewer::HdrViewer(QWidget *parent, bool ns, bool ncf, unsigned int neg, unsigned int naninf) : GenericViewer(parent, ns, ncf), pfsFrame(NULL), mappingMethod(MAP_GAMMA2_2), minValue(1.0f), maxValue(1.0f), naninfcol(naninf), negcol(neg) {

	flagUpdateImage = true;

	workArea[0] = workArea[1] = workArea[2] = NULL;
	setAttribute(Qt::WA_DeleteOnClose);

	QLabel *mappingMethodLabel = new QLabel( tr("&Mapping:"), toolBar );
	mappingMethodLabel->setAlignment( Qt::AlignRight | Qt::AlignVCenter );
	mappingMethodCB = new QComboBox( toolBar );
	toolBar->addWidget(mappingMethodLabel);
	toolBar->addWidget(mappingMethodCB);
	toolBar->addSeparator();
	mappingMethodLabel->setBuddy( mappingMethodCB );
	QStringList methods;
	methods+=tr("Linear");
	methods+=tr("Gamma 1.4");
	methods+=tr("Gamma 1.8");
	methods+=tr("Gamma 2.2");
	methods+=tr("Gamma 2.6");
	methods+=tr("Logarithmic");
	mappingMethodCB->addItems(methods);
	mappingMethodCB->setCurrentIndex( 3 );
	connect( mappingMethodCB, SIGNAL( activated( int ) ), this, SLOT( setLumMappingMethod(int) ) );
	QLabel *histlabel = new QLabel( tr("Histogram:"), toolBar );
	m_lumRange = new LuminanceRangeWidget( toolBar );
	toolBar->addWidget(histlabel);
	toolBar->addWidget(m_lumRange);
	toolBar->addSeparator();
	connect( m_lumRange, SIGNAL( updateRangeWindow() ), this, SLOT( updateRangeWindow() ) );
	toolBar->setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Fixed);


	progress = new QProgressDialog(0, 0, 0, 0, this);
     	progress->setWindowTitle(tr("Loading file..."));
     	progress->setWindowModality(Qt::WindowModal);
     	progress->setMinimumDuration(0);
}

void HdrViewer::updateHDR(pfs::Frame* inputframe) {
    assert(inputframe!=NULL);
    pfs::DOMIO pfsio;
    //delete previous pfs::Frame*. It must be done calling freeFrame()
    if (pfsFrame!=NULL)
       pfsio.freeFrame(pfsFrame);
    pfsFrame = inputframe;
    m_lumRange->setHistogramImage(getPrimaryChannel());
    m_lumRange->fitToDynamicRange();
    //fitToDynamicRange() already takes care -indirectly- to call updateImage()
    //zoom at original size, 100%
    //make the label use the image dimensions
    imageLabel.adjustSize();
    imageLabel.update();
}

void HdrViewer::setFlagUpdateImage(bool updateImage) {
	flagUpdateImage = updateImage;
}

void HdrViewer::updateImage() {
	if (flagUpdateImage) {
		assert( pfsFrame != NULL );

		QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );

		pfs::Channel *R, *G, *B;
		pfsFrame->getRGBChannels( R, G, B );
		assert(R!=NULL && G!=NULL && B!=NULL);

		//workarea  needed in updateMapping(...) called by mapFrameToImage(...)
		workArea[0]=R;
		workArea[1]=G;
		workArea[2]=B;
		assert( workArea[0] != NULL && workArea[1] != NULL && workArea[2] != NULL );

		int zoomedWidth = workArea[0]->getCols();
		int zoomedHeight = workArea[0]->getRows();

		image = QImage(zoomedWidth, zoomedHeight, QImage::Format_RGB32 );

		assert( workArea[0] != NULL && workArea[1] != NULL && workArea[2] != NULL );
		mapFrameToImage( /*workArea[0], workArea[1], workArea[2], &image,
				minValue, maxValue, mappingMethod*/ );

		//assign the mapped image to the label
		imageLabel.setPixmap(QPixmap::fromImage(image));
		scrollArea->scaleImage(); //scaleFactor is stored into scrollArea (1.0 by default)
		QApplication::restoreOverrideCursor();
	}
}

void HdrViewer::mapFrameToImage() {

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
	QRgb* line = (QRgb*)image.scanLine( (int)imgRow );
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

LuminanceRangeWidget* HdrViewer::lumRange() {
	return m_lumRange;
}

void HdrViewer::updateRangeWindow() {
 setRangeWindow( pow( 10, m_lumRange->getRangeWindowMin() ), pow( 10, m_lumRange->getRangeWindowMax() ) );
}

void HdrViewer::setRangeWindow( float min, float max ) {
	minValue = min;
	maxValue = max;
	updateImage();
}

int HdrViewer::getLumMappingMethod() {
	return mappingMethodCB->currentIndex();
}

void HdrViewer::setLumMappingMethod( int method ) {
	mappingMethodCB->setCurrentIndex( method );
	mappingMethod = (LumMappingMethod)method;
	updateImage();
}

void HdrViewer::update_colors( unsigned int neg, unsigned int naninf ) {
	naninfcol=naninf;
	negcol=neg;
	updateImage();
}

const pfs::Array2D *HdrViewer::getPrimaryChannel() {
    assert( pfsFrame != NULL );
    pfs::Channel *R, *G, *B;
    pfsFrame->getRGBChannels( R, G, B );
    return G;
}

HdrViewer::~HdrViewer() {
	pfs::DOMIO pfsio;
	//do not delete workarea, it shares the same memory area of pfsFrame
	if (pfsFrame) // It must be deleted calling freeFrame()
		pfsio.freeFrame(pfsFrame);
}

pfs::Frame* HdrViewer::getHDRPfsFrame() {
	return pfsFrame;
}

void HdrViewer::saveHdrPreview() {
	qDebug("filename=%s",qPrintable(filename));
	saveLDRImage(QFileInfo(filename).completeBaseName()+"_preview"+".jpg",image);
}

void HdrViewer::setMaximum(int max) {
        progress->setMaximum( max - 1 );
}

void HdrViewer::setValue(int value) {
        progress->setValue( value );
}

void HdrViewer::showLoadDialog(void) {
        progress->setValue( 1 );
}

void HdrViewer::hideLoadDialog(void) {
        progress->cancel();
}

//==================================================================================================//
//
// The following methods do nothing. They are defined here to keep tonemappingDialog.cpp code simple
//
void HdrViewer::levelsRequested(bool a) {} 

QString HdrViewer::getFilenamePostFix() { 
	return QString();
}


const QImage HdrViewer::getQImage() { 
	return QImage();
}

QString HdrViewer::getExifComment() {
	return QString();
}
//================================================================================================//
