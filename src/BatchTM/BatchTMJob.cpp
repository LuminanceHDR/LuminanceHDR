/**
 * This file is a part of LuminanceHDR package.
 * ----------------------------------------------------------------------
 * Copyright (C) 2011 Davide Anastasia
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
 * @author Davide Anastasia <davideanastasia@users.sourceforge.net>
 *
 */

#include <QByteArray>
#include <QDebug>
#include <QFileInfo>
#include <QImage>
#include <QScopedPointer>

#include <BatchTM/BatchTMJob.h>
#include <Exif/ExifOperations.h>
#include <Libpfs/frame.h>
#include <Libpfs/manip/copy.h>
#include <Libpfs/manip/gamma.h>
#include <Libpfs/manip/resize.h>
#include <Libpfs/progress.h>
#include <Libpfs/tm/TonemapOperator.h>

#include <Common/LuminanceOptions.h>
#include <Core/IOWorker.h>

BatchTMJob::BatchTMJob(int thread_id, const QString &filename,
                       const QList<TonemappingOptions *> *tm_options,
                       const QString &output_folder, const QString &format,
                       pfs::Params params)
    : m_thread_id(thread_id),
      m_file_name(filename),
      m_tm_options(tm_options),
      m_output_folder(output_folder),
      m_ldr_output_format(format),
      m_params(params) {
    // m_ldr_output_format = LuminanceOptions().getBatchTmLdrFormat();

    m_output_file_name_base =
        m_output_folder + "/" + QFileInfo(m_file_name).completeBaseName();
}

BatchTMJob::~BatchTMJob() {}

void BatchTMJob::run() {
    pfs::Progress prog_helper;
    IOWorker io_worker;

    emit add_log_message(tr("[T%1] Start processing %2")
                             .arg(m_thread_id)
                             .arg(QFileInfo(m_file_name).fileName()));

    // reference frame
    QScopedPointer<pfs::Frame> reference_frame(
        io_worker.read_hdr_frame(m_file_name));

    if (!reference_frame.isNull()) {
        // update message box
        emit add_log_message(tr("[T%1] Successfully load %2")
                                 .arg(m_thread_id)
                                 .arg(QFileInfo(m_file_name).fileName()));

        // update progress bar!
        emit increment_progress_bar(1);

        for (int idx = 0; idx < m_tm_options->size(); ++idx) {
            TonemappingOptions *opts = m_tm_options->at(idx);

            opts->tonemapSelection = false;  // just to be sure!
            opts->origxsize = reference_frame->getWidth();

            opts->xsize = (int)opts->origxsize * opts->xsize_percent / 100;

            QScopedPointer<pfs::Frame> temporary_frame;
            if (opts->origxsize == opts->xsize) {
                temporary_frame.reset(pfs::copy(reference_frame.data()));
            } else {
                temporary_frame.reset(pfs::resize(reference_frame.data(),
                                                  opts->xsize, BilinearInterp));
            }

            if (opts->pregamma != 1.0f) {
                pfs::applyGamma(temporary_frame.data(), opts->pregamma);
            }

            QScopedPointer<TonemapOperator> tm_operator(
                TonemapOperator::getTonemapOperator(opts->tmoperator));

            try {
                tm_operator->tonemapFrame(*temporary_frame, opts, prog_helper);
            } catch (...) {
                emit add_log_message(
                    tr("[T%1] ERROR: Failed to tonemap file: %2")
                        .arg(m_thread_id)
                        .arg(QFileInfo(m_file_name).fileName()));
                emit increment_progress_bar(m_tm_options->size() + 1);
                emit done(m_thread_id);
                return;
            }

            QString output_file_name = m_output_file_name_base + "_" +
                                       opts->getPostfix() + "." +
                                       m_ldr_output_format;

            if (io_worker.write_ldr_frame(
                    temporary_frame.data(), output_file_name,
                    "FromHdrFile",  // inform we tonemapped an
                                    // existing HDR with no exif
                                    // data
                    QVector<float>(), opts, m_params)) {
                emit add_log_message(
                    tr("[T%1] Successfully saved LDR file: %2")
                        .arg(m_thread_id)
                        .arg(QFileInfo(output_file_name).fileName()));
            } else {
                emit add_log_message(
                    tr("[T%1] ERROR: Cannot save to file: %2")
                        .arg(m_thread_id)
                        .arg(QFileInfo(output_file_name).fileName()));
            }

            emit increment_progress_bar(1);
        }
    } else {
        // update message box
        // emit add_log_message(error_message);
        emit add_log_message(tr("[T%1] ERROR: Loading of %2 failed")
                                 .arg(m_thread_id)
                                 .arg(QFileInfo(m_file_name).fileName()));

        // update progress bar!
        emit increment_progress_bar(m_tm_options->size() + 1);
    }

    emit done(m_thread_id);
}
