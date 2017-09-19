/*
 * This file is a part of LuminanceHDR package.
 * ----------------------------------------------------------------------
 * Copyright (C) 2015 Daniel Kaneider
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
 */

//! @author Daniel Kaneider <danielkaneider@users.sourceforge.net>

#include "formathelper.h"
#include "Common/config.h"

#include <UI/ImageQualityDialog.h>
#include "UI/TiffModeDialog.h"

namespace pfsadditions {

FormatHelper::FormatHelper()
    : QObject(), m_comboBox(NULL), m_settingsButton(NULL), m_hdr(false) {}

void FormatHelper::initConnection(QComboBox *comboBox,
                                  QAbstractButton *settingsButton, bool hdr) {
    m_comboBox = comboBox;
    m_settingsButton = settingsButton;
    m_hdr = hdr;

    if (hdr) {
        // FrameWriterFactory::sm_registry
        comboBox->addItem(QObject::tr("HDR"), QVariant(0));
        comboBox->addItem(QObject::tr("EXR"), QVariant(1));
        comboBox->addItem(QObject::tr("TIFF"), QVariant(2));
        comboBox->addItem(QObject::tr("PFS"), QVariant(3));
    } else {
        comboBox->addItem(QObject::tr("TIFF"), QVariant(20));
        comboBox->addItem(QObject::tr("JPEG"), QVariant(21));
        comboBox->addItem(QObject::tr("PNG"), QVariant(22));
        comboBox->addItem(QObject::tr("BMP"), QVariant(23));
        comboBox->addItem(QObject::tr("PPM"), QVariant(24));
        comboBox->addItem(QObject::tr("PBM"), QVariant(25));
    }

    connect(comboBox, static_cast<void (QComboBox::*)(int)>(
                          &QComboBox::currentIndexChanged),
            this, &FormatHelper::comboBoxIndexChanged);
    connect(settingsButton, &QAbstractButton::pressed, this,
            &FormatHelper::buttonPressed);

    const int format = hdr ? 0 : 20;
    setDefaultParams(format);
    updateButton(format);
}

QString FormatHelper::getFileExtension() {
    int format = m_comboBox->currentData().toInt();
    return getFileExtensionForFormat(format);
}

QString FormatHelper::getFileExtensionForFormat(int format) {
    switch (format) {
        case 0:
            return ("hdr");
        case 1:
            return ("exr");
        case 2:
            return ("tiff");
        case 3:
            return ("pfs");
        case 20:
            return ("tiff");
        case 21:
            return ("jpg");
        case 22:
            return ("png");
        case 23:
            return ("bmp");
        case 24:
            return ("ppm");
        case 25:
            return ("pbm");
    }
    return QStringLiteral("tiff");
}

void FormatHelper::setDefaultParams(int format) {
    m_params = getParamsForFormat(format);
}

pfs::Params FormatHelper::getParamsForFormat(int format) {
    pfs::Params params;
    switch (format) {
        case 0:
            params.set("format", std::string("hdr"));
            break;
        case 1:
            params.set("format", std::string("exr"));
            break;
        case 2:
            params.set("format", std::string("tiff"));
            break;
        case 3:
            params.set("format", std::string("pfs"));
            break;
        case 20:
            params.set("format", std::string("tiff"));
            break;
        case 21:
            params.set("format", std::string("jpg"));
            params.set("quality", 100);
            break;
        case 22:
            params.set("format", std::string("png"));
            params.set("quality", 100);
            break;
        case 23:
        case 24:
        case 25:
            // default params
            break;
        default:
            break;
    }
    params.set("fileextension", getFileExtensionForFormat(format));
    return params;
}

void FormatHelper::comboBoxIndexChanged(int idx) {
    int format = m_comboBox->currentData().toInt();
    setDefaultParams(format);
    updateButton(format);
}

void FormatHelper::buttonPressed() {
    int format = m_comboBox->currentData().toInt();
    switch (format) {
        case 2:
        case 20: {
            int tiffMode;
            if (!m_params.get("tiff_mode", tiffMode)) tiffMode = -1;

            TiffModeDialog t(format == 2, tiffMode, m_settingsButton);
            if (t.exec() == QDialog::Accepted) {
                m_params.set("tiff_mode", t.getTiffWriterMode());
            }
        } break;
        case 21:
        case 22: {
            size_t quality;
            int qual = -1;
            if (m_params.get("quality", quality)) qual = quality;

            ImageQualityDialog d(NULL, format == 21 ? "png" : "jpg", qual,
                                 m_settingsButton);
            if (d.exec() == QDialog::Accepted) {
                size_t quality = d.getQuality();
                m_params.set("quality", quality);
            }

        } break;
        case 3:
            m_params.set("format", std::string("pfs"));
            break;
        default:
            break;
    }
}

void FormatHelper::updateButton(int format) {
    bool enabled = format == 2       // tiff
                   || format == 20   // tiff-dr
                   || format == 21   // jpg
                   || format == 22;  // png
    m_settingsButton->setEnabled(enabled);
}

pfs::Params FormatHelper::getParams() { return m_params; }

void FormatHelper::loadFromSettings(const QString prefix) {
    int format = LuminanceOptions()
                     .value(prefix + "/" + KEY_EXPORT_FORMAT, m_hdr ? 0 : 20)
                     .toInt();
    m_comboBox->setCurrentIndex(m_comboBox->findData(format));
    m_params = FormatHelper::getParamsFromSettings(prefix, m_hdr);
}

void FormatHelper::writeSettings(const QString prefix) {
    LuminanceOptions().setValue(prefix + "/" + KEY_EXPORT_FORMAT,
                                m_comboBox->currentData().toInt());
    int tiffMode;
    if (m_params.get("tiff_mode", tiffMode)) {
        LuminanceOptions().setValue(prefix + "/" + KEY_EXPORT_TIFF_MODE,
                                    tiffMode);
    }
    size_t quality;
    if (m_params.get("quality", quality)) {
        int qual = quality;
        LuminanceOptions().setValue(prefix + "/" + KEY_EXPORT_QUALITY, qual);
    }
}

pfs::Params FormatHelper::getParamsFromSettings(const QString prefix,
                                                bool hdr) {
    int format = LuminanceOptions()
                     .value(prefix + "/" + KEY_EXPORT_FORMAT, hdr ? 0 : 20)
                     .toInt();
    pfs::Params params = FormatHelper::getParamsForFormat(format);

    int tiffMode = LuminanceOptions()
                       .value(prefix + "/" + KEY_EXPORT_TIFF_MODE, -1)
                       .toInt();
    if (tiffMode >= 0) params.set("tiff_mode", tiffMode);
    int quality =
        LuminanceOptions().value(prefix + "/" + KEY_EXPORT_QUALITY, -1).toInt();
    if (quality >= 0) {
        size_t qual = quality;
        params.set("quality", qual);
    }
    return params;
}
}
