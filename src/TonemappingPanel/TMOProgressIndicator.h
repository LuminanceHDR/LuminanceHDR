/*
 * This file is a part of LuminanceHDR package.
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
 *
 */

#ifndef TMOPROGRESSINDICATOR
#define TMOPROGRESSINDICATOR

#include <QHBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QWidget>

class TMOProgressIndicator : public QWidget {
    Q_OBJECT

   public:
    TMOProgressIndicator(QWidget *parent = 0);
    ~TMOProgressIndicator();
    bool isTerminated();
    void reset();
    void requestTermination() { emit terminate(true); }

   public slots:
    void setValue(int);
    void setMaximum(int);
    void setMinimum(int);
    void terminated();

   private:
    QHBoxLayout *m_hbl;
    QProgressBar *m_progressBar;
    QPushButton *m_abortButton;
    bool m_isTerminated;

   signals:
    void terminate(bool);
};

#endif
