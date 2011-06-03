/**
 * This file is a part of Luminance HDR package.
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

#include "PreviewPanel.h"

PreviewPanel::PreviewPanel(QWidget *parent) : QWidget(parent)
{
	setupUi(this);

	labelMantiuk06 = new PreviewLabel(frameMantiuk06, 0);
	labelMantiuk06->setText("Mantiuk '06");

	labelMantiuk08 = new PreviewLabel(frameMantiuk08, 1);
	labelMantiuk08->setText("Mantiuk '08");

	labelFattal = new PreviewLabel(frameFattal, 2);
	labelFattal->setText("Fattal");

	labelDrago = new PreviewLabel(frameDrago, 3);
	labelDrago->setText("Drago");

	labelDurand = new PreviewLabel(frameDurand, 4);
	labelDurand->setText("Durand");

	labelReinhard02= new PreviewLabel(frameReinhard02, 5);
	labelReinhard02->setText("Reinhard '02");

	labelReinhard05 = new PreviewLabel(frameReinhard05, 6);
	labelReinhard05->setText("Reinhard '05");

	labelAshikhmin = new PreviewLabel(frameAshikhmin, 7);
	labelAshikhmin->setText("Ashikhmin");

	labelPattanaik = new PreviewLabel(framePattanaik, 8);
	labelPattanaik->setText("Pattanaik");

	setupConnections();
}

void PreviewPanel::setupConnections()
{
	connect(labelMantiuk06,SIGNAL(clicked(int)),this,SIGNAL(clicked(int)));
	connect(labelMantiuk08,SIGNAL(clicked(int)),this,SIGNAL(clicked(int)));
	connect(labelFattal,SIGNAL(clicked(int)),this,SIGNAL(clicked(int)));
	connect(labelDrago,SIGNAL(clicked(int)),this,SIGNAL(clicked(int)));
	connect(labelDurand,SIGNAL(clicked(int)),this,SIGNAL(clicked(int)));
	connect(labelReinhard02,SIGNAL(clicked(int)),this,SIGNAL(clicked(int)));
	connect(labelReinhard05,SIGNAL(clicked(int)),this,SIGNAL(clicked(int)));
	connect(labelAshikhmin,SIGNAL(clicked(int)),this,SIGNAL(clicked(int)));
	connect(labelPattanaik,SIGNAL(clicked(int)),this,SIGNAL(clicked(int)));
}

void PreviewPanel::setPixmap(const QPixmap &p, int n)
{
	switch (n) {
		case 0:
			labelMantiuk06->setPixmap(p);
			labelMantiuk06->adjustSize();
		break;
		case 1:
			labelMantiuk08->setPixmap(p);
			labelMantiuk08->adjustSize();
		break;
		case 2:
			labelFattal->setPixmap(p);
			labelFattal->adjustSize();
		break;
		case 3:
			labelDrago->setPixmap(p);
			labelDrago->adjustSize();
		break;
		case 4:
			labelDurand->setPixmap(p);
			labelDurand->adjustSize();
		break;
		case 5:
			labelReinhard02->setPixmap(p);
			labelReinhard02->adjustSize();
		break;
		case 6:
			labelReinhard05->setPixmap(p);
			labelReinhard05->adjustSize();
		break;
		case 7:
			labelAshikhmin->setPixmap(p);
			labelAshikhmin->adjustSize();
		break;
		case 8:
			labelPattanaik->setPixmap(p);
			labelPattanaik->adjustSize();
		break;
	}
}

PreviewPanel::~PreviewPanel()
{
}
