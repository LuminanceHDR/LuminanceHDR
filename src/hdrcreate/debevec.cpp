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

#include "debevec.h"

//anti-saturation shouldn't be needed
int debevec_applyResponse( const float * arrayofexptime,
			pfs::Array2D* xj, const float* I1,
			pfs::Array2D* yj, const float* I2,
			pfs::Array2D* zj, const float* I3,
			const Array2DList &P,
			const bool ldrinput, ... ) {

int N=-1;
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
// } else {
// 	listhdrR=va_arg(arg_pointer,Array2DList*);
// 	listhdrG=va_arg(arg_pointer,Array2DList*);
// 	listhdrB=va_arg(arg_pointer,Array2DList*);
// 	// number of exposures
// 	N = listhdrR->size();
// }
va_end(arg_pointer); /* Clean up. */

	
	// frame size
	int width = xj->getCols();
	int height = xj->getRows();
	
	// number of saturated pixels
	int saturated_pixels = 0;
	
	// for all pixels
	for( int j=0 ; j<width*height ; j++ ) {
		float sum1 = 0.0f;
		float sum2 = 0.0f;
		float sum3 = 0.0f;
		
		float div1 = 0.0f;
		float div2 = 0.0f;
		float div3 = 0.0f;
		
		// for all exposures for each pixel
		for( int i=0 ; i<N ; i++ ) {
			//pick the 3 channel values
			int m1 = qRed(* ( (QRgb*)( (list->at(i) )->bits() ) + j ) );
			int m2 = qGreen(* ( (QRgb*)( (list->at(i) )->bits() ) + j ) );
			int m3 = qBlue(* ( (QRgb*)( (list->at(i) )->bits() ) + j ) );
			
			float ti = arrayofexptime[i];
			
			sum1 += (*P[i])(j) * I1[m1] / float(ti);
			div1 += (*P[i])(j);
			sum2 += (*P[i])(j) * I2[m2] / float(ti);
			div2 += (*P[i])(j);
			sum3 += (*P[i])(j) * I3[m3] / float(ti);
			div3 += (*P[i])(j);
		} //END for all the exposures
	
		if( div1==0.0f || div2==0.0f || div3==0.0f ) {
			saturated_pixels++;
		}
	
		if( div1!=0.0f && div2!=0.0f && div3!=0.0f ) {
			(*xj)(j) = sum1/div1;
			(*yj)(j) = sum2/div2;
			(*zj)(j) = sum3/div3;
		}
		else {
			(*xj)(j) = 0.0f;
			(*yj)(j) = 0.0f;
			(*zj)(j) = 0.0f;
		}
	}
	return saturated_pixels;
}



int debevec_applyResponse( const float * arrayofexptime,
			   pfs::Array2D* xj,  pfs::Array2D* yj,  pfs::Array2D* zj,
			   const float* Ir, const float* Ig, const float* Ib,
			   const float* w, int M,
			   const bool ldrinput, ... ) {
int N=-1; int width=-1; int height=-1;
QList<QImage*> *list=NULL;
Array2DList *listhdrR=NULL;
Array2DList *listhdrG=NULL;
Array2DList *listhdrB=NULL;
va_list arg_pointer;
va_start(arg_pointer,ldrinput); /* Initialize the argument list. */
if (ldrinput) {
	list=va_arg(arg_pointer,QList<QImage*>*);
	// number of exposures
	N = list->count();
	width=(list->at(0))->width();
	height=(list->at(0))->height();
}
else {
	listhdrR=va_arg(arg_pointer,Array2DList*);
	listhdrG=va_arg(arg_pointer,Array2DList*);
	listhdrB=va_arg(arg_pointer,Array2DList*);
	// number of exposures
	N = listhdrR->size();
	width =((*listhdrR)[0])->getCols();
	height=((*listhdrR)[0])->getRows();
}
va_end(arg_pointer); /* Clean up. */

// number of saturated pixels
int saturated_pixels = 0;

int minM = 0;
for( int m=0 ; m<M ; m++ )
	if( w[m]>0 ) {
		minM = m;
		break;
	}
int maxM = M-1;
for( int m=M-1 ; m>=0 ; m-- )
	if( w[m]>0 ) {
		maxM = m;
		break;
	}

//////////////////////LDR INPUT
if (ldrinput)
// for all pixels
for( int j=0 ; j<width*height ; j++ ) {
	float sumR = 0.0f;
	float sumG = 0.0f;
	float sumB = 0.0f;
	
	float divR = 0.0f;
	float divG = 0.0f;
	float divB = 0.0f;
	
	float maxti = -1e6f;
	float minti = +1e6f;
	
	int index_for_whiteR=-1;
	int index_for_whiteG=-1;
	int index_for_whiteB=-1;
	
	int index_for_blackR=-1;
	int index_for_blackG=-1;
	int index_for_blackB=-1;

//     bool saturated_exposure=false;

	// for all exposures
	for( int i=0 ; i<N ; i++ ) {
// 		NOT NEEDED ANYMORE, I HOPE.
// 		//we don't know yet if this exposure has saturated values or not
// 		saturated_exposure=false;
	
		//pick the 3 channel values
		int mR = qRed(* ( (QRgb*)( (list->at(i) )->bits() ) + j ) );
		int mG = qGreen(* ( (QRgb*)( (list->at(i) )->bits() ) + j ) );
		int mB = qBlue(* ( (QRgb*)( (list->at(i) )->bits() ) + j ) );
	
		float ti = arrayofexptime[i];
	
		//if at least one of the color channel's values are in the bright "not-trusted zone" and we have min exposure time
		if ( (mR>maxM || mG>maxM || mB>maxM) && (ti<minti) ) {
			//update the indexes_for_whiteRGB, minti
			index_for_whiteR=mR;
			index_for_whiteG=mG;
			index_for_whiteB=mB;
			minti=ti;
// 			saturated_exposure=true;
		}
	
		//if at least one of the color channel's values are in the dim "not-trusted zone" and we have max exposure time
		if ( (mR<minM || mG<minM || mB<minM) && (ti>maxti) ) {
			//update the indexes_for_blackRGB, maxti
			index_for_blackR=mR;
			index_for_blackG=mG;
			index_for_blackB=mB;
			maxti=ti;
// 			saturated_exposure=true;
		}
	
// 	NOT NEEDED ANYMORE, I HOPE.
// 	      //only if we managed not to end up in the white or black "not-trusted zone" for ALL the R,G and B channels, use the weighted average equation (for this exposure)
// 	      if(!saturated_exposure) {
// 	        //use (weighted average eq) only if we have a "ti" for the current exposure greater than the previous exposure's "ti" AND the RGB values are greater than those that we had in the previous exposure
// 	        bool OK_to_increment=( (ti>prev_exposure_ti) && ( (mR>prev_mR) && (mG>prev_mG) && (mB>prev_mB) ) );
// 	        // OR we have a "ti" for the current exposure smaller than the previous exposure's "ti" AND the RGB values are smaller than those that we had in the previous exposure
// 	        OK_to_increment=OK_to_increment || ( (ti<prev_exposure_ti) && ( (mR<prev_mR) && (mG<prev_mG) && (mB<prev_mB) ) );
// 	        // also, do it if this is the first exposure
// 	        OK_to_increment=OK_to_increment || (prev_exposure_ti==-1);
// 	
// 	        if ( OK_to_increment) {
// 			...
// 		}
	
		float w_average=(w[mR]+w[mG]+w[mB])/3.0f;
		sumR += w_average * Ir[mR] / float(ti);
		divR += w_average;
		sumG += w_average * Ig[mG] / float(ti);
		divG += w_average;
		sumB += w_average * Ib[mB] / float(ti);
		divB += w_average;

	} //END for all the exposures

	if( divR==0.0f || divG==0.0f || divB==0.0f ) {
        	saturated_pixels++;
		if (maxti>-1e6f) {
			sumR = Ir[index_for_blackR] / float(maxti);
			sumG = Ig[index_for_blackG] / float(maxti);
			sumB = Ib[index_for_blackB] / float(maxti);
			divR = divG = divB = 1.0f;
		}
		if (minti<+1e6f) {
			sumR = Ir[index_for_whiteR] / float(minti);
			sumG = Ig[index_for_whiteG] / float(minti);
			sumB = Ib[index_for_whiteB] / float(minti);
			divR = divG = divB = 1.0f;
		}
	}

	if( divR!=0.0f && divG!=0.0f && divB!=0.0f ) {
		(*xj)(j) = sumR/divR;
		(*yj)(j) = sumG/divG;
		(*zj)(j) = sumB/divB;
	} else {
		//we shouldn't be here anyway...
		(*xj)(j) = 0.0f;
		(*yj)(j) = 0.0f;
		(*zj)(j) = 0.0f;
	}
}//END for all pixels
else 
///////////////////////////////// HDR INPUT
// for all pixels
for( int j=0 ; j<width*height ; j++ ) {
	float sumR = 0.0f;
	float sumG = 0.0f;
	float sumB = 0.0f;
	float divR = 0.0f;
	float divG = 0.0f;
	float divB = 0.0f;
	float maxti = -1e6f;
	float minti = +1e6f;
	int index_for_whiteR=-1;
	int index_for_whiteG=-1;
	int index_for_whiteB=-1;
	int index_for_blackR=-1;
	int index_for_blackG=-1;
	int index_for_blackB=-1;

	// for all exposures
	for( int i=0 ; i<N ; i++ ) {
		//pick the 3 channel values
		int mR = (int) ( ( *( ( (*listhdrR)[i] ) ) ) (j) );
		int mG = (int) ( ( *( ( (*listhdrG)[i] ) ) ) (j) );
		int mB = (int) ( ( *( ( (*listhdrB)[i] ) ) ) (j) );
		float ti = arrayofexptime[i];

		//if at least one of the color channel's values are in the bright "not-trusted zone" and we have min exposure time
		if ( (mR>maxM || mG>maxM || mB>maxM) && (ti<minti) ) {
			//update the indexes_for_whiteRGB, minti
			index_for_whiteR=mR;
			index_for_whiteG=mG;
			index_for_whiteB=mB;
			minti=ti;
		}

		//if at least one of the color channel's values are in the dim "not-trusted zone" and we have max exposure time
		if ( (mR<minM || mG<minM || mB<minM) && (ti>maxti) ) {
			//update the indexes_for_blackRGB, maxti
			index_for_blackR=mR;
			index_for_blackG=mG;
			index_for_blackB=mB;
			maxti=ti;
		}
		float w_average=(w[mR]+w[mG]+w[mB])/3.0f;
		sumR += w_average * Ir[mR] / float(ti);
		divR += w_average;
		sumG += w_average * Ig[mG] / float(ti);
		divG += w_average;
		sumB += w_average * Ib[mB] / float(ti);
		divB += w_average;

	} //END for all the exposures

	if( divR==0.0f || divG==0.0f || divB==0.0f ) {
        	saturated_pixels++;
		if (maxti>-1e6f) {
			sumR = Ir[index_for_blackR] / float(maxti);
			sumG = Ig[index_for_blackG] / float(maxti);
			sumB = Ib[index_for_blackB] / float(maxti);
			divR = divG = divB = 1.0f;
		}
		if (minti<+1e6f) {
			sumR = Ir[index_for_whiteR] / float(minti);
			sumG = Ig[index_for_whiteG] / float(minti);
			sumB = Ib[index_for_whiteB] / float(minti);
			divR = divG = divB = 1.0f;
		}
	}

	if( divR!=0.0f && divG!=0.0f && divB!=0.0f ) {
		(*xj)(j) = sumR/divR;
		(*yj)(j) = sumG/divG;
		(*zj)(j) = sumB/divB;
	} else {
		//we shouldn't be here anyway...
		(*xj)(j) = 0.0f;
		(*yj)(j) = 0.0f;
		(*zj)(j) = 0.0f;
	}
}//END for all pixels
return saturated_pixels;
}
