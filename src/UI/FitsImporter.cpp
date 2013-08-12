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

#include "Common/CommonFunctions.h"
#include "Libpfs/manip/rotate.h"
#include "HdrCreation/mtb_alignment.h"

using namespace pfs;

FitsImporter::~FitsImporter() 
{
    if (m_align)
        delete m_align;
}

FitsImporter::FitsImporter(QWidget *parent)
    : QDialog(parent)
    , m_align(NULL)
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
    m_futureWatcher.setFuture( QtConcurrent::map(m_tmpdata.begin(), m_tmpdata.end(), LoadFile()) );
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
    m_align = new Align(&m_data, false, 2, 0.0f, 65535.0f); 
    connect(m_align, SIGNAL(finishedAligning(int)), this, SLOT(ais_finished(int)));
    connect(m_align, SIGNAL(failedAligning(QProcess::ProcessError)), this, SLOT(ais_failed_slot(QProcess::ProcessError)));
    connect(m_align, SIGNAL(dataReady(QByteArray)), this, SLOT(readData(QByteArray)));
  
    m_align->align_with_ais(m_ui->autoCropCheckBox->isChecked());

    m_ui->pushButtonOK->setEnabled(false);
}

void FitsImporter::ais_finished(int exitcode)
{
    m_align->removeTempFiles();
    if (exitcode != 0)
    {
        QApplication::restoreOverrideCursor();
        qDebug() << "align_image_stack exited with exit code " << exitcode;
        QMessageBox::warning(0,"", tr("align_image_stack exited with exit code %1").arg(exitcode), QMessageBox::Ok, QMessageBox::NoButton);
        m_data.clear();
        m_tmpdata.clear();
        m_channels.clear();
    }
    else {
        QApplication::restoreOverrideCursor();
        buildFrame();
    }
}

void FitsImporter::ais_failed_slot(QProcess::ProcessError error)
{
    qDebug() << "align_image_stack failed";
    m_align->removeTempFiles();
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

void FitsImporter::readData(QByteArray data)
{
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

void FitsImporter::on_pushButtonPreview_clicked()
{
    if (m_ui->pushButtonPreview->isChecked()) {
        m_previewLabel->setPixmap(*m_previewFrame->getLabel(m_previewFrame->getSelectedLabel())->pixmap());
        m_previewLabel->show();
    }
    else
        m_previewLabel->hide();
}

void FitsImporter::previewLabelSelected(int index)
{
    if (m_ui->pushButtonPreview->isChecked()) {
        m_previewLabel->setPixmap(*m_previewFrame->getLabel(m_previewFrame->getSelectedLabel())->pixmap());
    }
}

