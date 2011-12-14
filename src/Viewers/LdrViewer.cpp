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
#include <QScopedPointer>

#include "Viewers/LdrViewer.h"
#include "Viewers/IGraphicsPixmapItem.h"
#include "Core/TonemappingOptions.h"
#include "Libpfs/frame.h"
#include "Fileformat/pfsoutldrimage.h"

namespace
{
void parseOptions(const TonemappingOptions *opts, QString& caption)
{
    TMOptionsOperations tmopts(opts);
    //postfix = tmopts.getPostfix();
    caption = tmopts.getCaption();
    //exif_comment = tmopts.getExifComment();
}
}


LdrViewer::LdrViewer(pfs::Frame* frame, TonemappingOptions* opts, QWidget *parent, bool ns):
    GenericViewer(frame, parent, ns),
    mTonemappingOptions(opts),
    informativeLabel(new QLabel( tr("LDR image [%1 x %2]").arg(getWidth()).arg(getHeight()), mToolBar))
{
    mToolBar->addWidget(informativeLabel);

    mPixmap->disableSelectionTool(); // disable by default crop functionalities

    parseOptions(opts, caption);
    setWindowTitle(caption);
    setToolTip(caption);

    QScopedPointer<QImage> temp_qimage(fromLDRPFStoQImage(getFrame()));

    //mPixmap->setPixmap(QPixmap::fromImage(*temp_qimage));
    setQImage(*temp_qimage);

    updateView();
}

LdrViewer::~LdrViewer()
{
#ifdef QT_DEBUG
    std::cout << "LdrViewer::~LdrViewer()" << std::endl;
#endif
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

void LdrViewer::updatePixmap()
{
#ifdef QT_DEBUG
    qDebug() << "void LdrViewer::updatePixmap()";
#endif

    QScopedPointer<QImage> temp_qimage(fromLDRPFStoQImage(getFrame()));

    mPixmap->setPixmap(QPixmap::fromImage(*temp_qimage));
}

void LdrViewer::setTonemappingOptions(TonemappingOptions* tmopts)
{
    mTonemappingOptions = tmopts;
}

TonemappingOptions* LdrViewer::getTonemappingOptions()
{
    return mTonemappingOptions;
}

//! \brief returns max value of the handled frame
float LdrViewer::getMaxLuminanceValue()
{
    return 1.0f;
}

//! \brief returns min value of the handled frame
float LdrViewer::getMinLuminanceValue()
{
    return 0.0f;
}
