/**
 * This file is a part of LuminanceHDR package.
 * ----------------------------------------------------------------------
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
 * @author Daniel Kaneider
 */

#include <QDebug>
#include <QDomDocument>
#include <QDesktopServices>
#include <QNetworkRequest>

#include "Common/global.h"
#include "Common/config.h"
#include "UI/UMessageBox.h"
#include "UpdateChecker.h"

UpdateChecker::UpdateChecker(QWidget *parent) //, QNetworkAccessManager* networkManager)
    : QObject(parent)
    , m_tray(new QSystemTrayIcon(parent->windowIcon()))
    , m_networkManager(new QNetworkAccessManager())
{
    connect(this, SIGNAL(updateAvailable()), parent, SLOT(on_updateAvailable()));
    connect(m_networkManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(requestFinished(QNetworkReply*)));
    connect(m_tray, SIGNAL(messageClicked()), this, SLOT(trayMessageClicked()));
    connect(m_tray, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this, SLOT(trayMessageClicked()));

    m_networkManager->get(QNetworkRequest(QUrl(QString("http://qtpfsgui.sourceforge.net/updater/get.php?c=%1").arg(LUMINANCEVERSION_NUM))));
}

UpdateChecker::~UpdateChecker() {
    m_tray->hide();
    m_tray->deleteLater();
    m_networkManager->deleteLater();
}

void UpdateChecker::trayMessageClicked() const
{
    QDesktopServices::openUrl(QUrl(m_latestUrl));
    m_tray->hide();
}

void UpdateChecker::requestFinished(QNetworkReply* reply)
{
    if (reply->error() == QNetworkReply::NoError)
    {
        QDomDocument document;
        document.setContent(reply);

        //<?xml version="1.0"?>
        //<root>
        //<version>2.3.1</version>
        //<url>http://qtpfsgui.sourceforge.net/?p=251</url>
        //</root>

        QDomNode nodeRoot = document.documentElement();
        if (!nodeRoot.isNull())
        {
            QDomNode nodeVersion = nodeRoot.firstChildElement("version");
            QDomNode nodeUrl = nodeRoot.firstChildElement("url");
            if (!nodeVersion.isNull() && !nodeUrl.isNull())
            {
                m_version = nodeVersion.toElement().text();
                m_latestUrl = nodeUrl.toElement().text();
                
                if (m_version == LUMINANCEVERSION) return;

                qDebug() << m_version;

                QString msgTitle = QString("Luminance HDR %1").arg(m_version);
                QString msgContent = tr("A new release is ready for download!");
                QWidget* widgetP = (QWidget*)parent();

                emit updateAvailable();

#if defined(Q_WS_MAC) || defined(Q_WS_X11)
                if ( UMessageBox::question(msgTitle, msgContent + "\n\n" +
                                      tr("Do you want to open the webpage for download now?"),
                                           widgetP)
                     == QMessageBox::Yes )
                {
                    trayMessageClicked();
                }
#else
                if (QSystemTrayIcon::supportsMessages())
                {
                    m_tray->setToolTip(widgetP->windowTitle());
                    m_tray->show();
                    m_tray->showMessage(msgTitle, msgContent + "\n" +
                                        tr("Click to download, or select Help->Update!"));
                }
                else
                {
                    if ( UMessageBox::question(msgTitle, msgContent + "\n\n" +
                                          tr("Do you want to open the webpage for download now?"),
                                               widgetP)
                         == QMessageBox::Yes )
                    {
                        trayMessageClicked();
                    }
                }
#endif
            }        
        }
    }
}
