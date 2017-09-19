/**
 * This file is a part of Luminance HDR package.
 * ----------------------------------------------------------------------
 * Copyright (C) 2009 Franco Comida
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

#ifndef PREVIEWLABEL_IMPL_H
#define PREVIEWLABEL_IMPL_H

#include <QImage>
#include <QLabel>
#include <QMouseEvent>

#include "Core/TonemappingOptions.h"

class PreviewLabel : public QLabel {
    Q_OBJECT

   public:
    PreviewLabel(QWidget *parent = 0, TMOperator tm_operator = mantiuk06);
    PreviewLabel(QWidget *parent = 0,
                 TonemappingOptions *tonemappingOptions = 0, int index = -1);
    ~PreviewLabel();

    void setTonemappingOptions(TonemappingOptions *);
    TonemappingOptions *getTonemappingOptions();
    void setComment(QString);
    QString getComment();
    void setIndex(int);

   public Q_SLOTS:
    void assignNewQImage(QSharedPointer<QImage> new_qimage);

   protected:
    void mousePressEvent(QMouseEvent *event);
    void mouseDoubleClickEvent(QMouseEvent *event);

   signals:
    void clicked(TonemappingOptions *);
    void clicked(int);

   private:
    TonemappingOptions *m_TMOptions;
    int m_index;
    QString m_comment;
    bool m_isFromPanel;
};

inline TonemappingOptions *PreviewLabel::getTonemappingOptions() {
    return m_TMOptions;
}

inline void PreviewLabel::setTonemappingOptions(TonemappingOptions *tmopts) {
    if (m_TMOptions) delete m_TMOptions;
    m_TMOptions = new TonemappingOptions(*tmopts);
}
#endif
