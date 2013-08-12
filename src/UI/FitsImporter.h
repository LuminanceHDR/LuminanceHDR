/**
 * This file is a part of LuminanceHDR package.
 * ----------------------------------------------------------------------
 * Copyright (C) 2013 Franco Comida
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
 */

#ifndef FITSIMPORTER_H
#define FITSIMPORTER_H

#include <QDialog>
#include <QLineEdit>
#include <QProcess>
#include <QFutureWatcher>

#include "PreviewFrame.h"
#include "Common/LuminanceOptions.h"
#include "HdrWizard/HdrCreationItem.h"

namespace Ui {
class FitsImporter;
}

class FitsImporter : public QDialog 
{
    Q_OBJECT

public:
    FitsImporter(QWidget *parent = 0);
    ~FitsImporter();

    pfs::Frame *getFrame() { return m_frame; }
    
protected slots:
    void on_pushButtonLuminosity_clicked();
    void on_pushButtonRed_clicked();
    void on_pushButtonGreen_clicked();
    void on_pushButtonBlue_clicked();
    void on_pushButtonH_clicked();
    void on_pushButtonOK_clicked();
    void on_pushButtonLoad_clicked();
    void on_pushButtonClockwise_clicked();
    void on_pushButtonPreview_clicked();
    void loadFilesDone();
    void ais_finished(int,QProcess::ExitStatus);
    void alignedFilesLoaded();
    void ais_failed_slot(QProcess::ProcessError);
    void readData();
    void previewLabelSelected(int index);

signals:
    void setValue(int);
    void setRange(int, int);

protected:
    void checkLoadButton();
    bool framesHaveSameSize();
    void buildFrame();
    void align_with_ais();
    void align_with_mtb();

    pfs::Frame *m_frame;
    PreviewFrame *m_previewFrame;
    QLabel* m_previewLabel;

    LuminanceOptions m_luminance_options;

    QStringList m_channels;
    QString m_luminosityChannel;
    QString m_redChannel;
    QString m_greenChannel;
    QString m_blueChannel;
    QString m_hChannel;

    HdrCreationItemContainer m_data;
    HdrCreationItemContainer m_tmpdata;

    QFutureWatcher<void> m_futureWatcher;

    // align_image_stack
	QProcess *m_ais;

    QScopedPointer<Ui::FitsImporter> m_ui;

private:
    void selectInputFile(QLineEdit* textField, QString* channel);
};

#endif 
