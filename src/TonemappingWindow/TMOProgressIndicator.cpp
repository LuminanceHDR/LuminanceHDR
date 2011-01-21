/*
 * This file is a part of LuminanceHDR package.
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
 *
 */

#include "TMOProgressIndicator.h"

#include <iostream>

TMOProgressIndicator::TMOProgressIndicator(QWidget *parent) : 
	QWidget(parent), m_isTerminated(false) 
{
	m_progressBar = new QProgressBar(this);
	m_abortButton = new QPushButton(this);
	m_hbl         = new QHBoxLayout(this);
  
	m_abortButton->resize(22,22);
	m_abortButton->setIcon(QIcon(":/new/prefix1/images/remove.png"));
	m_abortButton->setToolTip(QString(tr("Abort computation")));
  
	m_hbl->addWidget(m_progressBar);
	m_hbl->addWidget(m_abortButton);
  
  m_hbl->setMargin(0);        // Design
  m_hbl->setSpacing(4);       // Design
  m_hbl->setContentsMargins(0, 0, 0, 0);
  
	connect(m_abortButton, SIGNAL(clicked()), this, SIGNAL(terminate()));
	connect(m_abortButton, SIGNAL(clicked()), this, SLOT(terminated()));

	m_progressBar->setValue(0);
}

TMOProgressIndicator::~TMOProgressIndicator()
{
  delete m_progressBar;
  delete m_abortButton;
  delete m_hbl;
}

void TMOProgressIndicator::terminated() {
	std::cout << "TMOProgressIndicator::terminated()" << std::endl;
	m_isTerminated = true;
	emit deleteMe();
}

bool TMOProgressIndicator::isTerminated() {
	return m_isTerminated;
}

void TMOProgressIndicator::setValue(int value) {
	m_progressBar->setValue(value);
}

void TMOProgressIndicator::setMaximum(int max) {
	m_progressBar->setMaximum(max);
}

void TMOProgressIndicator::setMinimum(int min) {
	m_progressBar->setMinimum(min);
}
