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
#include <QFileInfo>
#include <QDebug>

#include <cmath>
#include <assert.h>
#include "arch/math.h"

#include "Common/global.h"
#include "Common/msec_timer.h"
#include "Viewers/IGraphicsPixmapItem.h"
#include "Libpfs/array2d.h"
#include "Libpfs/channel.h"
#include "Libpfs/frame.h"
#include "Libpfs/domio.h"
#include "Viewers/LuminanceRangeWidget.h"
#include "Libpfs/vex.h"

namespace // anonymous namespace
{
// Implementation details stay inside an anonymous namespace
// In this way, we let know the compiler it can mess up as much as it wants with the code,
// because it will only used inside this compilation unit

template<class T>
inline T clamp( T val, T min, T max )
{
    if ( val < min ) return min;
    if ( val > max ) return max;
    return val;
}

const pfs::Array2D* getPrimaryChannel(pfs::Frame* frame)
{
    assert( frame != NULL );

    return frame->getChannel("Y")->getChannelData();
}

} // end anonymous namespace

class HdrViewerMapping
{
public:
    HdrViewerMapping(int nan_inf_color = 0, int neg_color = 0, float min_value = 1.0f, float max_value = 1.0f):
        m_NanInfColor(nan_inf_color),
        m_NegColor(neg_color),
        m_MappingMethod(MAP_GAMMA2_2)
    {
        setMinMax(min_value, max_value);
    }

    void setMinMax( float min, float max )
    {
        m_MinValue      = min;
        m_MaxValue      = max;
        m_Range         = max - min;
        m_LogRange      = log2f(max/min);
    }

    void updateErrorColors(int nan_inf_color, int neg_color)
    {
        m_NanInfColor   = nan_inf_color;
        m_NegColor      = neg_color;
    }

    void setMappingMethod(int method)
    {
        m_MappingMethod = static_cast<LumMappingMethod>(method);
    }

    int getMappingMethod()
    {
        return m_MappingMethod;
    }

    float getMinLuminance()
    {
        return m_MinValue;
    }

    float getMaxLuminance()
    {
        return m_MaxValue;
    }

    void getMapping(float r, float g, float b, QRgb& pixel)
    {
#ifdef __USE_SSE__
        v4sf rgb = (_mm_set_ps(0, b, g, r) - _mm_set1_ps(m_MinValue)) / _mm_set1_ps(m_Range);

        switch ( m_MappingMethod )
        {
        case MAP_GAMMA1_4:
            rgb = _mm_pow_ps(rgb, _mm_set1_ps(1.0f/1.4f));
            break;
        case MAP_GAMMA1_8:
            rgb = _mm_pow_ps(rgb, _mm_set1_ps(1.0f/1.8f));
            break;
        case MAP_GAMMA2_2:
            rgb = _mm_pow_ps(rgb, _mm_set1_ps(1.0f/2.2f));
            break;
        case MAP_GAMMA2_6:
            rgb = _mm_pow_ps(rgb, _mm_set1_ps(1.0f/2.6f));
            break;
        case MAP_LOGARITHMIC:
            // log(x) - log(y) = log(x/y)
            rgb = _mm_log2_ps(_mm_set_ps(0, b, g, r)/_mm_set1_ps(m_MinValue))
                    / _mm_set1_ps(m_LogRange);
            break;
        default:
        case MAP_LINEAR:
            // do nothing
            // final value is already calculated
            break;
        }
        rgb *= _mm_set1_ps(255.f);
        rgb = _mm_min_ps(rgb, _mm_set1_ps(255.f));
        rgb = _mm_max_ps(rgb, _mm_set1_ps(0.f));
        rgb += _mm_set1_ps(0.5f);
        float buf[4];
        _mm_store_ps(buf, rgb);
        pixel = qRgb( buf[0], buf[1], buf[2] );
#else
        float rgb[3] = {
            (r - m_MinValue)/m_Range,
            (g - m_MinValue)/m_Range,
            (b - m_MinValue)/m_Range
        };

        switch ( m_MappingMethod )
        {
        case MAP_GAMMA1_4:
            for ( int i = 0; i < 3; i++ )
                rgb[i] = powf(rgb[i], 1.0f/1.4f);
            break;
        case MAP_GAMMA1_8:
            for ( int i = 0; i < 3; i++ )
                rgb[i] = powf(rgb[i], 1.0f/1.8f);
            break;
        case MAP_GAMMA2_2:
            for ( int i = 0; i < 3; i++ )
                rgb[i] = powf(rgb[i], 1.0f/2.2f);
            break;
        case MAP_GAMMA2_6:
            for ( int i = 0; i < 3; i++ )
                rgb[i] = powf(rgb[i], 1.0f/2.6f);
            break;
        case MAP_LOGARITHMIC:
            // log(x) - log(y) = log(x/y)
            rgb[0] = log2f(r/m_MinValue)/m_LogRange;
            rgb[1] = log2f(g/m_MinValue)/m_LogRange;
            rgb[2] = log2f(b/m_MinValue)/m_LogRange;
            break;
        default:
        case MAP_LINEAR:
            // do nothing
            // final value is already calculated
            break;
        }
        pixel = qRgb( clamp(rgb[0]*255.f, 0.0f, 255.f) + 0.5f,
                      clamp(rgb[1]*255.f, 0.0f, 255.f) + 0.5f,
                      clamp(rgb[2]*255.f, 0.0f, 255.f) + 0.5f );
#endif
    }

    QImage mapFrameToImage(pfs::Frame* in_frame)
    {
#ifdef TIMER_PROFILING
        msec_timer __timer;
        __timer.start();
#endif

        // Channels X Y Z contain R G B data
        pfs::Channel *ChR, *ChG, *ChB;
        in_frame->getXYZChannels( ChR, ChG, ChB );
        assert( ChR != NULL && ChG != NULL && ChB != NULL);

        const float* R = ChR->getChannelData()->getRawData();
        const float* G = ChG->getChannelData()->getRawData();
        const float* B = ChB->getChannelData()->getRawData();

        assert( R != NULL && G != NULL && B != NULL );

        QImage return_qimage(in_frame->getWidth(), in_frame->getHeight(), QImage::Format_RGB32);
        QRgb *pixels = reinterpret_cast<QRgb*>(return_qimage.bits());

#pragma omp parallel for
        for ( int index = 0; index < in_frame->getWidth()*in_frame->getHeight(); ++index )
        {
            if ( !finite( R[index] ) || !finite( G[index] ) || !finite( B[index] ) )   // x is NaN or Inf
            {
                pixels[index] = m_NanInfColor;
            }
            else if ( R[index] < 0 || G[index]<0 || B[index]<0 )    // x is negative
            {
                pixels[index] = m_NegColor;
            }
            else
            {
                getMapping(R[index], G[index], B[index], pixels[index]);
            }
        }

#ifdef TIMER_PROFILING
        __timer.stop_and_update();
        std::cout << "HdrViewer::mapFrameToImage() [NEW]= " << __timer.get_time() << " msec" << std::endl;
#endif

        return return_qimage;
    }
private:
    // Current Visualization mode
    LumMappingMethod m_MappingMethod;
    float m_MinValue;
    float m_MaxValue;
    float m_Range;
    float m_LogRange;

    //! NaN or Inf color
    int m_NanInfColor;
    //! Neg color
    int m_NegColor;
}; // end HdrViewerMapping


HdrViewer::HdrViewer(pfs::Frame* frame, QWidget *parent, bool ns, unsigned int neg, unsigned int naninf):
    GenericViewer(frame, parent, ns),
    m_mappingImpl(new HdrViewerMapping(naninf, neg))
{
    init_ui();

    // I prefer to do everything by hand, so the flow of the calls is clear
    m_lumRange->blockSignals(true);

    m_lumRange->setHistogramImage(getPrimaryChannel(getFrame()));
    m_lumRange->fitToDynamicRange();

    m_mappingImpl->setMinMax(powf( 10.0f, m_lumRange->getRangeWindowMin() ),
                             powf( 10.0f, m_lumRange->getRangeWindowMax() ));

    mPixmap->setPixmap(QPixmap::fromImage(m_mappingImpl->mapFrameToImage(getFrame())));

    updateView();
    m_lumRange->blockSignals(false);
}

void HdrViewer::init_ui()
{
    QLabel *mappingMethodLabel = new QLabel( tr("&Mapping:"), mToolBar );
    mappingMethodLabel->setAlignment( Qt::AlignRight | Qt::AlignVCenter );
    mappingMethodCB = new QComboBox( mToolBar );
    mToolBar->addWidget(mappingMethodLabel);
    mToolBar->addWidget(mappingMethodCB);
    mToolBar->addSeparator();
    mappingMethodLabel->setBuddy( mappingMethodCB );
    QStringList methods;
    methods << tr("Linear")
            << tr("Gamma 1.4")
            << tr("Gamma 1.8")
            << tr("Gamma 2.2")
            << tr("Gamma 2.6")
            << tr("Logarithmic");

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

void HdrViewer::refreshPixmap()
{
#ifdef QT_DEBUG
    qDebug() << "void HdrViewer::refreshPixmap()";
#endif

    setCursor( Qt::WaitCursor );

    mPixmap->setPixmap(QPixmap::fromImage(m_mappingImpl->mapFrameToImage(getFrame())));

    unsetCursor();
}

void HdrViewer::updatePixmap()
{
#ifdef QT_DEBUG
    qDebug() << "void HdrViewer::updatePixmap()";
#endif

    m_lumRange->blockSignals(true);

    refreshPixmap();

    // I need to set the histogram again during the setFrame function
    m_lumRange->setHistogramImage(getPrimaryChannel(getFrame()));
    m_lumRange->fitToDynamicRange();
    m_lumRange->blockSignals(false);
}

LuminanceRangeWidget* HdrViewer::lumRange()
{
    return m_lumRange;
}

void HdrViewer::updateRangeWindow()
{
    setRangeWindow(powf(10.0f, m_lumRange->getRangeWindowMin()),
                   powf(10.0f, m_lumRange->getRangeWindowMax()));
}

void HdrViewer::setRangeWindow( float min, float max )
{
    m_mappingImpl->setMinMax(min, max);

    refreshPixmap();
}

int HdrViewer::getLumMappingMethod()
{
    return mappingMethodCB->currentIndex();
}

void HdrViewer::setLumMappingMethod( int method )
{
    mappingMethodCB->setCurrentIndex( method );
    m_mappingImpl->setMappingMethod(method);

    refreshPixmap();
}

void HdrViewer::update_colors(int neg, int naninf )
{
    m_mappingImpl->updateErrorColors(naninf, neg);

    refreshPixmap();
}

//! empty dtor
HdrViewer::~HdrViewer()
{}

QString HdrViewer::getFileNamePostFix()
{
    return QString("_hdr_preview");
}

QString HdrViewer::getExifComment()
{
    return QString("HDR Created with Luminance HDR");
}

//! \brief returns max value of the handled frame
float HdrViewer::getMaxLuminanceValue()
{
    return m_mappingImpl->getMaxLuminance();
}

//! \brief returns min value of the handled frame
float HdrViewer::getMinLuminanceValue()
{
    return m_mappingImpl->getMinLuminance();
}
