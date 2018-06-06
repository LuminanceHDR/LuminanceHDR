/**
 * This file is a part of Luminance HDR package
 * ----------------------------------------------------------------------
 * Copyright (C) 2006,2007 Giuseppe Rota
 * Copyright (C) 2010-2012 Franco Comida
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
 *
 * Manual and auto antighosting, improvements, bugfixing
 * @author Franco Comida <fcomida@users.sourceforge.net>
 *
 */

#include "HdrCreationManager.h"

#include <QApplication>
#include <QColor>
#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QScopedPointer>
#include <QtConcurrentFilter>
#include <QtConcurrentMap>

#include <algorithm>
#include <boost/bind.hpp>
#include <boost/limits.hpp>
#include <boost/numeric/conversion/bounds.hpp>
#include <cmath>
#include <iostream>
#include <vector>

#include <Common/CommonFunctions.h>
#include <Libpfs/colorspace/colorspace.h>
#include <Libpfs/colorspace/convert.h>
#include <Libpfs/colorspace/normalizer.h>
#include <Libpfs/frame.h>
#include <Libpfs/io/framereader.h>
#include <Libpfs/io/framereaderfactory.h>
#include <Libpfs/io/framewriter.h>
#include <Libpfs/io/framewriterfactory.h>
#include <Libpfs/io/tiffreader.h>
#include <Libpfs/io/tiffwriter.h>
#include <Libpfs/manip/copy.h>
#include <Libpfs/manip/cut.h>
#include <Libpfs/manip/shift.h>
#include <Libpfs/utils/msec_timer.h>
#include <Libpfs/utils/transform.h>

#include <Exif/ExifOperations.h>
#include <HdrCreation/mtb_alignment.h>
#include <HdrWizard/WhiteBalance.h>
#include <TonemappingOperators/fattal02/pde.h>
#include <arch/math.h>

using namespace std;
using namespace pfs;
using namespace pfs::io;
using namespace colorspace;
using namespace utils;
using namespace libhdr::fusion;

const FusionOperatorConfig predef_confs[6] = {
    {WEIGHT_TRIANGULAR, RESPONSE_LINEAR, DEBEVEC, QString(), QString()},
    {WEIGHT_TRIANGULAR, RESPONSE_GAMMA, DEBEVEC, QString(), QString()},
    {WEIGHT_PLATEAU, RESPONSE_LINEAR, DEBEVEC, QString(), QString()},
    {WEIGHT_PLATEAU, RESPONSE_GAMMA, DEBEVEC, QString(), QString()},
    {WEIGHT_GAUSSIAN, RESPONSE_LINEAR, DEBEVEC, QString(), QString()},
    {WEIGHT_GAUSSIAN, RESPONSE_GAMMA, DEBEVEC, QString(), QString()},
};

// --- NEW CODE ---
namespace {

QImage *shiftQImage(const QImage *in, int dx, int dy) {
    QImage *out = new QImage(in->size(), QImage::Format_ARGB32);
    assert(out != NULL);
    out->fill(qRgba(0, 0, 0, 0));  // transparent black
    for (int i = 0; i < in->height(); i++) {
        if ((i + dy) < 0) continue;
        if ((i + dy) >= in->height()) break;
        QRgb *inp = (QRgb *)in->scanLine(i);
        QRgb *outp = (QRgb *)out->scanLine(i + dy);
        for (int j = 0; j < in->width(); j++) {
            if ((j + dx) >= in->width()) break;
            if ((j + dx) >= 0) outp[j + dx] = *inp;
            inp++;
        }
    }
    return out;
}

void shiftItem(HdrCreationItem &item, int dx, int dy) {
    FramePtr shiftedFrame(pfs::shift(*item.frame(), dx, dy));
    item.frame().swap(shiftedFrame);
    shiftedFrame.reset();  // release memory

    QScopedPointer<QImage> img(shiftQImage(&item.qimage(), dx, dy));
    item.qimage().swap(*img);
    img.reset();  // release memory
}
}

static bool checkFileName(const HdrCreationItem &item, const QString &str) {
    return (item.filename().compare(str) == 0);
}

void HdrCreationManager::loadFiles(const QStringList &filenames) {
    for (const auto &filename : filenames) {
        qDebug() << QStringLiteral(
                        "HdrCreationManager::loadFiles(): Checking %1")
                        .arg(filename);
        HdrCreationItemContainer::iterator it =
            find_if(m_data.begin(), m_data.end(),
                    boost::bind(&checkFileName, _1, filename));
        // has the file been inserted already?
        if (it == m_data.end()) {
            qDebug() << QStringLiteral(
                            "HdrCreationManager::loadFiles(): \
                            Schedule loading for %1")
                            .arg(filename);
            m_tmpdata.push_back(HdrCreationItem(filename));
        } else {
            qDebug() << QStringLiteral(
                            "HdrCreationManager::loadFiles(): %1 \
                            has already been loaded")
                            .arg(filename);
        }
    }

    // parallel load of the data...
    connect(&m_futureWatcher, &QFutureWatcherBase::finished, this,
            &HdrCreationManager::loadFilesDone, Qt::DirectConnection);

    // Start the computation.
    m_futureWatcher.setFuture(
        QtConcurrent::map(m_tmpdata.begin(), m_tmpdata.end(), LoadFile()));
}

void HdrCreationManager::loadFilesDone() {
    qDebug() << "HdrCreationManager::loadFilesDone(): Data loaded ... move to \
                internal structure!";
    if (m_futureWatcher.isCanceled())  // LoadFile() threw an exception
    {
        emit errorWhileLoading(
            tr("HdrCreationManager::loadFilesDone(): Error loading a file."));
        disconnect(&m_futureWatcher, &QFutureWatcherBase::finished, this,
                   &HdrCreationManager::loadFilesDone);
        m_tmpdata.clear();
        return;
    }

    if (isLoadResponseCurve()) {
        try {
            m_response->readFromFile(
                QFile::encodeName(getResponseCurveInputFilename()).constData());
            setLoadResponseCurve(false);
        } catch (std::runtime_error &e) {
            emit errorWhileLoading(QString(e.what()));
        }
    }
    disconnect(&m_futureWatcher, &QFutureWatcherBase::finished, this,
               &HdrCreationManager::loadFilesDone);
    for (const auto &hdrCreationItem : m_tmpdata) {
        if (hdrCreationItem.isValid()) {
            qDebug() << QStringLiteral(
                            "HdrCreationManager::loadFilesDone(): \
                            Insert data for %1")
                            .arg(hdrCreationItem.filename());
            m_data.push_back(hdrCreationItem);
        }
    }
    m_tmpdata.clear();

    refreshEVOffset();

    if (!framesHaveSameSize()) {
        m_data.clear();
        emit errorWhileLoading(
            tr("HdrCreationManager::loadFilesDone(): The images have different "
               "size."));
    } else {
        emit finishedLoadingFiles();
    }
}

void HdrCreationManager::refreshEVOffset() {
    // no data
    if (m_data.size() <= 0) {
        m_evOffset = 0.f;
        return;
    }

    std::vector<float> evs;
    for (const auto &hdrCreationItem : m_data) {
        if (hdrCreationItem.hasEV()) {
            evs.push_back(hdrCreationItem.getEV());
        }
    }

    // no image has EV
    if (evs.size() <= 0) {
        m_evOffset = 0.f;
        return;
    }

    // only one image available
    if (evs.size() == 1) {
        m_evOffset = evs[0];
        return;
    }

    // sort...
    std::sort(evs.begin(), evs.end());
    m_evOffset = evs[(evs.size() + 1) / 2 - 1];

    qDebug() << QStringLiteral(
                    "HdrCreationManager::refreshEVOffset(): offset = %1")
                    .arg(m_evOffset);
}

float HdrCreationManager::getEVOffset() const { return m_evOffset; }

QStringList HdrCreationManager::getFilesWithoutExif() const {
    QStringList invalidFiles;
    foreach (const HdrCreationItem &fileData, m_data) {
        if (!fileData.hasAverageLuminance()) {
            invalidFiles.push_back(fileData.filename());
        }
    }
    return invalidFiles;
}

size_t HdrCreationManager::numFilesWithoutExif() const {
    size_t counter = 0;
    foreach (const HdrCreationItem &fileData, m_data) {
        if (!fileData.hasAverageLuminance()) {
            ++counter;
        }
    }
    return counter;
}

void HdrCreationManager::removeFile(int idx) {
    Q_ASSERT(idx >= 0);
    Q_ASSERT(idx < (int)m_data.size());

    m_data.erase(m_data.begin() + idx);

    refreshEVOffset();
}

HdrCreationManager::HdrCreationManager(bool fromCommandLine)
    : m_evOffset(0.f),
      m_response(new ResponseCurve(predef_confs[0].responseCurve)),
      m_weight(new WeightFunction(predef_confs[0].weightFunction)),
      m_responseCurveInputFilename(),
      m_agMask(NULL),
      m_align(),
      m_ais_crop_flag(false),
      fromCommandLine(fromCommandLine),
      m_isLoadResponseCurve(false) {
    // setConfig(predef_confs[0]);
    setFusionOperator(predef_confs[0].fusionOperator);

    for (int i = 0; i < agGridSize; i++) {
        for (int j = 0; j < agGridSize; j++) {
            m_patches[i][j] = false;
        }
    }

    connect(&m_futureWatcher, &QFutureWatcherBase::started, this,
            &HdrCreationManager::progressStarted, Qt::DirectConnection);
    connect(&m_futureWatcher, &QFutureWatcherBase::finished, this,
            &HdrCreationManager::progressFinished, Qt::DirectConnection);
    connect(this, &HdrCreationManager::progressCancel, &m_futureWatcher,
            &QFutureWatcherBase::cancel, Qt::DirectConnection);
    connect(&m_futureWatcher, &QFutureWatcherBase::progressRangeChanged, this,
            &HdrCreationManager::progressRangeChanged, Qt::DirectConnection);
    connect(&m_futureWatcher, &QFutureWatcherBase::progressValueChanged, this,
            &HdrCreationManager::progressValueChanged, Qt::DirectConnection);
}

void HdrCreationManager::setConfig(const FusionOperatorConfig &c) {
    if (!c.inputResponseCurveFilename.isEmpty()) {
        setLoadResponseCurve(true);
        setResponseCurveInputFilename(c.inputResponseCurveFilename);
    } else {
        m_response->setType(c.responseCurve);
    }
    getWeightFunction().setType(c.weightFunction);
    setFusionOperator(c.fusionOperator);
}

QVector<float> HdrCreationManager::getExpotimes() const {
    QVector<float> expotimes;
    for (HdrCreationItemContainer::const_iterator it = m_data.begin(),
                                                  itEnd = m_data.end();
         it != itEnd; ++it) {
        expotimes.push_back(it->getEV());
    }
    return expotimes;
}

bool HdrCreationManager::framesHaveSameSize() {
    size_t width = m_data[0].frame()->getWidth();
    size_t height = m_data[0].frame()->getHeight();
    for (HdrCreationItemContainer::const_iterator it = m_data.begin() + 1,
                                                  itEnd = m_data.end();
         it != itEnd; ++it) {
        if (it->frame()->getWidth() != width ||
            it->frame()->getHeight() != height)
            return false;
    }
    return true;
}

void HdrCreationManager::align_with_mtb() {
    // build temporary container...
    vector<FramePtr> frames;
    for (size_t i = 0; i < m_data.size(); ++i) {
        frames.push_back(m_data[i].frame());
    }

    // run MTB
    libhdr::mtb_alignment(frames);

    // rebuild previews
    QFutureWatcher<void> futureWatcher;
    futureWatcher.setFuture(
        QtConcurrent::map(m_data.begin(), m_data.end(), RefreshPreview()));
    futureWatcher.waitForFinished();

    // emit finished
    emit finishedAligning(0);
}

void HdrCreationManager::set_ais_crop_flag(bool flag) {
    m_ais_crop_flag = flag;
}

void HdrCreationManager::align_with_ais() {
    m_align.reset(new Align(m_data, fromCommandLine, 1));
    connect(m_align.get(), &Align::finishedAligning, this,
            &HdrCreationManager::finishedAligning);
    connect(m_align.get(), &Align::failedAligning, this,
            &HdrCreationManager::ais_failed);
    connect(m_align.get(), &Align::failedAligning, this,
            &HdrCreationManager::ais_failed_slot);
    connect(m_align.get(), &Align::dataReady, this,
            &HdrCreationManager::aisDataReady);

    m_align->align_with_ais(m_ais_crop_flag);
}

void HdrCreationManager::ais_failed_slot(QProcess::ProcessError error) {
    qDebug() << "align_image_stack failed";
    m_align->removeTempFiles();
}

pfs::Frame *HdrCreationManager::createHdr() {
    std::vector<FrameEnhanced> frames;

    for (size_t idx = 0; idx < m_data.size(); ++idx) {
        frames.push_back(
            FrameEnhanced(m_data[idx].frame(),
                          std::pow(2.f, m_data[idx].getEV() - m_evOffset)));
    }

    libhdr::fusion::FusionOperatorPtr fusionOperatorPtr =
        IFusionOperator::build(m_fusionOperator);
    pfs::Frame *outputFrame(
        fusionOperatorPtr->computeFusion(*m_response, *m_weight, frames));

    if (!m_responseCurveOutputFilename.isEmpty()) {
        m_response->writeToFile(
            QFile::encodeName(m_responseCurveOutputFilename).constData());
    }

    return outputFrame;
}

void HdrCreationManager::applyShiftsToItems(
    const QList<QPair<int, int>> &hvOffsets) {
    int size = m_data.size();
    // shift the frames and images
    for (int i = 0; i < size; i++) {
        if (hvOffsets[i].first == hvOffsets[i].second &&
            hvOffsets[i].first == 0) {
            continue;
        }
        shiftItem(m_data[i], hvOffsets[i].first, hvOffsets[i].second);
    }
}

void HdrCreationManager::cropItems(const QRect &ca) {
    // crop all frames and images
    int size = m_data.size();
    for (int idx = 0; idx < size; idx++) {
        std::unique_ptr<QImage> newimage(
            new QImage(m_data[idx].qimage().copy(ca)));
        if (newimage == NULL) {
            exit(1);  // TODO: exit gracefully
        }
        m_data[idx].qimage().swap(*newimage);
        newimage.reset();

        int x_ul, y_ur, x_bl, y_br;
        ca.getCoords(&x_ul, &y_ur, &x_bl, &y_br);

        FramePtr cropped(
            cut(m_data[idx].frame().get(), static_cast<size_t>(x_ul),
                static_cast<size_t>(y_ur), static_cast<size_t>(x_bl),
                static_cast<size_t>(y_br)));
        m_data[idx].frame().swap(cropped);
        cropped.reset();
    }
}

HdrCreationManager::~HdrCreationManager() {
    this->reset();
    delete m_agMask;
}

void HdrCreationManager::saveImages(const QString &prefix) {
    int idx = 0;
    for (HdrCreationItemContainer::const_iterator it = m_data.begin(),
                                                  itEnd = m_data.end();
         it != itEnd; ++it) {
        QString filename = prefix + QStringLiteral("_%1").arg(idx) + ".tiff";
        pfs::io::TiffWriter writer(QFile::encodeName(filename).constData());
        writer.write(*it->frame(), pfs::Params("tiff_mode", 1));

        QFileInfo qfi(filename);
        QString absoluteFileName = qfi.absoluteFilePath();
        QByteArray encodedName = QFile::encodeName(absoluteFileName);
        ExifOperations::copyExifData(
            QFile::encodeName(it->filename()).constData(),
            encodedName.constData(), false);
        ++idx;
    }
    emit imagesSaved();
}

int HdrCreationManager::computePatches(float threshold,
                                       bool patches[][agGridSize],
                                       float &percent,
                                       QList<QPair<int, int>> HV_offset) {
    qDebug() << "HdrCreationManager::computePatches";
    qDebug() << threshold;
#ifdef TIMER_PROFILING
    msec_timer stop_watch;
    stop_watch.start();
#endif
    const int width = m_data[0].frame()->getWidth();
    const int height = m_data[0].frame()->getHeight();
    const int gridX = width / agGridSize;
    const int gridY = height / agGridSize;
    const int size = m_data.size();
    assert(size >= 2);

    vector<float> HE(size);

    hueSquaredMean(m_data, HE);

    m_agGoodImageIndex = findIndex(HE.data(), size);
    qDebug() << "h0: " << m_agGoodImageIndex;

    for (int j = 0; j < agGridSize; j++) {
        for (int i = 0; i < agGridSize; i++) {
            m_patches[i][j] = false;
        }
    }

    for (int h = 0; h < size; h++) {
        if (h == m_agGoodImageIndex) continue;
        float deltaEV = log2(m_data[m_agGoodImageIndex].getAverageLuminance()) -
                        log2(m_data[h].getAverageLuminance());
        int dx = HV_offset[m_agGoodImageIndex].first - HV_offset[h].first;
        int dy = HV_offset[m_agGoodImageIndex].second - HV_offset[h].second;
        float sR, sG, sB;
        sdv(m_data[m_agGoodImageIndex], m_data[h], deltaEV, dx, dy, sR, sG, sB);
        //#pragma omp parallel for schedule(static)
        for (int j = 0; j < agGridSize; j++) {
            for (int i = 0; i < agGridSize; i++) {
                if (comparePatches(m_data[m_agGoodImageIndex], m_data[h], i, j,
                                   gridX, gridY, threshold, sR, sG, sB, deltaEV,
                                   dx, dy)) {
                    m_patches[i][j] = true;
                }
            }
        }
    }

    int count = 0;
    for (int i = 0; i < agGridSize; i++)
        for (int j = 0; j < agGridSize; j++)
            if (m_patches[i][j] == true) count++;
    percent = static_cast<float>(count) /
              static_cast<float>(agGridSize * agGridSize) * 100.0f;
    qDebug() << "Total patches: " << percent << "%";

    memcpy(patches, m_patches, agGridSize * agGridSize);

#ifdef TIMER_PROFILING
    stop_watch.stop_and_update();
    std::cout << "computePatches = " << stop_watch.get_time() << " msec"
              << std::endl;
#endif
    return m_agGoodImageIndex;
}

pfs::Frame *HdrCreationManager::doAntiGhosting(bool patches[][agGridSize],
                                               int h0, bool manualAg,
                                               ProgressHelper *ph) {
    qDebug() << "HdrCreationManager::doAntiGhosting";
#ifdef TIMER_PROFILING
    msec_timer stop_watch;
    stop_watch.start();
#endif
    const int width = m_data[0].frame()->getWidth();
    const int height = m_data[0].frame()->getHeight();
    const int gridX = width / agGridSize;
    const int gridY = height / agGridSize;
    connect(ph, &ProgressHelper::qtSetRange, this,
            &HdrCreationManager::progressRangeChanged);
    connect(ph, &ProgressHelper::qtSetValue, this,
            &HdrCreationManager::progressValueChanged);
    ph->setRange(0, 100);
    ph->setValue(0);
    emit progressStarted();

    float cmax[3];
    float cmin[3];
    float Max, Min;

    const Channel *Good_Rc, *Good_Gc, *Good_Bc;
    Channel *Ch_Good[3];
    m_data[h0].frame().get()->getXYZChannels(Ch_Good[0], Ch_Good[1],
                                             Ch_Good[2]);

    Good_Rc = Ch_Good[0];
    Good_Gc = Ch_Good[1];
    Good_Bc = Ch_Good[2];

    const Channel *Rc, *Gc, *Bc;
    Channel *Ch[3];
    std::unique_ptr<Frame> ghosted(createHdr());
    ghosted->getXYZChannels(Ch[0], Ch[1], Ch[2]);

    for (int c = 0; c < 3; c++) {
        cmax[c] = *max_element(Ch[c]->begin(), Ch[c]->end());
        cmin[c] = *min_element(Ch[c]->begin(), Ch[c]->end());
    }
    Max = std::max(cmax[0], std::max(cmax[1], cmax[2]));
    Min = std::min(cmin[0], std::min(cmin[1], cmin[2]));

    for (int c = 0; c < 3; c++) {
        replace_if(Ch[c]->begin(), Ch[c]->end(),
                   [](float f) { return !isnormal(f); }, Max);
        replace_if(Ch[c]->begin(), Ch[c]->end(),
                   [](float f) { return !isfinite(f); }, Max);
        transform(Ch[c]->begin(), Ch[c]->end(), Ch[c]->begin(),
                  Normalizer(Min, Max));
    }

    Rc = Ch[0];
    Gc = Ch[1];
    Bc = Ch[2];

    ph->setValue(5);
    if (ph->canceled()) return NULL;

    //Red channel
    std::unique_ptr<Array2Df> logIrradianceGood_R(new Array2Df(width, height));
    computeLogIrradiance(*logIrradianceGood_R, *Good_Rc);
    ph->setValue(10);
    if (ph->canceled()) {
        return NULL;
    }
    std::unique_ptr<Array2Df> logIrradiance_R(new Array2Df(width, height));
    computeLogIrradiance(*logIrradiance_R, *Rc);
    ph->setValue(13);
    if (ph->canceled()) {
        return NULL;
    }
    std::unique_ptr<Array2Df> gradientXGood_R(new Array2Df(width, height));
    std::unique_ptr<Array2Df> gradientYGood_R(new Array2Df(width, height));
    std::unique_ptr<Array2Df> gradientX_R(new Array2Df(width, height));
    std::unique_ptr<Array2Df> gradientY_R(new Array2Df(width, height));
    std::unique_ptr<Array2Df> gradientXBlended_R(new Array2Df(width, height));
    std::unique_ptr<Array2Df> gradientYBlended_R(new Array2Df(width, height));

    computeGradient(*gradientXGood_R, *gradientYGood_R, *logIrradianceGood_R);
    ph->setValue(15);
    if (ph->canceled()) {
        return NULL;
    }
    computeGradient(*gradientX_R, *gradientY_R, *logIrradiance_R);
    ph->setValue(20);
    if (ph->canceled()) {
        return NULL;
    }
    if (manualAg)
        blendGradients(*gradientXBlended_R, *gradientYBlended_R, *gradientX_R,
                       *gradientY_R, *gradientXGood_R, *gradientYGood_R,
                       *m_agMask);
    else
        blendGradients(*gradientXBlended_R, *gradientYBlended_R, *gradientX_R,
                       *gradientY_R, *gradientXGood_R, *gradientYGood_R,
                       patches, gridX, gridY);

    ph->setValue(25);
    if (ph->canceled()) {
        return NULL;
    }

    std::unique_ptr<Array2Df> divergence_R(new Array2Df(width, height));
    computeDivergence(*divergence_R, *gradientXBlended_R, *gradientYBlended_R);
    ph->setValue(28);
    if (ph->canceled()) {
        return NULL;
    }

    qDebug() << "solve_pde";
    solve_pde_dct(*divergence_R, *logIrradiance_R);
    ph->setValue(33);
    if (ph->canceled()) {
        return NULL;
    }

    //Green channel
    std::unique_ptr<Array2Df> logIrradianceGood_G = std::move(logIrradianceGood_R); // reuse of logIrradianceGood_R
    computeLogIrradiance(*logIrradianceGood_G, *Good_Gc);
    ph->setValue(36);
    if (ph->canceled()) {
        return NULL;
    }
    std::unique_ptr<Array2Df> logIrradiance_G(new Array2Df(width, height));
    computeLogIrradiance(*logIrradiance_G, *Gc);
    ph->setValue(38);
    if (ph->canceled()) {
        return NULL;
    }
    std::unique_ptr<Array2Df> gradientXGood_G = std::move(gradientXGood_R);
    std::unique_ptr<Array2Df> gradientYGood_G = std::move(gradientYGood_R);
    std::unique_ptr<Array2Df> gradientX_G = std::move(gradientX_R);
    std::unique_ptr<Array2Df> gradientY_G = std::move(gradientY_R);
    std::unique_ptr<Array2Df> gradientXBlended_G = std::move(gradientXBlended_R);
    std::unique_ptr<Array2Df> gradientYBlended_G = std::move(gradientYBlended_R);

    computeGradient(*gradientXGood_G, *gradientYGood_G, *logIrradianceGood_G);
    ph->setValue(43);
    if (ph->canceled()) {
        return NULL;
    }
    computeGradient(*gradientX_G, *gradientY_G, *logIrradiance_G);
    ph->setValue(48);
    if (ph->canceled()) {
        return NULL;
    }
    if (manualAg)
        blendGradients(*gradientXBlended_G, *gradientYBlended_G, *gradientX_G,
                       *gradientY_G, *gradientXGood_G, *gradientYGood_G,
                       *m_agMask);
    else
        blendGradients(*gradientXBlended_G, *gradientYBlended_G, *gradientX_G,
                       *gradientY_G, *gradientXGood_G, *gradientYGood_G,
                       patches, gridX, gridY);

    ph->setValue(53);
    if (ph->canceled()) {
        return NULL;
    }

    std::unique_ptr<Array2Df> divergence_G(new Array2Df(width, height));
    computeDivergence(*divergence_G, *gradientXBlended_G, *gradientYBlended_G);
    ph->setValue(60);
    if (ph->canceled()) {
        return NULL;
    }

    qDebug() << "solve_pde";
    solve_pde_dct(*divergence_G, *logIrradiance_G);
    ph->setValue(66);
    if (ph->canceled()) {
        return NULL;
    }

    //Blue channel
    std::unique_ptr<Array2Df> logIrradianceGood_B = std::move(logIrradianceGood_G); // reuse of logIrradianceGood_R
    computeLogIrradiance(*logIrradianceGood_B, *Good_Bc);
    ph->setValue(71);
    if (ph->canceled()) {
        return NULL;
    }
    std::unique_ptr<Array2Df> logIrradiance_B(new Array2Df(width, height));
    computeLogIrradiance(*logIrradiance_B, *Bc);
    ph->setValue(76);
    if (ph->canceled()) {
        return NULL;
    }
    std::unique_ptr<Array2Df> gradientXGood_B = std::move(gradientXGood_G);
    std::unique_ptr<Array2Df> gradientYGood_B = std::move(gradientYGood_G);
    std::unique_ptr<Array2Df> gradientX_B = std::move(gradientX_G);
    std::unique_ptr<Array2Df> gradientY_B = std::move(gradientY_G);
    std::unique_ptr<Array2Df> gradientXBlended_B = std::move(gradientXBlended_G);
    std::unique_ptr<Array2Df> gradientYBlended_B = std::move(gradientYBlended_G);

    computeGradient(*gradientXGood_B, *gradientYGood_B, *logIrradianceGood_B);
    ph->setValue(81);
    if (ph->canceled()) {
        return NULL;
    }
    computeGradient(*gradientX_B, *gradientY_B, *logIrradiance_B);
    ph->setValue(86);
    if (ph->canceled()) {
        return NULL;
    }
    if (manualAg)
        blendGradients(*gradientXBlended_B, *gradientYBlended_B, *gradientX_B,
                       *gradientY_B, *gradientXGood_B, *gradientYGood_B,
                       *m_agMask);
    else
        blendGradients(*gradientXBlended_B, *gradientYBlended_B, *gradientX_B,
                       *gradientY_B, *gradientXGood_B, *gradientYGood_B,
                       patches, gridX, gridY);

    ph->setValue(90);
    if (ph->canceled()) {
        return NULL;
    }

    std::unique_ptr<Array2Df> divergence_B(new Array2Df(width, height));
    computeDivergence(*divergence_B, *gradientXBlended_B, *gradientYBlended_B);
    ph->setValue(93);
    if (ph->canceled()) {
        return NULL;
    }

    qDebug() << "solve_pde";
    solve_pde_dct(*divergence_B, *logIrradiance_B);
    ph->setValue(94);
    if (ph->canceled()) {
        return NULL;
    }

    //Blend
    Frame *deghosted = new Frame(width, height);
    // Channel *Urc, *Ugc, *Ubc;
    Channel *Uc[3];
    deghosted->createXYZChannels(Uc[0], Uc[1], Uc[2]);

    computeIrradiance(*Uc[0], *logIrradiance_R);
    ph->setValue(95);
    if (ph->canceled()) {
        delete deghosted;
        return NULL;
    }
    computeIrradiance(*Uc[1], *logIrradiance_G);
    ph->setValue(97);
    if (ph->canceled()) {
        delete deghosted;
        return NULL;
    }
    computeIrradiance(*Uc[2], *logIrradiance_B);
    ph->setValue(99);
    if (ph->canceled()) {
        delete deghosted;
        return NULL;
    }
    // shadesOfGrayAWB(*Uc[0], *Uc[1], *Uc[2]);

    for (int c = 0; c < 3; c++) {
        cmax[c] = *max_element(Uc[c]->begin(), Uc[c]->end());
        cmin[c] = *min_element(Uc[c]->begin(), Uc[c]->end());
    }
    Max = std::max(cmax[0], std::max(cmax[1], cmax[2]));
    Min = std::min(cmin[0], std::min(cmin[1], cmin[2]));

    for (int c = 0; c < 3; c++) {
        replace_if(Uc[c]->begin(), Uc[c]->end(),
                   [](float f) { return !isnormal(f); }, Max);
        replace_if(Uc[c]->begin(), Uc[c]->end(),
                   [](float f) { return !isfinite(f); }, Max);
        transform(Uc[c]->begin(), Uc[c]->end(), Uc[c]->begin(),
                  Normalizer(Min, Max));
    }

    ph->setValue(100);

    emit progressFinished();
    //this->reset();
#ifdef TIMER_PROFILING
    stop_watch.stop_and_update();
    std::cout << "doAntiGhosting = " << stop_watch.get_time() << " msec"
              << std::endl;
#endif
    return deghosted;
}

void HdrCreationManager::getAgData(bool patches[][agGridSize], int &h0) {
    memcpy(patches, m_patches, agGridSize * agGridSize);

    h0 = m_agGoodImageIndex;
}

void HdrCreationManager::setPatches(bool patches[][agGridSize]) {
    memcpy(m_patches, patches, agGridSize * agGridSize);
}

void HdrCreationManager::reset() {
    if (m_align != NULL) {
        m_align->reset();
    }

    if (m_futureWatcher.isRunning()) {
        qDebug() << "Aborting loadFiles...";
        m_futureWatcher.cancel();
        m_futureWatcher.waitForFinished();
        emit loadFilesAborted();
    }

    disconnect(&m_futureWatcher, &QFutureWatcherBase::finished, this,
               &HdrCreationManager::loadFilesDone);
    m_data.clear();
    m_tmpdata.clear();
}
