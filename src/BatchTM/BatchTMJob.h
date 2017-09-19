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
 * This class splits the "Batch Tonemapping core" from the UI, to achieve a
 * simpler
 * code, faster, easier to maintain and more clear to read
 *
 */

#ifndef BATCHTMJOB_H
#define BATCHTMJOB_H

#include <QList>
#include <QString>
#include <QThread>

#include <Libpfs/params.h>

// Forward declaration
class TonemappingOptions;

class BatchTMJob : public QThread {
    Q_OBJECT
   public:
    BatchTMJob(int thread_id, const QString &filename,
               const QList<TonemappingOptions *> *tm_options,
               const QString &output_folder, const QString &ldr_output_format,
               pfs::Params params);
    virtual ~BatchTMJob();
   signals:
    void done(int thread_id);
    void add_log_message(const QString &);
    // void update_progress_bar();
    void increment_progress_bar(int);

   protected:
    void run();

   private:
    int m_thread_id;
    QString m_file_name;
    const QList<TonemappingOptions *> *m_tm_options;
    QString m_output_folder;
    QString m_output_file_name_base;
    QString m_ldr_output_format;
    pfs::Params m_params;
};

#endif  // BATCHTMJOB_H
