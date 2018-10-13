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

#include <Common/SavedParametersDialog.h>
#include <Common/ui_SavedParametersDialog.h>

SavedParametersDialog::SavedParametersDialog(QSqlDatabase &db, QWidget *parent)
    : QDialog(parent),
      m_model(new QSqlQueryModel()),
      m_db(db),
      m_Ui(new Ui::SavedParametersDialog) {
    m_Ui->setupUi(this);

    QString sqlQuery;
    sqlQuery += QLatin1String(
        "SELECT comment, 'ashikhmin' AS operator FROM ashikhmin UNION ");
    sqlQuery += QLatin1String(
        "SELECT comment, 'drago' AS operator FROM drago UNION ");
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
        "SELECT comment, 'reinhard05' AS operator FROM reinhard05 UNION ");
    sqlQuery += QLatin1String(
        "SELECT comment, 'ferwerda' AS operator FROM ferwerda UNION ");
    sqlQuery += QLatin1String(
        "SELECT comment, 'kimkautz' AS operator FROM kimkautz");
    m_model->setQuery(sqlQuery, m_db);

    m_model->setHeaderData(0, Qt::Horizontal, tr("Comment"));
    m_model->setHeaderData(1, Qt::Horizontal, tr("TM Operator"));

    m_Ui->tableView->setModel(m_model);
    m_Ui->tableView->horizontalHeader()->setSectionResizeMode(
        QHeaderView::ResizeToContents);
    m_Ui->tableView->show();
}

SavedParametersDialog::~SavedParametersDialog() { delete m_model; }

QModelIndex SavedParametersDialog::getCurrentIndex() {
    return m_Ui->tableView->currentIndex();
}

QSqlQueryModel *SavedParametersDialog::getModel() { return m_model; }

QModelIndexList SavedParametersDialog::getSelectedRows() {
    return m_Ui->tableView->selectionModel()->selectedRows();
}
