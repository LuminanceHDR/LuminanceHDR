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

#ifndef DNDOPTION_IMPL_H
#define DNDOPTION_IMPL_H

#include <QDialog>
#include <QStringList>

// forward declaration
namespace Ui {
class DnDOption;
}

class DnDOptionDialog : public QDialog {
    Q_OBJECT

   public:
    static const int ACTION_INVALID = 0;
    static const int ACTION_NEW_HDR = 1;
    static const int ACTION_OPEN_HDR = 2;

    static int showDndDialog(QWidget *parent,
                             QStringList files);  // 1=newHDR, 2=openHDR

    DnDOptionDialog(QWidget *parent, QStringList files, bool allHdrs,
                    bool allLdrs);
    ~DnDOptionDialog();

   protected slots:
    void on_btnCancel_clicked();
    void on_btnCreateNewHDR_clicked();
    void on_btnOpenHDR_clicked();

   private:
    QScopedPointer<Ui::DnDOption> ui;
    int result;  // 1=newHDR, 2=openHDR
};
#endif
