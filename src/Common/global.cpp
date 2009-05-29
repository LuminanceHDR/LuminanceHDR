/**
 * This file is a part of Qtpfsgui package.
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

#include <iostream>
#include <QFileDialog>
#include <QImage>
#include <QMessageBox>
#include <QUrl>
#include "config.h"
#include "options.h"
#include "global.h"
#include "imageQualityDialog.h"

QSettings settings("Qtpfsgui", "Qtpfsgui");

/**
 * \return "" when fail, out file name when successful
 */
QString saveLDRImage(const QString initialFileName, const QImage* image, bool batchMode) {
	QString outfname = QDir(settings.value(KEY_RECENT_PATH_SAVE_LDR,QDir::currentPath()).toString()).filePath(initialFileName);
	if (!batchMode) {
		QString filetypes = QObject::tr("All LDR formats (*.jpg *.jpeg *.png *.ppm *.pbm *.bmp);;");
		filetypes += "JPEG (*.jpg *.jpeg);;" ;
		filetypes += "PNG (*.png);;" ;
		filetypes += "PPM PBM (*.ppm *.pbm);;";
		filetypes += "BMP (*.bmp)";

		outfname = QFileDialog::getSaveFileName(
				0,
				QObject::tr("Save the LDR to..."),
				QDir(settings.value(KEY_RECENT_PATH_SAVE_LDR,QDir::currentPath()).toString()).filePath(initialFileName),
				filetypes
		);
	}

	if(!outfname.isEmpty()) {
		QFileInfo qfi(outfname);
		//save settings
		settings.setValue(KEY_RECENT_PATH_SAVE_LDR, qfi.path());
		QString format=qfi.suffix();
		if (qfi.suffix().isEmpty()) {
			// default as png
			format="png";
			outfname+=".png";
		}
		int quality = 100;
		if (format == "png" || format == "jpg") {
 			ImageQualityDialog savedFileQuality((QImage *)image, format);
			QString winTitle(QObject::tr("Save as "));
			winTitle += format.toUpper();
			savedFileQuality.setWindowTitle( winTitle );
			savedFileQuality.exec();
			quality = savedFileQuality.getQuality();
		}
		//std::cout << quality << std::endl;		

		if(!(image->save(outfname,format.toAscii().constData(),quality))) {
			QMessageBox::warning(0,"",QObject::tr("Failed to save <b>") + outfname + "</b>", QMessageBox::Ok, QMessageBox::NoButton);
			return "";
		}
	} //if(!outfname.isEmpty())
	return outfname;
}

bool matchesLdrFilename(QString file) {
	QRegExp exp(".*\\.(jpeg|jpg|tiff|tif|crw|cr2|nef|dng|mrw|orf|kdc|dcr|arw|raf|ptx|pef|x3f|raw|sr2|rw2)$", Qt::CaseInsensitive);
	return exp.exactMatch(file);
}

bool matchesHdrFilename(QString file) {
	QRegExp exp(".*\\.(exr|hdr|pic|tiff|tif|pfs|crw|cr2|nef|dng|mrw|orf|kdc|dcr|arw|raf|ptx|pef|x3f|raw|sr2|rw2)$", Qt::CaseInsensitive);
	return exp.exactMatch(file);
}

bool matchesValidHDRorLDRfilename(QString file) {
	return matchesLdrFilename(file) || matchesHdrFilename(file);
}

QStringList convertUrlListToFilenameList(QList<QUrl> urls) {
	QStringList files;
	for (int i = 0; i < urls.size(); ++i) {
		QString localFile = urls.at(i).toLocalFile();
		if (matchesValidHDRorLDRfilename(localFile))
			files.append(localFile);
	}
	return files;
}
