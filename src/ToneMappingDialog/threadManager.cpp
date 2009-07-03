/**
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

#include "threadManager.h"

ThreadManager::~ThreadManager() {}

ThreadManager::ThreadManager(QWidget *parent) : QDialog(parent) {
	setupUi(this);
	connect(clearButton,SIGNAL(clicked()),this,SLOT(clearAll()));
}

void ThreadManager::addWidget(QWidget *w) {
	verticalLayout->addWidget(w);	
    widgets.append(w);
}

void ThreadManager::clearAll() {
	foreach(QWidget *w, widgets) {
		verticalLayout->removeWidget(w);
		delete w;
	}
	widgets.clear();
}

void ThreadManager::closeEvent(QCloseEvent *event) {
	event->ignore();
}
