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

#include <QFileDialog>
#include <QImage>
#include <QMessageBox>
#include <QUrl>
#include <iostream>

#include "Common/config.h"
#include "Common/LuminanceOptions.h"
#include "Common/global.h"
#include "Common/ImageQualityDialog.h"

/**
 * \return "" when fail, out file name when successful
 */
QString saveLDRImage(QWidget *parent, const QString initialFileName, const QImage *image, bool batchMode)
{
    LuminanceOptions luminance_options;

    QString outfname = QDir(luminance_options.getDefaultPathLdrOut()).filePath(initialFileName);
    if (!batchMode)
    {
        QString filetypes = QObject::tr("All LDR formats") + " (*.jpg *.jpeg *.png *.ppm *.pbm *.bmp *.JPG *.JPEG *.PNG *.PPM *.PBM *.BMP);;";
        filetypes += "JPEG (*.jpg *.jpeg *.JPG *.JPEG);;" ;
        filetypes += "PNG (*.png *.PNG);;" ;
        filetypes += "PPM PBM (*.ppm *.pbm *.PPM *.PBM);;";
        filetypes += "BMP (*.bmp *.BMP)";

        outfname = QFileDialog::getSaveFileName(parent,
                                                QObject::tr("Save the LDR image as..."),
                                                QDir(luminance_options.getDefaultPathLdrOut()).filePath(initialFileName),
                                                filetypes);
    }

    if( !outfname.isEmpty() )
    {
        QFileInfo qfi(outfname);
        luminance_options.setDefaultPathLdrOut(qfi.path()); //save settings
        QString format = qfi.suffix();

        if ( qfi.suffix().isEmpty() )
        {
            // default as png
            format    =   "png";
            outfname  +=  ".png";
        }
        int quality = 100;
        if ((format == "png" || format == "jpg") && !batchMode)
        {
            ImageQualityDialog savedFileQuality(image, format, parent);
            QString winTitle(QObject::tr("Save as..."));
            winTitle += format.toUpper();
            savedFileQuality.setWindowTitle( winTitle );
            if ( savedFileQuality.exec() == QDialog::Rejected )
            {
                return "";
            }
            quality = savedFileQuality.getQuality();
        }
        //std::cout << quality << std::endl;
        if( !(image->save(outfname, format.toLocal8Bit(), quality)) )
        {
            //std::cout << "Failed to save" << std::endl;
            QMessageBox::warning(0,"",QObject::tr("Failed to save <b>") + outfname + "</b>", QMessageBox::Ok, QMessageBox::NoButton);
            return "";
        }
    } // if(!outfname.isEmpty())
    return outfname;
}

bool matchesLdrFilename(QString file)
{
	QRegExp exp(".*\\.(jpeg|jpg|tiff|tif|crw|cr2|nef|dng|mrw|orf|kdc|dcr|arw|raf|ptx|pef|x3f|raw|sr2|rw2)$", Qt::CaseInsensitive);
	return exp.exactMatch(file);
}

bool matchesHdrFilename(QString file)
{
	QRegExp exp(".*\\.(exr|hdr|pic|tiff|tif|pfs|crw|cr2|nef|dng|mrw|orf|kdc|dcr|arw|raf|ptx|pef|x3f|raw|sr2|rw2)$", Qt::CaseInsensitive);
	return exp.exactMatch(file);
}

bool matchesValidHDRorLDRfilename(QString file)
{
	return matchesLdrFilename(file) || matchesHdrFilename(file);
}

QStringList convertUrlListToFilenameList(QList<QUrl> urls)
{
    QStringList files;
    for (int i = 0; i < urls.size(); ++i)
    {
        QString localFile = urls.at(i).toLocalFile();
        if (matchesValidHDRorLDRfilename(localFile))
        {
            files.append(localFile);
        }
    }
    return files;
}
