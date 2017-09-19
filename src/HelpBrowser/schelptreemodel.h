/**
** This file is a part of Luminance HDR package.
** ----------------------------------------------------------------------
** Copyright (C) 2009-2016 Davide Anastasia, Franco Comida, Daniel Kaneider
**
*****************************************************************************
**
** Copyright (C) 2005-2007 Trolltech ASA. All rights reserved.
**
** This file is part of the example classes of the Qt Toolkit.
**
** This file may be used under the terms of the GNU General Public
** License version 2.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of
** this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
** http://www.trolltech.com/products/qt/opensource.html
**
** If you are unsure which license is appropriate for your use, please
** review the following information:
** http://www.trolltech.com/products/qt/licensing.html or contact the
** sales department at sales@trolltech.com.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#ifndef SCHELP_TREEMODEL_H
#define SCHELP_TREEMODEL_H

#include <QAbstractItemModel>
#include <QDomDocument>
#include <QModelIndex>
#include <QVariant>

#include "treemodel.h"

class ScHelpTreeModel : public TreeModel {
    // Q_OBJECT

   public:
    ScHelpTreeModel(const QString &dataFile, const QString &col1name,
                    const QString &col2name,
                    QMap<QString, QString> *indexToBuild, QObject *parent = 0);
    ~ScHelpTreeModel(){};

    void addRow(const QString &, const QString &, int i);

   private:
    void setupModelData(const QString &dataFile, TreeItem *parent,
                        QMap<QString, QString> *indexToBuild);
};

#endif
