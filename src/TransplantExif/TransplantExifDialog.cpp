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
 * @author Giuseppe Rota <grota@users.sourceforge.net>
 */

#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <exif.hpp>
#include <image.hpp>
#include <exiv2/exiv2.hpp>

#include "Common/config.h"
#include "Common/global.h"
#include "Exif/ExifOperations.h"

#include "TransplantExif/TransplantExifDialog.h"
#include "TransplantExif/ui_TransplantExifDialog.h"

TransplantExifDialog::TransplantExifDialog(QWidget *p)
    : QDialog(p),
      start_left(-1),
      stop_left(-1),
      start_right(-1),
      stop_right(-1),
      done(false),
      m_Ui(new Ui::TransplantExifDialog) {
    m_Ui->setupUi(this);

    m_Ui->progressBar->hide();

    if (!QIcon::hasThemeIcon(QStringLiteral("arrow-up"))) {
        m_Ui->moveup_right_button->setIcon(QIcon(":/program-icons/arrow-up"));
        m_Ui->moveup_left_button->setIcon(QIcon(":/program-icons/arrow-up"));
    }
    if (!QIcon::hasThemeIcon(QStringLiteral("arrow-down"))) {
        m_Ui->movedown_right_button->setIcon(
            QIcon(":/program-icons/arrow-down"));
        m_Ui->movedown_left_button->setIcon(
            QIcon(":/program-icons/arrow-down"));
    }

    m_Ui->Log_Widget->setWordWrap(true);
    connect(m_Ui->moveup_left_button, &QAbstractButton::clicked, this,
            &TransplantExifDialog::moveup_left);
    connect(m_Ui->moveup_right_button, &QAbstractButton::clicked, this,
            &TransplantExifDialog::moveup_right);
    connect(m_Ui->movedown_left_button, &QAbstractButton::clicked, this,
            &TransplantExifDialog::movedown_left);
    connect(m_Ui->movedown_right_button, &QAbstractButton::clicked, this,
            &TransplantExifDialog::movedown_right);
    connect(m_Ui->removeleft, &QAbstractButton::clicked, this,
            &TransplantExifDialog::remove_left);
    connect(m_Ui->removeright, &QAbstractButton::clicked, this,
            &TransplantExifDialog::remove_right);
    connect(m_Ui->addleft, &QAbstractButton::clicked, this,
            &TransplantExifDialog::append_left);
    connect(m_Ui->addright, &QAbstractButton::clicked, this,
            &TransplantExifDialog::append_right);
    connect(m_Ui->TransplantButton, &QAbstractButton::clicked, this,
            &TransplantExifDialog::transplant_requested);
    /*    connect(HelpButton, SIGNAL(clicked()),this,SLOT(help_requested())); */

    connect(m_Ui->filterComboBox, SIGNAL(activated(int)), this,
            SLOT(filterComboBoxActivated(int)));
    connect(m_Ui->filterLineEdit, &QLineEdit::textChanged, this,
            &TransplantExifDialog::filterChanged);
    full_Log_Model = new QStringListModel();
    log_filter = new QSortFilterProxyModel(this);
    log_filter->setDynamicSortFilter(true);
    log_filter->setSourceModel(full_Log_Model);
    m_Ui->Log_Widget->setModel(log_filter);

    // TODO: clean up this implementation: this guys should not be here
    RecentDirEXIFfrom =
        luminance_options.value(KEY_RECENT_PATH_EXIF_FROM, QDir::currentPath())
            .toString();
    RecentDirEXIFto =
        luminance_options.value(KEY_RECENT_PATH_EXIF_TO, QDir::currentPath())
            .toString();
}

TransplantExifDialog::~TransplantExifDialog() {
    m_Ui->leftlist->clear();
    m_Ui->rightlist->clear();

    delete full_Log_Model;
    delete log_filter;
}

// TODO
void TransplantExifDialog::help_requested() {
    /*    QDialog *help=new QDialog();
        help->setAttribute(Qt::WA_DeleteOnClose);
        Ui::HelpDialog ui;
        ui.setupUi(help);
        ui.tb->setSearchPaths(QStringList("/usr/share/luminance/html") <<
       "/usr/local/share/luminance/html" << "./html");
        ui.tb->setSource(QUrl("manual.html#copyexif"));
        help->exec();*/
}

void TransplantExifDialog::updateinterval(bool left) {
    if (left) {
        start_left = m_Ui->leftlist->count();
        stop_left = -1;
        for (int i = 0; i < m_Ui->leftlist->count(); i++) {
            if (m_Ui->leftlist->isItemSelected(m_Ui->leftlist->item(i))) {
                start_left = (start_left > i) ? i : start_left;
                stop_left = (stop_left < i) ? i : stop_left;
            }
        }
        //         qDebug("L %d-%d",start_left,stop_left);
    } else {
        start_right = m_Ui->rightlist->count();
        stop_right = -1;
        for (int i = 0; i < m_Ui->rightlist->count(); i++) {
            if (m_Ui->rightlist->isItemSelected(m_Ui->rightlist->item(i))) {
                start_right = (start_right > i) ? i : start_right;
                stop_right = (stop_right < i) ? i : stop_right;
            }
        }
        //         qDebug("R %d-%d",start_right,stop_right);
    }
}

void TransplantExifDialog::moveup_left() {
    updateinterval(true);
    if (m_Ui->leftlist->count() == 0 || start_left == -1 || stop_left == -1 ||
        start_left == 0)
        return;
    //"VIEW"
    // copy the before-first element to the past-end of the selection
    m_Ui->leftlist->insertItem(stop_left + 1,
                               QFileInfo(from.at(start_left - 1)).fileName());
    // remove the before-first element
    m_Ui->leftlist->takeItem(start_left - 1);
    //"MODEL"
    from.insert(stop_left + 1, from.at(start_left - 1));
    from.removeAt(start_left - 1);
    start_left--;
    stop_left--;
}

void TransplantExifDialog::moveup_right() {
    updateinterval(false);
    if (m_Ui->rightlist->count() == 0 || start_right == -1 ||
        stop_right == -1 || start_right == 0)
        return;
    //"VIEW"
    // copy the before-first element to the past-end of the selection
    m_Ui->rightlist->insertItem(stop_right + 1,
                                QFileInfo(to.at(start_right - 1)).fileName());
    // remove the before-first element
    m_Ui->rightlist->takeItem(start_right - 1);
    //"MODEL"
    to.insert(stop_right + 1, to.at(start_right - 1));
    to.removeAt(start_right - 1);
    start_right--;
    stop_right--;
}

void TransplantExifDialog::movedown_left() {
    updateinterval(true);
    if (m_Ui->leftlist->count() == 0 || start_left == -1 || stop_left == -1 ||
        stop_left == m_Ui->leftlist->count() - 1)
        return;
    //"VIEW"
    // copy the past-end to the before-first element of the selection
    m_Ui->leftlist->insertItem(start_left,
                               QFileInfo(from.at(stop_left + 1)).fileName());
    // remove the past-end
    m_Ui->leftlist->takeItem(stop_left + 2);
    //"MODEL"
    from.insert(start_left, from.at(stop_left + 1));
    from.removeAt(stop_left + 2);
    start_left++;
    stop_left++;
}

void TransplantExifDialog::movedown_right() {
    updateinterval(false);
    if (m_Ui->rightlist->count() == 0 || start_right == -1 ||
        stop_right == -1 || stop_right == m_Ui->rightlist->count() - 1)
        return;
    //"VIEW"
    // copy the past-end to the before-first element of the selection
    m_Ui->rightlist->insertItem(start_right,
                                QFileInfo(to.at(stop_right + 1)).fileName());
    // remove the past-end
    m_Ui->rightlist->takeItem(stop_right + 2);
    //"MODEL"
    to.insert(start_right, to.at(stop_right + 1));
    to.removeAt(stop_right + 2);
    start_right++;
    stop_right++;
}

void TransplantExifDialog::remove_left() {
    updateinterval(true);
    if (m_Ui->leftlist->count() == 0 || start_left == -1 || stop_left == -1)
        return;
    for (int i = stop_left - start_left + 1; i > 0; i--) {
        m_Ui->leftlist->takeItem(start_left);
        from.removeAt(start_left);
    }
    start_left = stop_left = -1;
    m_Ui->TransplantButton->setEnabled(m_Ui->leftlist->count() ==
                                           m_Ui->rightlist->count() &&
                                       m_Ui->rightlist->count() != 0);
}

void TransplantExifDialog::remove_right() {
    updateinterval(false);
    if (m_Ui->rightlist->count() == 0 || start_right == -1 || stop_right == -1)
        return;
    for (int i = stop_right - start_right + 1; i > 0; i--) {
        m_Ui->rightlist->takeItem(start_right);
        to.removeAt(start_right);
    }
    start_right = stop_right = -1;
    m_Ui->TransplantButton->setEnabled(
        (m_Ui->leftlist->count() == m_Ui->rightlist->count()) &&
        (m_Ui->rightlist->count() != 0));
}

void TransplantExifDialog::append_left() {
    QString filetypes = tr("All Supported formats");
    filetypes +=
        " (*.jpeg *.jpg *.tif *.tiff *.crw *.cr2 *.nef *.dng *.mrw *.orf *.kdc "
        "*.dcr *.arw *.ptx *.pef *.x3f *.raw *.rw2 *.sr2 "
        "*.JPEG *.JPG *.TIF *.TIFF *.CRW *.CR2 *.NEF *.DNG *.MRW *.ORF *.KDC "
        "*.DCR *.ARW *.PTX *.PEF *.X3F *.RAW *.RW2 *.SR2)";
    QStringList files = QFileDialog::getOpenFileNames(
        this, tr("Select the input images"), RecentDirEXIFfrom, filetypes);
    if (!files.isEmpty()) {
        QFileInfo qfi(files.at(0));
        // if the new dir, the one just chosen by the user, is different from
        // the
        // one stored in the settings, update the luminance_options.
        if (RecentDirEXIFfrom != qfi.path()) {
            // update internal field variable
            RecentDirEXIFfrom = qfi.path();
            luminance_options.setValue(KEY_RECENT_PATH_EXIF_FROM,
                                       RecentDirEXIFfrom);
        }
        QStringList::Iterator it = files.begin();
        while (it != files.end()) {
            QFileInfo *qfi = new QFileInfo(*it);
            m_Ui->leftlist->addItem(qfi->fileName());  // fill graphical list
            ++it;
            delete qfi;
        }
        from += files;  // add the new files to the "model"
        m_Ui->TransplantButton->setEnabled(
            (m_Ui->leftlist->count() == m_Ui->rightlist->count()) &&
            (m_Ui->rightlist->count() != 0));
    }
}

void TransplantExifDialog::append_right() {
    QString filetypes = tr("All Supported formats") +
                        " (*.jpeg *.jpg *.JPEG *.JPG "
#if EXIV2_TEST_VERSION(0, 18, 0)
                        + "*.tif *.tiff *.TIF *.TIFF "
#endif
                        +
                        "*.crw *.orf *.kdc *.dcr *.ptx *.x3f *.CRW *.ORF *.KDC "
                        "*.DCR *.PTX *.X3F)";
    QStringList files = QFileDialog::getOpenFileNames(
        this, tr("Select the input images"), RecentDirEXIFto, filetypes);
    if (!files.isEmpty()) {
        QFileInfo qfi(files.at(0));
        // if the new dir, the one just chosen by the user, is different from
        // the
        // one stored in the settings, update the luminance_options.
        if (RecentDirEXIFto != qfi.path()) {
            // update internal field variable
            RecentDirEXIFto = qfi.path();
            luminance_options.setValue(KEY_RECENT_PATH_EXIF_TO,
                                       RecentDirEXIFto);
        }
        QStringList::Iterator it = files.begin();
        while (it != files.end()) {
            QFileInfo *qfi = new QFileInfo(*it);
            m_Ui->rightlist->addItem(qfi->fileName());  // fill graphical list
            ++it;
            delete qfi;
        }
        to += files;  // add the new files to the "model"
        m_Ui->TransplantButton->setEnabled(
            (m_Ui->leftlist->count() == m_Ui->rightlist->count()) &&
            (m_Ui->rightlist->count() != 0));
    }
}

void TransplantExifDialog::transplant_requested() {
    if (done) {
        accept();
        return;
    }

    m_Ui->progressBar->show();
    m_Ui->progressBar->setMaximum(m_Ui->leftlist->count());
    // initialize string iterators to the beginning of the lists.
    QStringList::const_iterator i_source = from.constBegin();
    QStringList::const_iterator i_dest = to.constBegin();

    int index = 0;
    // for all the input files
    for (; i_source != from.constEnd(); ++i_source, ++i_dest) {
        try {
            add_log_message(*i_source + "-->" + *i_dest);
            // ExifOperations methods want a std::string, we need to use the
            // QFile::encodeName(QString).constData() trick to cope with local
            // 8-bit
            // encoding determined by the user's locale.
            ExifOperations::copyExifData(
                QFile::encodeName((*i_source)).constData(),
                QFile::encodeName((*i_dest)).constData(),
                m_Ui->checkBox_dont_overwrite->isChecked());
            m_Ui->rightlist->item(index)->setBackground(QBrush("#a0ff87"));
        } catch (Exiv2::AnyError &e) {
            add_log_message("ERROR:" + QString::fromStdString(e.what()));
            m_Ui->rightlist->item(index)->setBackground(QBrush("#ff743d"));
        }
        m_Ui->progressBar->setValue(m_Ui->progressBar->value() +
                                    1);  // increment progressbar
        index++;
    }

    done = true;
    m_Ui->TransplantButton->setText(tr("&Done"));
    m_Ui->moveup_left_button->setDisabled(true);
    m_Ui->moveup_right_button->setDisabled(true);
    m_Ui->movedown_left_button->setDisabled(true);
    m_Ui->movedown_right_button->setDisabled(true);
    m_Ui->removeleft->setDisabled(true);
    m_Ui->removeright->setDisabled(true);
    m_Ui->addleft->setDisabled(true);
    m_Ui->addright->setDisabled(true);
    m_Ui->cancelbutton->setDisabled(true);
}

void TransplantExifDialog::add_log_message(const QString &message) {
    full_Log_Model->insertRows(full_Log_Model->rowCount(), 1);
    full_Log_Model->setData(
        full_Log_Model->index(full_Log_Model->rowCount() - 1), message,
        Qt::DisplayRole);
    m_Ui->Log_Widget->scrollToBottom();
}

void TransplantExifDialog::filterChanged(const QString &newtext) {
    bool no_text = newtext.isEmpty();
    m_Ui->filterComboBox->setEnabled(no_text);
    m_Ui->filterLabel1->setEnabled(no_text);
    m_Ui->clearTextToolButton->setEnabled(!no_text);
    if (no_text)
        filterComboBoxActivated(m_Ui->filterComboBox->currentIndex());
    else
        log_filter->setFilterRegExp(
            QRegExp(newtext, Qt::CaseInsensitive, QRegExp::RegExp));
}

void TransplantExifDialog::filterComboBoxActivated(int index) {
    QString regexp;
    switch (index) {
        case 0:
            regexp = QStringLiteral(".*");
            break;
        case 1:
            regexp = QStringLiteral("error");
            break;
    }
    log_filter->setFilterRegExp(
        QRegExp(regexp, Qt::CaseInsensitive, QRegExp::RegExp));
}
