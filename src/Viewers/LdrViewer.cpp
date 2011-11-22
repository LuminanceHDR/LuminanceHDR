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

#include "Common/config.h"
#include "Common/GammaAndLevels.h"
#include "Viewers/LdrViewer.h"
#include "Viewers/IGraphicsPixmapItem.h"
#include "Core/TonemappingOptions.h"

LdrViewer::LdrViewer(QImage *i, quint16 *p, QWidget *parent, bool ns, bool ncf, const TonemappingOptions *opts):
        GenericViewer(parent, ns, ncf), informativeLabel(NULL)
{
    setAttribute(Qt::WA_DeleteOnClose);

    mImage = i;
    m_pixmap = p;

    if (mImage == 0) {
        mCols = 0;
        mRows = 0;
    }
    else {
        mCols = mImage->width();
        mRows = mImage->height();
    }

    temp_image = NULL;
    previewimage = NULL;

    informativeLabel = new QLabel( tr("LDR image [%1 x %2]").arg(mCols).arg(mRows), mToolBar);
    mToolBar->addWidget(informativeLabel);

    if (mImage == 0)
	return;

    mPixmap->setPixmap(QPixmap::fromImage(*mImage));
    mPixmap->disable(); // disable by default crop functionalities
    mScene->addItem(mPixmap);

    parseOptions(opts);
    setWindowTitle(caption);
    setToolTip(caption);
}

LdrViewer::~LdrViewer()
{
    std::cout << "LdrViewer::~LdrViewer()" << std::endl;

	delete [] m_pixmap;
    //delete origimage;
    //delete [] origimage.bits();
}

void LdrViewer::parseOptions(const TonemappingOptions *opts)
{
    TMOptionsOperations tmopts(opts);
    postfix = tmopts.getPostfix();
    caption = tmopts.getCaption();
    exif_comment = tmopts.getExifComment();
}

QString LdrViewer::getFilenamePostFix()
{
    return postfix;
}

const QImage* LdrViewer::getQImage()
{
    return mImage;
}

const quint16 *LdrViewer::getPixmap()
{
	return m_pixmap;
}

QString LdrViewer::getExifComment()
{
    return exif_comment;
}

void LdrViewer::levelsRequested(bool /*a*/)
{
    // TODO : Check this rubbish!

    temp_image = mImage;
    previewimage = new QImage( mCols, mRows, QImage::Format_RGB32 ); //image->copy() //copy original data

    GammaAndLevels *levels = new GammaAndLevels(this, mImage);
    levels->setAttribute(Qt::WA_DeleteOnClose);
    //when closing levels, inform the Tone Mapping dialog.
    connect(levels,SIGNAL(closing()), this, SIGNAL(levels_closed()));
    //refresh preview when a values changes
    connect(levels,SIGNAL(LUTrefreshed(unsigned char *)),this,SLOT(updatePreview(unsigned char *)));
    //accept the changes
    connect(levels,SIGNAL(accepted()),this,SLOT(finalize_levels()));
    //restore original on "cancel"
    connect(levels,SIGNAL(rejected()),this,SLOT(restore_original()));

    levels->exec();
}

void LdrViewer::updatePreview(unsigned char *LUT)
{
    const QRgb* src = (const QRgb*)mImage->bits();
    QRgb* dst = (QRgb*)previewimage->bits();

#ifdef _OPENMP
#pragma omp parallel for default(none) shared(src, dst, LUT)
#endif
    //printf("LdrViewer::updatePreview\n");
    // TODO : clean up this implementation... (in case I keep it in the final release)!!!
    for (int i=0; i < mCols*mRows; i++)
    {
	dst[i] = qRgb(LUT[qRed(src[i])],LUT[qGreen(src[i])],LUT[qBlue(src[i])]);
    }
    mPixmap->setPixmap(QPixmap::fromImage(*previewimage));
    //imageLabel.setPixmap(QPixmap::fromImage(*previewimage));
}

void LdrViewer::restore_original()
{
    //printf("LdrViewer::restoreoriginal() \n");
    mPixmap->setPixmap(QPixmap::fromImage(*mImage));

    delete previewimage;

    temp_image = NULL;
    previewimage = NULL;
}

void LdrViewer::finalize_levels()
{
    //printf("LdrViewer::finalize_levels() \n");
    mImage = previewimage;
    mPixmap->setPixmap(QPixmap::fromImage(*mImage));

    delete temp_image;

    temp_image = NULL;
    previewimage = NULL;
}

void LdrViewer::setImage(QImage *i)
{
    if (mImage != NULL)
        delete mImage;
    if (informativeLabel != NULL)
        delete informativeLabel;

    mImage = new QImage(*i);
    mPixmap->setPixmap(QPixmap::fromImage(*mImage));

    mCols = mImage->width();
    mRows = mImage->height();
    informativeLabel = new QLabel( tr("LDR image [%1 x %2]").arg(mCols).arg(mRows), mToolBar );
    mToolBar->addWidget(informativeLabel);
}
