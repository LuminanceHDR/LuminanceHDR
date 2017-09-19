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

#ifndef PROJECTIVE_TRANSF_H
#define PROJECTIVE_TRANSF_H

#include <QDialog>
#include <QFuture>
#include <QFutureWatcher>

namespace Ui {
class ProjectionsDialog;
}

namespace pfs {
class Frame;
}

class Projection;
class TransformInfo;

class ProjectionsDialog : public QDialog {
    Q_OBJECT
   public:
    ProjectionsDialog(QWidget *parent, pfs::Frame *orig);
    ~ProjectionsDialog();
    pfs::Frame *getTranformedFrame();
    TransformInfo *transforminfo;

   private:
    pfs::Frame *original, *transformed;
    QList<Projection *> projectionList;
    QScopedPointer<Ui::ProjectionsDialog> m_Ui;
    QFutureWatcher<void> m_futureWatcher;
    QFuture<void> m_future;
   private slots:
    void okClicked();
    void XRotChanged(int);
    void YRotChanged(int);
    void ZRotChanged(int);
    void oversampleChanged(int);
    void bilinearToggled(bool);
    void dstProjActivated(int);
    void srcProjActivated(int);
    void anglesAngularDestinationProj(int);
    void projectionFinished();
};

inline pfs::Frame *ProjectionsDialog::getTranformedFrame() {
    return transformed;
}

#endif
