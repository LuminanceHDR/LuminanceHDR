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

#include "TonemappingPanel/ThresholdWidget.h"
#include "TonemappingPanel/ui_ThresholdWidget.h"

ThresholdWidget::ThresholdWidget(QWidget *parent, Qt::WindowFlags flags):
    QFrame(parent, flags),
    m_Ui(new Ui::ThresholdWidget)
{
    m_Ui->setupUi(this);
}

ThresholdWidget::~ThresholdWidget()
{
}

float ThresholdWidget::threshold() const
{
    return m_Ui->thresholdDoubleSpinBox->value();
}

void ThresholdWidget::on_thresholdDoubleSpinBox_valueChanged(double value)
{
    int maxv = m_Ui->thresholdHorizontalSlider->maximum();
    m_Ui->thresholdHorizontalSlider->setValue((int)(value*(maxv+1)));
}

void ThresholdWidget::on_thresholdHorizontalSlider_valueChanged(int pos)
{
    int maxv = m_Ui->thresholdHorizontalSlider->maximum();
    m_Ui->thresholdDoubleSpinBox->setValue( (double)pos/(maxv+1) );
}

void ThresholdWidget::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        emit ready();
    }
}
