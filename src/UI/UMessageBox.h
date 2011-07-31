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

#include "ui_about.h"
#include "Common/config.h"
#include "Common/global.h"
#include "Common/options.h"
#include "Common/archs.h"

class UMessageBox : public QMessageBox
{
private:
    // workaround to set size
    QSpacerItem* m_horizontalSpacer;
    QGridLayout* m_layout;
public:
  explicit UMessageBox(QWidget *parent = 0);
  
  UMessageBox(const QString &title, const QString &text, Icon icon,
               int button0, int button1, int button2,
               QWidget *parent = 0,
               Qt::WindowFlags f = Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint);
  
  virtual void showEvent(QShowEvent *event);

  static void about(QWidget* parent = 0)
  {
      QDialog *about = new QDialog(parent);
      about->setAttribute(Qt::WA_DeleteOnClose);
      Ui::AboutLuminance ui;
      ui.setupUi(about);
      ui.authorsBox->setOpenExternalLinks(true);
      ui.thanksToBox->setOpenExternalLinks(true);
      ui.GPLbox->setTextInteractionFlags(Qt::TextSelectableByMouse);
      ui.label_version->setText(ui.label_version->text().append(QString(LUMINANCEVERSION)));

      bool license_file_not_found=true;
      QString docDir = QCoreApplication::applicationDirPath();
      docDir.append("/../Resources");
      QStringList paths = QStringList( BASEDIR "/share/doc/luminance-hdr") << BASEDIR "/share/luminance-hdr" << docDir << "/Applications/luminance.app/Contents/Resources" << "./" << QCoreApplication::applicationDirPath();
      foreach (QString path,paths)
      {
          QString fname(path+QString("/LICENSE"));
#ifdef WIN32
          fname+=".txt";
#endif
          if (QFile::exists(fname))
          {
              QFile file(fname);
              //try opening it
              if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
                  break;
              QTextStream ts(&file);
              ui.GPLbox->setAcceptRichText(false);
              ui.GPLbox->setPlainText(ts.readAll());
              license_file_not_found=false;
              break;
          }
      }
      if (license_file_not_found)
      {
          ui.GPLbox->setOpenExternalLinks(true);
          ui.GPLbox->setTextInteractionFlags(Qt::TextBrowserInteraction);
          ui.GPLbox->setHtml(tr("%1 License document not found, you can find it online: %2here%3","%2 and %3 are html tags").arg("<html>").arg("<a href=\"http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt\">").arg("</a></html>"));
      }
      about->show();
  }

  static int warning(QString title, QString description, QWidget* parent = 0)
  {
      UMessageBox WarningMsgBox(parent);
      WarningMsgBox.setText(title);
      WarningMsgBox.setInformativeText(description);
      WarningMsgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
      WarningMsgBox.setDefaultButton(QMessageBox::No);
      WarningMsgBox.setIcon(QMessageBox::Warning);

      return WarningMsgBox.exec();
  }

  static int saveDialog(QString title, QString description, QWidget* parent = 0)
  {
      UMessageBox WarningMsgBox(parent);
      WarningMsgBox.setText(title);
      WarningMsgBox.setInformativeText(description);
      WarningMsgBox.setStandardButtons(QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
      WarningMsgBox.setDefaultButton(QMessageBox::Cancel);
      WarningMsgBox.setIcon(QMessageBox::Warning);

      return WarningMsgBox.exec();
  }

  /*
   * Function not yet used, it will... :)
   */
  static void donationSplashMB(QWidget* parent = 0)
  {
      UMessageBox donationMB(parent);
      donationMB.setText(tr("Donation"));
      donationMB.setInformativeText(tr("Would you like to donate?"));
      donationMB.setIcon(QMessageBox::Question);

      QPushButton *yes = donationMB.addButton(tr("Yes, I'd love to!"), QMessageBox::ActionRole);
      QPushButton *dontBother = donationMB.addButton(tr("Stop Bothering Me"), QMessageBox::ActionRole);
      QPushButton *later = donationMB.addButton(tr("Remind me later"),  QMessageBox::ActionRole);

      donationMB.exec();

      if (donationMB.clickedButton() == yes)
      {
          qDebug() << "Open Donation Web page";
      }
      else if (donationMB.clickedButton() == dontBother)
      {
          qDebug() << "Stop bother me";

      } else if (donationMB.clickedButton() == later)
      {
          qDebug() << "Remind me later";
      }

  }
};

#endif
