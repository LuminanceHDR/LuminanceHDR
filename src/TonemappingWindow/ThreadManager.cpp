/**
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
 */

#include "Common/global.h"
#include "ThreadManager.h"

ThreadManager::~ThreadManager() {
	clearAll();
}

ThreadManager::ThreadManager(QWidget *parent) : QDialog(parent) {
	setupUi(this);
	setBackgroundRole(QPalette::Light);
	connect(clearButton,SIGNAL(clicked()),this,SLOT(clearAll()));
}

void ThreadManager::addProgressIndicator(TMOProgressIndicator *pi) {
	verticalLayout->addWidget(pi);	
	widgets.append(pi);
}

void ThreadManager::clearAll() {
	foreach(TMOProgressIndicator *pi, widgets) {
		if (pi->isTerminated()) {
			verticalLayout->removeWidget(pi);
			delete pi;
			widgets.removeOne(pi);
		}
	}
}

void ThreadManager::showEvent(QShowEvent *) {
	restoreGeometry(settings.value("ThreadManagerGeometry").toByteArray());
	QPoint pos = settings.value("ThreadManagerPos").toPoint();
	move(pos);
}

void ThreadManager::hideEvent(QHideEvent *) {
	settings.setValue("ThreadManagerGeometry", saveGeometry());
	settings.setValue("ThreadManagerPos", pos());
}

void ThreadManager::closeEvent(QCloseEvent *event) {
	emit closeRequested(false);
	event->ignore();
}
