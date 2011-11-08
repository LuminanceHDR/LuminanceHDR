/**
 * This file is a part of LuminanceHDR package.
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
 * Original Work
 * @author Giuseppe Rota <grota@users.sourceforge.net>
 * Improvements, bugfixing
 * @author Franco Comida <fcomida@users.sourceforge.net>
 *
 */

#include "HdrViewer.h"

#include <QLabel>
#include <QApplication>
#include <QFileInfo>

#include <cmath>
#include <assert.h>
#include "arch/math.h"

#include "Common/global.h"
#include "Common/msec_timer.h"
#include "Libpfs/domio.h"

template<class T>
T clamp( T val, T min, T max )
{
  if ( val < min ) return min;
  if ( val > max ) return max;
  return val;
}

inline int binarySearchPixels( float x, const float *lut, const int lutSize )
{
  int l = 0, r = lutSize;
  while ( true )
  {
    int m = (l+r)/2;
    if ( m == l ) break;
    if ( x < lut[m] )
        r = m;
    else
        l = m;
  }
  return l;
}

//==================================================================================
//**********************************************************************************
//==================================================================================

int HdrViewer::getMapping( float x )
{
    float final_value = (x - this->minValue)/(this->maxValue - this->minValue);

    switch ( this->mappingMethod )
    {
    case MAP_GAMMA1_4:
        final_value = powf(final_value, 1.f/1.4f);
        break;
    case MAP_GAMMA1_8:
        final_value = powf(final_value, 1.f/1.8f);
        break;
    case MAP_GAMMA2_2:
        final_value = powf(final_value, 1.f/2.2f);
        break;
    case MAP_GAMMA2_6:
        final_value = powf(final_value, 1.f/2.6f);
        break;
    case MAP_LOGARITHMIC:
        // log(x) - log(y) = log(x/y)
        final_value = (log10f(x/minValue))/(log10f(maxValue/minValue));
        break;
    default:
    case MAP_LINEAR:
        // do nothing
        // final value is already calculated
        break;
    }
    return (int)(clamp(final_value*255.f, 0.0f, 255.f) + 0.5f);   // (int)(x + 0.5) = round(x)
}

HdrViewer::HdrViewer(QWidget *parent, bool ns, bool ncf, unsigned int neg, unsigned int naninf):
        GenericViewer(parent, ns, ncf), mappingMethod(MAP_GAMMA2_2), minValue(1.0f), maxValue(1.0f), naninfcol(naninf), negcol(neg)
{
    init_ui();

    flagUpdateImage = true;
    flagFreeOnExit = true;

    pfsFrame = NULL;
    workArea[0] = NULL;
    workArea[1] = NULL;
    workArea[2] = NULL;
}

void HdrViewer::init_ui()
{
    setAttribute(Qt::WA_DeleteOnClose);

    QLabel *mappingMethodLabel = new QLabel( tr("&Mapping:"), mToolBar );
    mappingMethodLabel->setAlignment( Qt::AlignRight | Qt::AlignVCenter );
    mappingMethodCB = new QComboBox( mToolBar );
    mToolBar->addWidget(mappingMethodLabel);
    mToolBar->addWidget(mappingMethodCB);
    mToolBar->addSeparator();
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
    QLabel *histlabel = new QLabel( tr("Histogram:"), mToolBar );
    m_lumRange = new LuminanceRangeWidget( mToolBar );
    mToolBar->addWidget(histlabel);
    mToolBar->addWidget(m_lumRange);
    mToolBar->addSeparator();
    connect( m_lumRange, SIGNAL( updateRangeWindow() ), this, SLOT( updateRangeWindow() ) );
    mToolBar->setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Fixed);
}

void HdrViewer::updateHDR(pfs::Frame* inputframe)
{
#ifdef QT_DEBUG
    assert(inputframe != NULL);
#else
    if (inputframe == NULL) return;
#endif

    if ( pfsFrame != NULL )
    {
        delete pfsFrame;
        delete mImage;
    }
    // Update Pointer to HDR Frame
    pfsFrame = inputframe;

    // Update Pointers to Channels
    // Channels X Y Z contain R G B data
    pfs::Channel *R, *G, *B;
    pfsFrame->getXYZChannels( R, G, B );
    assert(R!=NULL && G!=NULL && B!=NULL);

    // workArea needed in mapFrameToImage(...)
    workArea[0] = R->getChannelData();
    workArea[1] = G->getChannelData();
    workArea[2] = B->getChannelData();
    assert( workArea[0] != NULL && workArea[1] != NULL && workArea[2] != NULL );

    // Update size of the frame
    mCols  = workArea[0]->getCols();
    mRows  = workArea[0]->getRows();

    mImage = new QImage(mCols, mRows, QImage::Format_RGB32);

    m_lumRange->setHistogramImage(getPrimaryChannel());

    //fitToDynamicRange() already takes care -indirectly- to call updateImage()
    m_lumRange->fitToDynamicRange();
}

void HdrViewer::setFlagUpdateImage(bool updateImage)
{
    flagUpdateImage = updateImage;
}

void HdrViewer::updateImage()
{
    if (!flagUpdateImage) return;

    assert( pfsFrame != NULL );

    setCursor(Qt::WaitCursor);
    //QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );

    // Start the computation.
    mapFrameToImage();

    //assign the mapped image to the label
    mPixmap->setPixmap(QPixmap::fromImage(*mImage));

    mScene->addItem(mPixmap); // ??

    //QApplication::restoreOverrideCursor();
    unsetCursor();
}

void HdrViewer::mapFrameToImage()
{
#ifdef TIMER_PROFILING
    msec_timer __timer;
    __timer.start();
#endif

    if (!flagUpdateImage) return;

    const float* R = workArea[0]->getRawData();
    const float* G = workArea[1]->getRawData();
    const float* B = workArea[2]->getRawData();

    QRgb *pixels = reinterpret_cast<QRgb*>(mImage->bits());

    int pr, pg, pb;

#pragma omp parallel for private(pr, pg, pb)
    for ( int index = 0; index < mRows*mCols; ++index )
    {
        if ( !finite( R[index] ) || !finite( G[index] ) || !finite( B[index] ) )   // x is NaN or Inf
        {
            pixels[index] = naninfcol;
        }
        else if ( R[index] < 0 || G[index]<0 || B[index]<0 )    // x is negative
        {
            pixels[index] = negcol;
        }
        else
        {
            pr = getMapping( R[index] );
            pg = getMapping( G[index] );
            pb = getMapping( B[index] );

            pixels[index] = qRgb( pr, pg, pb );
        }
    }

#ifdef TIMER_PROFILING
    __timer.stop_and_update();
    std::cout << "HdrViewer::mapFrameToImage() [NEW]= " << __timer.get_time() << " msec" << std::endl;
#endif
}

LuminanceRangeWidget* HdrViewer::lumRange()
{
    return m_lumRange;
}

void HdrViewer::updateRangeWindow()
{
  setRangeWindow( pow( 10, m_lumRange->getRangeWindowMin() ), pow( 10, m_lumRange->getRangeWindowMax() ) );
}

void HdrViewer::setRangeWindow( float min, float max )
{
    minValue = min;
    maxValue = max;

    updateImage();
}

int HdrViewer::getLumMappingMethod()
{
    return mappingMethodCB->currentIndex();
}

void HdrViewer::setLumMappingMethod( int method )
{
    mappingMethodCB->setCurrentIndex( method );
    mappingMethod = (LumMappingMethod)method;

    updateImage();
}

void HdrViewer::update_colors( unsigned int neg, unsigned int naninf )
{
    naninfcol=naninf;
    negcol=neg;

    updateImage();
}

const pfs::Array2D *HdrViewer::getPrimaryChannel()
{
    assert( pfsFrame != NULL );

    return pfsFrame->getChannel("Y")->getChannelData();
}

HdrViewer::~HdrViewer()
{
    //do not delete workarea, it shares the same memory area of pfsFrame
    if (pfsFrame && flagFreeOnExit) // It must be deleted calling freeFrame()
    {
        pfs::DOMIO pfsio;
        std::cout << "HdrViewer::~HdrViewer(): Freed pfsFrame" << std::endl;
        pfsio.freeFrame(pfsFrame);
    }
}

pfs::Frame* HdrViewer::getHDRPfsFrame()
{
    return pfsFrame;
}

void HdrViewer::saveHdrPreview()
{
    qDebug("filename=%s",qPrintable(filename));
    saveLDRImage(this, QFileInfo(filename).completeBaseName()+"_preview"+".jpg", mImage);
}

void HdrViewer::setFreePfsFrameOnExit(bool b)
{
    flagFreeOnExit = b;
}

//
//==================================================================================================//
//
// The following methods do nothing. They are defined here to keep tonemappingWindow.cpp code simple
//
void HdrViewer::levelsRequested(bool) {} 

QString HdrViewer::getFilenamePostFix() { 
	return QString();
}


const QImage* HdrViewer::getQImage()
{
        return NULL;
}

QString HdrViewer::getExifComment() {
	return QString();
}
//================================================================================================//
