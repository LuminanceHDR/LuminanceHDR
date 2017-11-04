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
 * @author Giuseppe Rota <grota@users.sourceforge.net>
 */

#include "Resize/ResizeDialog.h"
#include "Resize/ui_ResizeDialog.h"

#include "Libpfs/frame.h"
#include "Libpfs/manip/resize.h"

ResizeDialog::ResizeDialog(QWidget *parent, pfs::Frame *orig)
    : QDialog(parent),
      m_original(orig),
      m_resized(NULL),
      m_Ui(new Ui::ResizeDialog) {
    m_Ui->setupUi(this);

    orig_width = m_original->getWidth();
    orig_height = m_original->getHeight();
    resized_width = orig_width;
    resized_height = orig_height;

    m_Ui->widthSpinBox->setSuffix(QLatin1String(""));
    m_Ui->widthSpinBox->setDecimals(0);
    m_Ui->widthSpinBox->setMaximum(2 * orig_width);
    m_Ui->widthSpinBox->setMinimum(1);

    m_Ui->heightSpinBox->setSuffix(QLatin1String(""));
    m_Ui->heightSpinBox->setDecimals(0);
    m_Ui->heightSpinBox->setMaximum(2 * orig_height);
    m_Ui->heightSpinBox->setMinimum(1);
    // we are now in pixel mode, put directly original pixel values.
    m_Ui->widthSpinBox->setValue(orig_width);
    m_Ui->heightSpinBox->setValue(orig_height);

    from_other_spinbox = false;
    updatelabel();

    connect(m_Ui->scaleButton, &QAbstractButton::clicked, this,
            &ResizeDialog::scaledPressed);
    connect(m_Ui->widthSpinBox, &QAbstractSpinBox::editingFinished, this,
            &ResizeDialog::update_heightSpinBox);
    connect(m_Ui->widthSpinBox, SIGNAL(valueChanged(double)), this,
            SLOT(update_heightSpinBox()));
    connect(m_Ui->heightSpinBox, &QAbstractSpinBox::editingFinished, this,
            &ResizeDialog::update_widthSpinBox);
    connect(m_Ui->heightSpinBox, SIGNAL(valueChanged(double)), this,
            SLOT(update_widthSpinBox()));
    connect(m_Ui->px_or_percentage, SIGNAL(activated(int)), this,
            SLOT(switch_px_percentage(int)));
    connect(m_Ui->restoredefault, &QAbstractButton::clicked, this,
            &ResizeDialog::defaultpressed);
}

ResizeDialog::~ResizeDialog() {
    // we don't delete *original, because in maingui_impl.cpp we will later call
    // mdiwin->updateHDR which takes care of deleting its previous pfs::Frame*
    // buffer.
}

pfs::Frame *ResizeDialog::getResizedFrame() { return m_resized; }

void ResizeDialog::scaledPressed() {
    if (orig_width == resized_width) {
        emit reject();
        return;
    }
    m_resized = pfs::resize(m_original, resized_width, BilinearInterp);
    accept();
}

void ResizeDialog::switch_px_percentage(int px_per) {
    switch (px_per) {
        case 0:
            m_Ui->widthSpinBox->setMaximum(2 * orig_width);
            m_Ui->heightSpinBox->setMaximum(2 * orig_height);
            from_other_spinbox = true;
            m_Ui->widthSpinBox->setValue((int)(m_Ui->widthSpinBox->value() *
                                               (float)orig_width /
                                               100.0));  // from perc to px
            from_other_spinbox = true;
            m_Ui->heightSpinBox->setValue((int)(m_Ui->heightSpinBox->value() *
                                                (float)orig_height /
                                                100.0));  // from perc to px
            m_Ui->widthSpinBox->setSuffix(QLatin1String(""));
            m_Ui->widthSpinBox->setDecimals(0);
            m_Ui->widthSpinBox->setMinimum(1);
            m_Ui->heightSpinBox->setSuffix(QLatin1String(""));
            m_Ui->heightSpinBox->setDecimals(0);
            m_Ui->heightSpinBox->setMinimum(1);
            break;
        case 1:
            m_Ui->widthSpinBox->setDecimals(2);
            m_Ui->heightSpinBox->setDecimals(2);
            from_other_spinbox = true;
            m_Ui->widthSpinBox->setValue(100 * m_Ui->widthSpinBox->value() /
                                         (float)orig_width);  // from px to perc
            from_other_spinbox = true;
            m_Ui->heightSpinBox->setValue(
                100 * m_Ui->heightSpinBox->value() /
                (float)orig_height);  // from px to perc
            m_Ui->widthSpinBox->setSuffix(QStringLiteral("%"));
            m_Ui->widthSpinBox->setMaximum(200);
            m_Ui->widthSpinBox->setMinimum(1);
            m_Ui->heightSpinBox->setSuffix(QStringLiteral("%"));
            m_Ui->heightSpinBox->setMaximum(200);
            m_Ui->heightSpinBox->setMinimum(1);
            break;
    }
    from_other_spinbox = false;
    updatelabel();
}
// get a proper resized_width from a resized_height
int ResizeDialog::rw_from_rh() {
    return (int)((float)orig_width * (float)resized_height /
                 (float)orig_height);
}
// get a proper resized_height from a resized_width
int ResizeDialog::rh_from_rw() {
    return (int)((float)orig_height * (float)resized_width / (float)orig_width);
}
void ResizeDialog::update_heightSpinBox() {
    if (from_other_spinbox) {
        from_other_spinbox = false;
        return;
    }
    switch (m_Ui->px_or_percentage->currentIndex()) {
        case 0:
            resized_width = (int)m_Ui->widthSpinBox->value();
            resized_height = rh_from_rw();
            from_other_spinbox = true;
            // update directly resized_height
            m_Ui->heightSpinBox->setValue(resized_height);
            break;
        case 1:
            resized_width =
                (int)(orig_width * m_Ui->widthSpinBox->value() / 100.0);
            resized_height = rh_from_rw();
            from_other_spinbox = true;
            m_Ui->heightSpinBox->setValue((double)resized_height /
                                          (double)orig_height * 100.0);
            break;
    }
    updatelabel();
}
void ResizeDialog::update_widthSpinBox() {
    if (from_other_spinbox) {
        from_other_spinbox = false;
        return;
    }
    switch (m_Ui->px_or_percentage->currentIndex()) {
        case 0:
            resized_height = (int)m_Ui->heightSpinBox->value();
            resized_width = rw_from_rh();
            from_other_spinbox = true;
            // update directly resized_width
            m_Ui->widthSpinBox->setValue(resized_width);
            break;
        case 1:
            resized_height =
                (int)(orig_height * m_Ui->heightSpinBox->value() / 100.0);
            resized_width = rw_from_rh();
            from_other_spinbox = true;
            m_Ui->widthSpinBox->setValue((double)resized_width /
                                         (double)orig_width * 100.0);
            break;
    }
    updatelabel();
}
void ResizeDialog::updatelabel() {
    m_Ui->sizepreview->setText(
        QStringLiteral("%1x%2").arg(resized_width).arg(resized_height));
}

void ResizeDialog::defaultpressed() {
    resized_height = orig_height;
    resized_width = orig_width;
    switch (m_Ui->px_or_percentage->currentIndex()) {
        case 0:
            from_other_spinbox = true;
            m_Ui->widthSpinBox->setValue(resized_width);
            from_other_spinbox = true;
            m_Ui->heightSpinBox->setValue(resized_height);
            from_other_spinbox = false;
            break;
        case 1:
            from_other_spinbox = true;
            m_Ui->widthSpinBox->setValue(100);
            from_other_spinbox = true;
            m_Ui->heightSpinBox->setValue(100);
            from_other_spinbox = false;
            break;
    }
    updatelabel();
}
