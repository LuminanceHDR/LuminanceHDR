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

#include <QAbstractItemView>
#include <QDebug>
#include <QHeaderView>
#include <QModelIndex>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>

#include "SavedParametersDialog.h"
#include "ui_SavedParametersDialog.h"

SavedParametersDialog::SavedParametersDialog(QWidget *parent)
    : QDialog(parent),
      model(new QSqlQueryModel()),
      m_Ui(new Ui::SavedParametersDialog) {
    m_Ui->setupUi(this);

    QString sqlQuery;
    sqlQuery += QLatin1String(
        "SELECT comment, 'ashikhmin' AS operator FROM ashikhmin UNION ");
    sqlQuery +=
        QLatin1String("SELECT comment, 'drago' AS operator FROM drago UNION ");
    sqlQuery += QLatin1String(
        "SELECT comment, 'durand' AS operator FROM durand UNION ");
    sqlQuery += QLatin1String(
        "SELECT comment, 'fattal' AS operator FROM fattal UNION ");
    sqlQuery += QLatin1String(
        "SELECT comment, 'ferradans' AS operator FROM ferradans UNION ");
    sqlQuery += QLatin1String(
        "SELECT comment, 'mantiuk06' AS operator FROM mantiuk06 UNION ");
    sqlQuery += QLatin1String(
        "SELECT comment, 'mantiuk08' AS operator FROM mantiuk08 UNION ");
    sqlQuery += QLatin1String(
        "SELECT comment, 'pattanaik' AS operator FROM pattanaik UNION ");
    sqlQuery += QLatin1String(
        "SELECT comment, 'reinhard02' AS operator FROM reinhard02 UNION ");
    sqlQuery += QLatin1String(
        "SELECT comment, 'reinhard05' AS operator FROM reinhard05");
    model->setQuery(sqlQuery);

    model->setHeaderData(0, Qt::Horizontal, tr("Comment"));
    model->setHeaderData(1, Qt::Horizontal, tr("TM Operator"));

    m_Ui->tableView->setModel(model);
    m_Ui->tableView->horizontalHeader()->setSectionResizeMode(
        QHeaderView::ResizeToContents);
    m_Ui->tableView->show();
}

SavedParametersDialog::SavedParametersDialog(TMOperator op, QWidget *parent)
    : QDialog(parent),
      model(new QSqlTableModel()),
      m_Ui(new Ui::SavedParametersDialog) {
    m_Ui->setupUi(this);
    // QSqlDatabase db = QSqlDatabase::database();

    QSqlTableModel *tableModel = (QSqlTableModel *)model;
    int col = 0;
    switch (op) {
        case ashikhmin:
            tableModel->setTable(QStringLiteral("ashikhmin"));
            tableModel->select();
            model->setHeaderData(col++, Qt::Horizontal, tr("Simple"));
            model->setHeaderData(col++, Qt::Horizontal, tr("Equation 2"));
            model->setHeaderData(col++, Qt::Horizontal,
                                 tr("Local Contrast Threshold"));
            break;
        case drago:
            tableModel->setTable(QStringLiteral("drago"));
            tableModel->select();
            model->setHeaderData(col++, Qt::Horizontal, tr("Bias"));
            break;
        case durand:
            tableModel->setTable(QStringLiteral("durand"));
            tableModel->select();
            model->setHeaderData(col++, Qt::Horizontal,
                                 tr("Spatial Kernel Sigma"));
            model->setHeaderData(col++, Qt::Horizontal,
                                 tr("Range Kernel Sigma"));
            model->setHeaderData(col++, Qt::Horizontal, tr("Base Contrast"));
            break;
        case fattal:
            tableModel->setTable(QStringLiteral("fattal"));
            tableModel->select();
            model->setHeaderData(col++, Qt::Horizontal, tr("Alpha"));
            model->setHeaderData(col++, Qt::Horizontal, tr("Beta"));
            model->setHeaderData(col++, Qt::Horizontal, tr("Color Saturation"));
            model->setHeaderData(col++, Qt::Horizontal, tr("Noise Reduction"));
            model->setHeaderData(col++, Qt::Horizontal, tr("Old Fattal"));
            break;
        case ferradans:
            tableModel->setTable(QStringLiteral("ferradans"));
            tableModel->select();
            model->setHeaderData(col++, Qt::Horizontal, tr("Rho"));
            model->setHeaderData(col++, Qt::Horizontal, tr("InvAlpha"));
            break;
        case mantiuk06:
            tableModel->setTable(QStringLiteral("mantiuk06"));
            tableModel->select();
            model->setHeaderData(col++, Qt::Horizontal,
                                 tr("Contrast Equalization"));
            model->setHeaderData(col++, Qt::Horizontal, tr("Contrast Factor"));
            model->setHeaderData(col++, Qt::Horizontal,
                                 tr("Saturation Factor"));
            model->setHeaderData(col++, Qt::Horizontal, tr("Detail Factor"));
            break;
        case mantiuk08:
            tableModel->setTable(QStringLiteral("mantiuk08"));
            tableModel->select();
            model->setHeaderData(col++, Qt::Horizontal, tr("Color Saturation"));
            model->setHeaderData(col++, Qt::Horizontal,
                                 tr("Contrast Enhancement"));
            model->setHeaderData(col++, Qt::Horizontal, tr("Luminance Level"));
            model->setHeaderData(col++, Qt::Horizontal,
                                 tr("Manual Luminance Level"));
            break;
        case pattanaik:
            tableModel->setTable(QStringLiteral("pattanaik"));
            tableModel->select();
            model->setHeaderData(col++, Qt::Horizontal,
                                 tr("Cone and Rod based on Luminance"));
            model->setHeaderData(col++, Qt::Horizontal,
                                 tr("Local Tonemapping"));
            model->setHeaderData(col++, Qt::Horizontal, tr("Cone Level"));
            model->setHeaderData(col++, Qt::Horizontal, tr("Rod Level"));
            model->setHeaderData(col++, Qt::Horizontal, tr("Multiplier"));
            break;
        case reinhard02:
            tableModel->setTable(QStringLiteral("reinhard02"));
            tableModel->select();
            model->setHeaderData(col++, Qt::Horizontal, tr("Use Scales"));
            model->setHeaderData(col++, Qt::Horizontal, tr("Key Value"));
            model->setHeaderData(col++, Qt::Horizontal, tr("Phi Value"));
            model->setHeaderData(col++, Qt::Horizontal, tr("Range"));
            model->setHeaderData(col++, Qt::Horizontal, tr("Lower Scale"));
            model->setHeaderData(col++, Qt::Horizontal, tr("Upper Scale"));
            break;
        case reinhard05:
            tableModel->setTable(QStringLiteral("reinhard05"));
            tableModel->select();
            model->setHeaderData(col++, Qt::Horizontal, tr("Brightness"));
            model->setHeaderData(col++, Qt::Horizontal,
                                 tr("Chromatic Adaptation"));
            model->setHeaderData(col++, Qt::Horizontal, tr("Light Adaptation"));
            break;
        default:
            break;
    }
    model->setHeaderData(col++, Qt::Horizontal, tr("Pre-gamma"));
    model->setHeaderData(col++, Qt::Horizontal, tr("Comment"));

    m_Ui->tableView->setModel(model);
    m_Ui->tableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_Ui->tableView->horizontalHeader()->setSectionResizeMode(
        QHeaderView::ResizeToContents);
    m_Ui->tableView->show();
}

SavedParametersDialog::~SavedParametersDialog() { delete model; }

QModelIndex SavedParametersDialog::getCurrentIndex() {
    return m_Ui->tableView->currentIndex();
}

QSqlQueryModel *SavedParametersDialog::getModel() { return model; }

QModelIndexList SavedParametersDialog::getSelectedRows() {
    return m_Ui->tableView->selectionModel()->selectedRows();
}
