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

#ifndef FITSIMPORTER_H
#define FITSIMPORTER_H

#include <QDoubleSpinBox>
#include <QLineEdit>
#include <QProcess>
#include <QFutureWatcher>
#include <QSlider>
#include <QWizard>

#include "PreviewFrame.h"
#include "Alignment/Align.h"
#include "Common/LuminanceOptions.h"
#include "HdrWizard/HdrCreationItem.h"

namespace Ui {
class FitsImporter;
}

class FitsImporter : public QWizard
{
    Q_OBJECT

public:
    FitsImporter(QWidget *parent = 0);
    ~FitsImporter();

    pfs::Frame *getFrame() { return m_frame; }
    
protected slots:
    void on_pushButtonLuminosity_clicked();
    void on_pushButtonRed_clicked();
    void on_pushButtonGreen_clicked();
    void on_pushButtonBlue_clicked();
    void on_pushButtonH_clicked();
    void on_pushButtonOK_clicked();
    void on_pushButtonLoad_clicked();
    void on_pushButtonClockwise_clicked();
    void on_pushButtonPreview_clicked();
    void loadFilesDone();
    void ais_finished(int);
    void ais_failed_slot(QProcess::ProcessError);
    void readData(QByteArray);
    void previewLabelSelected(int index);

    void on_hsChannelRed_valueChanged(int newValue);
    void on_dsbChannelRed_valueChanged(double newValue);
    void on_hsChannelGreen_valueChanged(int newValue);
    void on_dsbChannelGreen_valueChanged(double newValue);
    void on_hsChannelBlue_valueChanged(int newValue);
    void on_dsbChannelBlue_valueChanged(double newValue);
    void on_hsChannelH_valueChanged(int newValue);
    void on_dsbChannelH_valueChanged(double newValue);

    void on_pushButtonColorRed_clicked();
    void on_pushButtonColorGreen_clicked();
    void on_pushButtonColorBlue_clicked();
    void on_pushButtonColorH_clicked();

signals:
    void setValue(int);
    void setRange(int, int);

protected:
    void checkLoadButton();
    bool framesHaveSameSize();
    void buildContents();
    void buildFrame();
    void buildPreview();
    void align_with_ais();
    void align_with_mtb();

    size_t m_width;
    size_t m_height;

    pfs::Frame *m_frame;
    PreviewFrame *m_previewFrame;
    QLabel* m_previewLabel;

    LuminanceOptions m_luminance_options;

    QStringList m_channels;
    QString m_luminosityChannel;
    QString m_redChannel;
    QString m_greenChannel;
    QString m_blueChannel;
    QString m_hChannel;

    HdrCreationItemContainer m_data;
    HdrCreationItemContainer m_tmpdata;
    std::vector<std::vector<float> > m_contents;
    std::vector<QImage> m_qimages;

    QFutureWatcher<void> m_futureWatcher;

    // alignment
	Align *m_align;

    QScopedPointer<Ui::FitsImporter> m_ui;

private:
    void selectInputFile(QLineEdit* textField, QString* channel);
    void hScrollValueChanged(int newValue, QDoubleSpinBox* spinBox);
    void spinBoxValueChanged(int newValue, QSlider* slider);
    void chooseColor(QWidget* colorWidget, QColor& mColor);
    void initColors();

    QColor m_colorRed;
    QColor m_colorGreen;
    QColor m_colorBlue;
    QColor m_colorH;


};

#endif 
