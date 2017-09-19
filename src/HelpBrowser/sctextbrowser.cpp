/**
 * This file is a part of LuminanceHDR package.
 * ----------------------------------------------------------------------
 * Copyright (C) 2001â€“2013 Franz Schmid and rest of the members of the
 *  Scribus Team.
 *  The "Scribus Team" is informally defined as the following people: Franz
 *  Schmid, Peter Linnell, Craig Bradney, Jean Ghali, Hermann Kraus, Riku
 *  Leino, Oleksandr M oskalenko, Christoph SchÃ¤fer, Petr VanÄ›k, Andreas
 *  Vox, and Jain Basil.
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
 *  along with this program; if not, see <http://www.gnu.org/licenses/>
 *
 *  In addition, as a special exception, the listed copyright holders above
 *  give permission to link the code of this program with the Trolltech AS
 *  Qt4 Commercially Licensed Dynamic Link Libraries  (or with modified
 *  versions of Trolltech AS Qt3 Commercially Licensed Dynamic Link
 *  Libraries that use the same license as Trolltech AS Qt4 Commercially
 *  Licensed Dynamic Link Libraries), and distribute linked combinations
 *  including the two. You must obey the GNU General Public  License in all
 *  respects for all of the code used other than Trolltech AS Qt3
 *  Commercially Licensed Dynamic Link Libraries.  If you modify this file,
 *  you may extend this exception to your version of the file, but you are
 *  not obligated to do so. If you do not wish to do so, delete this
 *  exception statement from your version.
 *
 */

/*
For general Scribus (>=1.3.2) copyright and licensing information please refer
to the COPYING file provided with the program. Following this notice may exist
a copyright and/or license notice that predates the release of Scribus 1.3.2
for which a new license (GPL+exception) is in place.
*/

#include <QApplication>
#include <QCursor>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QProcess>
#include <QString>
#include <QStringList>
#include <QToolTip>
#include <QUrl>

#include "sctextbrowser.h"

ScTextBrowser::ScTextBrowser(QWidget *parent) : QWebEngineView(parent) {}

void ScTextBrowser::home() {
    if (m_home.isValid()) load(m_home);
}
