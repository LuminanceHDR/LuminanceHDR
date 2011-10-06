/**
 * This file is a part of Luminance HDR package.
 * ----------------------------------------------------------------------
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
 * This file provides an unified style for all the QMessageBox in Luminance HDR
 *
 * Original Work
 * @author Davide Anastasia <davideanastasia@users.sourceforge.net>
 *
 *
 */

#include "UI/UMessageBox.h"

#define UMESSAGEBOX_WIDTH 450 // pixel

void UMessageBox::init()
{
    m_horizontalSpacer = new QSpacerItem(UMESSAGEBOX_WIDTH, 0, QSizePolicy::Minimum, QSizePolicy::Expanding);
    m_layout = (QGridLayout*)this->layout();
    m_layout->addItem(m_horizontalSpacer, m_layout->rowCount(), 0, 1, m_layout->columnCount());

    this->setWindowTitle("Luminance HDR "LUMINANCEVERSION);
#ifdef Q_WS_MAC
    this->setWindowModality(Qt::WindowModal); // In OS X, the QMessageBox is modal to the window
#endif
}

UMessageBox::UMessageBox(QWidget *parent) : QMessageBox(parent)
{
    init();
}

UMessageBox::UMessageBox(const QString &title, const QString &text, Icon icon,
             int button0, int button1, int button2,
             QWidget *parent,
             Qt::WindowFlags f) :
QMessageBox(title, text, icon, button0, button1, button2, parent, f)
{
    init();
}

void UMessageBox::showEvent(QShowEvent *event)
{
  QMessageBox::showEvent(event);
}
