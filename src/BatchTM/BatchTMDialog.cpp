/**
 * This file is a part of LuminanceHDR package.
 * ----------------------------------------------------------------------
 * Copyright (C) 2007 Giuseppe Rota
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
 * @author Giuseppe Rota <grota@users.sourceforge.net>
 * Improvements, bugfixing
 * @author Franco Comida <fcomida@users.sourceforge.net>
 * Improvements, bugfixing
 * @author Davide Anastasia <davideanastasia@users.sourceforge.net>
 *
 */

#include <cassert>
#include <climits>

#include <QFileDialog>
#include <QMessageBox>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QTextStream>

#include <BatchTM/BatchTMDialog.h>
#include <BatchTM/ui_BatchTMDialog.h>

#include <BatchTM/BatchTMJob.h>
#include <Common/SavedParametersDialog.h>
#include <Common/config.h>
#include <Core/TonemappingOptions.h>
#include <Exif/ExifOperations.h>
#include <OsIntegration/osintegration.h>

BatchTMDialog::BatchTMDialog(QWidget *p, QSqlDatabase db)
    : QDialog(p), m_Ui(new Ui::BatchTMDialog), m_abort(false), m_db(db) {
    m_Ui->setupUi(this);

    if (!QIcon::hasThemeIcon(QStringLiteral("vcs-added")))
        m_Ui->from_Database_Button->setIcon(QIcon(":/program-icons/vcs-added"));

    m_max_num_threads = LuminanceOptions().getBatchTmNumThreads();

    connect(m_Ui->add_dir_HDRs_Button, &QAbstractButton::clicked, this,
            &BatchTMDialog::add_dir_HDRs);
    connect(m_Ui->add_HDRs_Button, &QAbstractButton::clicked, this,
            &BatchTMDialog::add_HDRs);
    connect(m_Ui->add_dir_TMopts_Button, &QAbstractButton::clicked, this,
            &BatchTMDialog::add_dir_TMopts);
    connect(m_Ui->add_TMopts_Button, &QAbstractButton::clicked, this,
            &BatchTMDialog::add_TMopts);
    connect(m_Ui->out_folder_Button, &QAbstractButton::clicked, this,
            &BatchTMDialog::out_folder_clicked);
    connect(m_Ui->remove_HDRs_Button, &QAbstractButton::clicked, this,
            &BatchTMDialog::remove_HDRs);
    connect(m_Ui->remove_TMOpts_Button, &QAbstractButton::clicked, this,
            &BatchTMDialog::remove_TMOpts);
    connect(m_Ui->remove_all_HDRs_Button, &QAbstractButton::clicked, this,
            &BatchTMDialog::remove_all_HDRs);
    connect(m_Ui->remove_all_TMOpts_Button, &QAbstractButton::clicked, this,
            &BatchTMDialog::remove_all_TMOpts);
    connect(m_Ui->BatchGoButton, &QAbstractButton::clicked, this,
            &BatchTMDialog::batch_core);  // start_called()
    connect(m_Ui->cancelbutton, &QAbstractButton::clicked, this,
            &BatchTMDialog::abort);
    connect(m_Ui->from_Database_Button, &QAbstractButton::clicked, this,
            &BatchTMDialog::from_database);
    connect(m_Ui->filterLineEdit, &QLineEdit::textChanged, this,
            &BatchTMDialog::filterChanged);
    connect(m_Ui->filterComboBox,
            static_cast<void (QComboBox::*)(int)>(&QComboBox::activated), this,
            &BatchTMDialog::filterComboBoxActivated);
    connect(m_Ui->spinBox_Width,
            static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this,
            &BatchTMDialog::updateWidth);

    full_Log_Model = new QStringListModel();
    log_filter = new QSortFilterProxyModel(this);
    log_filter->setDynamicSortFilter(true);
    log_filter->setSourceModel(full_Log_Model);
    m_Ui->Log_Widget->setModel(log_filter);
    m_Ui->Log_Widget->setWordWrap(true);

    m_formatHelper.initConnection(m_Ui->comboBoxFormat,
                                  m_Ui->formatSettingsButton, false);

    m_thread_slot.release(m_max_num_threads);
    m_available_threads = new bool[m_max_num_threads];
    for (int r = 0; r < m_max_num_threads; r++)
        m_available_threads[r] = true;  // reset to true

    m_next_hdr_file = 0;
    m_is_batch_running = false;

    add_log_message(tr("Using %n thread(s)", "", m_max_num_threads));
    // add_log_message(tr("Saving using file format:
    // %1").arg(m_Ui->comboBoxFormat->currentText()));
    m_Ui->overallProgressBar->hide();
}

BatchTMDialog::~BatchTMDialog() {
    this->hide();

    delete log_filter;
    delete full_Log_Model;
    delete[] m_available_threads;

    QApplication::restoreOverrideCursor();
}

void BatchTMDialog::add_dir_HDRs() {
    QString dirname = QFileDialog::getExistingDirectory(
        this, tr("Choose a directory"),
        LuminanceOptions().getBatchTmPathHdrInput());
    if (!dirname.isEmpty()) {
        LuminanceOptions().setBatchTmPathHdrInput(dirname);  // update settings
        QStringList filters;
        filters << QStringLiteral("*.exr") << QStringLiteral("*.hdr")
                << QStringLiteral("*.pic") << QStringLiteral("*.tiff")
                << QStringLiteral("*.tif") << QStringLiteral("*.pfs")
                << QStringLiteral("*.crw") << QStringLiteral("*.cr2")
                << QStringLiteral("*.nef") << QStringLiteral("*.dng")
                << QStringLiteral("*.mrw") << QStringLiteral("*.orf")
                << QStringLiteral("*.kdc") << QStringLiteral("*.dcr")
                << QStringLiteral("*.arw") << QStringLiteral("*.raf")
                << QStringLiteral("*.ptx") << QStringLiteral("*.pef")
                << QStringLiteral("*.x3f") << QStringLiteral("*.raw")
                << QStringLiteral("*.sr2") << QStringLiteral("*.rw2")
                << QStringLiteral("*.srw");
        filters << QStringLiteral("*.EXR") << QStringLiteral("*.HDR")
                << QStringLiteral("*.PIC") << QStringLiteral("*.TIFF")
                << QStringLiteral("*.TIF") << QStringLiteral("*.PFS")
                << QStringLiteral("*.CRW") << QStringLiteral("*.CR2")
                << QStringLiteral("*.NEF") << QStringLiteral("*.DNG")
                << QStringLiteral("*.MRW") << QStringLiteral("*.ORF")
                << QStringLiteral("*.KDC") << QStringLiteral("*.DCR")
                << QStringLiteral("*.ARW") << QStringLiteral("*.RAF")
                << QStringLiteral("*.PTX") << QStringLiteral("*.PEF")
                << QStringLiteral("*.X3F") << QStringLiteral("*.RAW")
                << QStringLiteral("*.SR2") << QStringLiteral("*.RW2")
                << QStringLiteral("*.SRW");
        QDir chosendir(dirname);
        chosendir.setFilter(QDir::Files);
        chosendir.setNameFilters(filters);
        QStringList onlyhdrs = chosendir.entryList();
        // hack to prepend to this list the path as prefix.
        onlyhdrs.replaceInStrings(QRegExp("(.+)"), chosendir.path() + "/\\1");
        add_view_model_HDRs(onlyhdrs);
    }
}

void BatchTMDialog::add_HDRs() {
    QString filetypes = tr("All HDR images ");
    filetypes +=
        "(*.exr *.hdr *.pic *.tiff *.tif *.pfs *.crw *.cr2 *.nef *.dng *.mrw "
        "*.orf *.kdc *.dcr *.arw *.raf *.ptx *.pef "
        "*.x3f *.raw *.sr2 *.rw2 *.srw "
        "*.EXR *.HDR *.PIC *.TIFF *.TIF *.PFS *.CRW *.CR2 *.NEF *.DNG *.MRW "
        "*.ORF *.KDC *.DCR *.ARW *.RAF *.PTX *.PEF *.X3F *.RAW "
        "*.SR2 *.RW2 *.SRW)";
    QStringList onlyhdrs = QFileDialog::getOpenFileNames(
        this, tr("Select input images"),
        LuminanceOptions().getBatchTmPathHdrInput(), filetypes);
    if (!onlyhdrs.isEmpty()) {
        QFileInfo fi(onlyhdrs.first());
        LuminanceOptions().setBatchTmPathHdrInput(fi.absolutePath());
        add_view_model_HDRs(onlyhdrs);
    }
}

void BatchTMDialog::add_dir_TMopts() {
    QString dirname = QFileDialog::getExistingDirectory(
        this, tr("Choose a directory"),
        LuminanceOptions().getBatchTmPathTmoSettings());
    if (!dirname.isEmpty()) {
        LuminanceOptions().setBatchTmPathTmoSettings(
            dirname);  // update settings
        QStringList filters;
        filters << QStringLiteral("*.txt");
        QDir chosendir(dirname);
        chosendir.setFilter(QDir::Files);
        chosendir.setNameFilters(filters);
        QStringList onlytxts = chosendir.entryList();
        // hack to prepend to this list the path as prefix.
        onlytxts.replaceInStrings(QRegExp("(.+)"), chosendir.path() + "/\\1");
        add_view_model_TM_OPTs(onlytxts);
    }
}

void BatchTMDialog::add_TMopts() {
    QStringList onlytxts = QFileDialog::getOpenFileNames(
        this, tr("Load tone mapping settings text files..."),
        LuminanceOptions().getBatchTmPathTmoSettings(),
        tr("Luminance HDR tone mapping settings text file (*.txt)"));
    if (!onlytxts.isEmpty()) {
        QFileInfo fi(onlytxts.first());
        LuminanceOptions().setBatchTmPathTmoSettings(fi.absolutePath());
        add_view_model_TM_OPTs(onlytxts);
    }
}

TonemappingOptions *BatchTMDialog::parse_tm_opt_file(const QString &fname) {
    try {
        return TMOptionsOperations::parseFile(fname);
    } catch (QString &e) {
        add_log_message(e);
        return NULL;
    }
}

void BatchTMDialog::check_enable_start() {
    // at least 1 hdr AND at least 1 tm_opt AND output_dir not empty
    m_Ui->BatchGoButton->setEnabled(
        (!m_Ui->out_folder_widgets->text().isEmpty()) &&
        (m_Ui->listWidget_HDRs->count() != 0) &&
        (m_Ui->listWidget_TMopts->count() != 0));
}

void BatchTMDialog::out_folder_clicked() {
    QString proposedDirname = LuminanceOptions().getBatchTmPathLdrOutput();

    QString dirname = QFileDialog::getExistingDirectory(
        this, tr("Choose a directory"), proposedDirname);

    QFileInfo test(dirname);
    if (test.isWritable() && test.exists() && test.isDir() &&
        !dirname.isEmpty()) {
        LuminanceOptions().setBatchTmPathLdrOutput(dirname);

        m_Ui->out_folder_widgets->setText(dirname);
        check_enable_start();
    }
}

void BatchTMDialog::add_view_model_HDRs(const QStringList &list) {
    for (int idx = 0; idx < list.size(); ++idx) {
        // fill graphical list
        QListWidgetItem *item = new QListWidgetItem();
        item->setData(Qt::DisplayRole, QFileInfo(list.at(idx)).fileName());
        item->setData(Qt::UserRole + 1,
                      QFileInfo(list.at(idx)).absoluteFilePath());
        m_Ui->listWidget_HDRs->addItem(item);
    }
    // HDRs_list += list;
    check_enable_start();
}

void BatchTMDialog::add_view_model_TM_OPTs(const QStringList &list) {
    bool errors = false;

    for (int idx = 0; idx < list.size(); ++idx) {
        QString curr_tmo_options_file = list.at(idx);

        TonemappingOptions *i_th_tm_opt =
            parse_tm_opt_file(curr_tmo_options_file);

        if (i_th_tm_opt != NULL) {
            i_th_tm_opt->xsize_percent = m_Ui->spinBox_Width->value();

            // add to data structure
            m_tm_options_list.append(i_th_tm_opt);

            // add to UI
            m_Ui->listWidget_TMopts->addItem(
                QFileInfo(curr_tmo_options_file).fileName());
        } else {
            errors = true;
        }
    }
    if (errors) {
        QMessageBox::warning(0, tr("Warning"),
                             tr("An error occurred reading one or more "
                                "tonemapping options files."),
                             QMessageBox::Ok, QMessageBox::NoButton);
    }
    check_enable_start();
}

void BatchTMDialog::remove_HDRs() {
    qDeleteAll(m_Ui->listWidget_HDRs->selectedItems());
    check_enable_start();
}

void BatchTMDialog::remove_all_HDRs() {
    m_Ui->listWidget_HDRs->clear();
    check_enable_start();
}

void BatchTMDialog::remove_TMOpts() {
    // qDeleteAll(m_Ui->listWidget_TMopts->selectedItems());
    QList<QListWidgetItem *> items = m_Ui->listWidget_TMopts->selectedItems();
    for (auto &item : items) {
        int row = m_Ui->listWidget_TMopts->row(item);
        delete m_Ui->listWidget_TMopts->takeItem(row);
        delete m_tm_options_list.takeAt(row);
    }
    check_enable_start();
}

void BatchTMDialog::remove_all_TMOpts() {
    m_Ui->listWidget_TMopts->clear();
    qDeleteAll(m_tm_options_list);
    m_tm_options_list.clear();
    check_enable_start();
}

void BatchTMDialog::add_log_message(const QString &message) {
    // Mutex lock!
    m_add_log_message_mutex.lock();

    full_Log_Model->insertRows(full_Log_Model->rowCount(), 1);
    full_Log_Model->setData(
        full_Log_Model->index(full_Log_Model->rowCount() - 1), message,
        Qt::DisplayRole);
    m_Ui->Log_Widget->scrollToBottom();

    // Mutex unlock!
    m_add_log_message_mutex.unlock();
}

void BatchTMDialog::filterChanged(const QString &newtext) {
    bool no_text = newtext.isEmpty();
    m_Ui->filterComboBox->setEnabled(no_text);
    m_Ui->filterLabel1->setEnabled(no_text);
    m_Ui->clearTextToolButton->setEnabled(!no_text);
    if (no_text) {
        filterComboBoxActivated(m_Ui->filterComboBox->currentIndex());
    } else {
        log_filter->setFilterRegExp(
            QRegExp(newtext, Qt::CaseInsensitive, QRegExp::RegExp));
    }
}

void BatchTMDialog::filterComboBoxActivated(int index) {
    QString regexp;
    switch (index) {
        case 0:
            regexp = QStringLiteral(".*");
            break;
        case 1:
            regexp = QStringLiteral("error");
            break;
        case 2:
            regexp = QStringLiteral("successful");
            break;
    }
    log_filter->setFilterRegExp(
        QRegExp(regexp, Qt::CaseInsensitive, QRegExp::RegExp));
}

// Davide Anastasia <davideanastasia@users.sourceforge.net>

void BatchTMDialog::batch_core() {
    init_batch_tm_ui();
    for (int row = 0; row < m_Ui->listWidget_HDRs->count(); row++) {
        HDRs_list << m_Ui->listWidget_HDRs->item(row)
                         ->data(Qt::UserRole + 1)
                         .toString();
    }
    start_batch_thread();  // kick off the conversion!
}

void BatchTMDialog::start_batch_thread() {
    // lock the class resources in order to work free of race-condition
    m_class_data_mutex.lock();

    if (m_abort) {
        m_class_data_mutex.unlock();
        emit stop_batch_tm_ui();
        return;
    }

    if (m_next_hdr_file == HDRs_list.size()) {
        m_class_data_mutex.unlock();
        emit stop_batch_tm_ui();
    } else {
        int t_id = get_available_thread_id();
        if (t_id != INT_MAX) {
            // at least one thread free!
            // start thread
            // I create the thread with NEW, but I let it die on its own, so
            // don't
            // need to store its pointer somewhere
            QString fileExtension = m_formatHelper.getFileExtension();

            BatchTMJob *job_thread = new BatchTMJob(
                t_id, HDRs_list.at(m_next_hdr_file), &m_tm_options_list,
                m_Ui->out_folder_widgets->text(), fileExtension,
                m_formatHelper.getParams());

            // Thread deletes itself when it has done with its job
            connect(job_thread, &QThread::finished, job_thread,
                    &QObject::deleteLater);
            connect(job_thread, &BatchTMJob::done, this,
                    &BatchTMDialog::release_thread);  //, Qt::DirectConnection);
            connect(
                job_thread, &BatchTMJob::add_log_message, this,
                &BatchTMDialog::add_log_message);  //, Qt::DirectConnection);
            connect(job_thread, &BatchTMJob::increment_progress_bar, this,
                    &BatchTMDialog::
                        increment_progress_bar);  //, Qt::DirectConnection);

            job_thread->start();
            m_next_hdr_file++;

            m_class_data_mutex.unlock();
            emit start_batch_thread();
        } else {
            m_class_data_mutex.unlock();
            // no thread available!
            // I return without doing anything!
            return;
        }
    }
}

int BatchTMDialog::get_available_thread_id() {
    if (m_thread_slot.tryAcquire()) {
        // if I'm here, at least one thread is available

        m_thread_control_mutex.lock();
        int t_id;
        // look for the first available ID
        for (t_id = 0; t_id < m_max_num_threads; t_id++) {
            if (m_available_threads[t_id] == true) {
                m_available_threads[t_id] = false;
                break;
            }
        }
        m_thread_control_mutex.unlock();

        assert(t_id != m_max_num_threads);
        return (t_id);
    } else
        return INT_MAX;
}

void BatchTMDialog::release_thread(int t_id) {
    m_thread_control_mutex.lock();
    m_available_threads[t_id] = true;
    m_thread_control_mutex.unlock();

    m_thread_slot.release();

    emit start_batch_thread();
}

void BatchTMDialog::init_batch_tm_ui() {
    m_is_batch_running = true;

    // progress bar activated
    m_Ui->overallProgressBar->show();
    m_Ui->overallProgressBar->setMaximum(
        m_Ui->listWidget_HDRs->count() *
        (m_Ui->listWidget_TMopts->count() + 1));
    m_Ui->overallProgressBar->setValue(0);

    // disable all buttons!
    m_Ui->BatchGoButton->setDisabled(true);
    m_Ui->add_dir_HDRs_Button->setDisabled(true);
    m_Ui->add_HDRs_Button->setDisabled(true);
    m_Ui->remove_HDRs_Button->setDisabled(true);
    m_Ui->add_dir_TMopts_Button->setDisabled(true);
    m_Ui->add_TMopts_Button->setDisabled(true);
    m_Ui->remove_TMOpts_Button->setDisabled(true);
    m_Ui->from_Database_Button->setDisabled(true);
    m_Ui->remove_all_HDRs_Button->setDisabled(true);
    m_Ui->remove_all_TMOpts_Button->setDisabled(true);
    m_Ui->groupBoxOutput->setDisabled(true);

    // mouse pointer to busy
    QApplication::setOverrideCursor(QCursor(Qt::BusyCursor));

    // change the label of the processing button
    m_Ui->BatchGoButton->setText(tr("Processing..."));

    add_log_message(tr("Start processing..."));
}

void BatchTMDialog::stop_batch_tm_ui() {
    if (m_thread_slot.tryAcquire(m_max_num_threads)) {
        m_Ui->cancelbutton->setDisabled(false);
        m_Ui->cancelbutton->setText(tr("Close"));

        m_Ui->BatchGoButton->setText(tr("&Done"));

        if (m_abort)
            add_log_message(tr("Conversion aborted by user request."));
        else
            add_log_message(tr("All tasks completed."));

        QApplication::restoreOverrideCursor();

        m_is_batch_running = false;

        m_thread_slot.release(m_max_num_threads);
    }
}

void BatchTMDialog::closeEvent(QCloseEvent *ce) {
    OsIntegration::getInstance().setProgress(-1);

    if (m_is_batch_running)
        ce->ignore();
    else
        ce->accept();
}

void BatchTMDialog::increment_progress_bar(int inc) {
    int progressValue = m_Ui->overallProgressBar->value() + inc;
    m_Ui->overallProgressBar->setValue(progressValue);
    OsIntegration::getInstance().setProgress(
        progressValue - m_Ui->overallProgressBar->minimum(),
        m_Ui->overallProgressBar->maximum());
}

void BatchTMDialog::abort() {
    if (m_is_batch_running) {
        m_abort = true;
        m_Ui->cancelbutton->setText(tr("Aborting..."));
        m_Ui->cancelbutton->setEnabled(false);
    } else
        this->reject();
}

void BatchTMDialog::updateWidth(int newWidth_in_percent) {
    foreach (TonemappingOptions *opt, m_tm_options_list) {
        opt->xsize_percent = newWidth_in_percent;
    }
}

void BatchTMDialog::from_database() {
    SavedParametersDialog dialog(m_db, this);
    if (dialog.exec()) {
        QSqlQueryModel *model = dialog.getModel();
        QModelIndexList mil = dialog.getSelectedRows();
        foreach (const QModelIndex &mi, mil) {
            QString comment, tmOperator;
            comment = model->record(mi.row())
                          .value(QStringLiteral("comment"))
                          .toString();
            tmOperator = model->record(mi.row())
                             .value(QStringLiteral("operator"))
                             .toString();

            QSqlTableModel *temp_model = new QSqlTableModel(nullptr, m_db);
            temp_model->setTable(tmOperator);
            temp_model->select();
            QSqlQuery query("SELECT * from " + tmOperator +
                            " WHERE comment = '" + comment + "'", m_db);

            TonemappingOptions *tm_opt = new TonemappingOptions;
            if (tmOperator == QLatin1String("ashikhmin")) {
                m_Ui->listWidget_TMopts->addItem(tmOperator + ": " + comment);
                tm_opt->xsize_percent = m_Ui->spinBox_Width->value();
                tm_opt->tmoperator = ashikhmin;
                tm_opt->tonemapSelection = false;
                while (query.next()) {
                    tm_opt->operator_options.ashikhminoptions.simple =
                        query.value(0).toBool();
                    tm_opt->operator_options.ashikhminoptions.eq2 =
                        query.value(1).toBool();
                    tm_opt->operator_options.ashikhminoptions.lct =
                        query.value(2).toFloat();
                    tm_opt->pregamma = query.value(3).toFloat();
                }
            } else if (tmOperator == QLatin1String("drago")) {
                m_Ui->listWidget_TMopts->addItem(tmOperator + ": " + comment);
                tm_opt->xsize_percent = m_Ui->spinBox_Width->value();
                tm_opt->tmoperator = drago;
                tm_opt->tonemapSelection = false;
                while (query.next()) {
                    tm_opt->operator_options.dragooptions.bias =
                        query.value(0).toFloat();
                    tm_opt->pregamma = query.value(1).toFloat();
                }
            } else if (tmOperator == QLatin1String("durand")) {
                m_Ui->listWidget_TMopts->addItem(tmOperator + ": " + comment);
                tm_opt->xsize_percent = m_Ui->spinBox_Width->value();
                tm_opt->tmoperator = durand;
                tm_opt->tonemapSelection = false;
                while (query.next()) {
                    tm_opt->operator_options.durandoptions.spatial =
                        query.value(0).toFloat();
                    tm_opt->operator_options.durandoptions.range =
                        query.value(1).toFloat();
                    tm_opt->operator_options.durandoptions.base =
                        query.value(2).toFloat();
                    tm_opt->pregamma = query.value(3).toFloat();
                }
            } else if (tmOperator == QLatin1String("fattal")) {
                m_Ui->listWidget_TMopts->addItem(tmOperator + ": " + comment);
                tm_opt->xsize_percent = m_Ui->spinBox_Width->value();
                tm_opt->tmoperator = fattal;
                tm_opt->tonemapSelection = false;
                while (query.next()) {
                    tm_opt->operator_options.fattaloptions.alpha =
                        query.value(0).toFloat();
                    tm_opt->operator_options.fattaloptions.beta =
                        query.value(1).toFloat();
                    tm_opt->operator_options.fattaloptions.color =
                        query.value(2).toFloat();
                    tm_opt->operator_options.fattaloptions.noiseredux =
                        query.value(3).toFloat();
                    tm_opt->operator_options.fattaloptions.newfattal =
                        !query.value(4).toBool();
                    tm_opt->operator_options.fattaloptions.fftsolver =
                        !query.value(4).toBool();
                    tm_opt->pregamma = query.value(5).toFloat();
                }
            } else if (tmOperator == QLatin1String("ferradans")) {
                m_Ui->listWidget_TMopts->addItem(tmOperator + ": " + comment);
                tm_opt->xsize_percent = m_Ui->spinBox_Width->value();
                tm_opt->tmoperator = ferradans;
                tm_opt->tonemapSelection = false;
                while (query.next()) {
                    tm_opt->operator_options.ferradansoptions.rho =
                        query.value(0).toFloat();
                    tm_opt->operator_options.ferradansoptions.inv_alpha =
                        query.value(1).toFloat();
                    tm_opt->pregamma = query.value(2).toFloat();
                }
            } else if (tmOperator == QLatin1String("mantiuk06")) {
                m_Ui->listWidget_TMopts->addItem(tmOperator + ": " + comment);
                tm_opt->xsize_percent = m_Ui->spinBox_Width->value();
                tm_opt->tmoperator = mantiuk06;
                tm_opt->tonemapSelection = false;
                while (query.next()) {
                    tm_opt->operator_options.mantiuk06options.contrastfactor =
                        query.value(1).toFloat();
                    tm_opt->operator_options.mantiuk06options.saturationfactor =
                        query.value(2).toFloat();
                    tm_opt->operator_options.mantiuk06options.detailfactor =
                        query.value(3).toFloat();
                    tm_opt->operator_options.mantiuk06options
                        .contrastequalization = query.value(0).toBool();
                    tm_opt->pregamma = query.value(4).toFloat();
                }
            } else if (tmOperator == QLatin1String("mantiuk08")) {
                m_Ui->listWidget_TMopts->addItem(tmOperator + ": " + comment);
                tm_opt->xsize_percent = m_Ui->spinBox_Width->value();
                tm_opt->tmoperator = mantiuk08;
                tm_opt->tonemapSelection = false;
                while (query.next()) {
                    tm_opt->operator_options.mantiuk08options.colorsaturation =
                        query.value(0).toFloat();
                    tm_opt->operator_options.mantiuk08options
                        .contrastenhancement = query.value(1).toFloat();
                    tm_opt->operator_options.mantiuk08options.luminancelevel =
                        query.value(2).toFloat();
                    tm_opt->operator_options.mantiuk08options.setluminance =
                        query.value(3).toBool();
                    tm_opt->pregamma = query.value(4).toFloat();
                }
            } else if (tmOperator == QLatin1String("pattanaik")) {
                m_Ui->listWidget_TMopts->addItem(tmOperator + ": " + comment);
                tm_opt->xsize_percent = m_Ui->spinBox_Width->value();
                tm_opt->tmoperator = pattanaik;
                tm_opt->tonemapSelection = false;
                while (query.next()) {
                    tm_opt->operator_options.pattanaikoptions.autolum =
                        query.value(4).toBool();
                    tm_opt->operator_options.pattanaikoptions.local =
                        query.value(3).toBool();
                    tm_opt->operator_options.pattanaikoptions.cone =
                        query.value(1).toFloat();
                    tm_opt->operator_options.pattanaikoptions.rod =
                        query.value(2).toFloat();
                    tm_opt->operator_options.pattanaikoptions.multiplier =
                        query.value(0).toFloat();
                    tm_opt->pregamma = query.value(5).toFloat();
                }
            } else if (tmOperator == QLatin1String("reinhard02")) {
                m_Ui->listWidget_TMopts->addItem(tmOperator + ": " + comment);
                tm_opt->xsize_percent = m_Ui->spinBox_Width->value();
                tm_opt->tmoperator = reinhard02;
                tm_opt->tonemapSelection = false;
                while (query.next()) {
                    tm_opt->operator_options.reinhard02options.scales =
                        query.value(0).toBool();
                    tm_opt->operator_options.reinhard02options.key =
                        query.value(1).toFloat();
                    tm_opt->operator_options.reinhard02options.phi =
                        query.value(2).toFloat();
                    tm_opt->operator_options.reinhard02options.range =
                        query.value(3).toInt();
                    tm_opt->operator_options.reinhard02options.lower =
                        query.value(4).toInt();
                    tm_opt->operator_options.reinhard02options.upper =
                        query.value(5).toInt();
                    tm_opt->pregamma = query.value(6).toFloat();
                }
            } else if (tmOperator == QLatin1String("reinhard05")) {
                m_Ui->listWidget_TMopts->addItem(tmOperator + ": " + comment);
                tm_opt->xsize_percent = m_Ui->spinBox_Width->value();
                tm_opt->tmoperator = reinhard05;
                tm_opt->tonemapSelection = false;
                while (query.next()) {
                    tm_opt->operator_options.reinhard05options.brightness =
                        query.value(0).toFloat();
                    tm_opt->operator_options.reinhard05options
                        .chromaticAdaptation = query.value(1).toFloat();
                    tm_opt->operator_options.reinhard05options.lightAdaptation =
                        query.value(2).toFloat();
                    tm_opt->pregamma = query.value(3).toFloat();
                }
            } else if (tmOperator == QLatin1String("ferwerda")) {
                m_Ui->listWidget_TMopts->addItem(tmOperator + ": " + comment);
                tm_opt->xsize_percent = m_Ui->spinBox_Width->value();
                tm_opt->tmoperator = ferwerda;
                tm_opt->tonemapSelection = false;
                while (query.next()) {
                    tm_opt->operator_options.ferwerdaoptions.multiplier =
                        query.value(0).toFloat();
                    tm_opt->operator_options.ferwerdaoptions.adaptationluminance =
                        query.value(1).toFloat();
                    tm_opt->pregamma = query.value(2).toFloat();
                }
            } else if (tmOperator == QLatin1String("kimkautz")) {
                m_Ui->listWidget_TMopts->addItem(tmOperator + ": " + comment);
                tm_opt->xsize_percent = m_Ui->spinBox_Width->value();
                tm_opt->tmoperator = kimkautz;
                tm_opt->tonemapSelection = false;
                while (query.next()) {
                    tm_opt->operator_options.kimkautzoptions.c1 =
                        query.value(0).toFloat();
                    tm_opt->operator_options.kimkautzoptions.c2 =
                        query.value(1).toFloat();
                    tm_opt->pregamma = query.value(2).toFloat();
                }
            }
            m_tm_options_list.append(tm_opt);
            delete temp_model;
        }
    }
}
