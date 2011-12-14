/**
 * This file is a part of Luminance HDR package.
 * ----------------------------------------------------------------------
 * Copyright (C) 2011 Davide Anastasia
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
 * This file provides an unified style for all the QMessageBox in Luminance HDR
 *
 * Original Work
 * @author Davide Anastasia <davideanastasia@users.sourceforge.net>
 *
 */

#ifndef __UNIVERSAL_MESSAGE_BOX__
#define __UNIVERSAL_MESSAGE_BOX__

#include <QMessageBox>
#include <QString>
#include <QEvent>
#include <QSpacerItem>
#include <QGridLayout>
#include <QStringList>
#include <QFile>
#include <QTextStream>
#include <QDebug>

class UMessageBox : public QMessageBox
{
private:
    // workaround to set size
    QSpacerItem* m_horizontalSpacer;
    QGridLayout* m_layout;

    void init();
public:
  explicit UMessageBox(QWidget *parent = 0);
  
  UMessageBox(const QString &title, const QString &text, Icon icon,
               int button0, int button1, int button2,
               QWidget *parent = 0,
               Qt::WindowFlags f = Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint);

  virtual void showEvent(QShowEvent *event);

  static void about(QWidget* parent = 0);

  static int warning(QString title, QString description, QWidget* parent = 0);

  static int saveDialog(QString title, QString description, QWidget* parent = 0);

  /*
   * Function not yet used, it will... :)
   */
  static void donationSplashMB(QWidget* parent);
};

#endif
