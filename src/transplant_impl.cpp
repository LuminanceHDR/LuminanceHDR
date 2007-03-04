/**
 * This file is a part of Qtpfsgui package.
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

#include <QFileDialog>
#include <QMessageBox>
#include <QFileInfo>
#include <image.hpp>
#include <exif.hpp>
#include <iostream>
#include "transplant_impl.h"
#include "../generated_uic/ui_help_about.h"
static const QString helptext = 
"<hr> <h1>The ``Copy Exif data'' Dialog</h1> \
<p>In this dialog you will be able to transplant (i.e. copy) the exif data contained in a set of files (the sources, on the left) into another set of files (the destinations, on the right).<br>This is a one-to-one data copy, i.e. the first file in the destination list gets the exif data from the first file in the sources list and so on.</p> ";


TransplantExifDialog::TransplantExifDialog(QWidget *p) : QDialog(p), start_left(-1), stop_left(-1), start_right(-1), stop_right(-1), done(false) {
	setupUi(this);
	connect(moveup_left_button,	SIGNAL(clicked()),this,SLOT(moveup_left()));
	connect(moveup_right_button,	SIGNAL(clicked()),this,SLOT(moveup_right()));
	connect(movedown_left_button,	SIGNAL(clicked()),this,SLOT(movedown_left()));
	connect(movedown_right_button,	SIGNAL(clicked()),this,SLOT(movedown_right()));
	connect(removeleft,		SIGNAL(clicked()),this,SLOT(remove_left()));
	connect(removeright,		SIGNAL(clicked()),this,SLOT(remove_right()));
	connect(addleft,		SIGNAL(clicked()),this,SLOT(append_left()));
	connect(addright,		SIGNAL(clicked()),this,SLOT(append_right()));
	connect(TransplantButton,	SIGNAL(clicked()),this,SLOT(transplant_requested()));
	connect(HelpButton,		SIGNAL(clicked()),this,SLOT(help_requested()));
}

TransplantExifDialog::~TransplantExifDialog() {
// 	qDebug("start~");
	leftlist->clear();
	rightlist->clear();
// 	qDebug("end~");
}

void TransplantExifDialog::help_requested() {
	QDialog *help=new QDialog();
	help->setAttribute(Qt::WA_DeleteOnClose);
	Ui::HelpDialog ui;
	ui.setupUi(help);
	ui.tb->setHtml(helptext);
	help->exec();
}

void TransplantExifDialog::updateinterval(bool left) {
	if (left) {
		start_left=leftlist->count();
		stop_left=-1;
		for (int i=0; i<leftlist->count(); i++) {
			if (leftlist->isItemSelected(leftlist->item(i))) {
				start_left= (start_left>i) ? i : start_left;
				stop_left= (stop_left<i) ? i : stop_left;
			}
		}
		qDebug("L %d-%d",start_left,stop_left);
	} else {
		start_right=rightlist->count();
		stop_right=-1;
		for (int i=0; i<rightlist->count(); i++) {
			if (rightlist->isItemSelected(rightlist->item(i))) {
				start_right= (start_right>i) ? i : start_right;
				stop_right= (stop_right<i) ? i : stop_right;
			}
		}
		qDebug("R %d-%d",start_right,stop_right);
	}
}

void TransplantExifDialog::moveup_left() {
	updateinterval(true);
	if (leftlist->count()==0 || start_left==-1 || stop_left==-1 || start_left==0)
		return;
	//"VIEW"
	//copy the before-first element to the past-end of the selection
	leftlist->insertItem(stop_left+1,QFileInfo(from.at(start_left-1)).fileName());
	//remove the before-first element
	leftlist->takeItem(start_left-1);
	//"MODEL"
	from.insert(stop_left+1,from.at(start_left-1));
	from.removeAt(start_left-1);
	start_left--;
	stop_left--;
// 	for (QStringList::const_iterator i = from.constBegin(); i != from.constEnd(); ++i)
// 		qDebug((*i).toAscii());
}

void TransplantExifDialog::moveup_right() {
	updateinterval(false);
	if (rightlist->count()==0 || start_right==-1 || stop_right==-1 || start_right==0)
		return;
	//"VIEW"
	//copy the before-first element to the past-end of the selection
	rightlist->insertItem(stop_right+1,QFileInfo(to.at(start_right-1)).fileName());
	//remove the before-first element
	rightlist->takeItem(start_right-1);
	//"MODEL"
	to.insert(stop_right+1,to.at(start_right-1));
	to.removeAt(start_right-1);
	start_right--;
	stop_right--;
// 	for (QStringList::const_iterator i = to.constBegin(); i != to.constEnd(); ++i)
// 		qDebug((*i).toAscii());
}

void TransplantExifDialog::movedown_left() {
	updateinterval(true);
	if (leftlist->count()==0 || start_left==-1 || stop_left==-1 || stop_left==leftlist->count()-1)
		return;
	//"VIEW"
	//copy the past-end to the before-first element of the selection
	leftlist->insertItem(start_left,QFileInfo(from.at(stop_left+1)).fileName());
	//remove the past-end
	leftlist->takeItem(stop_left+2);
	//"MODEL"
	from.insert(start_left,from.at(stop_left+1));
	from.removeAt(stop_left+2);
	start_left++;
	stop_left++;
// 	for (QStringList::const_iterator i = from.constBegin(); i != from.constEnd(); ++i)
// 		qDebug((*i).toAscii());
}

void TransplantExifDialog::movedown_right() {
	updateinterval(false);
	if (rightlist->count()==0 || start_right==-1 || stop_right==-1 || stop_right==rightlist->count()-1)
		return;
	//"VIEW"
	//copy the past-end to the before-first element of the selection
	rightlist->insertItem(start_right,QFileInfo(to.at(stop_right+1)).fileName());
	//remove the past-end
	rightlist->takeItem(stop_right+2);
	//"MODEL"
	to.insert(start_right,to.at(stop_right+1));
	to.removeAt(stop_right+2);
	start_right++;
	stop_right++;
// 	for (QStringList::const_iterator i = to.constBegin(); i != to.constEnd(); ++i)
// 		qDebug((*i).toAscii());
}

void TransplantExifDialog::remove_left() {
	updateinterval(true);
	if (leftlist->count()==0 || start_left==-1 || stop_left==-1)
		return;
	for (int i=stop_left-start_left+1; i>0; i--) {
		leftlist->takeItem(start_left);
		from.removeAt(start_left);
	}
	start_left=stop_left=-1;
// 	for (QStringList::const_iterator i = from.constBegin(); i != from.constEnd(); ++i)
// 		qDebug((*i).toAscii());
}

void TransplantExifDialog::remove_right() {
	updateinterval(false);
	if (rightlist->count()==0 || start_right==-1 || stop_right==-1)
		return;
	for (int i=stop_right-start_right+1; i>0; i--) {
		rightlist->takeItem(start_right);
		to.removeAt(start_right);
	}
	start_right=stop_right=-1;
// 	for (QStringList::const_iterator i = to.constBegin(); i != to.constEnd(); ++i)
// 		qDebug((*i).toAscii());
}

void TransplantExifDialog::append_left() {
	QString filetypes = "All Supported Types (*.jpeg *.jpg *.JPG *.JPEG *.crw *.CRW *.cr2 *CR2 *.nef *.NEF *.dng *.DNG *.mrw *.MRW *.olf *.OLF *.kdc *.KDC *.dcr *DCR *.arw *.ARW *.ptx *.PTX *.pef *.PEF *.x3f *.X3F)";
	QStringList files = QFileDialog::getOpenFileNames(this, "Select the input Images", QDir::currentPath(), filetypes );
	if (!files.isEmpty()) {
		QStringList::Iterator it = files.begin();
		while( it != files.end() ) {
			QFileInfo *qfi=new QFileInfo(*it);
			leftlist->addItem(qfi->fileName()); //fill graphical list
			++it;
			delete qfi;
		}
		from+=files; // add the new files to the "model"
// 		for (QStringList::const_iterator i = from.constBegin(); i != from.constEnd(); ++i)
// 			qDebug((*i).toAscii());
	}
}

void TransplantExifDialog::append_right() {
	QString filetypes = "All Supported Types (*.jpeg *.jpg *.JPG *.JPEG *.crw *.CRW *.cr2 *CR2 *.nef *.NEF *.dng *.DNG *.mrw *.MRW *.olf *.OLF *.kdc *.KDC *.dcr *DCR *.arw *.ARW *.ptx *.PTX *.pef *.PEF *.x3f *.X3F)";
	QStringList files = QFileDialog::getOpenFileNames(this, "Select the input Images", QDir::currentPath(), filetypes );
	if (!files.isEmpty()) {
		QStringList::Iterator it = files.begin();
		while( it != files.end() ) {
			QFileInfo *qfi=new QFileInfo(*it);
			rightlist->addItem(qfi->fileName()); //fill graphical list
			++it;
			delete qfi;
		}
		to+=files; // add the new files to the "model"
// 		for (QStringList::const_iterator i = to.constBegin(); i != to.constEnd(); ++i)
// 			qDebug((*i).toAscii());
	}
}

void TransplantExifDialog::transplant_requested() {
	if (done) {
		accept();
		return;
	}

	//check if number of jpeg file in left and right lists is the same
	if (leftlist->count()!=rightlist->count()) {
		QMessageBox::critical(this,"different number of files", QString("<font color=\"#FF0000\"><h3><b>ERROR:</b></h3></font> Different number of files in the left and right columns."));
		return;
	}

	progressBar->setMaximum(leftlist->count());
	//initialize string iterators to the beginning of the lists.
	QStringList::const_iterator i_source = from.constBegin();
	QStringList::const_iterator i_dest = to.constBegin();
	//NOW CHECK THAT **ALL** THE SOURCE FILES HAVE THE EXIF DATA, AND THAT **ALL** THE SOURCE AND DEST FILES ARE EXIV2-COMPATIBLE
	for (; i_source != from.constEnd(); ++i_source) {
		try {
			//these 2 below throw an exception if file is not supported by exiv2
			Exiv2::Image::AutoPtr sourceimage = Exiv2::ImageFactory::open((*i_source).toStdString());
			Exiv2::Image::AutoPtr destimage = Exiv2::ImageFactory::open((*i_dest).toStdString());
			sourceimage->readMetadata();
			Exiv2::ExifData &src_exifData = sourceimage->exifData();
			// if source file has no EXIF data, abort
			if (src_exifData.empty()) {
				QMessageBox::critical(this,"No exif data found", QString("<font color=\"#FF0000\"><h3><b>ERROR:</b></h3></font>Aborting... the file <font color=\"#FF9500\"><i><b>%1</b</i></font> does not contain exif data.").arg(*i_source));
				return;
			}
		} catch (Exiv2::AnyError& e) {
			QMessageBox::critical(this,"File not supported", QString("<font color=\"#FF0000\"><h3><b>ERROR:</b></h3></font> %1").arg(QString::fromStdString(e.what())));
			return;
		}
	}

// 	qDebug("FROM");
// 	for (QStringList::const_iterator i = from.constBegin(); i != from.constEnd(); ++i)
// 		qDebug((*i).toAscii());
// 	qDebug("TO");
// 	for (QStringList::const_iterator i = to.constBegin(); i != to.constEnd(); ++i)
// 		qDebug((*i).toAscii());

	//HERE'S THE BEEF! :=)
	//initialize string iterators to the beginning of the lists.
	i_source = from.constBegin();
	i_dest = to.constBegin();
	//for all the input files
	for (; i_source != from.constEnd(); ++i_source, ++i_dest) {
// 		std::cerr << "processing file: " << (*i_source).toStdString() << " and " << (*i_dest).toStdString() << std::endl;
		//get source and destination exif data
		Exiv2::Image::AutoPtr sourceimage = Exiv2::ImageFactory::open((*i_source).toStdString());
		Exiv2::Image::AutoPtr destimage = Exiv2::ImageFactory::open((*i_dest).toStdString());
		sourceimage->readMetadata();
		destimage->readMetadata(); //doesn't matter if it is empty
		Exiv2::ExifData &src_exifData = sourceimage->exifData();
		Exiv2::ExifData &dest_exifData = destimage->exifData(); //doesn't matter if it is empty
		Exiv2::ExifData::const_iterator end_src = src_exifData.end(); //end delimiter for this source image data
		//for all the tags in the source exif data
		for (Exiv2::ExifData::const_iterator i = src_exifData.begin(); i != end_src; ++i) {
			//check if current source key exists in destination file
			Exiv2::ExifData::iterator maybe_exists = dest_exifData.findKey( Exiv2::ExifKey(i->key()) );
// 			//if exists AND we are told not to overwrite
			if (maybe_exists != dest_exifData.end() && checkBox_dont_overwrite->isChecked()) {
				continue;
			} else {
				//here we copy the value
				//we create a new tag in the destination file, the tag has the key of the source
				Exiv2::Exifdatum& dest_tag = dest_exifData[i->key()];
				//now the tag has also the value of the source
				dest_tag.setValue(&(i->value()));
			}
		}
		destimage->writeMetadata();
		progressBar->setValue(progressBar->value()+1); // increment progressbar
	}
	done=true;
	Done_label->setText("<center><font color=\"#00FF00\"><h3><b>All the exif tags have been successfully copied!</b></h3></font></center>");
	TransplantButton->setText("Done.");
	cancelbutton->setDisabled(true);
}












