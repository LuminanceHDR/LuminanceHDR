/**
 * This file is a part of LuminanceHDR package.
 * ----------------------------------------------------------------------
 * Copyright (C) 2010 Elizabeth Oldham
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
 */

#include <QFileDialog>
#include <QWhatsThis>
#include <QMessageBox>

#include "Common/config.h"
#include "TonemappingWarnDialog.h"

TonemappingWarningDialog::TonemappingWarningDialog(QWidget *p) : QDialog(p)
{
    setupUi(this);

    plainText->setPlainText(tr("This tonemapping operator depends on the size of the input image. Applying this operator on the full size image will most probably result in a different image.\n\nDo you want to continue?"));

    checkBoxAskAgain->setChecked(luminance_options.isShowFattalWarning());

    connect(buttonBox,SIGNAL(accepted()),this,SLOT(accepted()));
    yes=false;
}

void TonemappingWarningDialog::accepted()
{
    luminance_options.setShowFattalWarning( checkBoxAskAgain->isChecked() );

    accept();

    yes = true;
}

bool TonemappingWarningDialog::wasAccepted()
{
    return (yes);
}


TonemappingWarningDialog::~TonemappingWarningDialog()
{
}
