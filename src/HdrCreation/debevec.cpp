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

#include "debevec.h"

void debevec_applyResponse( const float * arrayofexptime,
			   pfs::Array2D* Rout,  pfs::Array2D* Gout,  pfs::Array2D* Bout,
			   const float* Ir, const float* Ig, const float* Ib,
			   const float* w, int M,
			   const bool ldrinput, ... ) {
int N=-1; int width=-1; int height=-1;
QList<QImage*> *listLDR=NULL;
Array2DList *listhdrR=NULL;
Array2DList *listhdrG=NULL;
Array2DList *listhdrB=NULL;
va_list arg_pointer;
va_start(arg_pointer,ldrinput); /* Initialize the argument list. */
if (ldrinput) {
	listLDR=va_arg(arg_pointer,QList<QImage*>*);
	// number of exposures
	N = listLDR->count();
	width=(listLDR->at(0))->width();
	height=(listLDR->at(0))->height();
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

// --- anti ghosting: for each image i, find images with
// the immediately higher and lower exposure times
int* i_lower = new int[N];
int* i_upper = new int[N];
for( int i=0 ; i<N ; i++ )
{
	i_lower[i]=-1;
	i_upper[i]=-1;
	float ti =  arrayofexptime[i];
	float ti_upper = +1e6;
	float ti_lower = -1e6;

	for( int j=0 ; j<N ; j++ )
		if( i!=j )
		{
			if( arrayofexptime[j]>ti && arrayofexptime[j]<ti_upper )
			{
				ti_upper=arrayofexptime[j];
				i_upper[i]=j;
			}
			if(arrayofexptime[j]<ti && arrayofexptime[j]>ti_lower )
			{
				ti_lower=arrayofexptime[j];
				i_lower[i]=j;
			}
		}
	if( i_lower[i]==-1 )
		i_lower[i]=i;
	if( i_upper[i]==-1 )
		i_upper[i]=i;
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

	// for all exposures
	for( int i=0 ; i<N ; i++ ) {
	
		//pick the 3 channel values + alpha
		int mR = qRed  (* ( (QRgb*)( (listLDR->at(i) )->bits() ) + j ) );
		int mG = qGreen(* ( (QRgb*)( (listLDR->at(i) )->bits() ) + j ) );
		int mB = qBlue (* ( (QRgb*)( (listLDR->at(i) )->bits() ) + j ) );
		int mA = qAlpha(* ( (QRgb*)( (listLDR->at(i) )->bits() ) + j ) );

		float ti = arrayofexptime[i];

		// --- anti ghosting: monotonous increase in time should result
		// in monotonous increase in intensity; make forward and
		// backward check, ignore value if condition not satisfied
		int R_lower = qRed  (* ( (QRgb*)( (listLDR->at(i_lower[i]) )->bits() ) + j ) );
		int R_upper = qRed  (* ( (QRgb*)( (listLDR->at(i_upper[i]) )->bits() ) + j ) );
		int G_lower = qGreen(* ( (QRgb*)( (listLDR->at(i_lower[i]) )->bits() ) + j ) );
		int G_upper = qGreen(* ( (QRgb*)( (listLDR->at(i_upper[i]) )->bits() ) + j ) );
		int B_lower = qBlue (* ( (QRgb*)( (listLDR->at(i_lower[i]) )->bits() ) + j ) );
		int B_upper = qBlue (* ( (QRgb*)( (listLDR->at(i_upper[i]) )->bits() ) + j ) );

		//if at least one of the color channel's values are in the bright "not-trusted zone" and we have min exposure time
		if ( (mR>maxM || mG>maxM || mB>maxM) && (ti<minti) ) {
			//update the indexes_for_whiteRGB, minti
			index_for_whiteR=mR;
			index_for_whiteG=mG;
			index_for_whiteB=mB;
			minti=ti;
// 			continue;
		}
	
		//if at least one of the color channel's values are in the dim "not-trusted zone" and we have max exposure time
		if ( (mR<minM || mG<minM || mB<minM) && (ti>maxti) ) {
			//update the indexes_for_blackRGB, maxti
			index_for_blackR=mR;
			index_for_blackG=mG;
			index_for_blackB=mB;
			maxti=ti;
// 			continue;
		}

//The OR condition seems to be required in order not to have large areas of "invalid" color, need to investigate more.
		if ( R_lower>mR || G_lower>mG || B_lower>mB) {
			//update the indexes_for_whiteRGB, minti
			index_for_whiteR=mR;
			index_for_whiteG=mG;
			index_for_whiteB=mB;
			minti=ti;
			continue;
		}
		if ( R_upper<mR || G_upper<mG || B_upper<mB) {
			//update the indexes_for_blackRGB, maxti
			index_for_blackR=mR;
			index_for_blackG=mG;
			index_for_blackB=mB;
			maxti=ti;
			continue;
		}

		// mA assumed to handle de-ghosting masks
		// mA values assumed to be in [0, 255]
		// mA=0 assummed to mean that the pixel should be excluded
		float w_average=mA*(w[mR]+w[mG]+w[mB])/(3.0f*255.0f);
		sumR += w_average * Ir[mR] / float(ti);
		divR += w_average;
		sumG += w_average * Ig[mG] / float(ti);
		divG += w_average;
		sumB += w_average * Ib[mB] / float(ti);
		divB += w_average;
	} //END for all the exposures

	if( divR==0.0f || divG==0.0f || divB==0.0f ) {
		if (maxti>-1e6f) {
			sumR = Ir[index_for_blackR] / float(maxti);
			sumG = Ig[index_for_blackG] / float(maxti);
			sumB = Ib[index_for_blackB] / float(maxti);
			divR = divG = divB = 1.0f;
		}
		else if (minti<+1e6f) {
			sumR = Ir[index_for_whiteR] / float(minti);
			sumG = Ig[index_for_whiteG] / float(minti);
			sumB = Ib[index_for_whiteB] / float(minti);
			divR = divG = divB = 1.0f;
		}
	}

	if( divR!=0.0f && divG!=0.0f && divB!=0.0f ) {
		(*Rout)(j) = sumR/divR;
		(*Gout)(j) = sumG/divG;
		(*Bout)(j) = sumB/divB;
	} else {
		//we shouldn't be here anyway...
		(*Rout)(j) = 0.0f;
		(*Gout)(j) = 0.0f;
		(*Bout)(j) = 0.0f;
	}

}//END for all pixels, LDR case
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

		// --- anti ghosting: monotonous increase in time should result
		// in monotonous increase in intensity; make forward and
		// backward check, ignore value if condition not satisfied
		int R_lower = (int) ( ( *( ( (*listhdrR)[i_lower[i]] ) ) ) (j) );
		int R_upper = (int) ( ( *( ( (*listhdrR)[i_upper[i]] ) ) ) (j) );
		int G_lower = (int) ( ( *( ( (*listhdrG)[i_lower[i]] ) ) ) (j) );
		int G_upper = (int) ( ( *( ( (*listhdrG)[i_upper[i]] ) ) ) (j) );
		int B_lower = (int) ( ( *( ( (*listhdrB)[i_lower[i]] ) ) ) (j) );
		int B_upper = (int) ( ( *( ( (*listhdrB)[i_upper[i]] ) ) ) (j) );
		
		if( ( R_lower>mR || R_upper<mR)||( G_lower>mG || G_upper<mG)||( B_lower>mB || B_upper<mB) )
			continue;

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
		(*Rout)(j) = sumR/divR;
		(*Gout)(j) = sumG/divG;
		(*Bout)(j) = sumB/divB;
	} else {
		//we shouldn't be here anyway...
		(*Rout)(j) = 0.0f;
		(*Gout)(j) = 0.0f;
		(*Bout)(j) = 0.0f;
	}
}//END for all pixels, HDR case

delete[] i_lower;
delete[] i_upper;

}
