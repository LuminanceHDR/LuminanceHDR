/*
 * This file is a part of Qtpfsgui package.
 * ----------------------------------------------------------------------
 * Copyright (C) 2009 Franco Comida
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
 */

#include <QVBoxLayout>
#include <QHBoxLayout>

#include "tmoProgressIndicator.h"

TmoProgressIndicator::TmoProgressIndicator(QWidget *parent, QString labeltext) : 
	QWidget(parent), m_isTerminated(false) 
{
	m_label = new QLabel(labeltext);

    m_progressBar = new QProgressBar;
	
	m_abortButton = new QPushButton;
	m_abortButton->resize(22,22);
	m_abortButton->setIcon(QIcon(":/new/prefix1/images/remove.png"));
	m_abortButton->setToolTip(QString(tr("Abort computation")));
	
	QHBoxLayout *hbl = new QHBoxLayout(0);
	hbl->addWidget(m_progressBar);
	hbl->addWidget(m_abortButton);

	QVBoxLayout *vbl = new QVBoxLayout(this);

	vbl->addWidget(m_label);
	vbl->addLayout(hbl);
	connect(m_abortButton, SIGNAL(clicked()), this, SIGNAL(terminate()));
	connect(m_abortButton, SIGNAL(clicked()), this, SLOT(terminated()));
}

TmoProgressIndicator::~TmoProgressIndicator() {
	delete m_label;
	delete m_progressBar;
	delete m_abortButton;
}


void TmoProgressIndicator::terminated() {
	m_isTerminated = true;
}

bool TmoProgressIndicator::isTerminated() {
	return m_isTerminated;
}

void TmoProgressIndicator::setValue(int value) {
	m_progressBar->setValue(value);
}

void TmoProgressIndicator::setMaximum(int max) {
	m_progressBar->setMaximum(max);
}

void TmoProgressIndicator::setMinimum(int min) {
	m_progressBar->setMinimum(min);
}
