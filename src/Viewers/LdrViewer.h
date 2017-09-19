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

#include <lcms2.h>

#include "GenericViewer.h"

// Forward declaration
class TonemappingOptions;  // #include "Core/TonemappingOptions.h"

class LdrViewer : public GenericViewer {
    Q_OBJECT
   public:
    LdrViewer(pfs::Frame *frame, TonemappingOptions *opts = 0,
              QWidget *parent = 0, bool ns = 0,
              const float devicePixelRatio = 1.0);
    virtual ~LdrViewer();
    QString getFileNamePostFix();
    QString getExifComment();

    //! \brief virtual function
    //! \return always return FALSE
    bool isHDR();

    //! \brief Set tonemap options that generated the handled frame
    //! \note Override default empty behavioru of GenericViewer
    virtual void setTonemappingOptions(TonemappingOptions *tmopts);

    //! \brief Get tonemap options that generated the handled frame
    //! \note Override default empty behavioru of GenericViewer
    virtual TonemappingOptions *getTonemappingOptions();

    //! \brief returns max value of the handled frame
    float getMaxLuminanceValue();

    //! \brief returns min value of the handled frame
    float getMinLuminanceValue();

    void doSoftProofing(bool);
    void undoSoftProofing();

   protected Q_SLOTS:
    virtual void updatePixmap();

   protected:
    virtual void retranslateUi();

   private:
    QString caption;  // ,postfix,exif_comment;
    QLabel *informativeLabel;

    TonemappingOptions *mTonemappingOptions;
};

inline bool LdrViewer::isHDR() { return false; }
#endif
