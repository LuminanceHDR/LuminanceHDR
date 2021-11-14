
/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Nokia Corporation and its Subsidiary(-ies) nor
**     the names of its contributors may be used to endorse or promote
**     products derived from this software without specific prior written
**     permission.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
** $QT_END_LICENSE$
**
****************************************************************************/

//File initially based on treemodel.h from Qt 4.x, hardly anything like it now

/*
    treemodel.cpp

    Provides a simple tree model to show how to create and use hierarchical
    models.
*/

#include <QtGui>
#include <QDomDocument>
#include <QFile>

#include "schelptreemodel.h"

ScHelpTreeModel::ScHelpTreeModel(const QString &dataFile, const QString &col1name, const QString &col2name, QMap<QString, QString>* indexToBuild, QObject *parent)
    : TreeModel(parent)
{
	QList<QVariant> rootData;
	rootData << col1name << col2name;
	m_rootItem = new TreeItem(rootData);
	if (!dataFile.isEmpty())
		setupModelData(dataFile, m_rootItem, indexToBuild);
}

void ScHelpTreeModel::setupModelData(const QString &dataFile, TreeItem *parent, QMap<QString, QString>* indexToBuild)
{
	QFile file( dataFile );
	if ( !file.open( QIODevice::ReadOnly ) )
		return;
	QDomDocument doc( "menuentries" );
	if ( !doc.setContent( &file ) )
	{
		file.close();
		return;
	}
	file.close();

	QList<TreeItem*> parents;
	QList<int> indentations;
	parents << parent;
	indentations << 0;
	QDomElement docElem = doc.documentElement();
	QDomNode n = docElem.firstChild();
//	bool haveTutorials=false;
	QList<QVariant> columnData;
	int position=0;
	while (!n.isNull())
	{
		QDomElement e = n.toElement(); // try to convert the node to an element.
		if (!e.isNull())
		{
			if (e.hasAttribute( "text" ) && e.hasAttribute( "file" ))
			{
				QDomAttr textAttr = e.attributeNode( "text" );
				QDomAttr fileAttr = e.attributeNode( "file" );
				columnData.clear();
				columnData << textAttr.value() <<  fileAttr.value();
				if (position > indentations.last()) 
				{
					// The last child of the current parent is now the new parent
					// unless the current parent has no children.
		
					if (parents.last()->childCount() > 0) 
					{
						parents << parents.last()->child(parents.last()->childCount()-1);
						indentations << position;
					}
				}
				else 
				{
					while (position < indentations.last() && parents.count() > 0) {
						parents.pop_back();
						indentations.pop_back();
					}
				}
				// Append a new item to the current parent's list of children.
				parents.last()->appendChild(new TreeItem(columnData, parents.last()));
				if (indexToBuild)
					indexToBuild->insert(textAttr.value(), fileAttr.value());
			}

			QDomNodeList nl=n.childNodes();
			if (nl.count()>0)
				position=1;
			for (int i=0 ; i<= nl.count() ; i++)
			{
				QDomNode child=nl.item(i);
				if (child.isElement())
				{
					QDomElement ec = child.toElement();
					if (!ec.isNull())
					{
						if (ec.hasAttribute( "text" ) && ec.hasAttribute( "file" ))
						{
							QDomAttr textAttr = ec.attributeNode( "text" );
							QDomAttr fileAttr = ec.attributeNode( "file" );
							columnData.clear();
							columnData << textAttr.value() <<  fileAttr.value();
							if (position > indentations.last()) 
							{
								// The last child of the current parent is now the new parent
								// unless the current parent has no children.
					
								if (parents.last()->childCount() > 0) 
								{
									parents << parents.last()->child(parents.last()->childCount()-1);
									indentations << position;
								}
							}
							else 
							{
								while (position < indentations.last() && parents.count() > 0) {
									parents.pop_back();
									indentations.pop_back();
								}
							}
							// Append a new item to the current parent's list of children.
							parents.last()->appendChild(new TreeItem(columnData, parents.last()));
							if (indexToBuild)
								indexToBuild->insert(textAttr.value(), fileAttr.value());
						}
						//3rd level
						QDomNodeList nl2=child.childNodes();
						if (nl2.count() > 0)
							position = 2;
						for (int i2 = 0 ; i2 <= nl2.count(); i2++)
						{
							QDomNode childchild = nl2.item(i2);
							if (childchild.isElement())
							{
								QDomElement ecc = childchild.toElement();
								if (!ecc.isNull())
								{
									QDomAttr textAttr = ecc.attributeNode( "text" );
									QDomAttr fileAttr = ecc.attributeNode( "file" );
									columnData.clear();
									columnData << textAttr.value() <<  fileAttr.value();
									if (position > indentations.last()) 
									{
										// The last child of the current parent is now the new parent
										// unless the current parent has no children.
							
										if (parents.last()->childCount() > 0) 
										{
											parents << parents.last()->child(parents.last()->childCount()-1);
											indentations << position;
										}
									}
									else 
									{
										while (position < indentations.last() && parents.count() > 0) {
											parents.pop_back();
											indentations.pop_back();
										}
									}
									// Append a new item to the current parent's list of children.
									parents.last()->appendChild(new TreeItem(columnData, parents.last()));
									if (indexToBuild)
										indexToBuild->insert(textAttr.value(), fileAttr.value());
								}
							}
						}
						position=1;
					}
				}
			}
			position=0;
		}
		n = n.nextSibling();
	}
}

void ScHelpTreeModel::addRow(const QString& s1, const QString& s2, int i)
{
	QList<TreeItem*> parents;
//	QList<int> indentations;
	parents << m_rootItem;
	if (parents.last()->childCount() > 0) 
		parents << parents.last()->child(parents.last()->childCount()-1);
	QList<QVariant> columnData;
	columnData << s1 << s2;// << i;
	parents.last()->appendChild(new TreeItem(columnData, parents.last()));
}
