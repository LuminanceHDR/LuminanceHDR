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
#include <QVBoxLayout>
#include <QMessageBox>
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

HdrViewer::HdrViewer(QWidget *parent, unsigned int neg, unsigned int naninf, bool ns, bool ncf) : GenericViewer(parent), NeedsSaving(ns), filename(""), image(NULL), pfsFrame(NULL), mappingMethod(MAP_GAMMA2_2), minValue(1.0f), maxValue(1.0f), naninfcol(naninf), negcol(neg), selection(false), noCloseFlag(ncf) {

	flagUpdateImage = true;

	workArea[0] = workArea[1] = workArea[2] = NULL;
	setAttribute(Qt::WA_DeleteOnClose);

	QVBoxLayout *VBL_L = new QVBoxLayout(this);
	VBL_L->setSpacing(0);
	VBL_L->setMargin(0);

	toolBar = new QToolBar(tr("Viewing Settings Toolbar"),this);
	QLabel *mappingMethodLabel = new QLabel( "&Mapping:", toolBar );
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
	lumRange = new LuminanceRangeWidget( toolBar );
	toolBar->addWidget(histlabel);
	toolBar->addWidget(lumRange);
	toolBar->addSeparator();
	connect( lumRange, SIGNAL( updateRangeWindow() ), this, SLOT( updateRangeWindow() ) );
	toolBar->setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Fixed);
	VBL_L->addWidget(toolBar);

	imageLabel = new QLabel(this);
	scrollArea = new SmartScrollArea(this,imageLabel);
	scrollArea->setBackgroundRole(QPalette::Shadow);
	VBL_L->addWidget(scrollArea);
	cornerButton=new QToolButton(this);
	cornerButton->setToolTip("Pan the image to a region");
	cornerButton->setIcon(QIcon(":/new/prefix1/images/move.png"));
	scrollArea->setCornerWidget(cornerButton);
	connect(cornerButton, SIGNAL(pressed()), this, SLOT(slotCornerButtonPressed()));
	connect(scrollArea, SIGNAL(selectionReady(bool)), this, SIGNAL(selectionReady(bool)));
	connect(scrollArea, SIGNAL(selectionReady(bool)), this, SLOT(setSelection(bool)));
	progress = new QProgressDialog(0, 0, 0, 0, this);
     	progress->setWindowTitle(tr("Loading file..."));
     	progress->setWindowModality(Qt::WindowModal);
     	progress->setMinimumDuration(0);
}

void HdrViewer::slotCornerButtonPressed() {
	panIconWidget=new PanIconWidget(this);
	panIconWidget->setImage(image);
	float zf=scrollArea->getScaleFactor();
	float leftviewpos=(float)(scrollArea->horizontalScrollBar()->value());
	float topviewpos=(float)(scrollArea->verticalScrollBar()->value());
	float wps_w=(float)(scrollArea->maximumViewportSize().width());
	float wps_h=(float)(scrollArea->maximumViewportSize().height());
	QRect r((int)(leftviewpos/zf), (int)(topviewpos/zf), (int)(wps_w/zf), (int)(wps_h/zf));
	panIconWidget->setRegionSelection(r);
	panIconWidget->setMouseFocus();
	connect(panIconWidget, SIGNAL(signalSelectionMoved(QRect, bool)), this, SLOT(slotPanIconSelectionMoved(QRect, bool)));
	QPoint g = scrollArea->mapToGlobal(scrollArea->viewport()->pos());
	g.setX(g.x()+ scrollArea->viewport()->size().width());
	g.setY(g.y()+ scrollArea->viewport()->size().height());
	panIconWidget->popup(QPoint(g.x() - panIconWidget->width()/2,
					g.y() - panIconWidget->height()/2));

	panIconWidget->setCursorToLocalRegionSelectionCenter();
}

void HdrViewer::slotPanIconSelectionMoved(QRect gotopos, bool mousereleased) {
	if (mousereleased) {
		scrollArea->horizontalScrollBar()->setValue((int)(gotopos.x()*scrollArea->getScaleFactor()));
		scrollArea->verticalScrollBar()->setValue((int)(gotopos.y()*scrollArea->getScaleFactor()));
		panIconWidget->close();
		slotPanIconHidden();
	}
}

void HdrViewer::slotPanIconHidden()
{
    cornerButton->blockSignals(true);
    cornerButton->animateClick();
    cornerButton->blockSignals(false);
}


void HdrViewer::updateHDR(pfs::Frame* inputframe) {
    assert(inputframe!=NULL);
    pfs::DOMIO pfsio;
    //delete previous pfs::Frame*. It must bu done calling freeFrame()
    if (pfsFrame!=NULL)
       pfsio.freeFrame(pfsFrame);
    pfsFrame = inputframe;
    lumRange->setHistogramImage(getPrimaryChannel());
    lumRange->fitToDynamicRange();
    //fitToDynamicRange() already takes care -indirectly- to call updateImage()
    //zoom at original size, 100%
    //make the label use the image dimensions
    imageLabel->adjustSize();
    imageLabel->update();
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

void HdrViewer::updateRangeWindow() {
 setRangeWindow( pow( 10, lumRange->getRangeWindowMin() ), pow( 10, lumRange->getRangeWindowMax() ) );
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

void HdrViewer::zoomIn() {
	scrollArea->zoomIn();
}
void HdrViewer::zoomOut() {
	scrollArea->zoomOut();
}
void HdrViewer::fitToWindow(bool checked) {
	scrollArea->fitToWindow(checked);
}
void HdrViewer::normalSize() {
	scrollArea->normalSize();
}
double HdrViewer::getScaleFactor() {
	return scrollArea->getScaleFactor();
}
bool HdrViewer::getFittingWin(){
	return scrollArea->isFitting();
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
	if (image) delete image;
	if (pfsFrame) // It must be deleted calling freeFrame()
		pfsio.freeFrame(pfsFrame);
	//std::cout << "HdrViewer::~HdrViewer()" << std::endl;
	//std::cout << pfsFrame << std::endl;
}

pfs::Frame*& HdrViewer::getHDRPfsFrame() {
	return pfsFrame;
}

void HdrViewer::closeEvent ( QCloseEvent * event ) {
	//std::cout << "HdrViewer::closeEvent()" << std::endl;
	//std::cout << pfsFrame << std::endl;
	if (NeedsSaving) {
		QMessageBox::StandardButton ret=QMessageBox::warning(this,tr("Unsaved changes..."),tr("This Hdr has unsaved changes.<br>Are you sure you want to close it?"),
		QMessageBox::No | QMessageBox::Yes, QMessageBox::No);
		if (ret==QMessageBox::Yes)
			event->accept();
		else
			event->ignore();
	} else if (noCloseFlag) {
		event->ignore();
	} else {
		event->accept();
	}
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

QRect HdrViewer::getSelectionRect(void) {
        return scrollArea->getSelectionRect();
}

void HdrViewer::setSelection(bool isReady) {
	isReady ? selection = false : selection = true;
}

void HdrViewer::removeSelection(void) {
	scrollArea->removeSelection();
	selection = false;
}

bool HdrViewer::hasSelection(void) {
	return selection;
}

