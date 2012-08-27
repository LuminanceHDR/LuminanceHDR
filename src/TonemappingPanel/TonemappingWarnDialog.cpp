/*
 * This file is a part of Luminance HDR package.
 * ----------------------------------------------------------------------
 * Copyright (C) 2010 Elizabeth Oldham
 * Copyright (C) 2012 Davide Anastasia
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
 * @author Elizabeth Oldham <bethatthehug@users.sourceforge.net>
 * @author Davide Anastasia <davideanastasia@users.sourceforge.net>
 *  Refactoring
 *
 */

#include <QDebug>

#include "TonemappingWarnDialog.h"
#include "Common/LuminanceOptions.h"

TonemappingWarningDialog::TonemappingWarningDialog(QWidget *p):
    UMessageBox(p)
{
    this->setText( tr("Fattal Warning") );
    this->setInformativeText( tr("This tonemapping operator depends on the size of the input "\
                                 " image. Applying this operator on the full size image will "\
                                 "most probably result in a different image. "\
                                 "\n\nDo you want to continue?") );
    this->setStandardButtons(QMessageBox::Yes | QMessageBox::YesToAll | QMessageBox::No);
    this->setDefaultButton(QMessageBox::No);
    this->setIcon(QMessageBox::Warning);
}

TonemappingWarningDialog::~TonemappingWarningDialog()
{}

