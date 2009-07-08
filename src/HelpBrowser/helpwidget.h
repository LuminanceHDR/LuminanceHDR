/***************************************************************************
 *   Copyright (C) 2007 by Pierre Marchand   *
 *   pierre@oep-h.com   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
#ifndef HELPWIDGET_H
#define HELPWIDGET_H

#include <QDialog>
#include <ui_help.h>

/**
	@author Pierre Marchand <pierre@oep-h.com>
*/
class HelpWidget : public QDialog, private Ui::Help
{
		Q_OBJECT
	public:
		HelpWidget ( QWidget *parent );
		~HelpWidget();

	private slots:
		void slotIsClosing();
		
		void slotWebLink(const QUrl & url );
		void slotWebStart();
		void slotWebLoad(int i);
		void slotWebFinished(bool);
		
	signals:
		void end();

};

#endif
