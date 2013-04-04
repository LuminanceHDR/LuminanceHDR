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

#ifndef UPDATECHECKER_IMPL_H
#define UPDATECHECKER_IMPL_H

#include <QDialog>
#include <QStringList>
#include <QSystemTrayIcon>
#include <QNetworkAccessManager>
#include <QNetworkReply>

#include "MainWindow.h"

class UpdateAvailableInfo
{
public:
    QString version;
    QString url;
};

class UpdateChecker : public QObject
{
    Q_OBJECT
public:
	static void conditionallyShowUpdateChecker(MainWindow* parent);

    virtual ~UpdateChecker();
signals: 
    void updateAvailable(UpdateAvailableInfo* info);

protected: 
    UpdateChecker(QWidget *parent, QNetworkAccessManager* networkManager);

protected slots:
    void trayMessageClicked();
    void requestFinished(QNetworkReply*);

private:
    QSystemTrayIcon* m_tray;
    QString latestUrl;
    QNetworkAccessManager* m_networkManager;
};
#endif
