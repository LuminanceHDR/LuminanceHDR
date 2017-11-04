/**
 * This file is a part of LuminanceHDR package.
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
 */

#include <QCloseEvent>
#include <QDebug>
#include <QLinearGradient>
#include <QPainter>

#include <cassert>
#include <cmath>

#include "UI/GammaAndLevels.h"
#include "UI/ui_GammaAndLevels.h"

namespace {
static inline int clamp(const float &v, const float &minV, const float &maxV) {
    if (v <= minV) return minV;
    if (v >= maxV) return maxV;
    return (int)(v + 0.5f);
}
}

GammaAndLevels::GammaAndLevels(QWidget *parent, const QImage &data)
    : QDialog(parent, Qt::Dialog),
      m_ReferenceQImage(data),
      blackin(0),
      whitein(255),
      blackout(0),
      whiteout(255),
      gamma(1.0f),
      m_Ui(new Ui::LevelsDialog) {
    m_Ui->setupUi(this);

    QVBoxLayout *qvl = new QVBoxLayout;
    qvl->setMargin(0);
    qvl->setSpacing(1);

    histogram = new HistogramLDR(this);

    histogram->setData(&m_ReferenceQImage);
    histogram->setFrame(false);  // remove histogram frame

    gb1 = new GrayBar(m_Ui->inputStuffFrame);

    connect(m_Ui->black_in_spinbox, SIGNAL(valueChanged(int)), gb1,
            SLOT(changeBlack(int)));
    connect(m_Ui->gamma_spinbox, SIGNAL(valueChanged(double)), gb1,
            SLOT(changeGamma(double)));
    connect(m_Ui->white_in_spinbox, SIGNAL(valueChanged(int)), gb1,
            SLOT(changeWhite(int)));

    connect(gb1, &GrayBar::black_changed, this, &GammaAndLevels::updateBlackIn);
    connect(gb1, &GrayBar::gamma_changed, this, &GammaAndLevels::updateGamma);
    connect(gb1, &GrayBar::white_changed, this, &GammaAndLevels::updateWhiteIn);
    connect(gb1, &GrayBar::default_gamma_black_white, this,
            &GammaAndLevels::defaultGammaBlackWhiteIn);

    qvl->addWidget(histogram);
    qvl->addWidget(gb1);
    m_Ui->inputStuffFrame->setLayout(qvl);

    QVBoxLayout *qvl2 = new QVBoxLayout;
    qvl2->setMargin(0);
    qvl2->setSpacing(1);
    gb2 = new GrayBar(m_Ui->out_levels, true);
    connect(m_Ui->black_out_spinbox, SIGNAL(valueChanged(int)), gb2,
            SLOT(changeBlack(int)));
    connect(m_Ui->white_out_spinbox, SIGNAL(valueChanged(int)), gb2,
            SLOT(changeWhite(int)));

    connect(gb2, &GrayBar::black_changed, this,
            &GammaAndLevels::updateBlackOut);
    connect(gb2, &GrayBar::white_changed, this,
            &GammaAndLevels::updateWhiteOut);
    connect(gb2, &GrayBar::default_black_white, this,
            &GammaAndLevels::defaultBlackWhiteOut);

    connect(m_Ui->ResetButton, &QAbstractButton::clicked, gb1,
            &GrayBar::resetvalues);
    connect(m_Ui->ResetButton, &QAbstractButton::clicked, gb2,
            &GrayBar::resetvalues);
    connect(m_Ui->ResetButton, &QAbstractButton::clicked, this,
            &GammaAndLevels::resetValues);

    qvl2->addWidget(gb2);
    m_Ui->out_levels->setLayout(qvl2);
}

GammaAndLevels::~GammaAndLevels() {
    delete gb1;
    delete gb2;
    delete histogram;
}

void GammaAndLevels::defaultGammaBlackWhiteIn() {
    qDebug("GammaAndLevels::defaultGammaBlackWhiteIn");
    blackin = 0;
    gamma = 1.0f;
    whitein = 255;
}

void GammaAndLevels::defaultBlackWhiteOut() {
    qDebug("GammaAndLevels::defaultBlackWhiteOut");
    blackout = 0;
    whiteout = 255;
}

void GammaAndLevels::updateBlackIn(int v) {
    qDebug("GammaAndLevels::updateBlackIn");
    m_Ui->black_in_spinbox->setValue(v);
    blackin = v;
    refreshLUT();
}
void GammaAndLevels::updateGamma(double v) {
    qDebug("GammaAndLevels::updateGamma");
    gb1->dont_emit = true;
    m_Ui->gamma_spinbox->setValue(v);
    gamma = v;
    refreshLUT();
}
void GammaAndLevels::updateWhiteIn(int v) {
    qDebug("GammaAndLevels::updateWhiteIn");
    m_Ui->white_in_spinbox->setValue(v);
    whitein = v;
    refreshLUT();
}
void GammaAndLevels::updateBlackOut(int v) {
    qDebug("GammaAndLevels::updateBlackOut");
    m_Ui->black_out_spinbox->setValue(v);
    blackout = v;
    refreshLUT();
}
void GammaAndLevels::updateWhiteOut(int v) {
    qDebug("GammaAndLevels::updateWhiteOut");
    m_Ui->white_out_spinbox->setValue(v);
    whiteout = v;
    refreshLUT();
}

void GammaAndLevels::resetValues() {
#ifdef QT_DEBUG
    qDebug("GammaAndLevels::resetValues");
#endif

    blackin = 0;
    gamma = 1.0f;
    whitein = 255;
    blackout = 0;
    whiteout = 255;
    gb1->dont_emit = true;
    m_Ui->black_in_spinbox->setValue(0);
    m_Ui->gamma_spinbox->setValue(1);
    m_Ui->white_in_spinbox->setValue(255);
    m_Ui->black_out_spinbox->setValue(0);
    m_Ui->white_out_spinbox->setValue(255);
    refreshLUT();
}

void GammaAndLevels::refreshLUT() {
#ifdef QT_DEBUG
    qDebug() << "Update Look-Up-Table and send update QImage to viewer";
#endif

    // values in 0..1 range
    float bin = static_cast<float>(blackin) / 255.0f;
    float win = static_cast<float>(whitein) / 255.0f;
    float expgamma = 1.0f / gamma;

    // Build new QImage from the reference one
    const QRgb *src = reinterpret_cast<const QRgb *>(m_ReferenceQImage.bits());

    QImage previewimage(m_ReferenceQImage.width(), m_ReferenceQImage.height(),
                        QImage::Format_RGB32);
    QRgb *dst = reinterpret_cast<QRgb *>(previewimage.bits());

#pragma omp parallel for shared(src, dst)
    for (int i = 0; i < m_ReferenceQImage.width() * m_ReferenceQImage.height();
         ++i) {
        float red = static_cast<float>(qRed(src[i])) / 255.f;
        float green = static_cast<float>(qGreen(src[i])) / 255.f;
        float blue = static_cast<float>(qBlue(src[i])) / 255.f;

        float L = 0.2126f * red + 0.7152f * green +
                  0.0722f * blue;  // number between [0..1]

        float c = powf(L, expgamma - 1.0f);

        red = (red - bin) / (win - bin);
        red *= c;

        green = (green - bin) / (win - bin);
        green *= c;

        blue = (blue - bin) / (win - bin);
        blue *= c;

        dst[i] =
            qRgb(clamp(blackout + red * (whiteout - blackout), 0.f, 255.f),
                 clamp(blackout + green * (whiteout - blackout), 0.f, 255.f),
                 clamp(blackout + blue * (whiteout - blackout), 0.f, 255.f));
    }

    emit updateQImage(previewimage);
}

QImage GammaAndLevels::getReferenceQImage() { return m_ReferenceQImage; }

float GammaAndLevels::getBlackPointInput() { return (float)blackin / 255.f; }

float GammaAndLevels::getBlackPointOutput() { return (float)blackout / 255.f; }

float GammaAndLevels::getWhitePointInput() { return (float)whitein / 255.f; }

float GammaAndLevels::getWhitePointOutput() { return (float)whiteout / 255.f; }

float GammaAndLevels::getGamma() { return (1.0f / gamma); }

HistogramLDR::HistogramLDR(QWidget *parent)
    : QWidget(parent), isDrawFrame(true), isDrawColorHist(false) {
    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
}

void HistogramLDR::setData(const QImage *data) {
    for (int i = 0; i < 256; ++i) m_GreyHist[i] = 0.0f;
    for (int i = 0; i < 256; ++i) m_RedHist[i] = 0.0f;
    for (int i = 0; i < 256; ++i) m_GreenHist[i] = 0.0f;
    for (int i = 0; i < 256; ++i) m_BlueHist[i] = 0.0f;

    if (data->isNull()) return;

    // Build histogram
    const QRgb *pixels = (const QRgb *)(data->bits());
    const int ELEMS = data->width() * data->height();
    for (int i = 0; i < ELEMS; ++i) {
        m_GreyHist[qGray(pixels[i])] += 1.0f;

        m_RedHist[qRed(pixels[i])] += 1.0f;
        m_GreenHist[qGreen(pixels[i])] += 1.0f;
        m_BlueHist[qBlue(pixels[i])] += 1.0f;
    }

    // find max
    float hist_max = m_GreyHist[0];
    for (int i = 0; i < 256; ++i) hist_max = qMax(hist_max, m_GreyHist[i]);
    for (int i = 0; i < 256; ++i) hist_max = qMax(hist_max, m_RedHist[i]);
    for (int i = 0; i < 256; ++i) hist_max = qMax(hist_max, m_GreenHist[i]);
    for (int i = 0; i < 256; ++i) hist_max = qMax(hist_max, m_BlueHist[i]);

    // normalize in the range [0...1]
    for (int i = 0; i < 256; ++i) m_GreyHist[i] /= hist_max;
    for (int i = 0; i < 256; ++i) m_RedHist[i] /= hist_max;
    for (int i = 0; i < 256; ++i) m_GreenHist[i] /= hist_max;
    for (int i = 0; i < 256; ++i) m_BlueHist[i] /= hist_max;

    // qDebug() << "hist_max = "<< hist_max << "grey_hist_max = " <<
    // grey_hist_max;
}

void HistogramLDR::paintEvent(QPaintEvent *) {
    const qreal skew = (qreal)width() / 256;

    QPainter painter(this);

    // painter.setRenderHint(QPainter::Antialiasing, true); // antialiasing

    QPolygonF pol_grey;

    if (isDrawColorHist) {
        // reuse pol_grey to print also red/green/blue components
        pol_grey.clear();
        pol_grey << QPointF(0.0, height());
        for (int i = 0; i < 256; ++i) {
            pol_grey << QPointF(i * skew, (1.0 - m_RedHist[i]) * height());
            pol_grey << QPointF((i + 1) * skew,
                                (1.0 - m_RedHist[i]) * height());
        }
        // last point, bottom right corner
        pol_grey << QPointF(width(), height());

        // Draw histogram
        painter.setBrush(Qt::NoBrush);  //( QColor(255, 0, 0, 160) ); //
                                        // semi-transparent brush
        painter.setPen(QPen(QBrush(QColor(255, 0, 0, 255)), 1.0, Qt::SolidLine,
                            Qt::RoundCap, Qt::RoundJoin));
        painter.drawConvexPolygon(pol_grey);

        // reuse pol_grey to print also red/green/blue components
        pol_grey.clear();  // reuse of pol_grey
        pol_grey << QPointF(0.0, height());
        for (int i = 0; i < 256; ++i) {
            pol_grey << QPointF(i * skew, (1.0 - m_GreenHist[i]) * height());
            pol_grey << QPointF((i + 1) * skew,
                                (1.0 - m_GreenHist[i]) * height());
        }
        // last point, bottom right corner
        pol_grey << QPointF(width(), height());

        // Draw histogram
        painter.setBrush(Qt::NoBrush);  //( QColor(0, 255, 0, 160) ); //
                                        // semi-transparent brush
        painter.setPen(QPen(QBrush(QColor(0, 255, 0, 255)), 1.0, Qt::SolidLine,
                            Qt::RoundCap, Qt::RoundJoin));
        painter.drawConvexPolygon(pol_grey);

        pol_grey.clear();  // reuse of pol_grey
        pol_grey << QPointF(0.0, height());
        for (int i = 0; i < 256; ++i) {
            pol_grey << QPointF(i * skew, (1.0 - m_BlueHist[i]) * height());
            pol_grey << QPointF((i + 1) * skew,
                                (1.0 - m_BlueHist[i]) * height());
        }
        // last point, bottom right corner
        pol_grey << QPointF(width(), height());

        // Draw histogram
        painter.setBrush(Qt::NoBrush);  // QColor(0, 0, 255, 160) ); //
                                        // semi-transparent brush
        painter.setPen(QPen(QBrush(QColor(0, 0, 255, 255)), 1.0, Qt::SolidLine,
                            Qt::RoundCap, Qt::RoundJoin));
        painter.drawConvexPolygon(pol_grey);
    }

    // first point, left bottom corner
    pol_grey.clear();
    pol_grey << QPointF(0.0, height());
    for (int i = 0; i < 256; ++i) {
        pol_grey << QPointF(i * skew, (1.0 - m_GreyHist[i]) * height());
        pol_grey << QPointF((i + 1) * skew, (1.0 - m_GreyHist[i]) * height());
    }
    // last point, bottom right corner
    pol_grey << QPointF(width(), height());

    // Draw histogram
    painter.setBrush(QColor(160, 160, 160, 50));  // semi-transparent brush
    painter.setPen(QPen(QBrush(QColor(20, 20, 20, 255)), 1.0, Qt::SolidLine,
                        Qt::RoundCap, Qt::RoundJoin));
    painter.drawConvexPolygon(pol_grey);

    // Draw frame
    if (isDrawFrame) {
        painter.setPen(Qt::black);
        painter.setBrush(Qt::NoBrush);
        painter.drawRect(QRect(0, 0, width() - 1, height() - 1));
    }
}

void HistogramLDR::mouseDoubleClickEvent(QMouseEvent *event) {
    // repaint
    if (event->button() == Qt::LeftButton) {
        isDrawColorHist = !isDrawColorHist;  // revert condition
        repaint();
    }
}

void HistogramLDR::setFrame(bool b) { isDrawFrame = b; }

void HistogramLDR::setColorHistogram(bool b) { isDrawColorHist = b; }

QSize HistogramLDR::sizeHint() const { return QSize(255, 120); }

QSize HistogramLDR::minimumSizeHint() const { return QSize(255, 120); }

HistogramLDR::~HistogramLDR() {}

GrayBar::GrayBar(QWidget *parent, bool two_handles)
    : QWidget(parent), dont_emit(false) {
    twohandles = two_handles;
    dragging = DRAGNONE;
    //     qDebug("width=%d, height=%d",width(),height());
}

QSize GrayBar::sizeHint() const { return QSize(500, 22); }
QSize GrayBar::minimumSizeHint() const { return QSize(400, 22); }

void GrayBar::mouseMoveEvent(QMouseEvent *e) {
    if (dragging == DRAGNONE) return;

    // here we have to make sure that we keep the "order": black,(gray),white
    if (dragging == DRAGBLACK) {
        if (e->x() <= whitepos && e->x() >= 0) {
            // update graphical position of gray handle
            gammapos = e->x() + (int)(blackgrayratio * (whitepos - e->x()));
            // update graphical position of black handle
            blackpos = e->x();
            update();
        }
        return;
    }
    if (dragging == DRAGWHITE) {
        if (e->x() >= blackpos && e->x() <= width()) {
            // update graphical position of gray handle
            gammapos =
                e->x() - (int)((1.0f - blackgrayratio) * (e->x() - blackpos));
            // update graphical position of white handle
            whitepos = e->x();
            update();
        }
        return;
    }
    if (dragging == DRAGGRAY) {
        if (e->x() >= blackpos && e->x() <= whitepos) {
            // update graphical position of gray handle
            blackgrayratio =
                (float)(e->x() - blackpos) / (float)(whitepos - blackpos);
            // update graphical position of white handle
            gammapos = e->x();
            update();
        }
        return;
    }
}

void GrayBar::mousePressEvent(QMouseEvent *e) {
    dragging = findHandle(e->x(), e->y());
}

void GrayBar::mouseReleaseEvent(QMouseEvent *e) {
    if (dragging == DRAGBLACK) {
        emit black_changed((int)(255 * ((float)(blackpos) / (float)(width()))));
    }
    if (dragging == DRAGWHITE) {
        emit white_changed((int)(255 * ((float)(whitepos) / (float)(width()))));
    }
    if (dragging == DRAGGRAY) {
        float mediumpos =
            (float)blackpos + ((float)whitepos - (float)blackpos) / 2.0f;
        if (e->x() > mediumpos) {
// exp10f is not defined on MinGW in windows.
#ifdef _GNU_SOURCE
            emit gamma_changed(exp10f((mediumpos - (float)e->x()) /
                                      ((float)(whitepos)-mediumpos)));
        } else {
            emit gamma_changed(exp10f((mediumpos - (float)e->x()) /
                                      (mediumpos - (float)(blackpos))));
#else
            emit gamma_changed(powf(
                10.0f,
                (mediumpos - (float)e->x()) / ((float)(whitepos)-mediumpos)));
        } else {
            emit gamma_changed(powf(
                10.0f,
                (mediumpos - (float)e->x()) / (mediumpos - (float)(blackpos))));
#endif
        }
    }
    dragging = DRAGNONE;
    update();
}

GrayBar::draggingT GrayBar::findHandle(int x, int y) {
    QRect black_rect(blackpos - 25, 1 + (height() - 1) / 2, 50,
                     (height() - 1) - (1 + (height() - 1) / 2));
    QRect white_rect(whitepos - 25, 1 + (height() - 1) / 2, 50,
                     (height() - 1) - (1 + (height() - 1) / 2));

    // mouse click belongs to both white and black rects, and there's some space
    // on the left of the black coordinate
    if (black_rect.contains(x, y, false) && white_rect.contains(x, y, false) &&
        blackpos != 0) {
        return DRAGBLACK;
    }
    // mouse click belongs to both white and black rects, and there's some space
    // on the right of the white coordinate
    if (black_rect.contains(x, y, false) && white_rect.contains(x, y, false) &&
        whitepos != width()) {
        return DRAGWHITE;
    }

    // check if we clicked on black handle
    if (black_rect.contains(x, y, false)) {
        return DRAGBLACK;
    }

    // check if we clicked on white handle
    if (white_rect.contains(x, y, false)) {
        return DRAGWHITE;
    }

    // check if we clicked on gray handle
    if (!twohandles) {
        QRect gray_rect(gammapos - 25, 1 + (height() - 1) / 2, 50,
                        (height() - 1) - (1 + (height() - 1) / 2));
        if (gray_rect.contains(x, y, false)) {
            return DRAGGRAY;
        }
    }
    return DRAGNONE;
}

void GrayBar::resizeEvent(QResizeEvent *) {
    qDebug("GrayBar::resizeEvent");
    resetvalues();
    // this one below does not work, we resetvalues for the time being.
    //     float
    //     factor=(float)(e->size().width())/(float)(e->oldSize().width());
    //     qDebug("factor=%f",factor);
    //     blackpos=(int)( (float)blackpos*( factor ) );
    //     whitepos=(int)( (float)whitepos*( factor ) );
    //     gammapos=(int)( (float)gammapos*( factor ) );
    //     update();
}

void GrayBar::paintEvent(QPaintEvent *) {
    QPainter painter(this);
    QLinearGradient linearGradient(0, height() / 2, width(), height() / 2);
    linearGradient.setColorAt(0.0, Qt::black);
    linearGradient.setColorAt(0.5, Qt::darkGray);
    linearGradient.setColorAt(1.0, Qt::white);
    painter.setBrush(linearGradient);
    painter.setPen(Qt::NoPen);
    painter.drawRect(QRect(0, 0, width() - 1, (height() - 1) / 2));

    painter.setPen(Qt::black);
    // draw black triangle
    static QPoint black_tri[3] = {
        QPoint(blackpos, 1 + (height() - 1) / 2),
        QPoint(blackpos - 5, (height() - 1)),
        QPoint(blackpos + 5, (height() - 1)),
    };
    black_tri[0].setX(blackpos);
    black_tri[1].setX(blackpos - 5);
    black_tri[2].setX(blackpos + 5);
    black_tri[0].setY(1 + (height() - 1) / 2);
    black_tri[1].setY(height() - 1);
    black_tri[2].setY(height() - 1);
    painter.setBrush(QBrush(Qt::black));
    painter.drawPolygon(black_tri, 3);
    // draw white triangle
    static QPoint white_tri[3] = {
        QPoint(whitepos, 1 + (height() - 1) / 2),
        QPoint(whitepos - 5, (height() - 1)),
        QPoint(whitepos + 5, (height() - 1)),
    };
    white_tri[0].setX(whitepos);
    white_tri[1].setX(whitepos - 5);
    white_tri[2].setX(whitepos + 5);
    white_tri[0].setY(1 + (height() - 1) / 2);
    white_tri[1].setY(height() - 1);
    white_tri[2].setY(height() - 1);
    painter.setBrush(QBrush(Qt::white));
    painter.drawPolygon(white_tri, 3);
    // in case, draw gray triangle
    if (!twohandles) {
        static QPoint gray_tri[3] = {
            QPoint(gammapos, 1 + (height() - 1) / 2),
            QPoint(gammapos - 5, (height() - 1)),
            QPoint(gammapos + 5, (height() - 1)),
        };
        gray_tri[0].setX(gammapos);
        gray_tri[1].setX(gammapos - 5);
        gray_tri[2].setX(gammapos + 5);
        gray_tri[0].setY(1 + (height() - 1) / 2);
        gray_tri[1].setY(height() - 1);
        gray_tri[2].setY(height() - 1);
        painter.setBrush(QBrush(Qt::darkGray));
        painter.drawPolygon(gray_tri, 3);
    }
    //     qDebug("paint width=%d, height=%d",width(),height());
    //     qDebug("blackpos=%d, gammapos=%d,
    //     whitepos=%d",blackpos,gammapos,whitepos);
}

void GrayBar::resetvalues() {
    qDebug("GrayBar::resetvalues");
    blackpos = 0;
    gammapos = width() / 2;
    blackgrayratio = 0.5f;
    whitepos = width();

    if (twohandles)
        emit default_black_white();
    else
        emit default_gamma_black_white();
    update();
}

void GrayBar::changeBlack(int v) {
    if ((int)(255 * ((float)(blackpos) / (float)(width()))) == v) return;
    qDebug("GrayBar::changeBlack");
    blackpos = (int)(v * width() / 255.0f) < whitepos
                   ? (int)(v * width() / 255.0f)
                   : blackpos;
    gammapos = blackpos + (int)(blackgrayratio * (whitepos - blackpos));
    update();
    emit black_changed(v);
}

void GrayBar::changeGamma(double v) {
    float mediumpos =
        (float)blackpos + ((float)whitepos - (float)blackpos) / 2.0f;
    if (v < 1.0f) {
        gammapos = (int)(mediumpos - ((float)(whitepos)-mediumpos) * log10f(v));
    } else {
        gammapos =
            (int)(mediumpos - (mediumpos - (float)(blackpos)) * log10f(v));
    }
    qDebug("GrayBar::changeGamma %f", v);
    blackgrayratio =
        (float)(gammapos - blackpos) / (float)(whitepos - blackpos);
    update();
    if (dont_emit) {
        dont_emit = false;
        return;
    }
    emit gamma_changed(v);
}

void GrayBar::changeWhite(int v) {
    if ((int)(255 * ((float)(whitepos) / (float)(width()))) == v) return;
    qDebug("GrayBar::changeWhite, %d", v);
    whitepos = (int)(v * width() / 255.0f) > blackpos
                   ? (int)(v * width() / 255.0f)
                   : whitepos;
    gammapos =
        whitepos - (int)((1.0f - blackgrayratio) * (whitepos - blackpos));
    update();
    emit white_changed(v);
}
