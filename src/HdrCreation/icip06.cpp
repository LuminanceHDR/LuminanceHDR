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

#include "icip06.h"
#include "debevec.h"
#include <iostream>
#define FLT_MIN 1e-6

void icip06_applyResponse( const float * arrayofexptime,
                          pfs::Array2D* Xj,
                          pfs::Array2D* Yj,
                          pfs::Array2D* Zj,
                          const float* Ir,
                          const float* Ig,
                          const float* Ib,
                          const float* w, const int iterations,
                          Array2DList &Ptemp,
                          Array2DList &P, const bool ldrinput, ... )  {
//TODO: here at least the hdr input is implemented, not so below.
assert(ldrinput);

QList<QImage*> *listldr=NULL;
Array2DList *listhdrR=NULL;
Array2DList *listhdrG=NULL;
Array2DList *listhdrB=NULL;
va_list arg_pointer;
va_start(arg_pointer,ldrinput); /* Initialize the argument list. */
int N=-1;

if (ldrinput) {
	listldr=va_arg(arg_pointer,QList<QImage*>*);
	// number of exposures
	N = listldr->count();
} else {
	listhdrR=va_arg(arg_pointer,Array2DList*);
	listhdrG=va_arg(arg_pointer,Array2DList*);
	listhdrB=va_arg(arg_pointer,Array2DList*);
	// number of exposures
	N = listhdrR->size();
}
va_end(arg_pointer); /* Clean up. */


int width = Xj->getCols();
int height = Yj->getRows();

// Create P and Ptemp, stacks of Array2DImpl; P will contain weigths computed by reinhard06_anti_ghosting funtion
for (int i=0;i<N;i++) {
	pfs::Array2D* toaddfinal = new pfs::Array2DImpl(width,height);
	pfs::Array2D* toaddtemp  = new pfs::Array2DImpl(width,height);
	assert(toaddfinal!=NULL);
	assert(toaddtemp!=NULL);
// 	if (toaddfinal==NULL || toaddtemp==NULL)
// 		throw pfs::Exception( "Cannot allocate memory for anti-ghosting weights" );
// 	else {
	P.push_back(toaddfinal);
	Ptemp.push_back(toaddtemp);
// 	}
}

//perform anti-ghosting
if (ldrinput) {
	reinhard06_anti_ghosting(w,iterations,Ptemp,P,true,listldr);
	//we don't need these anymore
	for(int i=0; i<N; i++ ) {
		delete Ptemp[i];
	}
	//apply debevec model to compute actual hdr image
	debevec_applyResponse(	arrayofexptime,
				Xj, Ir,
				Yj, Ig,
				Zj, Ib, P,
				true,listldr);
} else {
	reinhard06_anti_ghosting(w,iterations,Ptemp,P,false,listhdrR,listhdrG,listhdrB);
	//we don't need these anymore
	for(int i=0; i<N; i++ ) {
		delete Ptemp[i];
	}
	//apply debevec model to compute actual hdr image
	debevec_applyResponse(	arrayofexptime,
				Xj, Ir,
				Yj, Ig,
				Zj, Ib, P,
				false,listhdrR,listhdrG,listhdrB);
}


//we don't need these anymore
for(int i=0 ; i<N ; i++ )
	delete P[i];
}




void reinhard06_anti_ghosting(  const float* w, const int iterations,
                                Array2DList &Ptemp,
                                Array2DList &P,
                                const bool ldrinput, ... ) {


int N=-1; int width=-1; int height=-1;
QList<QImage*> *list=NULL;
// Array2DList *listhdrR=NULL;
// Array2DList *listhdrG=NULL;
// Array2DList *listhdrB=NULL;
va_list arg_pointer;
va_start(arg_pointer,ldrinput); /* Initialize the argument list. */

// if (ldrinput) {
//TODO hdr input...
	assert(ldrinput);
	list=va_arg(arg_pointer,QList<QImage*>*);
	// number of exposures
	N = list->count();
	// frame size
	width = (list->at(0))->width();
	height = (list->at(0))->height();
// } else {
// 	listhdrR=va_arg(arg_pointer,Array2DList*);
// 	listhdrG=va_arg(arg_pointer,Array2DList*);
// 	listhdrB=va_arg(arg_pointer,Array2DList*);
// 	// number of exposures
// 	N = listhdrR->size();
// 	// frame size
// 	width= ((*listhdrR)[0].yi)->getCols();
// 	height=((*listhdrR)[0].yi)->getRows();
// }
va_end(arg_pointer); /* Clean up. */


//First step, initialize the List of Weights Ptemp to its initial values, obtained via w
//for all pixels
// for all exposures for each pixel
for( int i=0 ; i<N ; i++ ) {
	QImage *p=list->at(i);
	for ( int j=0 ; j<width*height ; j++ ) {
		(*Ptemp[i])(j)=(float)(w[qRed(* ( (QRgb*)(p->bits()) +j) )]+w[qGreen(* ( (QRgb*)(p->bits()) +j) )]+w[qBlue(* ( (QRgb*)(p->bits()) +j) )])/3.0f;
	}
}


// constant used in next step
float k=pow(2.0f*M_PI,-2.5f);

//COMPUTE weightslist P for ITER iterations.
for ( int ITER=0; ITER<iterations; ITER++ ) { // for all the iterations ITER

	// update ALL the Ptemp stack using previous P*w[rgb]/3
	if (ITER!=0) {
		for ( int exposure=0; exposure<N; exposure++ )  {//for all the exposures
			QImage *p=list->at(exposure);
			for ( int j=0; j<width*height; j++ ) {
			(*Ptemp[exposure])(j) = (*P[exposure])(j) * (float)(w[qRed(* ((QRgb*)(p->bits()) +j) )]+w[qGreen(* ((QRgb*)(p->bits()) +j) )]+w[qBlue(* ((QRgb*)(p->bits()) +j) )])/3.0f;
			}
		}
	}

	for ( int exposure=0 ; exposure<N ; exposure++ ) { //for all the exposures
		QImage *cur_exposure=list->at(exposure);
		for ( int row=0 ; row<height; row++ ) { //for all the rows row (from 0 to height-1)
		for ( int col=0 ; col<width;  col++ ) { //for all the columns col (from 0 to width-1)

			// cumulative values for (*P[i])(col,row)
			float sum=0;
			float div=0;
			// get the 3 channel values for the current pixel, exposure, location col,row
// 			float Ch1x=logf(FLT_MIN+  qRed(cur_exposure->pixel(col,row)) );
// 			float Ch2x=logf(FLT_MIN+qGreen(cur_exposure->pixel(col,row)) );
// 			float Ch3x=logf(FLT_MIN+ qBlue(cur_exposure->pixel(col,row)) );
			float Ch1x=qRed(cur_exposure->pixel(col,row))/255.0;
			float Ch2x=qGreen(cur_exposure->pixel(col,row))/255.0;
			float Ch3x=qBlue(cur_exposure->pixel(col,row))/255.0;
			float Ch1y,Ch2y,Ch3y;

			// go through neighborhood, deep into the exposures, to update weight.
			// use Ptemp to read current values, and write in P -at the end of the for-.
			for ( int I=0 ; I<N ; I++ ) {
				QImage *exposure_in_neighb=list->at(I);
				// if top-left is in boundaries
				if (col-1>=0 && row-1>=0) {
// 					Ch1y=logf(FLT_MIN+  qRed(exposure_in_neighb->pixel(col-1,row-1)) );
// 					Ch2y=logf(FLT_MIN+qGreen(exposure_in_neighb->pixel(col-1,row-1)) );
// 					Ch3y=logf(FLT_MIN+ qBlue(exposure_in_neighb->pixel(col-1,row-1)) );
					Ch1y=qRed(exposure_in_neighb->pixel(col-1,row-1))/255.0;
					Ch2y=qGreen(exposure_in_neighb->pixel(col-1,row-1))/255.0;
					Ch3y=qBlue(exposure_in_neighb->pixel(col-1,row-1))/255.0;
					sum+=(*Ptemp[I])(col-1,row-1)*k*expf(-0.5*((Ch1x-Ch1y)*(Ch1x-Ch1y) + (Ch2x-Ch2y)*(Ch2x-Ch2y) + (Ch3x-Ch3y)*(Ch3x-Ch3y) + 2.0f ));
					div+=(*Ptemp[I])(col-1,row-1);
				}
				//if top is in boundaries
				if (row-1>=0) {
// 					Ch1y=logf(FLT_MIN+  qRed(exposure_in_neighb->pixel(col,row-1)) );
// 					Ch2y=logf(FLT_MIN+qGreen(exposure_in_neighb->pixel(col,row-1)) );
// 					Ch3y=logf(FLT_MIN+ qBlue(exposure_in_neighb->pixel(col,row-1)) );
					Ch1y=qRed(exposure_in_neighb->pixel(col,row-1))/255.0;
					Ch2y=qGreen(exposure_in_neighb->pixel(col,row-1))/255.0;
					Ch3y=qBlue(exposure_in_neighb->pixel(col,row-1))/255.0;
					float a=(*Ptemp[I])(col,row-1)*k*expf(-0.5*((Ch1x-Ch1y)*(Ch1x-Ch1y) + (Ch2x-Ch2y)*(Ch2x-Ch2y) + (Ch3x-Ch3y)*(Ch3x-Ch3y) + 1.0f ));
					float b=(*Ptemp[I])(col,row-1);
					sum+=a;
					div+=b;
				}
				// if top-right is in boundaries
				if (col+1<width && row-1>=0) {
// 					Ch1y=logf(FLT_MIN+  qRed(exposure_in_neighb->pixel(col+1,row-1)) );
// 					Ch2y=logf(FLT_MIN+qGreen(exposure_in_neighb->pixel(col+1,row-1)) );
// 					Ch3y=logf(FLT_MIN+ qBlue(exposure_in_neighb->pixel(col+1,row-1)) );
					Ch1y=qRed(exposure_in_neighb->pixel(col+1,row-1))/255.0;
					Ch2y=qGreen(exposure_in_neighb->pixel(col+1,row-1))/255.0;
					Ch3y=qBlue(exposure_in_neighb->pixel(col+1,row-1))/255.0;
					sum+=(*Ptemp[I])(col+1,row-1)*k*expf(-0.5*((Ch1x-Ch1y)*(Ch1x-Ch1y) + (Ch2x-Ch2y)*(Ch2x-Ch2y) + (Ch3x-Ch3y)*(Ch3x-Ch3y) + 2.0f ));
					div+=(*Ptemp[I])(col+1,row-1);
				}
				// if left is in boundaries
				if (col-1>=0) {
// 					Ch1y=logf(FLT_MIN+  qRed(exposure_in_neighb->pixel(col-1,row)) );
// 					Ch2y=logf(FLT_MIN+qGreen(exposure_in_neighb->pixel(col-1,row)) );
// 					Ch3y=logf(FLT_MIN+ qBlue(exposure_in_neighb->pixel(col-1,row)) );
					Ch1y=qRed(exposure_in_neighb->pixel(col-1,row))/255.0;
					Ch2y=qGreen(exposure_in_neighb->pixel(col-1,row))/255.0;
					Ch3y=qBlue(exposure_in_neighb->pixel(col-1,row))/255.0;
					sum+=(*Ptemp[I])(col-1,row)*k*expf(-0.5*((Ch1x-Ch1y)*(Ch1x-Ch1y) + (Ch2x-Ch2y)*(Ch2x-Ch2y) + (Ch3x-Ch3y)*(Ch3x-Ch3y) + 1.0f ));
					div+=(*Ptemp[I])(col-1,row);
				}
				// if right is in boundaries
				if (col+1<width) {
// 					Ch1y=logf(FLT_MIN+  qRed(exposure_in_neighb->pixel(col+1,row)) );
// 					Ch2y=logf(FLT_MIN+qGreen(exposure_in_neighb->pixel(col+1,row)) );
// 					Ch3y=logf(FLT_MIN+ qBlue(exposure_in_neighb->pixel(col+1,row)) );
					Ch1y=qRed(exposure_in_neighb->pixel(col+1,row))/255.0;
					Ch2y=qGreen(exposure_in_neighb->pixel(col+1,row))/255.0;
					Ch3y=qBlue(exposure_in_neighb->pixel(col+1,row))/255.0;
					sum+=(*Ptemp[I])(col+1,row)*k*expf(-0.5*((Ch1x-Ch1y)*(Ch1x-Ch1y) + (Ch2x-Ch2y)*(Ch2x-Ch2y) + (Ch3x-Ch3y)*(Ch3x-Ch3y) + 1.0f ));
					div+=(*Ptemp[I])(col+1,row);
				}
				// if bottom-left is in boundaries
				if (col-1>=0 && row+1<height) {
// 					Ch1y=logf(FLT_MIN+  qRed(exposure_in_neighb->pixel(col-1,row+1)) );
// 					Ch2y=logf(FLT_MIN+qGreen(exposure_in_neighb->pixel(col-1,row+1)) );
// 					Ch3y=logf(FLT_MIN+ qBlue(exposure_in_neighb->pixel(col-1,row+1)) );
					Ch1y=qRed(exposure_in_neighb->pixel(col-1,row+1))/255.0;
					Ch2y=qGreen(exposure_in_neighb->pixel(col-1,row+1))/255.0;
					Ch3y=qBlue(exposure_in_neighb->pixel(col-1,row+1))/255.0;
					sum+=(*Ptemp[I])(col-1,row+1)*k*expf(-0.5*((Ch1x-Ch1y)*(Ch1x-Ch1y) + (Ch2x-Ch2y)*(Ch2x-Ch2y) + (Ch3x-Ch3y)*(Ch3x-Ch3y) + 2.0f ));
					div+=(*Ptemp[I])(col-1,row+1);
				}
				// if bottom is in boundaries
				if (row+1<height) {
// 					Ch1y=logf(FLT_MIN+  qRed(exposure_in_neighb->pixel(col,row+1)) );
// 					Ch2y=logf(FLT_MIN+qGreen(exposure_in_neighb->pixel(col,row+1)) );
// 					Ch3y=logf(FLT_MIN+ qBlue(exposure_in_neighb->pixel(col,row+1)) );
					Ch1y=qRed(exposure_in_neighb->pixel(col,row+1))/255.0;
					Ch2y=qGreen(exposure_in_neighb->pixel(col,row+1))/255.0;
					Ch3y=qBlue(exposure_in_neighb->pixel(col,row+1))/255.0;
					sum+=(*Ptemp[I])(col,row+1)*k*expf(-0.5*((Ch1x-Ch1y)*(Ch1x-Ch1y) + (Ch2x-Ch2y)*(Ch2x-Ch2y) + (Ch3x-Ch3y)*(Ch3x-Ch3y) + 1.0f ));
					div+=(*Ptemp[I])(col,row+1);
				}
				// if bottom-right is in boundaries
				if (col+1<width && row+1<height) {
// 					Ch1y=logf(FLT_MIN+  qRed(exposure_in_neighb->pixel(col+1,row+1)) );
// 					Ch2y=logf(FLT_MIN+qGreen(exposure_in_neighb->pixel(col+1,row+1)) );
// 					Ch3y=logf(FLT_MIN+ qBlue(exposure_in_neighb->pixel(col+1,row+1)) );
					Ch1y=qRed(exposure_in_neighb->pixel(col+1,row+1))/255.0;
					Ch2y=qGreen(exposure_in_neighb->pixel(col+1,row+1))/255.0;
					Ch3y=qBlue(exposure_in_neighb->pixel(col+1,row+1))/255.0;
					sum+=(*Ptemp[I])(col+1,row+1)*k*expf(-0.5*((Ch1x-Ch1y)*(Ch1x-Ch1y) + (Ch2x-Ch2y)*(Ch2x-Ch2y) + (Ch3x-Ch3y)*(Ch3x-Ch3y) + 2.0f ));
					div+=(*Ptemp[I])(col+1,row+1);
				}
			} // exposure-wise neighborood
			if (div!=0.0f)
				(*P[exposure])(col,row)=(sum/div);
			else
				(*P[exposure])(col,row)=0.0f;
		} //rows in pixel col,row
		} //cols in pixel col,row
	} //exposures in iteration ITER
std::cerr << "finished iteration " << ITER+1 << " of " << iterations << std::endl;
} //iterations

}
