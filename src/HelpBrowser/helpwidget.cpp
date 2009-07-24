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
#include "helpwidget.h"
#include "fmpaths.h"
#include <QDebug>

HelpWidget::HelpWidget(QWidget *parent)
 : QDialog(parent)
{
	setupUi(this);
	
	theText->page()->setLinkDelegationPolicy(QWebPage::DelegateExternalLinks);
	//theText->load(QUrl::fromLocalFile(FMPaths::HelpFilePath()));
	theText->load(QUrl::fromLocalFile("/usr/share/luminance/index.html"));
	progressBar->hide();
	
	connect(closeButton,SIGNAL( clicked() ),this,SLOT( slotIsClosing() ));
	connect(this,SIGNAL( finished(int) ),this,SLOT( slotIsClosing() ));
	
	connect( theText, SIGNAL(linkClicked ( const QUrl& )), this, SLOT(slotWebLink(const QUrl&)));
	connect( theText, SIGNAL(loadStarted () ),this,SLOT(slotWebStart()));
	connect( theText, SIGNAL(loadProgress ( int )  ),this, SLOT(slotWebLoad(int)));
	connect( theText, SIGNAL(loadFinished ( bool ) ),this,SLOT(slotWebFinished(bool)));
}


HelpWidget::~HelpWidget()
{
}

void HelpWidget::slotIsClosing()
{
	emit end();
}

void HelpWidget::slotWebLink(const QUrl & url)
{
	qDebug()<<"slotWebLink("<<url<<")";
	urlLabel->setText(url.toString());
	theText->load(url);
}

void HelpWidget::slotWebStart()
{
	progressBar->setRange(0,100);
	progressBar->show();
	
}

void HelpWidget::slotWebLoad(int i)
{
	progressBar->setValue(i);
}

void HelpWidget::slotWebFinished(bool )
{
	progressBar->hide();
}


