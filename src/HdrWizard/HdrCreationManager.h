/**
 * This file is a part of Luminance HDR package
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
 * Improvements, bugfixing
 * @author Franco Comida <fcomida@users.sourceforge.net>
 *
 */

#ifndef HDRCREATIONMANAGER_H
#define HDRCREATIONMANAGER_H

#include <cmath>
#include <cstddef>
#include <vector>

#include <QFutureWatcher>
#include <QPair>
#include <QProcess>
#include <QSharedPointer>

#include <HdrCreation/createhdr.h>
#include <HdrCreation/fusionoperator.h>
#include <Libpfs/frame.h>

#include <Alignment/Align.h>
#include <Common/LuminanceOptions.h>
#include <Common/ProgressHelper.h>
#include <HdrWizard/AutoAntighosting.h>
#include <HdrWizard/HdrCreationItem.h>
#include <arch/math.h>

// Some other file expect this to be available
extern const FusionOperatorConfig predef_confs[6];

class HdrCreationManager : public QObject {
    Q_OBJECT

   private:
    HdrCreationItemContainer m_data;
    HdrCreationItemContainer m_tmpdata;

   public:
    explicit HdrCreationManager(bool fromCommandLine = false);
    ~HdrCreationManager();

    // ----- NEW FUNCTIONS ------
    HdrCreationItem &getFile(size_t idx) { return m_data[idx]; }
    const HdrCreationItem &getFile(size_t idx) const { return m_data[idx]; }

    void loadFiles(const QStringList &filenames);
    void removeFile(int idx);
    void clearFiles() {
        m_data.clear();
        m_tmpdata.clear();
    }
    size_t availableInputFiles() const { return m_data.size(); }

    QStringList getFilesWithoutExif() const;
    size_t numFilesWithoutExif() const;

    void setLoadResponseCurve(bool b) { m_isLoadResponseCurve = b; }
    bool isLoadResponseCurve() const { return m_isLoadResponseCurve; }
    void setResponseCurveInputFilename(const QString &fn) {
        m_responseCurveInputFilename = fn;
    }
    QString &getResponseCurveInputFilename() {
        return m_responseCurveInputFilename;
    }

    // iterators
    typedef HdrCreationItemContainer::iterator iterator;
    typedef HdrCreationItemContainer::const_iterator const_iterator;

    iterator begin() { return m_data.begin(); }
    iterator end() { return m_data.end(); }
    const_iterator begin() const { return m_data.begin(); }
    const_iterator end() const { return m_data.end(); }

    const libhdr::fusion::ResponseCurve &getResponseCurve() const {
        return *m_response;
    }
    libhdr::fusion::ResponseCurve &getResponseCurve() { return *m_response; }

    const libhdr::fusion::WeightFunction &getWeightFunction() const {
        return *m_weight;
    }
    libhdr::fusion::WeightFunction &getWeightFunction() { return *m_weight; }

    void setFusionOperator(libhdr::fusion::FusionOperator fo) {
        m_fusionOperator = fo;
    }
    libhdr::fusion::FusionOperator getFusionOperator() {
        return m_fusionOperator;
    }

    void setResponseCurveOutputFile(const QString &filename) {
        m_responseCurveOutputFilename = filename;
    }
    const QString &responseCurveOutputFile() const {
        return m_responseCurveOutputFilename;
    }

    void setConfig(const FusionOperatorConfig &cfg);

    pfs::Frame *createHdr();

    void set_ais_crop_flag(bool flag);
    void align_with_ais();
    void align_with_mtb();

    const HdrCreationItemContainer &getData() const { return m_data; }
    // const QList<QImage*>& getAntiGhostingMasksList() const  { return
    // m_antiGhostingMasksList; }
    // void setAntiGhostingMasksList(QList<QImage*>& list)     {
    // m_antiGhostingMasksList.swap(list); }
    void setAntiGhostingMask(QImage *mask) { m_agMask = new QImage(*mask); }
    QVector<float> getExpotimes() const;

    void applyShiftsToItems(const QList<QPair<int, int>> &);
    void cropItems(const QRect &ca);
    void cropAgMasks(const QRect &ca);

    void saveImages(const QString &prefix);
    // void doAntiGhosting(int);
    int computePatches(float threshold, bool patches[][agGridSize],
                       float &percent, QList<QPair<int, int>> HV_offset);
    pfs::Frame *doAntiGhosting(bool patches[][agGridSize], int h0,
                               bool manualAg, ProgressHelper *ph);
    void getAgData(bool patches[][agGridSize], int &h0);
    void setPatches(bool patches[][agGridSize]);

    float getEVOffset() const;

    void reset();

   signals:
    // computation progress
    void progressStarted();
    void progressFinished();
    void progressCancel();
    void progressRangeChanged(int, int);
    void progressValueChanged(int);
    void finishedLoadingFiles();

    // legacy code
    void finishedLoadingInputFiles(const QStringList &filesLackingExif);
    void errorWhileLoading(const QString &message);  // also for !valid size

    void fileLoaded(int index, const QString &fname, float expotime);

    void finishedAligning(int);
    void expotimeValueChanged(float, int);
    void ais_failed(QProcess::ProcessError);
    void aisDataReady(QByteArray &data);
    void processed();
    void imagesSaved();
    void loadFilesAborted();

   private:
    bool framesHaveSameSize();
    void refreshEVOffset();

    float m_evOffset;

    std::unique_ptr<libhdr::fusion::ResponseCurve> m_response;
    std::unique_ptr<libhdr::fusion::WeightFunction> m_weight;
    libhdr::fusion::FusionOperator m_fusionOperator;
    QString m_responseCurveInputFilename;
    QString m_responseCurveOutputFilename;

    QFutureWatcher<void> m_futureWatcher;
    // QList<QImage*> m_antiGhostingMasksList;  //QImages used for manual
    // anti-ghosting
    QImage *m_agMask;
    LuminanceOptions m_luminance_options;

    // alignment
    std::unique_ptr<Align> m_align;

    bool m_ais_crop_flag;
    bool fromCommandLine;
    int m_agGoodImageIndex;
    bool m_patches[agGridSize][agGridSize];
    bool m_isLoadResponseCurve;

   private slots:
    void ais_failed_slot(QProcess::ProcessError);
    void loadFilesDone();
};
#endif
