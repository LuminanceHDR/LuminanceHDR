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

#include <iostream>
#include <QDebug>

#include "Common/config.h"
#include "Common/GammaAndLevels.h"
#include "Viewers/LdrViewer.h"
#include "Viewers/IGraphicsPixmapItem.h"
#include "Core/TonemappingOptions.h"
#include "Libpfs/channel.h"
#include "Libpfs/frame.h"
#include "Common/msec_timer.h"

namespace
{

//! \brief clamp the input between minV and maxV and round it to the nearest integer
inline int clamp_and_round_2int(const float v, const float minV, const float maxV)
{
    if ( v < minV ) return (int)minV;
    if ( v > maxV ) return (int)maxV;
    return (int)(v + 0.5f); // round
}

//! \brief computes a QImage, RGB 8 bit/channel from the input frame
//! \param[in] in_frame pointer to pfs::Frame
//! \return QImage
QImage mapFrameToImage(pfs::Frame* in_frame)
{
#ifdef QT_DEBUG
    msec_timer stopWatch;
    stopWatch.start();
#endif

    pfs::Channel *Xc, *Yc, *Zc;
    in_frame->getXYZChannels( Xc, Yc, Zc );
    assert( Xc != NULL && Yc != NULL && Zc != NULL );

    const int width   = in_frame->getWidth();
    const int height  = in_frame->getHeight();

    QImage temp_qimage(width, height, QImage::Format_ARGB32);

    const float* p_R = Xc->getChannelData()->getRawData();
    const float* p_G = Yc->getChannelData()->getRawData();
    const float* p_B = Zc->getChannelData()->getRawData();

    int red, green, blue;
    QRgb *pixels = reinterpret_cast<QRgb*>(temp_qimage.bits());

#pragma omp parallel for private(red, green, blue) shared(pixels)
    for (int idx = 0; idx < height*width; ++idx)
    {
        red = clamp_and_round_2int(p_R[idx]*255.f, 0.0f, 255.f);
        green = clamp_and_round_2int(p_G[idx]*255.f, 0.0f, 255.f);
        blue = clamp_and_round_2int(p_B[idx]*255.f, 0.0f, 255.f);

        pixels[idx] = qRgb(red, green, blue);
    }

#ifdef QT_DEBUG
    stopWatch.stop_and_update();
    qDebug() << "LdrViewer::mapFrameToImage() = " << stopWatch.get_time() << " msec";
#endif

    return temp_qimage;
}

} // anonymous namespace

LdrViewer::LdrViewer(pfs::Frame* frame, const TonemappingOptions* opts, QWidget *parent, bool ns):
    GenericViewer(frame, parent, ns),
    mTonemappingOptions(opts),
    informativeLabel(new QLabel( tr("LDR image [%1 x %2]").arg(getWidth()).arg(getHeight()), mToolBar))
{
    temp_image = NULL;
    previewimage = NULL;

    mToolBar->addWidget(informativeLabel);

    mPixmap->disableSelectionTool(); // disable by default crop functionalities

    parseOptions(opts);
    setWindowTitle(caption);
    setToolTip(caption);

    mPixmap->setPixmap(QPixmap::fromImage(mapFrameToImage(getFrame())));

    updateView();
}

LdrViewer::~LdrViewer()
{
#ifdef QT_DEBUG
    std::cout << "LdrViewer::~LdrViewer()" << std::endl;
#endif
}

void LdrViewer::parseOptions(const TonemappingOptions *opts)
{
    TMOptionsOperations tmopts(opts);
    //postfix = tmopts.getPostfix();
    caption = tmopts.getCaption();
    //exif_comment = tmopts.getExifComment();
}

QString LdrViewer::getFileNamePostFix()
{
    if ( mTonemappingOptions )
    {
        TMOptionsOperations tm_ops(mTonemappingOptions);
        return tm_ops.getPostfix();
    } else
        return QString();
}

QString LdrViewer::getExifComment()
{
    if ( mTonemappingOptions )
    {
        TMOptionsOperations tm_ops(mTonemappingOptions);
        return tm_ops.getExifComment();
    } else
        return QString();
}

void LdrViewer::levelsRequested(bool /*a*/)
{
//    // TODO : Check this rubbish!

//    temp_image = mImage;
//    previewimage = new QImage( mCols, mRows, QImage::Format_RGB32 ); //image->copy() //copy original data

//    GammaAndLevels *levels = new GammaAndLevels(this, mImage);
//    levels->setAttribute(Qt::WA_DeleteOnClose);
//    //when closing levels, inform the Tone Mapping dialog.
//    connect(levels,SIGNAL(closing()), this, SIGNAL(levels_closed()));
//    //refresh preview when a values changes
//    connect(levels,SIGNAL(LUTrefreshed(unsigned char *)),this,SLOT(updatePreview(unsigned char *)));
//    //accept the changes
//    connect(levels,SIGNAL(accepted()),this,SLOT(finalize_levels()));
//    //restore original on "cancel"
//    connect(levels,SIGNAL(rejected()),this,SLOT(restore_original()));

//    levels->exec();
}

// TODO : clean up this implementation... (in case I keep it in the final release)!!!
void LdrViewer::updatePreview(unsigned char *LUT)
{
//#ifdef QT_DEBUG
//    qDebug() << "LdrViewer::updatePreview\n";
//#endif

//    const QRgb* src = (const QRgb*)mImage->bits();
//    QRgb* dst = (QRgb*)previewimage->bits();

//#pragma omp parallel for default(none) shared(src, dst, LUT)
//    for (int i=0; i < getWidth()*getHeight(); ++i)
//    {
//	dst[i] = qRgb(LUT[qRed(src[i])],LUT[qGreen(src[i])],LUT[qBlue(src[i])]);
//    }
//    mPixmap->setPixmap(QPixmap::fromImage(*previewimage));
}

void LdrViewer::restore_original()
{
//    //printf("LdrViewer::restoreoriginal() \n");
//    mPixmap->setPixmap(QPixmap::fromImage(*mImage));

//    delete previewimage;

//    temp_image = NULL;
//    previewimage = NULL;
}

void LdrViewer::finalize_levels()
{
//    //printf("LdrViewer::finalize_levels() \n");
//    mImage = previewimage;
//    mPixmap->setPixmap(QPixmap::fromImage(*mImage));

//    delete temp_image;

//    temp_image = NULL;
//    previewimage = NULL;
}

//void LdrViewer::setImage(QImage *i)
//{
//    if (mImage != NULL)
//        delete mImage;
//    if (informativeLabel != NULL)
//        delete informativeLabel;

//    mImage = new QImage(*i);
//    mPixmap->setPixmap(QPixmap::fromImage(*mImage));

//    mCols = mImage->width();
//    mRows = mImage->height();
//    informativeLabel = new QLabel( tr("LDR image [%1 x %2]").arg(mCols).arg(mRows), mToolBar );
//    mToolBar->addWidget(informativeLabel);
//}

void LdrViewer::updatePixmap()
{
#ifdef QT_DEBUG
    qDebug() << "void LdrViewer::updatePixmap()";
#endif

    mPixmap->setPixmap(QPixmap::fromImage(mapFrameToImage(getFrame())));
}
