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

/*
    treeitem.cpp

    A container for items of data supplied by the simple tree model.
*/

#include <QStringList>

#include "treeitem.h"

TreeItem::TreeItem(const QList<QVariant> &data, TreeItem *parent) {
    parentItem = parent;
    itemData = data;
}

TreeItem::~TreeItem() { qDeleteAll(childItems); }

void TreeItem::appendChild(TreeItem *item) { childItems.append(item); }

TreeItem *TreeItem::child(int row) { return childItems.value(row); }

int TreeItem::childCount() const { return childItems.count(); }

int TreeItem::columnCount() const { return itemData.count(); }

QVariant TreeItem::data(int column) const { return itemData.value(column); }

TreeItem *TreeItem::parent() { return parentItem; }

int TreeItem::row() const {
    if (parentItem)
        return parentItem->childItems.indexOf(const_cast<TreeItem *>(this));

    return 0;
}
