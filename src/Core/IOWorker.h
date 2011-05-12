/**
 * This file is a part of Luminance HDR package.
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
 * This class provides I/O of HDR images
 * This class is inspired by LoadHdrThread and borrow most of its code
 * but it is not derived from QThread
 *
 * Original Work
 * @author Davide Anastasia <davideanastasia@users.sourceforge.net>
 *
 */

#ifndef __IO_WORKER_H__
#define __IO_WORKER_H__

#include <QObject>
#include <QString>
#include <QStringList>

#include "Libpfs/pfs.h"
#include "Viewers/HdrViewer.h"
#include "Viewers/LdrViewer.h"
#include "Common/options.h"

class IOWorker : public QObject
{
    Q_OBJECT

private:
    LuminanceOptions* luminance_options;

    void get_frame(QString fname);
public:
    IOWorker(QObject* parent = 0);
    virtual ~IOWorker();

public slots:
    void read_frame(QString filename);
    void read_frames(QStringList filenames);

    void write_hdr_frame(HdrViewer* frame, QString filename);
    void write_ldr_frame(LdrViewer* frame, QString filename, int quality);
signals:
    void read_failed(QString error_message);
    void read_success(pfs::Frame*, QString fname);

    void write_hdr_failed();
    void write_hdr_success(HdrViewer*, QString);
    void write_ldr_failed();
    void write_ldr_success(LdrViewer*, QString);

    void setMaximum(int);
    void setValue(int);

    void IO_init();
    void IO_finish();
};

#endif
