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
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QStringList>
#include <QSystemTrayIcon>

class UpdateChecker : public QObject {
    Q_OBJECT
   public:
    explicit UpdateChecker(QWidget *parent);
    virtual ~UpdateChecker();

   public slots:
    void trayMessageClicked() const;

   protected slots:
    void requestFinished(QNetworkReply *);

   signals:
    void updateAvailable();

   private:
    QSystemTrayIcon *m_tray;
    QNetworkAccessManager *m_networkManager;

    QString m_latestUrl;
    QString m_version;
};

#endif
