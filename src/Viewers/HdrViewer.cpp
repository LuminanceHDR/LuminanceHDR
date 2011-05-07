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

#include <cmath>
#include <assert.h>
#include <QApplication>
#include <QFileInfo>
#include <QtConcurrentRun>
 #include <QFuture>

#include "Common/global.h"
#include "Common/msec_timer.h"
#include "HdrViewer.h"

template<class T>
T clamp( T val, T min, T max )
{
  if ( val < min ) return min;
  if ( val > max ) return max;
  return val;
}

float HdrViewer::getInverseMapping( float v )
{
  switch( this->mappingMethod )
  {
    case MAP_GAMMA1_4:
      return powf( v, 1.4f )*(this->maxValue - this->minValue) + this->minValue;
    case MAP_GAMMA1_8:
      return powf( v, 1.8f )*(this->maxValue - this->minValue) + this->minValue;
    case MAP_GAMMA2_2:
      return powf( v, 2.2f )*(this->maxValue - this->minValue) + this->minValue;
    case MAP_GAMMA2_6:
      return powf( v, 2.6f )*(this->maxValue - this->minValue) + this->minValue;
    case MAP_LINEAR:
      return v*(this->maxValue - this->minValue) + this->minValue;
    case MAP_LOGARITHMIC:
      return powf( 10.f, v * (log10f(this->maxValue) - log10f(this->minValue)) + log10f( this->minValue )  );
    default:
      assert(0);
      return 0.f;
  }
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
}

void HdrViewer::updateHDR(pfs::Frame* inputframe)
{
    assert(inputframe != NULL);
    pfs::DOMIO pfsio;

    if ( pfsFrame != NULL )
    {
        //delete previous pfs::Frame*. It must be done calling freeFrame()
        pfsio.freeFrame(pfsFrame);
        delete m_image;
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
    m_cols  = workArea[0]->getCols();
    m_rows  = workArea[0]->getRows();

    m_image = new QImage(m_cols, m_rows, QImage::Format_RGB32);

    m_lumRange->setHistogramImage(getPrimaryChannel());

    //fitToDynamicRange() already takes care -indirectly- to call updateImage()
    m_lumRange->fitToDynamicRange();

    //zoom at original size, 100%
    //make the label use the image dimensions
    imageLabel.adjustSize();
    imageLabel.update();
}

void HdrViewer::setFlagUpdateImage(bool updateImage)
{
    flagUpdateImage = updateImage;
}

void HdrViewer::updateImage()
{
    if (!flagUpdateImage) return;

    assert( pfsFrame != NULL );

    QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );
    emit S_init();

    // Start the computation.
    mapFrameToImage();

    //assign the mapped image to the label
    imageLabel.setPixmap(QPixmap::fromImage(*m_image));

    //scaleFactor is stored into scrollArea (1.0 by default)
    scrollArea->scaleImage();

    emit S_end();
    QApplication::restoreOverrideCursor();
}

#define NEW_MAP_FRAME 1

#ifdef NEW_MAP_FRAME
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

    QRgb *pixels = reinterpret_cast<QRgb*>(m_image->bits());

    int pr, pg, pb;

#pragma omp parallel for private(pr, pg, pb)
    for ( int index = 0; index < m_rows*m_cols; ++index )
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

#else

void HdrViewer::mapFrameToImage()
{
#ifdef TIMER_PROFILING
    msec_timer __timer;
    __timer.start();
#endif

    const int lutSize = 256;    // 256 levels!

    const int rows = workArea[0]->getRows();
    const int cols = workArea[0]->getCols();

    float lutPixFloor[lutSize/*257*2*/];

    //lutPixFloor[0] = 0.0f;
    for ( int p = 0; p < lutSize; p++ )
    {
        //float p_left = ((float)p - 1.f)/255.f; // Should be -1.5f, but we don't want negative nums
        float p_left = ((float)p)/255.f; // Should be -1.5f, but we don't want negative nums
        lutPixFloor[p] = getInverseMapping( p_left );
    }

    QRgb *pixels = reinterpret_cast<QRgb*>(m_image->bits());

    for ( int index = 0; index < rows*cols; ++index )
    {
        if ( !finite( (*workArea[0])(index) ) || !finite( (*workArea[1])(index) ) || !finite( (*workArea[2])(index) ) )   // x is NaN or Inf
        {
            pixels[index] = naninfcol;
        }
        else if( (*workArea[0])(index)<0 || (*workArea[1])(index)<0 || (*workArea[2])(index)<0 )    // x is negative
        {
            pixels[index] = negcol;
        }
        else
        {
            int pr, pg, pb;

            // Color channels
            pr = binarySearchPixels( (*workArea[0])(index), lutPixFloor, lutSize );
            pg = binarySearchPixels( (*workArea[1])(index), lutPixFloor, lutSize );
            pb = binarySearchPixels( (*workArea[2])(index), lutPixFloor, lutSize );

            // Clipping
            pixels[index] = qRgb(clamp( pr /*-1 */, 0, 255 ),
                                 clamp( pg /*-1 */, 0, 255 ),
                                 clamp( pb /*-1 */, 0, 255 ));
        }

    }
#ifdef TIMER_PROFILING
    __timer.stop_and_update();
    std::cout << "HdrViewer::mapFrameToImage() [OLD]= " << __timer.get_time() << " msec" << std::endl;
#endif
}
#endif

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
    saveLDRImage(this, QFileInfo(filename).completeBaseName()+"_preview"+".jpg", m_image);
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
