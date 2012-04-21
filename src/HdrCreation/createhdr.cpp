/**
 * This file is a part of Luminance HDR package
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

#include <cmath>
#include <cstdio>
#include <vector>

#include <QFile>

#include "HdrCreation/createhdr.h"
#include "Libpfs/frame.h"
#include "Libpfs/domio.h"

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

pfs::Frame* createHDR(const float* const arrayofexptime, const config_triple* const chosen_config, bool antighosting, int iterations, const bool ldrinput, ...)
{
    //only one of these two is used by casting extra parameters:
    //listldr is used when input is a list of jpegs (and LDR tiffs).
    //listhdr is used when input is a list of hdrs (raw image formats, or HDR tiffs).
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
    TResponse opt_response = chosen_config->response_curve;
    TModel opt_model = chosen_config->model;

    if (ldrinput)
    {
        listldr=va_arg(arg_pointer,QList<QImage*>*);
        width=(listldr->at(0))->width();
        height=(listldr->at(0))->height();
        opt_bpp=8;
    }
    else
    {
        listhdrR=va_arg(arg_pointer,Array2DList*);
        listhdrG=va_arg(arg_pointer,Array2DList*);
        listhdrB=va_arg(arg_pointer,Array2DList*);
        width =((*listhdrR)[0])->getCols();
        height=((*listhdrR)[0])->getRows();
        opt_bpp=16;
    }
    va_end(arg_pointer); /* Clean up. */

    //either 256(LDRs) or 65536(hdr RAW images).
    const int M = (int) powf(2.0f, opt_bpp);
    const int minResponse=0;
    const int maxResponse=M;

    // weighting function representing confidence in pixel values
    std::vector<float> w(M); // float* w = new float[M];

    if (antighosting)
    {
    	qDebug("antighosting, setting model debevec and weights plateau");
        opt_weight = PLATEAU;
        opt_model = DEBEVEC;
    }

    //PRINT WEIGHTS PREFERENCES
    switch ( opt_weight )
    {
    case TRIANGULAR:
        qDebug("using TRIANGULAR-shaped weights");
        break;
    case GAUSSIAN:
        qDebug( "using GAUSSIAN-shaped weights");
        break;
    case PLATEAU:
        qDebug( "using PLATEAU-shaped weights");
        break;
    default:
        opt_weight = GAUSSIAN;
        qDebug("weights not recognized, hence using GAUSSIAN-shaped weights");
        break;
    }

    //PRINT RESPONSE CURVES PREFERENCES
    switch ( opt_response )
    {
    case FROM_FILE:
        qDebug("loading response curve from file");
        break;
    case LINEAR:
        qDebug( "initial response: LINEAR" );
        break;
    case GAMMA:
        qDebug( "initial response: GAMMA");
        break;
    case LOG10:
        qDebug("initial response: LOGARITHMIC");
        break;
    case FROM_ROBERTSON:
        qDebug( "response curve TO BE FOUND from image set");
        break;
    default:
        opt_response = GAMMA;
        qDebug( "Undefined standard response, hence initial response: GAMMA");
        break;
    }

    //PRINT HDR-GENERATION MODEL PREFERENCES
    switch ( opt_model )
    {
    case ROBERTSON:
        qDebug("using ROBERTSON model");
        break;
    case DEBEVEC:
        qDebug("using DEBEVEC model");
        break;
    default:
        opt_model = DEBEVEC;
        qDebug("Undefined hdr generation model, hence using DEBEVEC model");
        break;
    }

    //1) fill in w based on weights preferences
    switch ( opt_weight )
    {
    case TRIANGULAR:
        weights_triangle(w.data(), M/*, minResponse, maxResponse*/);
        break;
    case GAUSSIAN:
        weightsGauss(w.data(), M, minResponse, maxResponse, opt_gauss );
        break;
    case PLATEAU:
        exposure_weights_icip06(w.data(), M, minResponse, maxResponse);
        break;
    }
    // create channels for output
    pfs::Frame *frameout = pfsio.createFrame(width, height);
    pfs::Channel *Rj_, *Gj_, *Bj_;
    frameout->createXYZChannels( Rj_, Gj_, Bj_ );

    pfs::Array2D *Rj = Rj_->getChannelData();
    pfs::Array2D *Gj = Gj_->getChannelData();
    pfs::Array2D *Bj = Bj_->getChannelData();

    // camera response functions for each channel
    std::vector<float> Ir(M);
    std::vector<float> Ig(M);
    std::vector<float> Ib(M);

    //2) response curves, either predefined (log,gamma,lin,from_file) or calibration from the set of images.
    switch( opt_response )
    {
    case FROM_FILE:
    {
        FILE* respfile = fopen(QFile::encodeName(chosen_config->LoadCurveFromFilename).constData(),"r");
        // read camera response from file
        bool load_ok = responseLoad(respfile, Ir.data(), Ig.data(), Ib.data(), M);
        fclose(respfile);

        if ( !load_ok)
        {
            responseGamma(Ir.data(), M);
            responseGamma(Ig.data(), M);
            responseGamma(Ib.data(), M);
        }
    }
        break;
    case LINEAR:
        responseLinear(Ir.data(), M);
        responseLinear(Ig.data(), M);
        responseLinear(Ib.data(), M);
        break;
    case GAMMA:
        responseGamma(Ir.data(), M);
        responseGamma(Ig.data(), M);
        responseGamma(Ib.data(), M);
        break;
    case LOG10:
        responseLog10(Ir.data(), M);
        responseLog10(Ig.data(), M);
        responseLog10(Ib.data(), M);
        break;
    case FROM_ROBERTSON:
        responseLinear(Ir.data(), M);
        responseLinear(Ig.data(), M);
        responseLinear(Ib.data(), M);
        //call robertson02_getResponse method which computes both the Ir,Ig,Ib and the output HDR (i.e. its channels Rj,Gj,Bj).
        if (ldrinput) {
            robertson02_getResponse(Rj, arrayofexptime, Ir.data(), w.data(), M, 1, true, listldr);
            robertson02_getResponse(Gj, arrayofexptime, Ig.data(), w.data(), M, 2, true, listldr);
            robertson02_getResponse(Bj, arrayofexptime, Ib.data(), w.data(), M, 3, true, listldr);
        } else {
            robertson02_getResponse(Rj, arrayofexptime, Ir.data(), w.data(), M, 1, false, listhdrR);
            robertson02_getResponse(Gj, arrayofexptime, Ig.data(), w.data(), M, 2, false, listhdrG);
            robertson02_getResponse(Bj, arrayofexptime, Ib.data(), w.data(), M, 3, false, listhdrB);
        }
        break;
    }

	//save response curves if required (variable not empty)
	if (chosen_config->SaveCurveToFilename != "") {
		FILE * respfile=fopen(QFile::encodeName(chosen_config->SaveCurveToFilename).constData(), "w");
		responseSave(respfile, Ir.data(), Ig.data(), Ib.data(), M);
		fclose(respfile);
	}

    //3) apply model to generate hdr.
    switch ( opt_model )
    {
    case ROBERTSON:
        // If model is robertson and user preference was to compute response curve from image dataset using robertson algorithm
        // i.e. to calibrate, we are done, the robertson02_getResponse function has already computed the HDR (i.e. its channels).
        if ( opt_response == FROM_ROBERTSON)
            break;
        else {
            //apply robertson model
            if (ldrinput) {
                robertson02_applyResponse(Rj, arrayofexptime, Ir.data(), w.data(), M, 1, true, listldr);
                robertson02_applyResponse(Gj, arrayofexptime, Ig.data(), w.data(), M, 2, true, listldr);
                robertson02_applyResponse(Bj, arrayofexptime, Ib.data(), w.data(), M, 3, true, listldr);
            } else {
                robertson02_applyResponse(Rj, arrayofexptime, Ir.data(), w.data(), M, 1, false, listhdrR);
                robertson02_applyResponse(Gj, arrayofexptime, Ig.data(), w.data(), M, 2, false, listhdrG);
                robertson02_applyResponse(Bj, arrayofexptime, Ib.data(), w.data(), M, 3, false, listhdrB);
            }
        }
        break;
    case DEBEVEC:
        //apply debevec model
        if (ldrinput)
            debevec_applyResponse(arrayofexptime, Rj, Gj, Bj, Ir.data(), Ig.data(), Ib.data(), w.data(), M, true, listldr);
        else
            debevec_applyResponse(arrayofexptime, Rj, Gj, Bj, Ir.data(), Ig.data(), Ib.data(), w.data(), M, false, listhdrR, listhdrG, listhdrB);
        break;
    } //end switch

	return frameout;
}
