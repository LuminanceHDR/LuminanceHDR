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

#include <QDebug>
#include <QApplication>
#include <QFileInfo>
#include <QFile>
#include <QColor>
#include <QtConcurrentMap>
#include <QtConcurrentFilter>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <vector>
#include <boost/make_shared.hpp>
#include <boost/bind.hpp>
#include <boost/foreach.hpp>
#include <boost/numeric/conversion/bounds.hpp>
#include <boost/limits.hpp>

#include <Libpfs/frame.h>
#include <Libpfs/utils/msec_timer.h>
#include <Libpfs/io/tiffwriter.h>
#include <Libpfs/io/tiffreader.h>
#include <Libpfs/io/framereader.h>
#include <Libpfs/io/framereaderfactory.h>
#include <Libpfs/io/framewriter.h>
#include <Libpfs/io/framewriterfactory.h>
#include <Libpfs/utils/transform.h>
#include <Libpfs/manip/cut.h>
#include <Libpfs/colorspace/convert.h>
#include <Libpfs/colorspace/rgbremapper.h>
#include "Libpfs/colorspace/colorspace.h"

#include "TonemappingOperators/fattal02/pde.h"
#include "Exif/ExifOperations.h"
#include "HdrCreation/mtb_alignment.h"
#include "HdrCreationManager.h"
#include "arch/math.h"

using namespace std;
using namespace pfs;
using namespace pfs::io;

const config_triple predef_confs[6]= {
    {TRIANGULAR, LINEAR,DEBEVEC, "", ""},
    {TRIANGULAR, GAMMA, DEBEVEC, "", ""},
    {PLATEAU, LINEAR, DEBEVEC, "", ""},
    {PLATEAU, GAMMA, DEBEVEC, "", ""},
    {GAUSSIAN, LINEAR, DEBEVEC, "", ""},
    {GAUSSIAN, GAMMA, DEBEVEC, "", ""},
};


// --- NEW CODE ---


struct ConvertToQRgb {
    void operator()(float r, float g, float b, QRgb& rgb) const {
        uint8_t r8u = colorspace::convertSample<uint8_t>(r);
        uint8_t g8u = colorspace::convertSample<uint8_t>(g);
        uint8_t b8u = colorspace::convertSample<uint8_t>(b);

        rgb = qRgb(r8u, g8u, b8u);
    }
};

struct LoadFile {
    void operator()(HdrCreationItem& currentItem)
    {
        QFileInfo qfi(currentItem.filename());
        qDebug() << QString("Loading data for %1").arg(currentItem.filename());

        try
        {
            FrameReaderPtr reader = FrameReaderFactory::open(
                        QFile::encodeName(qfi.filePath()).constData() );
            reader->read( *currentItem.frame(), Params() );

            // read Average Luminance
            currentItem.setAverageLuminance(
                        ExifOperations::getAverageLuminance(
                            QFile::encodeName(qfi.filePath()).constData() )
                        );

            // read Exposure Time
            currentItem.setExposureTime(
                        ExifOperations::getExposureTime(
                            QFile::encodeName(qfi.filePath()).constData() )
                        );

            qDebug() << QString("HdrCreationItem: Average Luminance for %1 is %2")
                        .arg(currentItem.filename())
                        .arg(currentItem.getAverageLuminance());

            // build QImage
            QImage tempImage(currentItem.frame()->getWidth(),
                             currentItem.frame()->getHeight(),
                             QImage::Format_ARGB32_Premultiplied);

            QRgb* qimageData = reinterpret_cast<QRgb*>(tempImage.bits());

            Channel* red;
            Channel* green;
            Channel* blue;
            currentItem.frame()->getXYZChannels(red, green, blue);

            if (red == NULL || green == NULL || blue == NULL)
                throw std::runtime_error("Null frame");

            utils::transform(red->begin(), red->end(), green->begin(), blue->begin(),
                             qimageData, ConvertToQRgb());

            currentItem.qimage()->swap( tempImage );
        }
        catch (std::runtime_error& err)
        {
            qDebug() << QString("Cannot load %1: %2")
                        .arg(currentItem.filename())
                        .arg(QString::fromStdString(err.what()));
        }
    }
};

struct RefreshPreview {
    void operator()(HdrCreationItem& currentItem)
    {
        qDebug() << QString("Refresh preview for %1").arg(currentItem.filename());

        try
        {
            // build QImage
            QImage tempImage(currentItem.frame()->getWidth(),
                             currentItem.frame()->getHeight(),
                             QImage::Format_ARGB32_Premultiplied);

            QRgb* qimageData = reinterpret_cast<QRgb*>(tempImage.bits());

            Channel* red;
            Channel* green;
            Channel* blue;
            currentItem.frame()->getXYZChannels(red, green, blue);

            utils::transform(red->begin(), red->end(), green->begin(), blue->begin(),
                             qimageData, ConvertToQRgb());

            currentItem.qimage()->swap( tempImage );
        }
        catch (std::runtime_error& err)
        {
            qDebug() << QString("Cannot load %1: %2")
                        .arg(currentItem.filename())
                        .arg(QString::fromStdString(err.what()));
        }
    }
};

struct SaveFile {
    void operator()(HdrCreationItem& currentItem)
    {
        QString inputFilename = currentItem.filename();
        QFileInfo qfi(inputFilename);
        QString base = qfi.completeBaseName(); 
        QString filename = base + ".tif";
        QString tempdir = LuminanceOptions().getTempDir();
        qDebug() << QString("Saving data for %1 on %2").arg(filename).arg(tempdir);
        

        QString completeFilename = tempdir + "/" + filename;

        // save pfs::Frame as tiff 16bits
        try
        {
            Params p;
            p.set("tiff_mode", 1); // 16bits
            FrameWriterPtr writer = FrameWriterFactory::open(
                        QFile::encodeName(completeFilename).constData());
            writer->write( *currentItem.frame(), p );

        }
        catch (std::runtime_error& err)
        {
            qDebug() << QString("Cannot save %1: %2")
                        .arg(currentItem.filename())
                        .arg(QString::fromStdString(err.what()));
        }
    }
};

static
bool checkFileName(const HdrCreationItem& item, const QString& str) {
    return (item.filename().compare(str) == 0);
}
  
void HdrCreationManager::loadFiles(const QStringList &filenames)
{
#ifndef LHDR_CXX11_ENABLED
    for (const QString& i, filenames) {
#else
    BOOST_FOREACH(const QString& i, filenames) {
#endif
        qDebug() << QString("Checking %1").arg(i);
        HdrCreationItemContainer::iterator it = find_if(m_data.begin(), m_data.end(),
                                                        boost::bind(&checkFileName, _1, i));
        // has the file been inserted already?
        if ( it == m_data.end() ) {
            qDebug() << QString("Schedule loading for %1").arg(i);
            m_tmpdata.push_back( HdrCreationItem(i) );
        }
    }

    // parallel load of the data...
    connect(&m_futureWatcher, SIGNAL(finished()), this, SLOT(loadFilesDone()), Qt::DirectConnection);

    // Start the computation.
    m_futureWatcher.setFuture( QtConcurrent::map(m_tmpdata.begin(), m_tmpdata.end(), LoadFile()) );
}

void HdrCreationManager::loadFilesDone()
{ 
    qDebug() << "Data loaded ... move to internal structure!";
    disconnect(&m_futureWatcher, SIGNAL(finished()), this, SLOT(loadFilesDone()));
    BOOST_FOREACH(const HdrCreationItem& i, m_tmpdata) {
        if ( i.isValid() ) {
            qDebug() << QString("Insert data for %1").arg(i.filename());
            m_data.push_back(i);
        }
    }
    //qDebug() << QString("Read %1 out of %2").arg(m_tmpdata.size()).arg(filenames.size());

    if (!framesHaveSameSize()) {
        emit errorWhileLoading(tr("The images have different size."));
        m_data.clear();
    }
    else
        emit finishedLoadingFiles();
}

QStringList HdrCreationManager::getFilesWithoutExif() const
{
    QStringList invalidFiles;
    foreach (const HdrCreationItem& fileData, m_data) {
        if ( !fileData.hasAverageLuminance() ) {
            invalidFiles.push_back( fileData.filename() );
        }
    }
    return invalidFiles;
}

size_t HdrCreationManager::numFilesWithoutExif() const {
    size_t counter = 0;
    foreach (const HdrCreationItem& fileData, m_data) {
        if ( !fileData.hasAverageLuminance() ) {
            ++counter;
        }
    }
    return counter;
}

void HdrCreationManager::removeFile(int idx)
{
    Q_ASSERT(idx >= 0);
    Q_ASSERT(idx < (int)m_data.size());

    m_data.erase(m_data.begin() + idx);
}

using namespace libhdr::fusion;
HdrCreationManager::HdrCreationManager(bool fromCommandLine)
    : chosen_config( predef_confs[0] )
    , m_agMask( NULL )
    , ais( NULL )
    , m_ais_crop_flag(false)
    , fromCommandLine( fromCommandLine )
{
    for (int i = 0; i < agGridSize; i++)
        for (int j = 0; j < agGridSize; j++)
            m_patches[i][j] = false;

    connect(&m_futureWatcher, SIGNAL(started()), this, SIGNAL(progressStarted()), Qt::DirectConnection);
    connect(&m_futureWatcher, SIGNAL(finished()), this, SIGNAL(progressFinished()), Qt::DirectConnection);
    connect(this, SIGNAL(progressCancel()), &m_futureWatcher, SLOT(cancel()), Qt::DirectConnection);
    connect(&m_futureWatcher, SIGNAL(progressRangeChanged(int,int)), this, SIGNAL(progressRangeChanged(int,int)), Qt::DirectConnection);
    connect(&m_futureWatcher, SIGNAL(progressValueChanged(int)), this, SIGNAL(progressValueChanged(int)), Qt::DirectConnection);
}

void HdrCreationManager::setConfig(const config_triple &c)
{
    chosen_config = c;
}

const QVector<float> HdrCreationManager::getExpotimes() const
{
    QVector<float> expotimes;
    for ( HdrCreationItemContainer::const_iterator it = m_data.begin(), 
          itEnd = m_data.end(); it != itEnd; ++it) {
        expotimes.push_back(it->getEV());
    }
    return expotimes;
}

bool HdrCreationManager::framesHaveSameSize()
{
    size_t width = m_data[0].frame()->getWidth();
    size_t height = m_data[0].frame()->getHeight();
    for ( HdrCreationItemContainer::const_iterator it = m_data.begin() + 1, 
          itEnd = m_data.end(); it != itEnd; ++it) {
        if (it->frame()->getWidth() != width || it->frame()->getHeight() != height)
            return false; 
    }
    return true;
}

void HdrCreationManager::align_with_mtb()
{
    // build temporary container...
    vector<FramePtr> frames;
    for (size_t i = 0; i < m_data.size(); ++i) {
        frames.push_back( m_data[i].frame() );
    }

    // run MTB
    libhdr::mtb_alignment(frames);

    // rebuild previews
    QFutureWatcher<void> futureWatcher;
    futureWatcher.setFuture( QtConcurrent::map(m_data.begin(), m_data.end(), RefreshPreview()) );
    futureWatcher.waitForFinished();

    // emit finished
    emit finishedAligning(0);
}

void HdrCreationManager::set_ais_crop_flag(bool flag)
{
    m_ais_crop_flag = flag;
}

void HdrCreationManager::align_with_ais()
{
    ais = new QProcess(this);
    if (ais == NULL) exit(1);       // TODO: exit gracefully
    if (!fromCommandLine) {
        ais->setWorkingDirectory(m_luminance_options.getTempDir());
    }
    QStringList env = QProcess::systemEnvironment();
#ifdef WIN32
    QString separator(";");
#else
    QString separator(":");
#endif
    env.replaceInStrings(QRegExp("^PATH=(.*)", Qt::CaseInsensitive), "PATH=\\1"+separator+QCoreApplication::applicationDirPath());
    ais->setEnvironment(env);
    connect(ais, SIGNAL(finished(int,QProcess::ExitStatus)), this, SLOT(ais_finished(int,QProcess::ExitStatus)));
    connect(ais, SIGNAL(error(QProcess::ProcessError)), this, SIGNAL(ais_failed(QProcess::ProcessError)));
    connect(ais, SIGNAL(error(QProcess::ProcessError)), this, SLOT(ais_failed_slot(QProcess::ProcessError)));
    connect(ais, SIGNAL(readyRead()), this, SLOT(readData()));
    
    QStringList ais_parameters = m_luminance_options.getAlignImageStackOptions();

    if (m_ais_crop_flag) { ais_parameters << "-C"; }

    QFutureWatcher<void> futureWatcher;

    // Start the computation.
    futureWatcher.setFuture( QtConcurrent::map(m_data.begin(), m_data.end(), SaveFile()) );
    futureWatcher.waitForFinished();

    if (futureWatcher.isCanceled()) return;

    for ( HdrCreationItemContainer::const_iterator it = m_data.begin(), 
          itEnd = m_data.end(); it != itEnd; ++it) {
        QFileInfo qfi(it->filename());
        QString base = qfi.completeBaseName(); 
        QString filename = base + ".tif";
        QString tempdir = m_luminance_options.getTempDir();
        QString completeFilename = tempdir + "/" + filename;
        ais_parameters << completeFilename; 
    }
    qDebug() << "ais_parameters " << ais_parameters;
#ifdef Q_WS_MAC
    qDebug() << QCoreApplication::applicationDirPath()+"/align_image_stack";
    ais->start(QCoreApplication::applicationDirPath()+"/align_image_stack", ais_parameters );
#else
    ais->start("align_image_stack", ais_parameters );
#endif
    qDebug() << "ais started";
}

void HdrCreationManager::ais_finished(int exitcode, QProcess::ExitStatus exitstatus)
{
    if (exitstatus != QProcess::NormalExit)
    {
        qDebug() << "ais failed";
        //emit ais_failed(QProcess::Crashed);
        return;
    }
    if (exitcode == 0)
    {
        // TODO: try-catch
        // DAVIDE _ HDR CREATION
        m_tmpdata.clear();
        int i = 0;
        for ( HdrCreationItemContainer::iterator it = m_data.begin(), 
              itEnd = m_data.end(); it != itEnd; ++it) {
            QString inputFilename = it->filename(), filename;
            if (!fromCommandLine)
                filename = QString(m_luminance_options.getTempDir() + "/aligned_" + QString("%1").arg(i,4,10,QChar('0'))+".tif");
            else
                filename = QString("aligned_" + QString("%1").arg(i,4,10,QChar('0'))+".tif");
            
            m_tmpdata.push_back( HdrCreationItem(filename) );
            ExifOperations::copyExifData(inputFilename.toStdString(), filename.toStdString(), false, "", false, true); 
            i++;
        }

        // parallel load of the data...
        // Start the computation.
        connect(&m_futureWatcher, SIGNAL(finished()), this, SLOT(alignedFilesLoaded()), Qt::DirectConnection);
        m_futureWatcher.setFuture( QtConcurrent::map(m_tmpdata.begin(), m_tmpdata.end(), LoadFile()) );
    }
    else
    {
        qDebug() << "align_image_stack exited with exit code " << exitcode;
        emit finishedAligning(exitcode);
    }
}

void HdrCreationManager::alignedFilesLoaded()
{
    disconnect(&m_futureWatcher, SIGNAL(finished()), this, SLOT(alignedFilesLoaded()));
    for ( HdrCreationItemContainer::iterator it = m_data.begin(), 
          itEnd = m_data.end(); it != itEnd; ++it) {
        QFileInfo qfi(it->filename());
        QString base = qfi.completeBaseName(); 
        QString filename = base + ".tif";
        QString tempdir = m_luminance_options.getTempDir();
        QString completeFilename = tempdir + "/" + filename;
        QFile::remove(QFile::encodeName(completeFilename).constData());
        qDebug() << "void HdrCreationManager::ais_finished: remove " << filename;
    }

    m_data.swap(m_tmpdata);
    QFile::remove(m_luminance_options.getTempDir() + "/hugin_debug_optim_results.txt");
    emit finishedAligning(0);
}

void HdrCreationManager::ais_failed_slot(QProcess::ProcessError error)
{
    qDebug() << "align_image_stack failed";
}

void HdrCreationManager::removeTempFiles()
{
    int i = 0;
    for ( HdrCreationItemContainer::iterator it = m_data.begin(), 
          itEnd = m_data.end(); it != itEnd; ++it) {
        QString filename;
        if (!fromCommandLine) {
            filename = QString(m_luminance_options.getTempDir() + "/aligned_" + QString("%1").arg(i,4,10,QChar('0'))+".tif");
        }
        else {
            filename = QString("aligned_" + QString("%1").arg(i,4,10,QChar('0'))+".tif");
        }
        QFile::remove(filename);
        qDebug() << "void HdrCreationManager::ais_finished: remove " << filename;
        ++i;
    }
}

/*
void HdrCreationManager::removeTempFiles()
{
    foreach (QString tempfname, filesToRemove) {
        qDebug() << "void HdrCreationManager::removeTempFiles(): "
                 << qPrintable(tempfname);
        if (!tempfname.isEmpty()) {
            QFile::remove(tempfname);
        }
    }
    filesToRemove.clear();
}

void HdrCreationManager::checkEVvalues()
{
    float max=-20, min=+20;
    for (int i = 0; i < fileList.size(); i++) {
        float ev_val = log2f(expotimes[i]);
        if (ev_val > max)
            max = ev_val;
        if (ev_val < min)
            min = ev_val;
    }
    //now if values are out of bounds, add an offset to them.
    if (max > 10) {
        for (int i = 0; i < fileList.size(); i++) {
            float new_ev = log2f(expotimes[i]) - (max - 10);
            expotimes[i] = exp2f(new_ev);
            emit expotimeValueChanged(exp2f(new_ev), i);
        }
    } else if (min < -10) {
        for (int i = 0; i < fileList.size(); i++) {
            float new_ev = log2f(expotimes[i]) - (min + 10);
            expotimes[i] = exp2f(new_ev);
            emit expotimeValueChanged(exp2f(new_ev), i);
        }
    }
    //qDebug("HCM::END checkEVvalues");
}

void HdrCreationManager::setEV(float new_ev, int image_idx)
{
    if (expotimes[image_idx] == -1) {
        //remove always the first one
        //after the initial printing we only need to know the size of the list
        filesLackingExif.removeAt(0);
    }
    expotimes[image_idx] = exp2f(new_ev);
    emit expotimeValueChanged(exp2f(new_ev), image_idx);
}
*/


pfs::Frame* HdrCreationManager::createHdr(bool /*ag*/, int /*iterations*/)
{
    std::vector< FrameEnhanced > frames;
    for ( size_t idx = 0; idx < m_data.size(); ++idx ) {
        frames.push_back(
                    FrameEnhanced(m_data[idx].frame(),
                                  m_data[idx].getAverageLuminance())
                    );
    }

    libhdr::fusion::FusionOperatorPtr fusionOperatorPtr = IFusionOperator::build(m_fusionOperator);
    fusionOperatorPtr->setResponseFunction(m_responseFunction);
    fusionOperatorPtr->setWeightFunction(m_weightFunction);

    return fusionOperatorPtr->computeFusion( frames );
}

void HdrCreationManager::applyShiftsToItems(const QList<QPair<int,int> >& hvOffsets)
{
    int size = m_data.size();
    //shift the frames and images
    for (int i = 0; i < size; i++)
    {
        if ( hvOffsets[i].first == hvOffsets[i].second &&
             hvOffsets[i].first == 0 )
        {
            continue;
        }
        shiftItem(m_data[i],
                  hvOffsets[i].first,
                  hvOffsets[i].second);
    }
}

void HdrCreationManager::cropItems(const QRect& ca)
{
    //crop all frames and images
    int size = m_data.size();
    for (int idx = 0; idx < size; idx++) {
        QImage *newimage = new QImage(m_data[idx].qimage()->copy(ca));
        if (newimage == NULL)
            exit(1); // TODO: exit gracefully
        m_data[idx].qimage()->swap(*newimage);
        int x_ul, y_ur, x_bl, y_br;
        ca.getCoords(&x_ul, &y_ur, &x_bl, &y_br);
        Frame* cropped = cut(m_data[idx].frame().get(), static_cast<size_t>(x_ul), static_cast<size_t>(y_ur), 
                                                        static_cast<size_t>(x_bl), static_cast<size_t>(y_br));
        FramePtr shared(cropped);
        m_data[idx].frame().swap(shared);
    
    }
    //cropAgMasks(ca);
}

HdrCreationManager::~HdrCreationManager()
{
    if (ais != NULL && ais->state() != QProcess::NotRunning) {
        ais->kill();
        delete ais;
    }
    delete m_agMask;
}

/*
void HdrCreationManager::clearlists(bool deleteExpotimeAsWell)
{
    startedProcessing.clear();
    filesLackingExif.clear();

    if (deleteExpotimeAsWell)
    {
        fileList.clear();
        expotimes.clear();
    }
    if (ldrImagesList.size() != 0)
    {
        qDeleteAll(ldrImagesList);
        ldrImagesList.clear();
        qDeleteAll(m_antiGhostingMasksList);
        m_antiGhostingMasksList.clear();
    }
    if (listmdrR.size()!=0 && listmdrG.size()!=0 && listmdrB.size()!=0)
    {
        Array2DfList::iterator itR=listmdrR.begin(), itG=listmdrG.begin(), itB=listmdrB.begin();
        for (; itR!=listmdrR.end(); itR++,itG++,itB++ )
        {
            delete *itR;
            delete *itG;
            delete *itB;
        }
        listmdrR.clear();
        listmdrG.clear();
        listmdrB.clear();
        qDeleteAll(mdrImagesList);
        mdrImagesList.clear();
        qDeleteAll(mdrImagesToRemove);
        mdrImagesToRemove.clear();
        qDeleteAll(m_antiGhostingMasksList);
        m_antiGhostingMasksList.clear();
    }
}

void HdrCreationManager::makeSureLDRsHaveAlph->()
{
    if (ldrImagesList.at(0)->format()==QImage::Format_RGB32) {
        int origlistsize = ldrImagesList.size();
        for (int image_idx = 0; image_idx < origlistsize; image_idx++) {
            QImage *newimage = new QImage(ldrImagesList.at(0)->convertToFormat(QImage::Format_ARGB32));
            if (newimage == NULL)
                exit(1); // TODO: exit gracefully;
            ldrImagesList.append(newimage);
            delete ldrImagesList.takeAt(0);
        }
    }
}

void HdrCreationManager::applyShiftsToImageStack(const QList<QPair<int,int> >& hvOffsets)
{
    int originalsize = ldrImagesList.count();
    //shift the images
    for (int i = 0; i < originalsize; i++)
    {
        if ( hvOffsets[i].first == hvOffsets[i].second &&
             hvOffsets[i].first == 0 )
        {
            continue;
        }
        QImage *shifted = shiftQImage(ldrImagesList[i],
                                      hvOffsets[i].first,
                                      hvOffsets[i].second);
        delete ldrImagesList.takeAt(i);
        ldrImagesList.insert(i, shifted);
    }
}

void HdrCreationManager::applyShiftsToMdrImageStack(const QList<QPair<int,int> >& hvOffsets)
{
    qDebug() << "HdrCreationManager::applyShiftsToMdrImageStack";
    int originalsize = mdrImagesList.count();
    for (int i = 0; i < originalsize; i++)
    {
        if ( hvOffsets[i].first == hvOffsets[i].second &&
             hvOffsets[i].first == 0 )
        {
            continue;
        }
        pfs::Array2Df *shiftedR = shift(*listmdrR[i],
                                        hvOffsets[i].first,
                                        hvOffsets[i].second);
        pfs::Array2Df *shiftedG = shift(*listmdrG[i],
                                        hvOffsets[i].first,
                                        hvOffsets[i].second);
        pfs::Array2Df *shiftedB = shift(*listmdrB[i],
                                        hvOffsets[i].first,
                                        hvOffsets[i].second);
        delete listmdrR[i];
        delete listmdrG[i];
        delete listmdrB[i];
        listmdrR[i] = shiftedR;
        listmdrG[i] = shiftedG;
        listmdrB[i] = shiftedB;
    }
}


void HdrCreationManager::cropLDR(const QRect& ca)
{
    //crop all the images
    int origlistsize = ldrImagesList.size();
    for (int image_idx = 0; image_idx < origlistsize; image_idx++) {
        QImage *newimage = new QImage(ldrImagesList.at(0)->copy(ca));
        if (newimage == NULL)
            exit(1); // TODO: exit gracefully
        ldrImagesList.append(newimage);
        delete ldrImagesList.takeAt(0);
    }
    cropAgMasks(ca);
}

void HdrCreationManager::cropMDR(const QRect& ca)
{
    int x_ul, y_ul, x_br, y_br;
    ca.getCoords(&x_ul, &y_ul, &x_br, &y_br);

    int newWidth = x_br-x_ul;
    int newHeight = y_br-y_ul;

    
    // crop all the images
    //int origlistsize = listmdrR.size();
    //pfs::Frame *frame;
    //pfs::Channel *Xc, *Yc, *Zc;
    //pfs::Frame *cropped_frame;
    

    // all R channels
    for ( size_t idx = 0; idx < listmdrR.size(); ++idx )
    {
        pfs::Array2Df tmp( newWidth, newHeight );
        pfs::cut(listmdrR[idx], &tmp, x_ul, y_ul, x_br, y_br);
        listmdrR[idx]->swap( tmp );
    }

    // all G channels
    for ( size_t idx = 0; idx < listmdrG.size(); ++idx )
    {
        pfs::Array2Df tmp( newWidth, newHeight );
        pfs::cut(listmdrG[idx], &tmp, x_ul, y_ul, x_br, y_br);
        listmdrG[idx]->swap( tmp );
    }

    // all B channel
    for ( size_t idx = 0; idx < listmdrB.size(); ++idx )
    {
        pfs::Array2Df tmp( newWidth, newHeight );
        pfs::cut(listmdrB[idx], &tmp, x_ul, y_ul, x_br, y_br);
        listmdrB[idx]->swap( tmp );
    }

    for ( int idx = 0; idx < mdrImagesList.size(); ++idx )
    {

        QImage *newimage = new QImage(mdrImagesList.at(0)->copy(ca));
        if (newimage == NULL)
            exit(1); // TODO: exit gracefully
        mdrImagesList.append(newimage);
        mdrImagesToRemove.append(mdrImagesList.takeAt(0));
        QImage *img = new QImage(newWidth, newHeight, QImage::Format_ARGB32);
        img->fill(qRgba(0,0,0,0));
        m_antiGhostingMasksList.append(img);
        m_antiGhostingMasksList.takeAt(0);
    }
    m_mdrWidth = newWidth;
    m_mdrHeight = newHeight;
    cropAgMasks(ca);
}

void HdrCreationManager::reset()
{
    ais = NULL;
    m_shift = 0;
    chosen_config = predef_confs[0];
    inputType = UNKNOWN_INPUT_TYPE;
    fileList.clear();
    // DAVIDE _ HDR CREATION
    // clearlists(true);
    removeTempFiles();
}
*/

/*
void HdrCreationManager::remove(int index)
{
    switch (inputType) {
    case LDR_INPUT_TYPE:
    {
        ldrImagesList.removeAt(index);          
    }
        break;
    case MDR_INPUT_TYPE:
    {
            Array2DfList::iterator itR = listmdrR.begin() + index;
            delete *itR;
            listmdrR.erase(itR);

            Array2DfList::iterator itG = listmdrG.begin() + index;
            delete *itG;
            listmdrG.erase(itG);

            Array2DfList::iterator itB = listmdrB.begin() + index;
            delete *itB;
            listmdrB.erase(itB);
            
            delete mdrImagesList[index];            
            mdrImagesList.removeAt(index);          
            
            QString fname = filesToRemove.at(index);
            qDebug() << "void HdrCreationManager::remove(int index): filename " << fname;
            QFile::remove(fname);
    }
        break;
        // ...in this case, do nothing!
    case UNKNOWN_INPUT_TYPE:
    default:{}
        break;
    }
    fileList.removeAt(index);
    filesToRemove.remove(index);
    expotimes.remove(index);
    startedProcessing.removeAt(index);
}
*/

void HdrCreationManager::readData()
{
    QByteArray data = ais->readAll();
    emit aisDataReady(data);
}

/*

namespace {

inline float toFloat(int value) {
    return (static_cast<float>(value)/255.f);
}

void interleavedToPlanar(const QImage* image,
                         pfs::Array2Df* r, pfs::Array2Df* g, pfs::Array2Df* b)
{
    for (int row = 0; row < image->height(); ++row)
    {
        const QRgb* data = reinterpret_cast<const QRgb*>(image->scanLine(row));
        pfs::Array2Df::iterator itRed = r->row_begin(row);
        pfs::Array2Df::iterator itGreen = g->row_begin(row);
        pfs::Array2Df::iterator itBlue = g->row_begin(row);

        for ( int col = 0; col < image->width(); ++col )
        {
            *itRed++ = toFloat(qRed(*data));
            *itGreen++ = toFloat(qGreen(*data));
            *itBlue++ = toFloat(qBlue(*data));

            ++data;
        }
    }
}

} // anonymous namespace

*/

void HdrCreationManager::saveImages(const QString& prefix)
{
    int idx = 0;
    for ( HdrCreationItemContainer::const_iterator it = m_data.begin(), 
          itEnd = m_data.end(); it != itEnd; ++it) {

        QString filename = prefix + QString("_%1").arg(idx) + ".tiff";
        pfs::io::TiffWriter writer(QFile::encodeName(filename).constData());
        writer.write( *it->frame(), pfs::Params("tiff_mode", 1) );

        QFileInfo qfi(filename);
        QString absoluteFileName = qfi.absoluteFilePath();
        QByteArray encodedName = QFile::encodeName(absoluteFileName);
        ExifOperations::copyExifData(QFile::encodeName(it->filename()).constData(), encodedName.constData(), false);
        ++idx;
    }
    emit imagesSaved();
}
/*
void HdrCreationManager::saveLDRs(const QString& filename)
{
#ifdef QT_DEBUG
    qDebug() << "HdrCreationManager::saveLDRs";
#endif

    for (int idx = 0, origlistsize = ldrImagesList.size(); idx < origlistsize;
         ++idx)
    {
        QImage* currentImage = ldrImagesList[idx];

        QString fname = filename + QString("_%1").arg(idx) + ".tiff";

        pfs::Frame frame(currentImage->width(), currentImage->height());
        pfs::Channel* R;
        pfs::Channel* G;
        pfs::Channel* B;
        frame.createXYZChannels(R, G, B);
        interleavedToPlanar(currentImage, R, G, B);

        pfs::io::TiffWriter writer(QFile::encodeName(fname).constData());
        writer.write( frame, pfs::Params("tiff_mode", 1) );

        // DAVIDE_TIFF
        // TiffWriter writer(QFile::encodeName(fname).constData(), ldrImagesList[idx]);
        // writer.write8bitTiff();

        QFileInfo qfi(filename);
        QString absoluteFileName = qfi.absoluteFilePath();
        QByteArray encodedName = QFile::encodeName(absoluteFileName + QString("_%1").arg(idx) + ".tiff");
        ExifOperations::copyExifData(QFile::encodeName(fileList[idx]).constData(), encodedName.constData(), false);
    }
    emit imagesSaved();
}

void HdrCreationManager::saveMDRs(const QString& filename)
{
#ifdef QT_DEBUG
    qDebug() << "HdrCreationManager::saveMDRs";
#endif

    int origlistsize = listmdrR.size();
    for (int idx = 0; idx < origlistsize; idx++)
    {
        QString fname = filename + QString("_%1").arg(idx) + ".tiff";

//        pfs::Frame *frame = pfs::DOMIO::createFrame( m_mdrWidth, m_mdrHeight );
//        pfs::Channel *Xc, *Yc, *Zc;
//        frame->createXYZChannels( Xc, Yc, Zc );
//        Xc->setChannelData(listmdrR[idx]);
//        Yc->setChannelData(listmdrG[idx]);
//        Zc->setChannelData(listmdrB[idx]);

//        TiffWriter writer(, frame);
//        writer.writePFSFrame16bitTiff();

        pfs::Frame frame( m_mdrWidth, m_mdrHeight );
        pfs::Channel* R;
        pfs::Channel* G;
        pfs::Channel* B;
        frame.createXYZChannels(R, G, B);

        pfs::copy(listmdrR[idx], R);
        pfs::copy(listmdrG[idx], G);
        pfs::copy(listmdrB[idx], B);

        pfs::io::TiffWriter writer( QFile::encodeName(fname).constData() );
        // tiff_mode = 2 (16 bit tiff)
        // min_luminance = 0
        // max_luminance = 2^16 - 1
        // note: this is due to the fact the reader do read the native
        // data into float, without doing any conversion into the [0, 1] range
        // (definitely something to think about when we touch the readers)
        writer.write(frame,
                     pfs::Params("tiff_mode", 2)
                        ("min_luminance", (float)0)
                        ("max_luminance", (float)65535) );

        QFileInfo qfi(filename);
        QString absoluteFileName = qfi.absoluteFilePath();
        QByteArray encodedName = QFile::encodeName(
                    absoluteFileName + QString("_%1").arg(idx) + ".tiff");
        ExifOperations::copyExifData(
                    QFile::encodeName(fileList[idx]).constData(),
                    encodedName.constData(), false);
    }
    emit imagesSaved();
}
*/
/*
void HdrCreationManager::doAntiGhosting(int goodImageIndex)
{
    Channel *red_goodImage, *green_goodImage, *blue_goodImage;
    m_data[goodImageIndex].frame()->getXYZChannels( red_goodImage, green_goodImage, blue_goodImage);
    Array2Df& R_goodImage = *red_goodImage;
    Array2Df& G_goodImage = *green_goodImage;
    Array2Df& B_goodImage = *blue_goodImage;

    int size = m_data.size();
    for (int idx = 0; idx < size; idx++) {
        if (idx == goodImageIndex) continue;
        Channel *red, *green, *blue;
        m_data[idx].frame()->getXYZChannels( red, green, blue);
        Array2Df& R = *red;
        Array2Df& G = *green;
        Array2Df& B = *blue;
        blend( R, G, B, 
               R_goodImage, G_goodImage, B_goodImage,
               *m_antiGhostingMasksList[idx],
               *m_antiGhostingMasksList[goodImageIndex] );
    }
}
void HdrCreationManager::cropAgMasks(const QRect& ca, QList<QImage*>& antiGhostingMasksList) {
    int origlistsize = antiGhostingMasksList.size();
    for (int image_idx = 0; image_idx < origlistsize; image_idx++) {
        QImage *newimage = new QImage(antiGhostingMasksList.at(0)->copy(ca));
        if (newimage == NULL)
            exit(1); // TODO: exit gracefully
        antiGhostingMasksList.append(newimage);
        delete antiGhostingMasksList.takeAt(0);
    }
}
*/

int HdrCreationManager::computePatches(float threshold, bool patches[][agGridSize], float &percent, QList <QPair<int, int> > HV_offset)
{
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
        if (h == m_agGoodImageIndex) 
            continue;
        float deltaEV;
        int dx, dy;
        #pragma omp parallel for private (deltaEV, dx, dy) schedule(static)
        for (int j = 0; j < agGridSize; j++) {
            for (int i = 0; i < agGridSize; i++) {
                    deltaEV = log(m_data[m_agGoodImageIndex].getExposureTime()) - log(m_data[h].getExposureTime());
                    #pragma omp critical (get_dx)
                    dx = HV_offset[m_agGoodImageIndex].first - HV_offset[h].first;
                    #pragma omp critical (get_dy)
                    dy = HV_offset[m_agGoodImageIndex].second - HV_offset[h].second;
                    if (comparePatches(m_data[m_agGoodImageIndex],
                                       m_data[h],
                                       i, j, gridX, gridY, threshold, deltaEV, dx, dy)) {
                        m_patches[i][j] = true;
                    }
            }                      
        }
    }

    int count = 0;
    for (int i = 0; i < agGridSize; i++)
        for (int j = 0; j < agGridSize; j++)
            if (m_patches[i][j] == true)
                count++;
    percent = static_cast<float>(count) / static_cast<float>(agGridSize*agGridSize) * 100.0f;
    qDebug() << "Total patches: " << percent << "%";

    memcpy(patches, m_patches, agGridSize*agGridSize);

#ifdef TIMER_PROFILING
    stop_watch.stop_and_update();
    std::cout << "computePatches = " << stop_watch.get_time() << " msec" << std::endl;
#endif
    return m_agGoodImageIndex;
}

pfs::Frame *HdrCreationManager::doAntiGhosting(bool patches[][agGridSize], int h0, bool manualAg, ProgressHelper *ph)
{
    qDebug() << "HdrCreationManager::doAntiGhosting";
#ifdef TIMER_PROFILING
    msec_timer stop_watch;
    stop_watch.start();
#endif
    const int width = m_data[0].frame()->getWidth();
    const int height = m_data[0].frame()->getHeight();
    const int gridX = width / agGridSize;
    const int gridY = height / agGridSize;
    connect(ph, SIGNAL(qtSetRange(int, int)), this, SIGNAL(progressRangeChanged(int, int)));
    connect(ph, SIGNAL(qtSetValue(int)), this, SIGNAL(progressValueChanged(int)));
    ph->setRange(0,100);
    ph->setValue(0);
    emit progressStarted();

    const Channel *Good_Rc, *Good_Gc, *Good_Bc;
    m_data[h0].frame().get()->getXYZChannels(Good_Rc, Good_Gc, Good_Bc);

    const Channel *Rc, *Gc, *Bc;
    Frame* ghosted = createHdr(false, 1);
    ghosted->getXYZChannels(Rc, Gc, Bc);
    ph->setValue(20);
    if (ph->canceled()) return NULL;

    Array2Df* logIrradianceGood_R = new Array2Df(width, height);
    computeLogIrradiance(logIrradianceGood_R, Good_Rc);
    ph->setValue(22);
    if (ph->canceled()) { 
        delete logIrradianceGood_R;
        return NULL;
    }
    Array2Df* logIrradianceGood_G = new Array2Df(width, height);
    computeLogIrradiance(logIrradianceGood_G, Good_Gc);
    ph->setValue(24);
    if (ph->canceled()) { 
        delete logIrradianceGood_G;
        return NULL;
    }
    Array2Df* logIrradianceGood_B = new Array2Df(width, height);
    computeLogIrradiance(logIrradianceGood_B, Good_Bc);
    ph->setValue(26);
    if (ph->canceled()) { 
        delete logIrradianceGood_B;
        return NULL;
    }
    Array2Df* logIrradiance_R = new Array2Df(width, height);
    computeLogIrradiance(logIrradiance_R, Rc);
    ph->setValue(28);
    if (ph->canceled()) { 
        delete logIrradiance_R;
        return NULL;
    }
    Array2Df* logIrradiance_G = new Array2Df(width, height);
    computeLogIrradiance(logIrradiance_G, Gc);
    ph->setValue(30);
    if (ph->canceled()) { 
        delete logIrradiance_G;
        return NULL;
    }
    Array2Df* logIrradiance_B = new Array2Df(width, height);
    computeLogIrradiance(logIrradiance_B, Bc);
    ph->setValue(32);
    if (ph->canceled()) { 
        delete logIrradiance_B;
        return NULL;
    }

    Array2Df* gradientXGood_R = new Array2Df(width, height);
    Array2Df* gradientYGood_R = new Array2Df(width, height);
    Array2Df* gradientX_R = new Array2Df(width, height);
    Array2Df* gradientY_R = new Array2Df(width, height);
    Array2Df* gradientXBlended_R = new Array2Df(width, height);
    Array2Df* gradientYBlended_R = new Array2Df(width, height);
    computeGradient(gradientXGood_R, gradientYGood_R, logIrradianceGood_R);
    delete logIrradianceGood_R;
    ph->setValue(33);
    if (ph->canceled()) { 
        delete gradientXGood_R;
        delete gradientYGood_R;
        delete gradientX_R;
        delete gradientY_R;
        delete gradientXBlended_R;
        delete gradientYBlended_R;
        return NULL;
    }
    computeGradient(gradientX_R, gradientY_R, logIrradiance_R);
    ph->setValue(34);
    if (ph->canceled()) { 
        delete gradientXGood_R;
        delete gradientYGood_R;
        delete gradientX_R;
        delete gradientY_R;
        delete gradientXBlended_R;
        delete gradientYBlended_R;
        return NULL;
    }
    if (manualAg)
        blendGradients(gradientXBlended_R, gradientYBlended_R,
                       gradientX_R, gradientY_R,
                       gradientXGood_R, gradientYGood_R,
                       *m_agMask);
    else
        blendGradients(gradientXBlended_R, gradientYBlended_R,
                       gradientX_R, gradientY_R,
                       gradientXGood_R, gradientYGood_R,
                       patches, gridX, gridY);
    delete gradientX_R;
    delete gradientY_R;
    delete gradientXGood_R;
    delete gradientYGood_R;
    ph->setValue(35);
    if (ph->canceled()) { 
        delete gradientXBlended_R;
        delete gradientYBlended_R;
        return NULL;
    }

    Array2Df* gradientXGood_G = new Array2Df(width, height);
    Array2Df* gradientYGood_G = new Array2Df(width, height);
    Array2Df* gradientX_G = new Array2Df(width, height);
    Array2Df* gradientY_G = new Array2Df(width, height);
    Array2Df* gradientXBlended_G = new Array2Df(width, height);
    Array2Df* gradientYBlended_G = new Array2Df(width, height);
    computeGradient(gradientXGood_G, gradientYGood_G, logIrradianceGood_G);
    delete logIrradianceGood_G;
    ph->setValue(36);
    if (ph->canceled()) { 
        delete gradientXGood_G;
        delete gradientYGood_G;
        delete gradientX_G;
        delete gradientY_G;
        delete gradientXBlended_G;
        delete gradientYBlended_G;
        return NULL;
    }
    computeGradient(gradientX_G, gradientY_G, logIrradiance_G);
    ph->setValue(37);
    if (ph->canceled()) { 
        delete gradientXGood_G;
        delete gradientYGood_G;
        delete gradientX_G;
        delete gradientY_G;
        delete gradientXBlended_G;
        delete gradientYBlended_G;
        return NULL;
    }
    if (manualAg)
        blendGradients(gradientXBlended_G, gradientYBlended_G,
                       gradientX_G, gradientY_G,
                       gradientXGood_G, gradientYGood_G,
                       *m_agMask);
    else
        blendGradients(gradientXBlended_G, gradientYBlended_G,
                       gradientX_G, gradientY_G,
                       gradientXGood_G, gradientYGood_G,
                       patches, gridX, gridY);
    delete gradientX_G;
    delete gradientY_G;
    delete gradientXGood_G;
    delete gradientYGood_G;
    ph->setValue(38);
    if (ph->canceled()) { 
        delete gradientXBlended_G;
        delete gradientYBlended_G;
        return NULL;
    }

    Array2Df* gradientXGood_B = new Array2Df(width, height);
    Array2Df* gradientYGood_B = new Array2Df(width, height);
    Array2Df* gradientX_B = new Array2Df(width, height);
    Array2Df* gradientY_B = new Array2Df(width, height);
    Array2Df* gradientXBlended_B = new Array2Df(width, height);
    Array2Df* gradientYBlended_B = new Array2Df(width, height);
    computeGradient(gradientXGood_B, gradientYGood_B, logIrradianceGood_B);
    delete logIrradianceGood_B;
    ph->setValue(39);
    if (ph->canceled()) { 
        delete gradientXGood_B;
        delete gradientYGood_B;
        delete gradientX_B;
        delete gradientY_B;
        delete gradientXBlended_B;
        delete gradientYBlended_B;
        return NULL;
    }
    computeGradient(gradientX_B, gradientY_B, logIrradiance_B);
    ph->setValue(40);
    if (ph->canceled()) { 
        delete gradientXGood_B;
        delete gradientYGood_B;
        delete gradientX_B;
        delete gradientY_B;
        delete gradientXBlended_B;
        delete gradientYBlended_B;
        return NULL;
    }
    if (manualAg)
        blendGradients(gradientXBlended_B, gradientYBlended_B,
                       gradientX_B, gradientY_B,
                       gradientXGood_B, gradientYGood_B,
                       *m_agMask);
    else
        blendGradients(gradientXBlended_B, gradientYBlended_B,
                       gradientX_B, gradientY_B,
                       gradientXGood_B, gradientYGood_B,
                       patches, gridX, gridY);
    delete gradientX_B;
    delete gradientY_B;
    delete gradientXGood_B;
    delete gradientYGood_B;
    ph->setValue(41);
    if (ph->canceled()) { 
        delete gradientXBlended_B;
        delete gradientYBlended_B;
        return NULL;
    }

    Array2Df* divergence_R = new Array2Df(width, height);
    computeDivergence(divergence_R, gradientXBlended_R, gradientYBlended_R);
    delete gradientXBlended_R;
    delete gradientYBlended_R;
    ph->setValue(42);
    if (ph->canceled()) { 
        delete divergence_R;
        return NULL;
    }
    Array2Df* divergence_G = new Array2Df(width, height);
    computeDivergence(divergence_G, gradientXBlended_G, gradientYBlended_G);
    delete gradientXBlended_G;
    delete gradientYBlended_G;
    ph->setValue(43);
    if (ph->canceled()) { 
        delete divergence_G;
        return NULL;
    }
    Array2Df* divergence_B = new Array2Df(width, height);
    computeDivergence(divergence_B, gradientXBlended_B, gradientYBlended_B);
    delete gradientXBlended_B;
    delete gradientYBlended_B;
    ph->setValue(44);
    if (ph->canceled()) { 
        delete divergence_B;
        return NULL;
    }

    qDebug() << "solve_pde";
    //solve_pde_dft(divergence_R, logIrradiance_R);
    solve_pde_dct(*divergence_R, *logIrradiance_R);
    //solve_pde_fft(divergence_R, logIrradiance_R, ph-> true);
    //solve_pde_multigrid(divergence_R, logIrradiance_R, ph->;
    //solve_poisson(divergence_R, logIrradiance_R, logIrradiance_R);
    qDebug() << "residual: " << residual_pde(logIrradiance_R, divergence_R);
    delete divergence_R;
    ph->setValue(60);
    if (ph->canceled()) { 
        delete logIrradiance_R;
        return NULL;
    }

    qDebug() << "solve_pde";
    //solve_pde_dft(divergence_G, logIrradiance_G);
    solve_pde_dct(*divergence_G, *logIrradiance_G);
    //solve_pde_fft(divergence_G, logIrradiance_G, ph-> true);
    //solve_pde_multigrid(divergence_G, logIrradiance_G, ph->;
    //solve_poisson(divergence_G, logIrradiance_G, logIrradiance_G);
    qDebug() << "residual: " << residual_pde(logIrradiance_G, divergence_G);
    delete divergence_G;
    ph->setValue(76);
    if (ph->canceled()) { 
        delete logIrradiance_G;
        return NULL;
    }

    qDebug() << "solve_pde";
    //solve_pde_dft(divergence_B, logIrradiance_B);
    solve_pde_dct(*divergence_B, *logIrradiance_B);
    //solve_pde_fft(divergence_B, logIrradiance_B, ph-> true);
    //solve_pde_multigrid(divergence_B, logIrradiance_B, ph->;
    //solve_poisson(divergence_B, logIrradiance_B, logIrradiance_B);
    qDebug() << "residual: " << residual_pde(logIrradiance_B, divergence_B);
    delete divergence_B;
    ph->setValue(93);
    if (ph->canceled()) { 
        delete logIrradiance_B;
        return NULL;
    }

    Frame* deghosted = new Frame(width, height);
    Channel *Urc, *Ugc, *Ubc;
    deghosted->createXYZChannels(Urc, Ugc, Ubc);

    computeIrradiance(Urc, logIrradiance_R);
    delete logIrradiance_R;
    ph->setValue(94);
    if (ph->canceled()) { 
        delete deghosted;
        return NULL;
    }
    computeIrradiance(Ugc, logIrradiance_G);
    delete logIrradiance_G;
    ph->setValue(95);
    if (ph->canceled()) { 
        delete deghosted;
        return NULL;
    }
    computeIrradiance(Ubc, logIrradiance_B);
    delete logIrradiance_B;
    ph->setValue(96);
    if (ph->canceled()) { 
        delete deghosted;
        return NULL;
    }

/*
    int i, j;
    for (i = 0; i < agGridSize; i++)
        for (j = 0; j < agGridSize; j++)
            if (patches[i][j] == false)
                break;

    int x = i*gridX;
    int y = j*gridY;
    float sf1 = (*Rc)(x, y) - (*Urc)(x, y);
    float sf2 = (*Gc)(x, y) - (*Ugc)(x, y);
    float sf3 = (*Bc)(x, y) - (*Ubc)(x, y);
    float sf = (sf1+sf2+sf3) / 3.0f;

    #pragma omp parallel for schedule(static)
    for (int i = 0; i < width*height; i++)
        (*Urc)(i) -= sf1;
    #pragma omp parallel for schedule(static)
    for (int i = 0; i < width*height; i++)
        (*Ugc)(i) -= sf2;
    #pragma omp parallel for schedule(static)
    for (int i = 0; i < width*height; i++)
        (*Ubc)(i) -= sf3;
    colorBalance(*Urc, *Rc, i*gridX, j*gridY);
    ph->setValue(97);
    colorBalance(*Ugc, *Gc, i*gridX, j*gridY);
    ph->setValue(98);
    colorBalance(*Ubc, *Bc, i*gridX, j*gridY);
*/
    qDebug() << min(Urc);
    qDebug() << max(Urc);
    qDebug() << min(Ugc);
    qDebug() << max(Ugc);
    qDebug() << min(Ubc);
    qDebug() << max(Ubc);

    float mr = min(Urc);
    float mg = min(Ugc);
    float mb = min(Ubc);
    float t = min(mr, mg);
    float m = min(t,mb);

    clampToZero(*Urc, *Ugc, *Ubc, m);

    qDebug() << min(Urc);
    qDebug() << max(Urc);
    qDebug() << min(Ugc);
    qDebug() << max(Ugc);
    qDebug() << min(Ubc);
    qDebug() << max(Ubc);
    
    colorbalance_rgb_f32(*Urc, *Ugc, *Ubc, width*height, 3, 97);
    //robustAWB(Urc, Ugc, Ubc);

    ph->setValue(100);

    emit progressFinished();
    delete ghosted;
#ifdef TIMER_PROFILING
    stop_watch.stop_and_update();
    std::cout << "doAntiGhosting = " << stop_watch.get_time() << " msec" << std::endl;
#endif
    return deghosted;
}

void HdrCreationManager::getAgData(bool patches[][agGridSize], int &h0)
{
    memcpy(patches, m_patches, agGridSize*agGridSize);

    h0 = m_agGoodImageIndex;
}

void HdrCreationManager::setPatches(bool patches[][agGridSize])
{
    memcpy(m_patches, patches, agGridSize*agGridSize);
}

void HdrCreationManager::reset()
{
    if (ais != NULL && ais->state() != QProcess::NotRunning) {
        ais->kill();
        delete ais;
        ais = NULL;
    }
    if (m_futureWatcher.isRunning()) {
        qDebug() << "Aborting loadFiles...";
        m_futureWatcher.cancel();
        m_futureWatcher.waitForFinished();
        emit loadFilesAborted();
    }
    disconnect(&m_futureWatcher, SIGNAL(finished()), this, SLOT(loadFilesDone()));
    disconnect(&m_futureWatcher, SIGNAL(finished()), this, SLOT(alignedFilesLoaded()));
    removeTempFiles();
    m_data.clear();
    m_tmpdata.clear();
}

