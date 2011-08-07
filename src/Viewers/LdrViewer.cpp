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

#include "Common/config.h"
#include "Common/GammaAndLevels.h"
#include "LdrViewer.h"

LdrViewer::LdrViewer(QImage *i, QWidget *parent, bool ns, bool ncf, const TonemappingOptions *opts) :
        GenericViewer(parent, ns, ncf), informativeLabel(NULL)
{
    setAttribute(Qt::WA_DeleteOnClose);

    m_image = i;

    if (m_image == 0) {
		m_cols = 0;
		m_rows = 0;
    }
    else {
    	m_cols = m_image->width();
    	m_rows = m_image->height();
    }

    temp_image = NULL;
    previewimage = NULL;

    informativeLabel = new QLabel( tr("LDR image [%1 x %2]").arg(m_cols).arg(m_rows), toolBar );
    toolBar->addWidget(informativeLabel);

    if (m_image == 0)
	return;

    imageLabel.setPixmap( QPixmap::fromImage(*m_image) );

    parseOptions(opts);
    setWindowTitle(caption);
    setToolTip(caption);
}

LdrViewer::~LdrViewer()
{
    std::cout << "LdrViewer::~LdrViewer()" << std::endl;

    //delete origimage;
    //delete [] origimage.bits();
}

void LdrViewer::parseOptions(const TonemappingOptions *opts)
{
    TMOptionsOperations tmopts(opts);
    postfix=tmopts.getPostfix();
    caption=tmopts.getCaption();
    exif_comment=tmopts.getExifComment();
}

QString LdrViewer::getFilenamePostFix()
{
    return postfix;
}

const QImage* LdrViewer::getQImage()
{
    return m_image;
}

QString LdrViewer::getExifComment()
{
    return exif_comment;
}

void LdrViewer::levelsRequested(bool /*a*/)
{
    // TODO : Check this rubbish!

    temp_image = m_image;
    previewimage = new QImage( m_cols, m_rows, QImage::Format_RGB32 ); //image->copy() //copy original data

    GammaAndLevels *levels = new GammaAndLevels(this, m_image);
    levels->setAttribute(Qt::WA_DeleteOnClose);
    //when closing levels, inform the Tone Mapping dialog.
    connect(levels,SIGNAL(closing()), this, SIGNAL(levels_closed()));
    //refresh preview when a values changes
    connect(levels,SIGNAL(LUTrefreshed(unsigned char *)),this,SLOT(updatePreview(unsigned char *)));
    //restore original on "cancel"
    connect(levels,SIGNAL(rejected()),this,SLOT(restore_original()));

    levels->exec();
}

void LdrViewer::updatePreview(unsigned char *LUT)
{
    //printf("LdrViewer::updatePreview\n");
    // TODO : clean up this implementation... (in case I keep it in the final release)!!!
    for (int x=0; x < m_cols; x++)
    {
        for (int y=0; y < m_rows; y++)
        {
            QRgb rgb = m_image->pixel(x,y);
            QRgb withgamma = qRgb(LUT[qRed(rgb)],LUT[qGreen(rgb)],LUT[qBlue(rgb)]);
            previewimage->setPixel(x,y,withgamma);
        }
    }
    imageLabel.setPixmap(QPixmap::fromImage(*previewimage));
}

void LdrViewer::restore_original()
{
    //printf("LdrViewer::restoreoriginal() \n");
    imageLabel.setPixmap( QPixmap::fromImage(*m_image) );

    delete previewimage;

    temp_image = NULL;
    previewimage = NULL;
}

void LdrViewer::finalize_levels()
{
    //printf("LdrViewer::finalize_levels() \n");
    m_image = previewimage;
    imageLabel.setPixmap( QPixmap::fromImage(*m_image) );

    delete temp_image;

    temp_image = NULL;
    previewimage = NULL;
}

void LdrViewer::setImage(QImage *i)
{
    if (m_image != NULL)
        delete m_image;
    if (informativeLabel != NULL)
		delete informativeLabel;
    m_image = new QImage(*i);
    imageLabel.setPixmap( QPixmap::fromImage(*m_image) );
    imageLabel.adjustSize();
    m_cols = m_image->width();
    m_rows = m_image->height();
    informativeLabel = new QLabel( tr("LDR image [%1 x %2]").arg(m_cols).arg(m_rows), toolBar );
    toolBar->addWidget(informativeLabel);
}
