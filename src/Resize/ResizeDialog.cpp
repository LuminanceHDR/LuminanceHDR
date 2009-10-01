/**
 * This file is a part of LuminanceHDR package.
 * ---------------------------------------------------------------------- 
 * Copyright (C) 2006,2007 Giuseppe Rota
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
 * @author Giuseppe Rota <grota@users.sourceforge.net>
 */

#include "ResizeDialog.h"

pfs::Frame* resizeFrame(pfs::Frame* inpfsframe, int _xSize);

ResizeDialog::ResizeDialog(QWidget *parent,pfs::Frame *orig) : QDialog(parent),original(orig) {
	setupUi(this);
	orig_width=original->getWidth();
	orig_height=original->getHeight();
	resized_width=orig_width;
	resized_height=orig_height;

	widthSpinBox->setSuffix("");
	widthSpinBox->setDecimals(0);
	widthSpinBox->setMaximum(2*orig_width);
	widthSpinBox->setMinimum(1);
	heightSpinBox->setSuffix("");
	heightSpinBox->setDecimals(0);
	heightSpinBox->setMaximum(2*orig_height);
	heightSpinBox->setMinimum(1);
	//we are now in pixel mode, put directly original pixel values.
	widthSpinBox->setValue(orig_width);
	heightSpinBox->setValue(orig_height);
	from_other_spinbox=false;
	updatelabel();

	connect(scaleButton,SIGNAL(clicked()),this,SLOT(scaledPressed()));
	connect(widthSpinBox,SIGNAL(editingFinished()),this,SLOT(update_heightSpinBox()));
	connect(widthSpinBox,SIGNAL(valueChanged(double)),this,SLOT(update_heightSpinBox()));
	connect(heightSpinBox,SIGNAL(editingFinished()),this,SLOT(update_widthSpinBox()));
	connect(heightSpinBox,SIGNAL(valueChanged(double)),this,SLOT(update_widthSpinBox()));
	connect(px_or_percentage,SIGNAL(activated(int)),this,SLOT(switch_px_percentage(int)));
	connect(restoredefault,SIGNAL(clicked()),this,SLOT(defaultpressed()));
}

ResizeDialog::~ResizeDialog() {
//we don't delete *original, because in maingui_impl.cpp we will later call mdiwin->updateHDR which takes care of deleting its previous pfs::Frame* buffer.
}

pfs::Frame* ResizeDialog::getResizedFrame() {
	return resized;
}

void ResizeDialog::scaledPressed() {
	if (orig_width==resized_width) {
		emit reject();
		return;
	}
	resized=resizeFrame(original,resized_width);
	accept();
}

void ResizeDialog::switch_px_percentage(int px_per) {

	switch (px_per) {
	case 0:
		widthSpinBox->setMaximum(2*orig_width);
		heightSpinBox->setMaximum(2*orig_height);
		from_other_spinbox=true;
		widthSpinBox->setValue((int)(widthSpinBox->value()*(float)orig_width/100.0)); //from perc to px
		from_other_spinbox=true;
		heightSpinBox->setValue((int)(heightSpinBox->value()*(float)orig_height/100.0)); //from perc to px
		widthSpinBox->setSuffix("");
		widthSpinBox->setDecimals(0);
		widthSpinBox->setMinimum(1);
		heightSpinBox->setSuffix("");
		heightSpinBox->setDecimals(0);
		heightSpinBox->setMinimum(1);
		break;
	case 1:
		widthSpinBox->setDecimals(2);
		heightSpinBox->setDecimals(2);
		from_other_spinbox=true;
		widthSpinBox->setValue(100*widthSpinBox->value()/(float)orig_width); //from px to perc
		from_other_spinbox=true;
		heightSpinBox->setValue(100*heightSpinBox->value()/(float)orig_height); //from px to perc
		widthSpinBox->setSuffix("%");
		widthSpinBox->setMaximum(200);
		widthSpinBox->setMinimum(1);
		heightSpinBox->setSuffix("%");
		heightSpinBox->setMaximum(200);
		heightSpinBox->setMinimum(1);
		break;
	}
	from_other_spinbox=false;
	updatelabel();
}
//get a proper resized_width from a resized_height
int ResizeDialog::rw_from_rh() {
	return (int)((float)orig_width*(float)resized_height/(float)orig_height);
}
//get a proper resized_height from a resized_width
int ResizeDialog::rh_from_rw() {
	return (int)((float)orig_height*(float)resized_width/(float)orig_width);
}
void ResizeDialog::update_heightSpinBox() {
	if (from_other_spinbox) {
		from_other_spinbox=false;
		return;
	}
	switch (px_or_percentage->currentIndex()) {
	case 0:
		resized_width=(int)widthSpinBox->value();
		resized_height=rh_from_rw();
		from_other_spinbox=true;
		//update directly resized_height
		heightSpinBox->setValue(resized_height);
	break;
	case 1:
		resized_width=(int)(orig_width*widthSpinBox->value()/100.0);
		resized_height=rh_from_rw();
		from_other_spinbox=true;
		heightSpinBox->setValue((double)resized_height/(double)orig_height*100.0);
	break;
	}
	updatelabel();
}
void ResizeDialog::update_widthSpinBox() {
	if (from_other_spinbox) {
		from_other_spinbox=false;
		return;
	}
	switch (px_or_percentage->currentIndex()) {
	case 0:
		resized_height=(int)heightSpinBox->value();
		resized_width=rw_from_rh();
		from_other_spinbox=true;
		//update directly resized_width
		widthSpinBox->setValue(resized_width);
		break;
	case 1:
		resized_height=(int)(orig_height*heightSpinBox->value()/100.0);
		resized_width=rw_from_rh();
		from_other_spinbox=true;
		widthSpinBox->setValue((double)resized_width/(double)orig_width*100.0);
		break;
	}
	updatelabel();
}
void ResizeDialog::updatelabel() {
	sizepreview->setText(QString("%1x%2").arg(resized_width).arg(resized_height));
}

void ResizeDialog::defaultpressed() {
	resized_height=orig_height;
	resized_width=orig_width;
	switch (px_or_percentage->currentIndex()) {
	case 0:
		from_other_spinbox=true;
		widthSpinBox->setValue(resized_width);
		from_other_spinbox=true;
		heightSpinBox->setValue(resized_height);
		from_other_spinbox=false;
		break;
	case 1:
		from_other_spinbox=true;
		widthSpinBox->setValue(100);
		from_other_spinbox=true;
		heightSpinBox->setValue(100);
		from_other_spinbox=false;
		break;
	}
	updatelabel();
}
