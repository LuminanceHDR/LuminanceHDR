/**
 * This file is a part of Luminance HDR package.
 * ----------------------------------------------------------------------
 * Copyright (C) 2011 Franco Comida
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
 * @author Franco Comida <fcomida@users.sourceforge.net>
 *
 */

#include <QDebug>
#include <QAbstractItemView>
#include <QHeaderView>
#include <QModelIndex>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>

#include "SavedParametersDialog.h"
#include "ui_SavedParametersDialog.h"

SavedParametersDialog::SavedParametersDialog(QWidget *parent):
    QDialog(parent),
    model(new QSqlTableModel()),
    m_Ui(new Ui::SavedParametersDialog)
{
    m_Ui->setupUi(this);
    //QSqlDatabase db = QSqlDatabase::database();

    // Create a temp table
    QSqlQuery query;
    bool res = query.exec("CREATE TABLE comments (comment varchar(150), operator varchar(15));");
    if (res == false)
        qDebug() << query.lastError();

    /*			res = query.exec("SELECT ashikhmin.comment, drago.comment,"
                                            "durand.comment, fattal.comment, mantiuk06.comment,"
                                            "mantiuk08.comment, pattanaik.comment,"
                                            "reinhard02.comment, reinhard05.comment"
                                            " FROM ashikhmin, drago,"
                                            "durand, fattal, mantiuk06, mantiuk08, pattanaik, reinhard02, reinhard05");
*/
    // Ashikhmin
    res = query.exec("SELECT ashikhmin.comment FROM ashikhmin");
    if (res == false)
        qDebug() << query.lastError();
    while (query.next())
    {
        QSqlQuery insertQuery;
        insertQuery.prepare("INSERT into comments (comment, operator)"
                            "VALUES (:comment, :operator)");
        insertQuery.bindValue(":comment", query.value(0).toString());
        insertQuery.bindValue(":operator", "ashikhmin");
        res = insertQuery.exec();
        if (res == false)
            qDebug() << insertQuery.lastError();
        qDebug() << query.value(0).toString();
    }
    // Drago
    res = query.exec("SELECT drago.comment FROM drago");
    if (res == false)
        qDebug() << query.lastError();
    while (query.next())
    {
        QSqlQuery insertQuery;
        insertQuery.prepare("INSERT into comments (comment, operator)"
                            "VALUES (:comment, :operator)");
        insertQuery.bindValue(":comment", query.value(0).toString());
        insertQuery.bindValue(":operator", "drago");
        res = insertQuery.exec();
        if (res == false)
            qDebug() << insertQuery.lastError();
        qDebug() << query.value(0).toString();
    }
    // Durand
    res = query.exec("SELECT durand.comment FROM durand");
    if (res == false)
        qDebug() << query.lastError();
    while (query.next())
    {
        QSqlQuery insertQuery;
        insertQuery.prepare("INSERT into comments (comment, operator)"
                            "VALUES (:comment, :operator)");
        insertQuery.bindValue(":comment", query.value(0).toString());
        insertQuery.bindValue(":operator", "durand");
        res = insertQuery.exec();
        if (res == false)
            qDebug() << insertQuery.lastError();
        qDebug() << query.value(0).toString();
    }
    // Fattal
    res = query.exec("SELECT fattal.comment FROM fattal");
    if (res == false)
        qDebug() << query.lastError();
    while (query.next())
    {
        QSqlQuery insertQuery;
        insertQuery.prepare("INSERT into comments (comment, operator)"
                            "VALUES (:comment, :operator)");
        insertQuery.bindValue(":comment", query.value(0).toString());
        insertQuery.bindValue(":operator", "fattal");
        res = insertQuery.exec();
        if (res == false)
            qDebug() << insertQuery.lastError();
        qDebug() << query.value(0).toString();
    }
    // Mantiuk 06
    res = query.exec("SELECT mantiuk06.comment FROM mantiuk06");
    if (res == false)
        qDebug() << query.lastError();
    while (query.next())
    {
        QSqlQuery insertQuery;
        insertQuery.prepare("INSERT into comments (comment, operator)"
                            "VALUES (:comment, :operator)");
        insertQuery.bindValue(":comment", query.value(0).toString());
        insertQuery.bindValue(":operator", "mantiuk06");
        res = insertQuery.exec();
        if (res == false)
            qDebug() << insertQuery.lastError();
        qDebug() << query.value(0).toString();
    }
    // Mantiuk 08
    res = query.exec("SELECT mantiuk08.comment FROM mantiuk08");
    if (res == false)
        qDebug() << query.lastError();
    while (query.next())
    {
        QSqlQuery insertQuery;
        insertQuery.prepare("INSERT into comments (comment, operator)"
                            "VALUES (:comment, :operator)");
        insertQuery.bindValue(":comment", query.value(0).toString());
        insertQuery.bindValue(":operator", "mantiuk08");
        res = insertQuery.exec();
        if (res == false)
            qDebug() << insertQuery.lastError();
        qDebug() << query.value(0).toString();
    }
    // Pattanaik
    res = query.exec("SELECT pattanaik.comment FROM pattanaik");
    if (res == false)
        qDebug() << query.lastError();
    while (query.next())
    {
        QSqlQuery insertQuery;
        insertQuery.prepare("INSERT into comments (comment, operator)"
                            "VALUES (:comment, :operator)");
        insertQuery.bindValue(":comment", query.value(0).toString());
        insertQuery.bindValue(":operator", "pattanaik");
        res = insertQuery.exec();
        if (res == false)
            qDebug() << insertQuery.lastError();
        qDebug() << query.value(0).toString();
    }
    // Reinhard 02
    res = query.exec("SELECT reinhard02.comment FROM reinhard02");
    if (res == false)
        qDebug() << query.lastError();
    while (query.next())
    {
        QSqlQuery insertQuery;
        insertQuery.prepare("INSERT into comments (comment, operator)"
                            "VALUES (:comment, :operator)");
        insertQuery.bindValue(":comment", query.value(0).toString());
        insertQuery.bindValue(":operator", "reinhard02");
        res = insertQuery.exec();
        if (res == false)
            qDebug() << insertQuery.lastError();
        qDebug() << query.value(0).toString();
    }
    // Reinhard 05
    res = query.exec("SELECT reinhard05.comment FROM reinhard05");
    if (res == false)
        qDebug() << query.lastError();
    while (query.next())
    {
        QSqlQuery insertQuery;
        insertQuery.prepare("INSERT into comments (comment, operator)"
                            "VALUES (:comment, :operator)");
        insertQuery.bindValue(":comment", query.value(0).toString());
        insertQuery.bindValue(":operator", "reinhard05");
        res = insertQuery.exec();
        if (res == false)
            qDebug() << insertQuery.lastError();
        qDebug() << query.value(0).toString();
    }

    model->setTable("comments");
    model->select();
    model->setHeaderData(0, Qt::Horizontal, tr("Comment"));
    model->setHeaderData(1, Qt::Horizontal, tr("TM Operator"));

    m_Ui->tableView->setModel(model);
    m_Ui->tableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_Ui->tableView->horizontalHeader()->setResizeMode(QHeaderView::ResizeToContents);
    m_Ui->tableView->show();
}


SavedParametersDialog::SavedParametersDialog(TMOperator op, QWidget *parent):
    QDialog(parent),
    model(new QSqlTableModel()),
    m_Ui(new Ui::SavedParametersDialog)
{
    m_Ui->setupUi(this);
    //QSqlDatabase db = QSqlDatabase::database();

    switch (op)
    {
    case ashikhmin:
        model->setTable("ashikhmin");
        model->select();
        model->setHeaderData(0, Qt::Horizontal, tr("Simple"));
        model->setHeaderData(1, Qt::Horizontal, tr("Equation 2"));
        model->setHeaderData(2, Qt::Horizontal, tr("Local Contrast Threshold"));
        model->setHeaderData(3, Qt::Horizontal, tr("Pre-gamma"));
        model->setHeaderData(4, Qt::Horizontal, tr("Comment"));
        break;
    case drago:
        model->setTable("drago");
        model->select();
        model->setHeaderData(0, Qt::Horizontal, tr("Bias"));
        model->setHeaderData(1, Qt::Horizontal, tr("Pre-gamma"));
        model->setHeaderData(2, Qt::Horizontal, tr("Comment"));
        break;
    case durand:
        model->setTable("durand");
        model->select();
        model->setHeaderData(0, Qt::Horizontal, tr("Spatial Kernel Sigma"));
        model->setHeaderData(1, Qt::Horizontal, tr("Range Kernel Sigma"));
        model->setHeaderData(2, Qt::Horizontal, tr("Base Contrast"));
        model->setHeaderData(3, Qt::Horizontal, tr("Pre-gamma"));
        model->setHeaderData(4, Qt::Horizontal, tr("Comment"));
        break;
    case fattal:
        model->setTable("fattal");
        model->select();
        model->setHeaderData(0, Qt::Horizontal, tr("Alpha"));
        model->setHeaderData(1, Qt::Horizontal, tr("Beta"));
        model->setHeaderData(2, Qt::Horizontal, tr("Color Saturation"));
        model->setHeaderData(3, Qt::Horizontal, tr("Noise Reduction"));
        model->setHeaderData(4, Qt::Horizontal, tr("Old Fattal"));
        model->setHeaderData(5, Qt::Horizontal, tr("Pre-gamma"));
        model->setHeaderData(6, Qt::Horizontal, tr("Comment"));
        break;
    case mantiuk06:
        model->setTable("mantiuk06");
        model->select();
        model->setHeaderData(0, Qt::Horizontal, tr("Contrast Equalization"));
        model->setHeaderData(1, Qt::Horizontal, tr("Contrast Factor"));
        model->setHeaderData(2, Qt::Horizontal, tr("Saturation Factor"));
        model->setHeaderData(3, Qt::Horizontal, tr("Detail Factor"));
        model->setHeaderData(4, Qt::Horizontal, tr("Pre-gamma"));
        model->setHeaderData(5, Qt::Horizontal, tr("Comment"));
        break;
    case mantiuk08:
        model->setTable("mantiuk08");
        model->select();
        model->setHeaderData(0, Qt::Horizontal, tr("Color Saturation"));
        model->setHeaderData(1, Qt::Horizontal, tr("Contrast Enhancement"));
        model->setHeaderData(2, Qt::Horizontal, tr("Luminance Level"));
        model->setHeaderData(3, Qt::Horizontal, tr("Manual Luminance Level"));
        model->setHeaderData(4, Qt::Horizontal, tr("Pre-gamma"));
        model->setHeaderData(5, Qt::Horizontal, tr("Comment"));
        break;
    case pattanaik:
        model->setTable("pattanaik");
        model->select();
        model->setHeaderData(0, Qt::Horizontal, tr("Cone and Rod based on Luminance"));
        model->setHeaderData(1, Qt::Horizontal, tr("Local Tonemapping"));
        model->setHeaderData(2, Qt::Horizontal, tr("Cone Level"));
        model->setHeaderData(3, Qt::Horizontal, tr("Rod Level"));
        model->setHeaderData(4, Qt::Horizontal, tr("Multiplier"));
        model->setHeaderData(5, Qt::Horizontal, tr("Pre-gamma"));
        model->setHeaderData(6, Qt::Horizontal, tr("Comment"));
        break;
    case reinhard02:
        model->setTable("reinhard02");
        model->select();
        model->setHeaderData(0, Qt::Horizontal, tr("Use Scales"));
        model->setHeaderData(1, Qt::Horizontal, tr("Key Value"));
        model->setHeaderData(2, Qt::Horizontal, tr("Phi Value"));
        model->setHeaderData(3, Qt::Horizontal, tr("Range"));
        model->setHeaderData(4, Qt::Horizontal, tr("Lower Scale"));
        model->setHeaderData(5, Qt::Horizontal, tr("Upper Scale"));
        model->setHeaderData(6, Qt::Horizontal, tr("Pre-gamma"));
        model->setHeaderData(7, Qt::Horizontal, tr("Comment"));
        break;
    case reinhard05:
        model->setTable("reinhard05");
        model->select();
        model->setHeaderData(0, Qt::Horizontal, tr("Brightness"));
        model->setHeaderData(1, Qt::Horizontal, tr("Chromatic Adaptation"));
        model->setHeaderData(2, Qt::Horizontal, tr("Light Adaptation"));
        model->setHeaderData(3, Qt::Horizontal, tr("Pre-gamma"));
        model->setHeaderData(4, Qt::Horizontal, tr("Comment"));
        break;
    }
    m_Ui->tableView->setModel(model);
    m_Ui->tableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_Ui->tableView->horizontalHeader()->setResizeMode(QHeaderView::ResizeToContents);
    m_Ui->tableView->show();
}

SavedParametersDialog::~SavedParametersDialog()
{
#ifdef QT_DEBUG
    qDebug() << "SavedParametersDialog::~SavedParametersDialog()";
#endif
    QSqlQuery query("DROP TABLE IF EXISTS comments");
#ifdef QT_DEBUG
    qDebug() << query.lastError();
#endif
    delete model;
}

QModelIndex SavedParametersDialog::getCurrentIndex()
{
    return m_Ui->tableView->currentIndex();
}

QSqlTableModel* SavedParametersDialog::getModel()
{
    return model;
}

