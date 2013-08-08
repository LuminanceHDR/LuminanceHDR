/**
 * This file is a part of LuminanceHDR package.
 * ----------------------------------------------------------------------
 * Copyright (C) 2013 Franco Comida
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
 * @author Franco Comida <fcomida@users.sourceforge.net>
 */

#include <QLabel>
#include <QMessageBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QFile>
#include <QtConcurrentMap>
#include <QtConcurrentFilter>
#include <QDebug>

#include <valarray> 

#include <boost/foreach.hpp>
#include <boost/bind.hpp>

#include "FitsImporter.h"
#include "ui_FitsImporter.h"

#include <Libpfs/frame.h>
#include <Libpfs/utils/msec_timer.h>
#include <Libpfs/utils/minmax.h>
#include <Libpfs/io/framereader.h>
#include <Libpfs/io/framereaderfactory.h>
#include <Libpfs/io/framewriter.h>
#include <Libpfs/io/framewriterfactory.h>
#include <Libpfs/utils/transform.h>
#include <Libpfs/colorspace/convert.h>
#include <Libpfs/colorspace/rgbremapper.h>
#include "Libpfs/colorspace/colorspace.h"
#include "Libpfs/manip/rotate.h"
#include "HdrCreation/mtb_alignment.h"

using namespace pfs;
using namespace pfs::io;

namespace {

inline
void rgb2hsl(float r, float g, float b, float& h, float& s, float& l)
{
    float v, m, vm, r2, g2, b2;
    h = 0.0f;
    s = 0.0f;
    l = 0.0f;

    pfs::utils::minmax(r, g, b, m, v);

    l = (m + v) / 2.0f;
    if (l <= 0.0f)
        return;
    vm = v - m;
    s = vm;
    //if (s >= 0.0f)
    if (s > 0.0f)
        s /= (l <= 0.5f) ? (v + m) : (2.0f - v - m);
    else return;
    r2 = (v - r) / vm;
    g2 = (v - g) / vm;
    b2 = (v - b) / vm;
    if (r == v)
        h = (g == m ? 5.0f + b2 : 1.0f - g2);
    else if (g == v)
        h = (b == m ? 1.0f + r2 : 3.0f - b2);
    else
        h = (r == m ? 3.0f + g2 : 5.0f - r2);
    h /= 6.0f;
}

inline
void hsl2rgb(float h, float sl, float l, float& r, float& g, float& b)
{
    float v;
    r = l;
    g = l;
    b = l;
    v = (l <= 0.5f) ? (l * (1.0f + sl)) : (l + sl - l * sl);
    if (v > 0.0f)
    {
        float m;
        float sv;
        int sextant;
        float fract, vsf, mid1, mid2;
        m = l + l - v;
        sv = (v - m ) / v;
        h *= 6.0f;
        sextant = (int)h;
        fract = h - sextant;
        vsf = v * sv * fract;
        mid1 = m + vsf;
        mid2 = v - vsf;
        switch (sextant)
        {
        case 0:
            r = v;
            g = mid1;
            b = m;
            break;
        case 1:
            r = mid2;
            g = v;
            b = m;
            break;
        case 2:
            r = m;
            g = v;
            b = mid1;
            break;
        case 3:
            r = m;
            g = mid2;
            b = v;
            break;
        case 4:
            r = mid1;
            g = m;
            b = v;
            break;
        case 5:
            r = v;
            g = m;
            b = mid2;
            break;
        }
    }
}

struct Normalize {
    float m;
    float M;
    Normalize(float m, float M) : m(m), M(M) {}

    float operator()(float i) {
        return (i - m)/(M-m);
    }
};

const float GAMMA_2_2 = 1.0f/2.2f;
struct ConvertToQRgb {
    float m;
    float M;
    ConvertToQRgb(float m, float M) : m(m), M(M) {}
    void operator()(float r, float g, float b, QRgb& rgb) const {
/*
        float logRange = std::log2f(M/m);
        uint8_t r8u = colorspace::convertSample<uint8_t>(std::log2(r/m)/logRange);
        uint8_t g8u = colorspace::convertSample<uint8_t>(std::log2(g/m)/logRange);
        uint8_t b8u = colorspace::convertSample<uint8_t>(std::log2(b/m)/logRange);
*/
        uint8_t r8u = colorspace::convertSample<uint8_t>(std::pow(r, GAMMA_2_2));
        uint8_t g8u = colorspace::convertSample<uint8_t>(std::pow(g, GAMMA_2_2));
        uint8_t b8u = colorspace::convertSample<uint8_t>(std::pow(b, GAMMA_2_2));

        rgb = qRgb(r8u, g8u, b8u);
    }
};

struct LoadFile {

    FitsImporter *instance;
    LoadFile(FitsImporter *w): instance(w) {}

    void operator()(HdrCreationItem& currentItem)
    {
        QFileInfo qfi(currentItem.filename());
        qDebug() << QString("Loading data for %1").arg(currentItem.filename());

        try
        {
            instance->m_fitsreader_mutex.lock();

            FrameReaderPtr reader = FrameReaderFactory::open(
                        QFile::encodeName(qfi.filePath()).constData() );
            reader->read( *currentItem.frame(), Params() );

            instance->m_fitsreader_mutex.unlock();
            
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

            Channel redNorm(currentItem.frame()->getWidth(),
                            currentItem.frame()->getHeight(), "X");
            float m = *std::min_element(red->begin(), red->end());
            float M = *std::max_element(red->begin(), red->end());
            Normalize normalize(m, M);
            ConvertToQRgb convert(m, M);
            std::transform(red->begin(), red->end(), redNorm.begin(), normalize);

            utils::transform(redNorm.begin(), redNorm.end(), redNorm.begin(), redNorm.begin(),
                             qimageData, convert);

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
        qDebug() << currentItem.isValid();
        QString inputFilename = currentItem.filename();
        QFileInfo qfi(inputFilename);
        QString base = qfi.completeBaseName(); 
        QString filename = base + ".tif";
        QString tempdir = LuminanceOptions().getTempDir();
        qDebug() << QString("Saving data for %1 on %2").arg(filename).arg(tempdir);
        

        QString completeFilename = tempdir + "/" + filename;

        // save pfs::Frame as tiff 32bits
        try
        {
            Params p;
            p.set("tiff_mode", 2); // 32bits
            p.set("min_luminance", 0.0f);
            p.set("max_luminance", 65535.0f);
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

            Channel redNorm(currentItem.frame()->getWidth(),
                            currentItem.frame()->getHeight(), "X");
            float m = *std::min_element(red->begin(), red->end());
            float M = *std::max_element(red->begin(), red->end());
            Normalize normalize(m, M);
            ConvertToQRgb convert(m, M);
            std::transform(red->begin(), red->end(), redNorm.begin(), normalize);

            utils::transform(redNorm.begin(), redNorm.end(), redNorm.begin(), redNorm.begin(),
                             qimageData, convert);

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

}

FitsImporter::~FitsImporter() {}

FitsImporter::FitsImporter(QWidget *parent)
    : QDialog(parent)
    , m_ui(new Ui::FitsImporter)
{
    m_ui->setupUi(this);

#ifdef Q_WS_MAC
    this->setWindowModality(Qt::WindowModal); // In OS X, the QMessageBox is modal to the window
#endif
    m_previewLabel = new QLabel(this);
    m_previewLabel->resize(600,400);
    m_previewLabel->setScaledContents(true);
    QPalette* palette = new QPalette(); 
    palette->setColor(QPalette::Foreground,Qt::red);
    m_previewLabel->setPalette(*palette);
    m_previewLabel->setFrameStyle(QFrame::Box);
    m_previewLabel->setLineWidth(3);
    m_previewLabel->hide();
    m_previewFrame = new PreviewFrame;
    m_ui->verticalLayoutPreviews->addWidget(m_previewFrame);
    m_previewFrame->addLabel(new SimplePreviewLabel(0));
    m_previewFrame->addLabel(new SimplePreviewLabel(1));
    m_previewFrame->addLabel(new SimplePreviewLabel(2));
    m_previewFrame->addLabel(new SimplePreviewLabel(3));
    m_previewFrame->addLabel(new SimplePreviewLabel(4));
    m_previewFrame->show();
    m_previewFrame->getLabel(0)->setEnabled(false);
    m_previewFrame->getLabel(1)->setEnabled(false);
    m_previewFrame->getLabel(2)->setEnabled(false);
    m_previewFrame->getLabel(3)->setEnabled(false);
    m_previewFrame->getLabel(4)->setEnabled(false);
    m_ui->groupBoxMessages->setVisible(false);
    m_ui->progressBar->setVisible(false);
    connect(this, SIGNAL(setValue(int)), m_ui->progressBar, SLOT(setValue(int)), Qt::DirectConnection);
    connect(this, SIGNAL(setRange(int,int)), m_ui->progressBar, SLOT(setRange(int,int)), Qt::DirectConnection);
    connect(m_previewFrame->getLabel(0), SIGNAL(selected(int)), this, SLOT(previewLabelSelected(int)));
    connect(m_previewFrame->getLabel(1), SIGNAL(selected(int)), this, SLOT(previewLabelSelected(int)));
    connect(m_previewFrame->getLabel(2), SIGNAL(selected(int)), this, SLOT(previewLabelSelected(int)));
    connect(m_previewFrame->getLabel(3), SIGNAL(selected(int)), this, SLOT(previewLabelSelected(int)));
    connect(m_previewFrame->getLabel(4), SIGNAL(selected(int)), this, SLOT(previewLabelSelected(int)));
}

void FitsImporter::selectInputFile(QLineEdit* textField, QString* channel)
{
    QString filetypes = "FITS (*.fit *.FIT *.fits *.FITS);;";
    *channel = QFileDialog::getOpenFileName(this, tr("Load one FITS image..."),
                                                      m_luminance_options.getDefaultPathHdrIn(),
                                                      filetypes );
    textField->setText(*channel);
    checkLoadButton();

    if (!channel->isEmpty())
    {
        QFileInfo qfi(*channel);
        m_luminance_options.setDefaultPathHdrIn( qfi.path() );
    }

    if (!m_luminosityChannel.isEmpty() && !m_redChannel.isEmpty() && !m_greenChannel.isEmpty() &&
        !m_blueChannel.isEmpty() && !m_hChannel.isEmpty())
    {
        on_pushButtonLoad_clicked();
    }
}

void FitsImporter::on_pushButtonLuminosity_clicked()
{
    selectInputFile(m_ui->lineEditLuminosity, &m_luminosityChannel);
}

void FitsImporter::on_pushButtonRed_clicked()
{
    selectInputFile(m_ui->lineEditRed, &m_redChannel);
}

void FitsImporter::on_pushButtonGreen_clicked()
{
    selectInputFile(m_ui->lineEditGreen, &m_greenChannel);
}

void FitsImporter::on_pushButtonBlue_clicked()
{
    selectInputFile(m_ui->lineEditBlue, &m_blueChannel);
}

void FitsImporter::on_pushButtonH_clicked()
{
    selectInputFile(m_ui->lineEditH, &m_hChannel);
}

void FitsImporter::checkLoadButton()
{
    m_ui->pushButtonLoad->setEnabled(!m_luminosityChannel.isEmpty() && 
                                     !m_redChannel.isEmpty() &&
                                     !m_greenChannel.isEmpty() &&
                                     !m_blueChannel.isEmpty());
}

void FitsImporter::on_pushButtonLoad_clicked()
{
    QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );
    m_ui->pushButtonLoad->setEnabled(false);
    m_data.clear();
    m_tmpdata.clear();
    m_channels.clear();
    m_channels << m_luminosityChannel 
               << m_redChannel
               << m_greenChannel
               << m_blueChannel;
    if (!m_hChannel.isEmpty())
        m_channels << m_hChannel;

    BOOST_FOREACH(const QString& i, m_channels) {
        m_tmpdata.push_back( HdrCreationItem(i) );
    }

    // parallel load of the data...
    connect(&m_futureWatcher, SIGNAL(finished()), this, SLOT(loadFilesDone()), Qt::DirectConnection);

    // Start the computation.
    LoadFile loadfile(this);
    m_futureWatcher.setFuture( QtConcurrent::map(m_tmpdata.begin(), m_tmpdata.end(), loadfile) );
}

void FitsImporter::loadFilesDone()
{ 
    qDebug() << "Data loaded ... move to internal structure!";

    int idx = 0;
    BOOST_FOREACH(const HdrCreationItem& i, m_tmpdata) {
        if ( i.isValid() ) {
            qDebug() << QString("Insert data for %1").arg(i.filename());
            m_data.push_back(i);
            m_previewFrame->getLabel(idx)->setPixmap(QPixmap::fromImage(*(i.qimage())));
            m_previewFrame->getLabel(idx++)->setEnabled(true);
        }
        else {
            QMessageBox::warning(0,"", tr("Cannot load FITS image %1").arg(i.filename()), QMessageBox::Ok, QMessageBox::NoButton);
            m_data.clear();
            m_tmpdata.clear();
            m_channels.clear();
            m_ui->pushButtonLoad->setEnabled(true);
            QApplication::restoreOverrideCursor();
            return;
        }
    }
    m_previewFrame->labelSelected(0);
    if (!framesHaveSameSize()) {
        QMessageBox::warning(0,"", tr("FITS images have different size"), QMessageBox::Ok, QMessageBox::NoButton);
        m_data.clear();
        m_tmpdata.clear();
        m_channels.clear();
        m_ui->pushButtonLoad->setEnabled(true);
        m_ui->pushButtonPreview->setEnabled(true);
        QApplication::restoreOverrideCursor();
        return;
    }
    m_ui->pushButtonLoad->setEnabled(true);
    m_ui->pushButtonOK->setEnabled(true);
    m_ui->pushButtonClockwise->setEnabled(true);
    m_ui->pushButtonPreview->setEnabled(true);
    QApplication::restoreOverrideCursor();
}

void FitsImporter::on_pushButtonOK_clicked()
{
    if (m_ui->alignCheckBox->isChecked())
        if (m_ui->ais_radioButton->isChecked()) {
            m_ui->groupBoxMessages->setVisible(true);
            m_ui->progressBar->setVisible(true);
            align_with_ais();
        }
        else
            align_with_mtb();
    else
        buildFrame();
}

void FitsImporter::buildFrame()
{
    const int width = m_data[0].frame()->getWidth();
    const int height = m_data[0].frame()->getHeight();

    std::vector<float> contentsH(width*height);
    if (m_hChannel == "") { 
        std::fill(contentsH.begin(), contentsH.end(), 0.0f);
    }
    else {
        Channel *Hc = m_data[4].frame()->getChannel("X");
        std::copy(Hc->begin(), Hc->end(), contentsH.begin()); 
    }

    m_frame = new Frame(width, height);
    Channel *Xc, *Yc, *Zc;
    m_frame->createXYZChannels(Xc, Yc, Zc);

    std::vector<FramePtr> frames;
    std::vector<Channel *> channels;
    
    for ( HdrCreationItemContainer::iterator it = m_data.begin(), 
          itEnd = m_data.end(); it != itEnd; ++it) {
        frames.push_back(it->frame());
    }
    
    for ( std::vector<FramePtr>::iterator it = frames.begin(), 
          itEnd = frames.end(); it != itEnd; ++it) {
        channels.push_back((*it)->getChannel("X"));
    }
    
    for (long i = 0; i < width*height; i++) 
    {
        float r, g, b, h, s, l;
        rgb2hsl((*channels[1])(i), (*channels[2])(i), (*channels[3])(i), h, s, l);
        hsl2rgb(h,s, (*channels[0])(i), r, g, b);
        (*Xc)(i) = r + contentsH[i];
        (*Yc)(i) = g;
        (*Zc)(i) = b;
    }     
    accept();
}

void FitsImporter::align_with_ais()
{
    m_ui->pushButtonOK->setEnabled(false);
    m_ais = new QProcess(this);
    if (m_ais == NULL) exit(1);       // TODO: exit gracefully
    m_ais->setWorkingDirectory(m_luminance_options.getTempDir());
    QStringList env = QProcess::systemEnvironment();
#ifdef WIN32
    QString separator(";");
#else
    QString separator(":");
#endif
    env.replaceInStrings(QRegExp("^PATH=(.*)", Qt::CaseInsensitive), "PATH=\\1"+separator+QCoreApplication::applicationDirPath());
    m_ais->setEnvironment(env);
    connect(m_ais, SIGNAL(finished(int,QProcess::ExitStatus)), this, SLOT(ais_finished(int,QProcess::ExitStatus)));
    connect(m_ais, SIGNAL(error(QProcess::ProcessError)), this, SLOT(ais_failed_slot(QProcess::ProcessError)));
    connect(m_ais, SIGNAL(readyRead()), this, SLOT(readData()));
    
    QStringList ais_parameters = m_luminance_options.getAlignImageStackOptions();

    if (m_ui->autoCropCheckBox->isChecked()) { ais_parameters << "-C"; }

    QFutureWatcher<void> futureWatcher;

    // Start the computation.
    futureWatcher.setFuture( QtConcurrent::map(m_data.begin(), m_data.end(), SaveFile()) );
    futureWatcher.waitForFinished();

    if (futureWatcher.isCanceled()) return;

    BOOST_FOREACH(const QString& i, m_channels) {
        QFileInfo qfi(i);
        QString base = qfi.completeBaseName(); 
        QString filename = base + ".tif";
        QString tempdir = m_luminance_options.getTempDir();
        QString completeFilename = tempdir + "/" + filename;
        ais_parameters << completeFilename; 
    }

    qDebug() << "ais_parameters " << ais_parameters;
#ifdef Q_WS_MAC
    qDebug() << QCoreApplication::applicationDirPath()+"/align_image_stack";
    m_ais->start(QCoreApplication::applicationDirPath()+"/align_image_stack", ais_parameters );
#else
    m_ais->start("align_image_stack", ais_parameters );
#endif
    qDebug() << "ais started";
}

void FitsImporter::ais_finished(int exitcode, QProcess::ExitStatus exitstatus)
{
    if (exitstatus != QProcess::NormalExit)
    {
        qDebug() << "ais failed";
        m_data.clear();
        m_tmpdata.clear();
        m_channels.clear();
        QApplication::restoreOverrideCursor();
        return;
    }
    if (exitcode == 0)
    {
        m_tmpdata.clear();
        int i = 0;
        for ( HdrCreationItemContainer::iterator it = m_data.begin(), 
              itEnd = m_data.end(); it != itEnd; ++it) {
            QString inputFilename = it->filename(), filename;
            filename = QString(m_luminance_options.getTempDir() + "/aligned_" + QString("%1").arg(i,4,10,QChar('0'))+".tif");
            
            m_tmpdata.push_back( HdrCreationItem(filename) );
            i++;
        }

        // parallel load of the data...
        // Start the computation.
        disconnect(&m_futureWatcher, SIGNAL(finished()), this, SLOT(loadFilesDone()));
        connect(&m_futureWatcher, SIGNAL(finished()), this, SLOT(alignedFilesLoaded()), Qt::DirectConnection);
      
        LoadFile loadfile(this);
        m_futureWatcher.setFuture( QtConcurrent::map(m_tmpdata.begin(), m_tmpdata.end(), loadfile) );
        
    }
    else
    {
        QApplication::restoreOverrideCursor();
        qDebug() << "align_image_stack exited with exit code " << exitcode;
        QMessageBox::warning(0,"", tr("align_image_stack exited with exit code %1").arg(exitcode), QMessageBox::Ok, QMessageBox::NoButton);
        m_data.clear();
        m_tmpdata.clear();
        m_channels.clear();
    }
}

void FitsImporter::alignedFilesLoaded()
{
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
    QApplication::restoreOverrideCursor();
    buildFrame();
}

void FitsImporter::ais_failed_slot(QProcess::ProcessError error)
{
    qDebug() << "align_image_stack failed";
    QApplication::restoreOverrideCursor();
    QMessageBox::warning(0,"", tr("align_image_stack failed with error"), QMessageBox::Ok, QMessageBox::NoButton);
    m_data.clear();
    m_tmpdata.clear();
    m_channels.clear();
}

bool FitsImporter::framesHaveSameSize()
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

void FitsImporter::readData()
{
    QByteArray data = m_ais->readAll();
    qDebug() << data;
    if (data.contains("[1A"))
        data.replace("[1A", "");
    if (data.contains("[2A"))
        data.replace("[2A", "");
    if (data.contains(QChar(0x01B).toAscii()))
        data.replace(QChar(0x01B).toAscii(), "");

    m_ui->textEdit->append(data);
    if (data.contains(": remapping")) {
        QRegExp exp("\\:\\s*(\\d+)\\s*");
        exp.indexIn(QString(data.data()));
        emit setRange(0, 100);
        emit setValue(exp.cap(1).toInt());
    }
}

void FitsImporter::align_with_mtb()
{
    // build temporary container...
    vector<FramePtr> frames;
    for (size_t i = 0; i < m_data.size(); ++i) {
        frames.push_back( m_data[i].frame() );
    }

    // run MTB
    libhdr::mtb_alignment(frames);

    buildFrame();
}

void FitsImporter::on_pushButtonClockwise_clicked()
{
    QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );
    m_ui->pushButtonClockwise->setEnabled(false);
    int index = m_previewFrame->getSelectedLabel();
    Frame *toRotate = (m_data[index].frame()).get();
    Frame *rotatedHalf = pfs::rotate(toRotate, true);
    Frame *rotated = pfs::rotate(rotatedHalf, true);
    m_data[index].frame()->swap(*rotated);
    delete rotatedHalf;
    RefreshPreview refresh;
    refresh(m_data[index]);
    m_previewFrame->getLabel(index)->setPixmap(QPixmap::fromImage(*(m_data[index].qimage())));
    if (m_ui->pushButtonPreview->isChecked()) {
        m_previewLabel->setPixmap(*m_previewFrame->getLabel(m_previewFrame->getSelectedLabel())->pixmap());
    }
    m_ui->pushButtonClockwise->setEnabled(true);
    QApplication::restoreOverrideCursor();
}

void FitsImporter::on_pushButtonPreview_pressed()
{
    m_previewLabel->setPixmap(*m_previewFrame->getLabel(m_previewFrame->getSelectedLabel())->pixmap());
    m_previewLabel->show();
}

void FitsImporter::on_pushButtonPreview_released()
{
    m_previewLabel->hide();
}

void FitsImporter::previewLabelSelected(int index)
{
    if (m_ui->pushButtonPreview->isChecked()) {
        m_previewLabel->setPixmap(*m_previewFrame->getLabel(m_previewFrame->getSelectedLabel())->pixmap());
    }
}

