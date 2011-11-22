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

#ifndef IMAGELDRVIEWER_H
#define IMAGELDRVIEWER_H

#include <QLabel>

#include "GenericViewer.h"

// Forward declaration
class TonemappingOptions;       // #include "Core/TonemappingOptions.h"

class LdrViewer: public GenericViewer
{
    Q_OBJECT
public:
    LdrViewer(pfs::Frame* frame, const TonemappingOptions* opts = 0, QWidget *parent = 0, bool ns = 0);
    virtual ~LdrViewer();
    void levelsRequested(bool);
    QString getFileNamePostFix();
    QString getExifComment();

    //! \brief virtual function
    //! \return always return FALSE
    bool isHDR();
    //void setImage(QImage *i);
    //const quint16 * getPixmap();

signals:
    void levels_closed();

protected Q_SLOTS:
    virtual void updatePixmap();

private Q_SLOTS:
    void updatePreview(unsigned char *);
    void restore_original();
    void finalize_levels();

private:
    void parseOptions(const TonemappingOptions *opts);
    QString caption; // ,postfix,exif_comment;
    QLabel *informativeLabel;
    QImage *previewimage;
    QImage *temp_image;

    const TonemappingOptions* mTonemappingOptions;
};

inline bool LdrViewer::isHDR()
{
    return false;
}
#endif
