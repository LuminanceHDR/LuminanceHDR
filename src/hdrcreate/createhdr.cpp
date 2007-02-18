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

#include <math.h>
#include <stdio.h>
#include "createhdr.h"

extern "C" {
inline float max3( float a, float b, float c ) {
  float max = (a>b) ? a : b;
  return (c>max) ? c : max;
}

inline float min3( float a, float b, float c ) {
  // ignore zero values
  if( int(a)==0 ) a=1e8;
  if( int(b)==0 ) b=1e8;
  if( int(c)==0 ) c=1e8;

  float min = (a<b) ? a : b;
  return (c<min) ? c : min;
}

pfs::Frame* createHDR(const float * arrayofexptime, config_triple* chosen_config, bool antighosting, int iterations, const bool ldrinput, ...){

//only one of these two is used by casting extra parameters:
//listldr is used when input is a list of jpegs (and LDR tiffs, not yet implemented).
//listhdr is used when input is a list of hdrs (raw image formats, or HDR tiffs(not yet implemented)).
QList<QImage*> *listldr=NULL;
Array2DList *listhdrR=NULL;
Array2DList *listhdrG=NULL;
Array2DList *listhdrB=NULL;
va_list arg_pointer;
va_start(arg_pointer,ldrinput); /* Initialize the argument list. */

int opt_bpp;
float opt_gauss = 8.0f;
int width=-1;
int height=-1;
pfs::DOMIO pfsio;

TWeight opt_weight = chosen_config->weights;
// TResponse opt_response = chosen_config->response_curve;
TModel opt_model = chosen_config->model;

if (ldrinput) {
	listldr=va_arg(arg_pointer,QList<QImage*>*);
	width=(listldr->at(0))->width();
	height=(listldr->at(0))->height();
	opt_bpp=8;
} else {
	listhdrR=va_arg(arg_pointer,Array2DList*);
	listhdrG=va_arg(arg_pointer,Array2DList*);
	listhdrB=va_arg(arg_pointer,Array2DList*);
	width =((*listhdrR)[0])->getCols();
	height=((*listhdrR)[0])->getRows();
	opt_bpp=16;
}
va_end(arg_pointer); /* Clean up. */

//either 256(LDRs) or 65536(hdr RAW images).
int M = (int) powf(2.0f,opt_bpp);
// weighting function representing confidence in pixel values
float* w = new float[M];
int minResponse=M;
int maxResponse=0;


    if (antighosting) {
    	qDebug("antighosting, setting model debevec and weights plateau");
    	opt_weight=PLATEAU;
    	opt_model=DEBEVEC;
    }

    //PRINT WEIGHTS PREFERENCES
    switch( opt_weight )
    {
    case TRIANGULAR:
	qDebug("using triangular-shaped weights");
	break;
    case GAUSSIAN:
	qDebug( "using gaussian-shaped weights");
	break;
    case PLATEAU:
	qDebug( "using plateau-shaped weights");
	break;
    default:
	throw pfs::Exception("weights not recognized");
    }

//PRINT RESPONSE CURVES PREFERENCES
    switch( chosen_config->response_curve )
    {
    case FROM_FILE:
	qDebug("loading response curve from file");
	break;
    case LINEAR:
	qDebug( "initial response: linear" );
	break;
    case GAMMA:
	qDebug( "initial response: gamma");
	break;
    case LOG10:
	qDebug("initial response: logarithmic");
	break;
    case FROM_ROBERTSON:
	qDebug( "response curve to be found from image set");
	break;
    default:
	throw pfs::Exception("undefined standard response");
	break;
  }

//PRINT HDR-GENERATION MODEL PREFERENCES
    switch( chosen_config->model )
    {
    case ROBERTSON:
	qDebug("using robertson model");
	break;
    case DEBEVEC:
	qDebug("using debevec model");
	break;
    default:
	throw pfs::Exception("hdr generation method not set or not supported");
    }


    //0) examine input to get minResponse and maxResponse
if (ldrinput) {
	for( int i=0 ; i<listldr->size() ; i++ ) {
		uchar * ithimage=((listldr->at(i))->bits());
		for( int j=0 ; j<width*height ; j++ ) {
			int maxval = (int) max3(qRed(*((QRgb*)ithimage+j)),qGreen(*((QRgb*)ithimage+j)),qBlue(*((QRgb*)ithimage+j)));
			int minval = (int) min3(qRed(*((QRgb*)ithimage+j)),qGreen(*((QRgb*)ithimage+j)),qBlue(*((QRgb*)ithimage+j)));
			maxResponse = (maxResponse>maxval) ? maxResponse : maxval;
			minResponse = (minResponse<minval) ? minResponse : minval;
		}
	}
} else {
	for( int i=0 ; i<listhdrR->size() ; i++ ) {
		for( int j=0 ; j<width*height ; j++ ) {
			int maxval = (int) max3((*( (*listhdrR)[i] ))(j),(*( (*listhdrG)[i] ))(j),(*( (*listhdrB)[i] ))(j));
			int minval = (int) min3((*( (*listhdrR)[i] ))(j),(*( (*listhdrG)[i] ))(j),(*( (*listhdrB)[i] ))(j));
			maxResponse = (maxResponse>maxval) ? maxResponse : maxval;
			minResponse = (minResponse<minval) ? minResponse : minval;
		}
	}
}

qDebug("maxResponse=%d",maxResponse);
qDebug("minResponse=%d",minResponse);

    //1) fill in w based on weights preferences
    switch( opt_weight )
    {
    case TRIANGULAR:
	weights_triangle(w, M, minResponse, maxResponse);
	break;
    case GAUSSIAN:
	weightsGauss( w, M, minResponse, maxResponse, opt_gauss );
	break;
    case PLATEAU:
	exposure_weights_icip06(w, M, minResponse, maxResponse);
	break;
    }
    // create channels for output
    pfs::Frame *frameout = pfsio.createFrame( width, height );
    pfs::Channel *Rj, *Gj, *Bj;
    frameout->createRGBChannels( Rj, Gj, Bj );
    // camera response functions for each channel
    float* Ir = new float[M];
    float* Ig = new float[M];
    float* Ib = new float[M];
    //2) response curves, either predefined (log,gamma,lin,from_file) or calibration from the set of images.
    switch( chosen_config->response_curve )
    {
    case FROM_FILE:
	{
	    FILE * respfile=fopen(chosen_config->CurveFilename.toAscii().constData(),"r");
	    // read camera response from file
	    bool loadR_ok = responseLoad(respfile, Ir, M);
	    bool loadG_ok = responseLoad(respfile, Ig, M);
	    bool loadB_ok = responseLoad(respfile, Ib, M);
	    fclose(respfile);
	    if( !loadR_ok || !loadG_ok || !loadB_ok )
		throw pfs::Exception( "could not load response curve from file" );
	}
	break;
    case LINEAR:
	responseLinear( Ir, M );
	responseLinear( Ig, M );
	responseLinear( Ib, M );
	break;
    case GAMMA:
	responseGamma( Ir, M );
	responseGamma( Ig, M );
	responseGamma( Ib, M );
	break;
    case LOG10:
	responseLog10( Ir, M );
	responseLog10( Ig, M );
	responseLog10( Ib, M );
	break;
    case FROM_ROBERTSON:
	responseLinear( Ir, M );
	responseLinear( Ig, M );
	responseLinear( Ib, M );
	//call robertson02_getResponse method which computes both the Ir,Ig,Ib and the output HDR (i.e. its channels Rj,Gj,Bj).
	if (ldrinput) {
		robertson02_getResponse( Rj, arrayofexptime, Ir, w, M, 1, true, listldr );
		robertson02_getResponse( Gj, arrayofexptime, Ig, w, M, 2, true, listldr );
		robertson02_getResponse( Bj, arrayofexptime, Ib, w, M, 3, true, listldr );
	} else {
		robertson02_getResponse( Rj, arrayofexptime, Ir, w, M, 1, false, listhdrR );
		robertson02_getResponse( Gj, arrayofexptime, Ig, w, M, 2, false, listhdrG );
		robertson02_getResponse( Bj, arrayofexptime, Ib, w, M, 3, false, listhdrB );
	}
	break;
    }

    //3) apply model to generate hdr.
    switch( chosen_config->model ) {
    case ROBERTSON:
	//If model is robertson and user preference was to compute response curve from image dataset using robertson algorithm, i.e. to calibrate, we are done, the robertson02_getResponse function has already computed the HDR (i.e. its channels).
	if (chosen_config->response_curve==FROM_ROBERTSON)
	    break;
	else {
		//apply robertson model
		if (ldrinput) {
			robertson02_applyResponse( Rj, arrayofexptime, Ir, w, M, 1, true, listldr );
			robertson02_applyResponse( Gj, arrayofexptime, Ig, w, M, 2, true, listldr );
			robertson02_applyResponse( Bj, arrayofexptime, Ib, w, M, 3, true, listldr );
		} else {
			robertson02_applyResponse( Rj, arrayofexptime, Ir, w, M, 1, false, listhdrR );
			robertson02_applyResponse( Gj, arrayofexptime, Ig, w, M, 2, false, listhdrG );
			robertson02_applyResponse( Bj, arrayofexptime, Ib, w, M, 3, false, listhdrB );
		}
	}
	break;
    case DEBEVEC:
	//apply debevec model
	if (antighosting) {
		Array2DList Ptemp,P;
		if (ldrinput)
			icip06_applyResponse(arrayofexptime, Rj, Gj, Bj, Ir, Ig, Ib, w, iterations, Ptemp, P, ldrinput, listldr);
		else
			break; //TODO icip06 with raw data not implemented yet
	}
	else {
		if (ldrinput)
			debevec_applyResponse(arrayofexptime, Rj, Gj, Bj,  Ir, Ig, Ib, w, M, true, listldr);
		else
			debevec_applyResponse(arrayofexptime, Rj, Gj, Bj,  Ir, Ig, Ib, w, M, false, listhdrR,listhdrG,listhdrB);
	}
	break;
    } //end switch

	delete[] w;
	delete[] Ir;
	delete[] Ig;
	delete[] Ib;
	return frameout;
}
}//extern "C"
