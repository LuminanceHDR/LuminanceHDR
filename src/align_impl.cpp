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
#include <QDir>
#include <QMessageBox>
#include "align_impl.h"

AlignDialog::AlignDialog(QWidget *p) : QDialog(p), current_new_pair(NULL), current_master(NULL), prevclickleft(false), prevclickright(false), sel_start(-1), sel_stop(-1), update_tabs(true),update_graphical_list(false) {
	setupUi(this);
	connect(moveup_button,	SIGNAL(clicked()),this,SLOT(moveup_files()));
	connect(movedown_button,SIGNAL(clicked()),this,SLOT(movedown_files()));
	connect(remove_button,	SIGNAL(clicked()),this,SLOT(remove_files()));
	connect(append_button,	SIGNAL(clicked()),this,SLOT(append_files()));
	connect(Current_Images_Del_CP_Button,	SIGNAL(clicked()),this,SLOT(del_cps_of_files()));
	connect(Current_Image_setMaster_Button,	SIGNAL(clicked()),this,SLOT(setmaster()));
	connect(MainTabWidget,SIGNAL(currentChanged(int)),this,SLOT(progress_tab(int)));
	connect(right_tabs,SIGNAL(currentChanged(int)),this,SLOT(image_tab_changed(int)));
	connect(left_tabs,SIGNAL(currentChanged(int)),this,SLOT(image_tab_changed(int)));
	connect(slide_left_button,SIGNAL(clicked()),this,SLOT(slide_left()));
	connect(slide_right_button,SIGNAL(clicked()),this,SLOT(slide_right()));
	connect(Remove_Current_pair_button,SIGNAL(clicked()),this,SLOT(remove_current_pair()));
// 	connect(,SIGNAL(),this,SLOT());
}

AlignDialog::~AlignDialog(){
	qDeleteAll(listimages);
}

void AlignDialog::updateinterval() {
		sel_start=TableWidget->rowCount();
		sel_stop=-1;
		for (int i=0; i<TableWidget->rowCount(); i++) {
			if (TableWidget->isItemSelected(TableWidget->item(i,0))) {
				sel_start= (sel_start>i) ? i : sel_start;
				sel_stop= (sel_stop<i) ? i : sel_stop;
			}
		}
		qDebug("L %d-%d",sel_start,sel_stop);
}

void AlignDialog::moveup_files(){
	updateinterval();
	if (listimages.count()==0 || sel_start==-1 || sel_stop==-1 || sel_start==0)
		return;
	//"MODEL"
	listimages.insert(sel_stop+1,listimages.at(sel_start-1));
	listimages.removeAt(sel_start-1);
// 	qDebug("modified model:");
// 	for (QList<Image*>::const_iterator i = listimages.constBegin(); i != listimages.constEnd(); ++i)
// 		qDebug((*i)->filename.toAscii());
// 	qDebug("-----------------------");
	//"VIEW"
	//copy the before-first element to the past-end of the selection
	TableWidget->insertRow(sel_stop+1);
	TableWidget->setItem(sel_stop+1,0,(TableWidget->item(sel_start-1,0))->clone());
	TableWidget->setItem(sel_stop+1,1,(TableWidget->item(sel_start-1,1))->clone());
	TableWidget->setItem(sel_stop+1,2,(TableWidget->item(sel_start-1,2))->clone());
	//remove the before-first element
	TableWidget->removeRow(sel_start-1);
	sel_start--;
	sel_stop--;
	update_tabs=true;
}

void AlignDialog::movedown_files(){
	updateinterval();
	if (listimages.count()==0 || sel_start==-1 || sel_stop==-1 || sel_stop==listimages.count()-1)
		return;
	//"MODEL"
	listimages.insert(sel_start,listimages.at(sel_stop+1));
	listimages.removeAt(sel_stop+2);
// 	qDebug("modified model:");
// 	for (QList<Image*>::const_iterator i = listimages.constBegin(); i != listimages.constEnd(); ++i)
// 		qDebug((*i)->filename.toAscii());
// 	qDebug("-----------------------");
	//"VIEW"
	//copy the past-end to the before-first element of the selection
	TableWidget->insertRow(sel_start);
	TableWidget->setItem(sel_start,0,(TableWidget->item(sel_stop+2,0))->clone());
	TableWidget->setItem(sel_start,1,(TableWidget->item(sel_stop+2,1))->clone());
	TableWidget->setItem(sel_start,2,(TableWidget->item(sel_stop+2,2))->clone());
	//remove the past-end
	TableWidget->removeRow(sel_stop+2);
	sel_start++;
	sel_stop++;
	update_tabs=true;
}

//remove graphically selected files
void AlignDialog::remove_files(){
	updateinterval();
	if (listimages.count()==0 || sel_start==-1 || sel_stop==-1)
		return;
	for (int j=sel_stop-sel_start+1; j>0; j--) {
		//remove graphical row
		TableWidget->removeRow(sel_start);
		//remove all the pairs which have a point belonging to the image
		//this clears the relevant entries in listcps and frees the memory allocated for the CPs
		remove_pairs_belonging_to(sel_start); //(sel_start is index in listimages)
		//free the memory pointed by the Image*
// 		delete listimages.at(sel_start)->payload; //QImage*
// 		delete listimages.at(sel_start); //free the Image*//FIXME crashes if enabled, why?
		//if I'm deleting the master pic I set current_master to NULL
		if (listimages.at(sel_start)->master)
			current_master=NULL;
		//clear entry in listimages
		listimages.removeAt(sel_start);
	}
	sel_start=sel_stop=-1;
	update_tabs=true;
	refresh_graphical_list();
}

//remove points belonging to the graphically selected files
void AlignDialog::del_cps_of_files(){
	updateinterval();
	if (listimages.count()==0 || sel_start==-1 || sel_stop==-1)
		return;
	for (int j=sel_start; j<=sel_stop; j++) {
		//this iterates through the list of pairs, and if a pair has at least one point belonging to the j-th image, it deletes the pair and removes the entry in the listcps
		remove_pairs_belonging_to(j); //j is index in listimages
		//empty the list of points owned by this file
		listimages.at(j)->listcps.clear();
	}
	update_tabs=true;
	refresh_graphical_list();
}

//remove all the pairs which have a point belonging to the image, listimgs_idx is an index in listimages
void AlignDialog::remove_pairs_belonging_to(int listimgs_idx) {
	//iterate over global list of pairs
	for (int i=0; i<listcps.count(); i++) {
		//pick the i-th pair
		QPair<CP*,CP*> p=listcps.at(i);
		//if i-th pair has a point belonging to the left image
		if ( p.first->whobelongsto==listimages.at(listimgs_idx) ) {
			//pick the other image that has this pair
			Image *otherimage=p.second->whobelongsto;
			//now remove its image->listcps entry for this pair
			for (int j=0; j<otherimage->listcps.count(); j++) {
				//if the j-th point belonging to the other image is also the other point in the pair
				if (otherimage->listcps[j]==p.second) {
					//remove its entry.
					otherimage->listcps.removeAt(j);
				}
			}
			//delete the points (frees the memory)
			if(p.first) delete p.first; p.first=NULL;
			if(p.second) delete p.second; p.second=NULL;
			//clear entry in listcps
			listcps.removeAt(i);
			i--; //move up a little bit, since you just removed an entry
		} else
		//if i-th pair has a point belonging to the left image
		if( p.second->whobelongsto==listimages.at(listimgs_idx) ) {
			//pick the other image that has this pair
			Image *otherimage=p.first->whobelongsto;
			//now remove its image->listcps entry for this pair
			for (int j=0; j<otherimage->listcps.count(); j++) {
				//if the j-th point belonging to the other image is also the other point in the pair
				if (otherimage->listcps[j]==p.first) {
					//remove its entry.
					otherimage->listcps.removeAt(j);
				}
			}
			//delete the points (frees the memory)
			if(p.first) delete p.first; p.first=NULL;
			if(p.second) delete p.second; p.second=NULL;
			//clear entry in listcps
			listcps.removeAt(i);
			i--; //move up a little bit, since you just removed an entry
		}
	}
}

void AlignDialog::append_files(){
	QString filetypes = "JPEG Files (*.jpeg *.jpg *.JPG *.JPEG)";
	QStringList files = QFileDialog::getOpenFileNames(this, "Select the input Images", QDir::currentPath()+"/JPGS", filetypes );
	if (!files.isEmpty()) {
		QStringList::Iterator it = files.begin();
		while( it != files.end() ) {
			//update listimages "MODEL"
			QFileInfo *qfi=new QFileInfo(*it);
			Image *newIm=new Image();
			current_master= (listimages.count()==0) ? newIm : current_master;
			newIm->master=listimages.count()==0? true:false;
			newIm->filename=qfi->fileName(); //for displaying purposes
			newIm->payload=new QImage(qfi->filePath()); //fill with data
			listimages.append(newIm);
			//update "VIEW"
			TableWidget->setRowCount(TableWidget->rowCount()+1);
			TableWidget->setItem(listimages.count()-1,0,new QTableWidgetItem(newIm->filename));
			TableWidget->setItem(listimages.count()-1,1,new QTableWidgetItem(QString("%1").arg((newIm->listcps).count())));
			TableWidget->setItem(listimages.count()-1,2,new QTableWidgetItem(newIm->master?"YES":"NO"));
			++it;
		}
// 		for (QList<Image*>::const_iterator i = listimages.constBegin(); i != listimages.constEnd(); ++i)
// 			qDebug((*i)->filename.toAscii());
		update_tabs=true;
	}
}

void AlignDialog::setmaster(){
	assert(TableWidget->rowCount()==listimages.count());
	int row=TableWidget->currentRow();
	if (current_master) {
		current_master->master=false;
	}
	listimages.at(row)->master=true;
	refresh_graphical_list();
}

void AlignDialog::progress_tab(int newpage){
	if (newpage==0) {
		if(update_graphical_list) {
			refresh_graphical_list();
		}
	} else if (newpage==1) { //CP tab
		if(listimages.count()<2) { //not enough files
			MainTabWidget->setCurrentIndex(0);
			QMessageBox::warning(this,"not enough files","you need to load at least 2 files",
				QMessageBox::Ok, QMessageBox::NoButton);
			return;
		} else {
			//if we have to update the view
			if (update_tabs) {
// 				qDebug("have to update tabs");
				//delete all tabs
				assert(left_tabs->count()==right_tabs->count());
// 				qDebug("current # of tabs=%d",left_tabs->count());
				while(left_tabs->count()>0) {
					QWidget *l=left_tabs->widget(0);
// 					qDebug("before removetab LEFT");
					//FIXME cast obj necessary? and check memory leaks
					left_tabs->removeTab(0);
// 					qDebug("after removetab LEFT");
					//disconnect
					delete l;
					QWidget *r=right_tabs->widget(0);
// 					qDebug("before removetab RIGHT");
					//FIXME cast obj necessary? and check memory leaks
					right_tabs->removeTab(0);
// 					qDebug("after removetab RIGHT");
					delete r;
				}
				//create new ones
				qDebug("before create tabs");
				create_tabs();
				qDebug("after create tabs");
				assert(left_tabs->count()==listimages.count());
			}
			update_tabs=false;
		}
	}
}

void AlignDialog::create_tabs() {
	for(int i=0; i<listimages.count(); i++) {
		///////////////////// LEFT
		QWidget * newtabL= new QWidget();
		QVBoxLayout *VBL_L = new QVBoxLayout(newtabL);
		VBL_L->setSpacing(0);
		VBL_L->setMargin(0);
		QFrame * inner_frameL= new QFrame(newtabL);
		inner_frameL->setFrameShape(QFrame::StyledPanel);
		inner_frameL->setFrameShadow(QFrame::Raised);
		VBL_L->addWidget(inner_frameL);
		ShowImage *imageL = new ShowImage(&listimages,i,&listcps,true);
		connect(imageL,SIGNAL(finished_point_in_image(int,int)),this,SLOT(newpoint_in_leftarea(int,int)));
		connect(imageL,SIGNAL(point_finished_moving(CP*)),this,SLOT(point_finished_moving(CP*)));
		connect(this,SIGNAL(clear_temp_CP()),imageL,SLOT(clear_temp_CP()));
		ScrollArea *scrollAreaL=new ScrollArea(imageL);
		inner_frameL->setLayout(new QVBoxLayout);
		inner_frameL->layout()->setMargin(0);
		inner_frameL->layout()->addWidget(scrollAreaL);
		left_tabs->addTab(newtabL,QString("Image %1").arg(i+1));
		//////////////////// RIGHT
		QWidget * newtabR= new QWidget();
		QVBoxLayout *VBL_R = new QVBoxLayout(newtabR);
		VBL_R->setSpacing(0);
		VBL_R->setMargin(0);
		QFrame * inner_frameR= new QFrame(newtabR);
		inner_frameR->setFrameShape(QFrame::StyledPanel);
		inner_frameR->setFrameShadow(QFrame::Raised);
		VBL_R->addWidget(inner_frameR);
		ShowImage *imageR = new ShowImage(&listimages,i,&listcps,false);
		connect(imageR,SIGNAL(finished_point_in_image(int,int)),this,SLOT(newpoint_in_rightarea(int,int)));
		connect(imageR,SIGNAL(point_finished_moving(CP*)),this,SLOT(point_finished_moving(CP*)));
		connect(this,SIGNAL(clear_temp_CP()),imageR,SLOT(clear_temp_CP()));
		ScrollArea *scrollAreaR=new ScrollArea(imageR);
		inner_frameR->setLayout(new QVBoxLayout);
		inner_frameR->layout()->setMargin(0);
		inner_frameR->layout()->addWidget(scrollAreaR);
		right_tabs->addTab(newtabR,QString("Image %1").arg(i+1));

		connect(left_tabs, SIGNAL(currentChanged(int)),imageR,SLOT(update_other_idx(int)));
		connect(right_tabs,SIGNAL(currentChanged(int)),imageL,SLOT(update_other_idx(int)));
	}
}

void AlignDialog::newpoint_in_leftarea(int x, int y) {
	//they can never be true at the same time;
	assert(!(prevclickright && prevclickleft));
	if (right_tabs->currentIndex()==left_tabs->currentIndex() && prevclickright) {
		QMessageBox::warning(this,"error","You are trying to set 2 control points in the same image, that doesn't make sense.",
			QMessageBox::Ok, QMessageBox::NoButton);
		return;
	}
	//do nothing if click again in same panel
	if (prevclickleft)
		return;
	//if we are here it means that the click is valid, we can create a new point
// 	qDebug("Dialog, SLOT x leftclick in LEFT area, tab_%d",left_tabs->currentIndex()+1);
	//create new point, it belongs to the currently selected image.
	CP* p=new CP; p->x=x;p->y=y;p->whobelongsto=listimages.at(left_tabs->currentIndex());
	//reach the showimage, ugly code, maybe change in design? but how? FIXME using signal, as in "else" here below.
	ShowImage *si=qobject_cast<ShowImage *>(sender());
	assert(si);
	//new pair initiation
	if (!prevclickleft && !prevclickright) {
		//initiate pair, from the left side
		//do not touch listimages[.]->listcps & listcps until we complete the pair
		qDebug("initiating pair in left side");
		assert(current_new_pair==NULL); //pair has to be blank
		current_new_pair=new QPair<CP*,CP*>; //prepare the pair, now incomplete
		current_new_pair->first=p; //assign new point to the yet incomplete pair
		si->update_firstPoint(p); //new point is the temp for left side
		//conclusion, we have clicked left and we are ready for right
		prevclickleft=true;
		prevclickright=false;
	} else {
		//here we complete the pair, we add it to listcps, and we modify listimages[?]->listcps
		qDebug("in left, completing pair");
		assert(current_new_pair!=NULL); //pair has to be initialized
		current_new_pair->first=p; //assign new point to the now complete pair
		//add the pair to the global list of pairs of CPs
		listcps.append(*current_new_pair);
		//add the new point to the list of points owned by the image in the left tab
		(listimages.at(left_tabs->currentIndex()))->listcps.append(current_new_pair->first);
		//now that the pair is complete, add also the previously selected point to the list of points owned by the image in the right tab.
		(listimages.at(right_tabs->currentIndex()))->listcps.append(current_new_pair->second);
		emit clear_temp_CP(); //no more temp
		//MODEL
		of_two_imgs.append(*current_new_pair); //update the list of the pairs belonging to the two images
		//VIEW
		append_pair_tothe_table(current_new_pair,false); //updates the CP details table
		current_new_pair=NULL; //for next initiation
// 		si->update();
		//conclusion, both false so that next click will be the first one
		prevclickleft=false;
		prevclickright=false;
		update_graphical_list=true;
	}
}

void AlignDialog::newpoint_in_rightarea(int x, int y) {
	//they can never be true at the same time;
	assert(!(prevclickright && prevclickleft));
	if (right_tabs->currentIndex()==left_tabs->currentIndex() && prevclickleft) {
		QMessageBox::warning(this,"error","You are trying to set 2 control points in the same image, that doesn't make sense.",
			QMessageBox::Ok, QMessageBox::NoButton);
		return;
	}
	//do nothing if click again in same panel
	if (prevclickright)
		return;
	//if we are here it means that the click is valid, we can create a new point
// 	qDebug("Dialog, SLOT x leftclick in RIGHT area, tab_%d",right_tabs->currentIndex()+1);
	//create new point, it belongs to the currently selected image.
	CP* p=new CP; p->x=x;p->y=y;p->whobelongsto=listimages.at(right_tabs->currentIndex());
	//reach the showimage, ugly code, maybe change in design? but how? FIXME using signal, as in "else" here below.
	ShowImage *si=qobject_cast<ShowImage *>(sender());
	assert(si);
	//new pair initiation
	if (!prevclickleft && !prevclickright) {
		//initiate pair, from the right side
		//do not touch listimages[.]->listcps & listcps until we complete the pair
		qDebug("initiating pair in right side");
		assert(current_new_pair==NULL); //pair has to be blank
		current_new_pair=new QPair<CP*,CP*>; //prepare the pair, now incomplete
		current_new_pair->second=p; //assign new point to the yet incomplete pair (".second" so that ".first" is always on the left)
		si->update_firstPoint(p); //new point is the temp for left side
		//conclusion, we have clicked right and we are ready for left
		prevclickleft=false;
		prevclickright=true;
	} else {
		//here we complete the pair, we add it to listcps, and we modify listimages[?]->listcps
		qDebug("in right, completing pair");
		assert(current_new_pair!=NULL); //pair has to be initialized
		current_new_pair->second=p; //assign new point to the now complete pair
		//add the pair to the global list of pairs of CPs
		listcps.append(*current_new_pair);
		//add the new point to the list of points owned by the image in the right tab
		(listimages.at(right_tabs->currentIndex()))->listcps.append(current_new_pair->second);
		//now that the pair is complete, add also the previously selected point to the list of points owned by the image in the left tab.
		(listimages.at(left_tabs->currentIndex()))->listcps.append(current_new_pair->first);
		emit clear_temp_CP(); //no more temp
		//MODEL
		of_two_imgs.append(*current_new_pair); //update the list of the pairs belonging to the two images
		//VIEW
		append_pair_tothe_table(current_new_pair,false); //updates the CP details table
		current_new_pair=NULL; //for next initiation
// 		si->update();
		//conclusion, both false so that next click will be the first one
		prevclickleft=false;
		prevclickright=false;
		update_graphical_list=true;
	}
}

void AlignDialog::point_finished_moving(CP* point){
	//TODO
// 	qDebug("dialog, point finished moving");
	//number of rows in graphical list has to be in sync with model
	assert(Details_CP_table->rowCount()==of_two_imgs.count());
	//if temp in progress, do nothing
	if (current_new_pair) {
		qDebug("temp in progress, doing nothing");
		return;
	} else {
		int insertindex=-1;
		for (int i=0; i<of_two_imgs.count(); i++) {
			QPair<CP*,CP*> p=of_two_imgs.at(i);
			if (p.first==point) {
				insertindex=i;
				Details_CP_table->setItem(insertindex,0,new QTableWidgetItem(QString("%1").arg(insertindex)));
				Details_CP_table->setItem(insertindex,1,new QTableWidgetItem(QString("%1").arg(point->x)));
				Details_CP_table->setItem(insertindex,2,new QTableWidgetItem(QString("%1").arg(point->y)));
				break;
			} else if (p.second==point) {
				insertindex=i;
				Details_CP_table->setItem(insertindex,0,new QTableWidgetItem(QString("%1").arg(insertindex)));
				Details_CP_table->setItem(insertindex,3,new QTableWidgetItem(QString("%1").arg(point->x)));
				Details_CP_table->setItem(insertindex,4,new QTableWidgetItem(QString("%1").arg(point->y)));
				break;
			}
		}
		//I *have* to find the point that is being moved
		assert(insertindex!=-1);
	}
}

void AlignDialog::image_tab_changed(int) {
	//if we change tab, left or right it doesn't matter, we abort any initiated pair.
	if (current_new_pair) {
		QMessageBox::warning(this,"warning","aborting initiated pair",
			QMessageBox::Ok, QMessageBox::NoButton);
		emit clear_temp_CP();
		if(current_new_pair->first) delete current_new_pair->first; current_new_pair->first=NULL;
		if(current_new_pair->second) delete current_new_pair->second; current_new_pair->second=NULL;
		delete current_new_pair; current_new_pair=NULL;
		prevclickleft=false;
		prevclickright=false;
	}
 	//recreate CP table details
 	//clear the model
	of_two_imgs.clear();
	//now clear the table view
	Details_CP_table->setRowCount(0);
	//and go through the listcps and fill the model and view
	for (int i=0;i<listcps.count();i++) {
		QPair<CP*,CP*> p=listcps.at(i);
		//if i-th pair has a point belonging to the left and right image
		if (p.first->whobelongsto==listimages.at(left_tabs->currentIndex()) && p.second->whobelongsto==listimages.at(right_tabs->currentIndex())) {
			//MODEL
			of_two_imgs.append(p);
			//VIEW
			append_pair_tothe_table(&p,false);
		}
		//if i-th pair has a point belonging to the right and left image
		else if (p.first->whobelongsto==listimages.at(right_tabs->currentIndex()) && p.second->whobelongsto==listimages.at(left_tabs->currentIndex())) {
			//MODEL
			of_two_imgs.append(p);
			//VIEW
			append_pair_tothe_table(&p,true);
		}
	}
}

void AlignDialog::append_pair_tothe_table(QPair<CP*,CP*> *newpair, bool swapleftright) {
	//here we only append the new pair graphically, changing tab (both left and right) will clear the details table and iterate through the global list to populate the table again.
	//the global list already has this pair.
	Details_CP_table->setRowCount(Details_CP_table->rowCount()+1);
	int insertindex=Details_CP_table->rowCount()-1;
	//what we add here is the last point int he global list
	Details_CP_table->setItem(insertindex,0,new QTableWidgetItem(QString("%1").arg(insertindex)));
	Details_CP_table->setItem(insertindex,swapleftright? 3 : 1,new QTableWidgetItem(QString("%1").arg(newpair->first->x)));
	Details_CP_table->setItem(insertindex,swapleftright? 4 : 2,new QTableWidgetItem(QString("%1").arg(newpair->first->y)));
	Details_CP_table->setItem(insertindex,swapleftright? 1 : 3,new QTableWidgetItem(QString("%1").arg(newpair->second->x)));
	Details_CP_table->setItem(insertindex,swapleftright? 2 : 4,new QTableWidgetItem(QString("%1").arg(newpair->second->y)));
}

void AlignDialog::remove_current_pair() {
	//TODO multiple selections, calling smthing similar to update_selection
	//number of rows in graphical list has to be in sync with model
	assert(Details_CP_table->rowCount()==of_two_imgs.count());
	int idxlist=Details_CP_table->currentRow();
	//MODEL
	of_two_imgs.removeAt(idxlist);
	//VIEW
	Details_CP_table->removeRow(idxlist);
	//TODO not so easy, e' simile a remove all CPs in first tab
}

void AlignDialog::refresh_graphical_list() {
	assert(TableWidget->rowCount()==listimages.count());
	for (int i=0; i<TableWidget->rowCount(); i++) {
		Image *Im=listimages.at(i);
		TableWidget->setItem(i,0,new QTableWidgetItem(Im->filename));
		TableWidget->setItem(i,1,new QTableWidgetItem(QString("%1").arg((Im->listcps).count())));
		TableWidget->setItem(i,2,new QTableWidgetItem(Im->master?"YES":"NO"));
	}
}

void AlignDialog::slide_right() {
	int newidxleft= (left_tabs->currentIndex()==left_tabs->count()-1)? 0 : left_tabs->currentIndex()+1;
	int newidxright= (right_tabs->currentIndex()==right_tabs->count()-1)? 0 : right_tabs->currentIndex()+1;
	left_tabs->setCurrentIndex(newidxleft);
	right_tabs->setCurrentIndex(newidxright);
}

void AlignDialog::slide_left() {
	int newidxleft= (left_tabs->currentIndex()==0)? left_tabs->count()-1 : left_tabs->currentIndex()-1;
	int newidxright= (right_tabs->currentIndex()==0)? right_tabs->count()-1 : right_tabs->currentIndex()-1;
	left_tabs->setCurrentIndex(newidxleft);
	right_tabs->setCurrentIndex(newidxright);
}
