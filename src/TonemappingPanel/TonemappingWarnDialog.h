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

#ifndef TMOWARN_IMPL_H
#define TMOWARN_IMPL_H

#include <QDialog>

#include "Common/LuminanceOptions.h"
#include "Common/global.h"

namespace Ui {
    class TonemappingWarningDialog;
}

class TonemappingWarningDialog : public QDialog
{
Q_OBJECT
public:
	TonemappingWarningDialog(QWidget *p);
	bool wasAccepted();
	~TonemappingWarningDialog();
private:
    LuminanceOptions luminance_options;
	bool yes;
	
    QScopedPointer<Ui::TonemappingWarningDialog> m_Ui;
private slots:
	void accepted();
};

#endif
