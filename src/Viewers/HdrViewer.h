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

#include <QImage>
#include <QComboBox>
#include <QLabel>
#include <QScopedPointer>

#include "GenericViewer.h"
#include "Common/FloatRgbToQRgb.h"

#include <iostream>

// Forward declaration
namespace pfs {
    class Array2D;
    class Frame;
}

class LuminanceRangeWidget;     // #include "LuminanceRangeWidget.h"
// class HdrViewerMapping;

class HdrViewer : public GenericViewer
{
    Q_OBJECT
private:
    void init_ui();

public:
    HdrViewer(pfs::Frame* frame, QWidget *parent = 0, bool ns = false, unsigned int negcol = 0, unsigned int naninfcol = 0);
    virtual ~HdrViewer();

    LuminanceRangeWidget* lumRange();
    void update_colors(int negcol, int naninfcol);

    QString getFileNamePostFix();
    QString getExifComment();

    //! \brief virtual function
    //! \return always return TRUE
    bool isHDR();

    //! \brief returns max value of the handled frame
    float getMaxLuminanceValue();

    //! \brief returns min value of the handled frame
    float getMinLuminanceValue();

    LumMappingMethod getLuminanceMappingMethod();

public Q_SLOTS:
    void updateRangeWindow();
    int getLumMappingMethod();
    void setLumMappingMethod( int method );

protected Q_SLOTS:
    virtual void updatePixmap();

protected:
    // Methods
	virtual void retranslateUi();
    void setRangeWindow(float min, float max);

    // UI
    LuminanceRangeWidget *m_lumRange;
    QComboBox *mappingMethodCB;
    QLabel *mappingMethodLabel;
    QLabel *histlabel;

private:
    void refreshPixmap();

    LumMappingMethod m_MappingMethod;
    float m_MinValue;
    float m_MaxValue;

    //! \brief NaN or Inf color
    int m_NanInfColor;
    //! \brief Neg color
    int m_NegColor;

    QImage mapFrameToImage(pfs::Frame* in_frame);
};

inline bool HdrViewer::isHDR()
{
    return true;
}

#endif
