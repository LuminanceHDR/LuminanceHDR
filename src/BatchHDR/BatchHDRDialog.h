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
 * Original Work
 * @author Franco Comida <fcomida@users.sourceforge.net>
 *
 */

#ifndef BATCH_HDR_IMPL_H
#define BATCH_HDR_IMPL_H

#include <QDialog>
#include <QFuture>
#include <QFutureWatcher>

#include "Common/LuminanceOptions.h"
#include "Common/ProgressHelper.h"
#include "HdrWizard/HdrCreationManager.h"
#include "LibpfsAdditions/formathelper.h"

// Forward declaration
class IOWorker;
class HdrCreationManager;

namespace Ui {
class BatchHDRDialog;
}

class BatchHDRDialog : public QDialog {
    Q_OBJECT
   private:
    QScopedPointer<Ui::BatchHDRDialog> m_Ui;

   public:
    BatchHDRDialog(QWidget *parent = 0);
    ~BatchHDRDialog();

   signals:
    void setValue(int);

   protected slots:
    void num_bracketed_changed(int);
    void on_selectInputFolder_clicked();
    void on_selectOutputFolder_clicked();
    void add_output_directory(QString dir = QString());
    void on_startButton_clicked();
    void batch_hdr();
    void align();
    void create_hdr(int);
    void error_while_loading(const QString &);
    void writeAisData(QByteArray &);
    void check_start_button();
    void on_cancelButton_clicked();
    void align_selection_clicked();
    void processed();
    void try_to_continue();
    void updateThresholdSlider(int);
    void updateThresholdSpinBox(double);
    void ais_failed(QProcess::ProcessError);
    void createHdrFinished();
    void loadFilesAborted();

   protected:
    // Application-wide settings, loaded via QSettings
    QString m_batchHdrInputDir;
    QString m_batchHdrOutputDir;
    QString m_tempDir;

    QStringList m_bracketed;
    QString m_output_file_name_base;
    IOWorker *m_IO_Worker;
    HdrCreationManager *m_hdrCreationManager;
    int m_numProcessed;
    int m_processed;
    int m_total;
    bool m_errors;
    bool m_loading_error;
    bool m_abort;
    bool m_processing;
    QVector<FusionOperatorConfig> m_customConfig;
    QFutureWatcher<void> m_futureWatcher;
    QFuture<pfs::Frame *> m_future;
    ProgressHelper m_ph;
    bool m_patches[agGridSize][agGridSize];
    pfsadditions::FormatHelper m_formatHelper;
};
#endif
