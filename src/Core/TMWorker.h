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
 * Original Work
 * @author Davide Anastasia <davideanastasia@users.sourceforge.net>
 *
 */

#ifndef TMWORKER_H
#define TMWORKER_H

#include <QObject>
#include <QString>

#include <Common/global.h>
#include <Libpfs/params.h>

// Forward declaration
namespace pfs {
class Frame;
}

class TonemappingOptions;
class ProgressHelper;

class TMWorker : public QObject {
    Q_OBJECT

   public:
    TMWorker(QObject *parent = 0);
    ~TMWorker();

   public Q_SLOTS:
    //!
    //!  This function creates a copy of the input frame, tonemap the copy
    //!  and then returns it
    //!
    pfs::Frame *computeTonemap(/* const */ pfs::Frame *, TonemappingOptions *,
                               InterpolationMethod m);

    void computeTonemapAndExport(/* const */ pfs::Frame *, TonemappingOptions *,
                                 pfs::Params, QString exportDir,
                                 QString hdrName, QString inputfname,
                                 QVector<float> inputExpoTimes,
                                 InterpolationMethod m);

    //!
    //! This function tonemap the input frame
    //!
    void tonemapFrame(pfs::Frame *, TonemappingOptions *);

   private:
    pfs::Frame *preprocessFrame(pfs::Frame *, TonemappingOptions *,
                                InterpolationMethod m);
    void postprocessFrame(pfs::Frame *, TonemappingOptions *);

   Q_SIGNALS:
    void tonemapSuccess(pfs::Frame *, TonemappingOptions *);
    void tonemapFailed(QString);

    void tonemapBegin();
    void tonemapEnd();
    void tonemapSetMaximum(int);
    void tonemapSetMinimum(int);
    void tonemapSetValue(int);
    void tonemapRequestTermination(bool);

   private:
    ProgressHelper *m_Callback;
};

#endif  // TMWORKER_H
