/**
 * This file is a part of LuminanceHDR package.
 * ----------------------------------------------------------------------
 * Copyright (C) 2006,2007 Giuseppe Rota
 * Copyright (C) 2011 Davide Anastasia
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
 * Improvements, new functionalities
 * @author Davide Anastasia <davideanastasia@users.sourceforge.net>
 *
 */

#ifndef IMAGEHDRVIEWER_H
#define IMAGEHDRVIEWER_H

#include <QComboBox>
#include <QImage>
#include <QKeyEvent>
#include <QLabel>
#include <QScopedPointer>
#include <iostream>

#include "GenericViewer.h"

// Forward declaration
namespace pfs {
class Frame;
}

class LuminanceRangeWidget;

class HdrViewer : public GenericViewer {
    Q_OBJECT

   public:
    HdrViewer(pfs::Frame *frame, QWidget *parent = 0, bool ns = false);
    virtual ~HdrViewer();

    LuminanceRangeWidget *lumRange();

    QString getFileNamePostFix();
    QString getExifComment();

    //! \brief virtual function
    //! \return always return TRUE
    bool isHDR();

    //! \brief returns max value of the handled frame
    float getMaxLuminanceValue();

    //! \brief returns min value of the handled frame
    float getMinLuminanceValue();

    RGBMappingType getLuminanceMappingMethod();

   public Q_SLOTS:
    void updateRangeWindow();
    int getLumMappingMethod();
    void setLumMappingMethod(int method);

   protected Q_SLOTS:
    virtual void updatePixmap();

   protected:
    // Methods
    virtual void retranslateUi();
    void setRangeWindow(float min, float max);
    void keyPressEvent(QKeyEvent *event);

    // UI
    LuminanceRangeWidget *m_lumRange;
    QComboBox *m_mappingMethodCB;
    QLabel *m_mappingMethodLabel;
    QLabel *m_histLabel;

   private:
    void initUi();
    void refreshPixmap();

    RGBMappingType m_mappingMethod;
    float m_minValue;
    float m_maxValue;

    QImage *mapFrameToImage(pfs::Frame *in_frame);
};

inline bool HdrViewer::isHDR() { return true; }

#endif
