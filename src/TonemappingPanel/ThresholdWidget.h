/**
 * This file is a part of Luminance HDR package.
 * ----------------------------------------------------------------------
 * Copyright (C) 2011 Franco Comida
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
 * @author Franco Comida <fcomida@users.sourceforge.net>
 *
 */

#ifndef THRESHOLDWIDGET_H
#define THRESHOLDWIDGET_H

#include <QFrame>
#include <QHideEvent>
#include <QKeyEvent>

namespace Ui {
class ThresholdWidget;
}

class ThresholdWidget : public QFrame {
    Q_OBJECT

   public:
    ThresholdWidget(QWidget *parent = 0, Qt::WindowFlags flags = Qt::Popup);
    ~ThresholdWidget();
    float threshold() const;

   protected:
    void keyPressEvent(QKeyEvent *event);
    void hideEvent(QHideEvent *event);
    QScopedPointer<Ui::ThresholdWidget> m_Ui;

   Q_SIGNALS:
    void ready();

   protected Q_SLOTS:
    void on_thresholdDoubleSpinBox_valueChanged(double value);
    void on_thresholdHorizontalSlider_valueChanged(int pos);
};

#endif
