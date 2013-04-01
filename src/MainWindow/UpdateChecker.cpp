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
#include <QDesktopServices>
#include <QNetworkRequest>
#include <QDomDocument>


#include "Common/global.h"
#include "Common/config.h"
#include "UpdateChecker.h"

UpdateChecker::UpdateChecker(QWidget *parent, QNetworkAccessManager* networkManager):
    QObject(parent),
    m_networkManager(networkManager),
    m_tray(NULL)
{
}

UpdateChecker::~UpdateChecker() {
    if (m_tray)
    {
        m_tray->hide();
        m_tray->deleteLater();
        m_networkManager->deleteLater();
    }
}

void UpdateChecker::trayMessageClicked()
{
    QDesktopServices::openUrl(QUrl(latestUrl));
    m_tray->hide();
    m_tray->deleteLater();
    m_tray = NULL;
    this->deleteLater();
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
                QString version = nodeVersion.toElement().text();
                latestUrl = nodeUrl.toElement().text();
                if (QSystemTrayIcon::supportsMessages())
                {
                    QWidget* widgetP = (QWidget*)parent();
                    m_tray = new QSystemTrayIcon(widgetP->windowIcon(), this);
                    m_tray->setToolTip(widgetP->windowTitle());
                    m_tray->show();
                    connect(m_tray, SIGNAL(messageClicked()), this, SLOT(trayMessageClicked()));
                    connect(m_tray, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this, SLOT(trayMessageClicked()));
                    m_tray->showMessage(tr("New Release: %1").arg(version), tr("A new release..."));
                }
                UpdateAvailableInfo* info = new UpdateAvailableInfo();
                info->version = version;
                info->url = latestUrl;
                emit updateAvailable(info);
            }        
        }
    }
}

void UpdateChecker::conditionallyShowUpdateChecker(MainWindow* parent) {
    QNetworkAccessManager* manager = new QNetworkAccessManager();
    UpdateChecker* dl = new UpdateChecker(parent, manager);
    connect(dl, SIGNAL(updateAvailable(UpdateAvailableInfo*)), parent, SLOT(on_updateAvailable(UpdateAvailableInfo*)));
    connect(manager, SIGNAL(finished(QNetworkReply*)), dl, SLOT(requestFinished(QNetworkReply*)));
    manager->get(QNetworkRequest(QUrl(QString("http://qtpfsgui.sourceforge.net/updater/get.php?c=%1").arg(LUMINANCEVERSION_NUM))));
}
