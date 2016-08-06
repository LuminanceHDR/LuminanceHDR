/**
 * @file tmo_ferradans11.cpp
 * Implementation of the algorithm presented in : 
 *
 * An Analysis of Visual Adaptation and Contrast Perception for Tone Mapping
 * S. Ferradans, M. Bertalmio, E. Provenzi, V. Caselles
 * In IEEE Trans. Pattern Analysis and Machine Intelligence
 *
*
 * @author Sira Ferradans Copyright (C) 2013
 *
 * 
 * This file is a part of LuminanceHDR package, based on pfstmo.
 * ---------------------------------------------------------------------- 
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
 */

#include <vector>
#include <algorithm>

#include <fftw3.h>
#include <assert.h>

#include "Imagen.h"

#include <cstring>
#include <iostream>

#include <stdlib.h>

#include "Libpfs/array2d.h"
#include "Libpfs/progress.h"
#include "Libpfs/utils/msec_timer.h"
#include "TonemappingOperators/pfstmo.h"
#include "tmo_ferradans11.h"


using namespace std;


//for debugging purposes
#if 0
#define PFSEOL "\x0a"
static void dumpPFS( const char *fileName, const pfstmo::Array2D *data, const char *channelName )
{
   FILE *fh = fopen( fileName, "wb" );
   assert( fh != NULL );

   int width = data->getCols();
   int height = data->getRows();

   fprintf( fh, "PFS1" PFSEOL "%d %d" PFSEOL "1" PFSEOL "0" PFSEOL
     "%s" PFSEOL "0" PFSEOL "ENDH", width, height, channelName );

   for( int y = 0; y < height; y++ )
     for( int x = 0; x < width; x++ ) {
       float d = (*data)(x,y);       
       fwrite( &d, sizeof( float ), 1, fh );
     }
  
   fclose( fh );
}
#endif


//--------------------------------------------------------------------


/*Implementation, hardcoded of the R function */
float apply_arctg_slope10(float Ip,float I,float I2,float I3,float I4,float I5,float I6,float I7)
{
    //Arctan, slope 10
    float  gr1,gr2,gr3,gr4,gr5,gr6,gr7;
    gr1=Ip-I;
    gr2=Ip*Ip-2*Ip*I+I2;
    gr3=Ip*Ip*Ip-3*Ip*Ip*I+3*Ip*I2-I3;
    gr4=Ip*Ip*Ip*Ip+4*Ip*Ip*I2+I4-4*Ip*I3+2*Ip*Ip*(I2-2*Ip*I);
    gr5=Ip*gr4-Ip*Ip*Ip*Ip*I-4*Ip*Ip*I3-I5+4*Ip*I4-2*Ip*Ip*(I3-2*Ip*I2);
    gr6=Ip*Ip*gr4-Ip*Ip*Ip*Ip*Ip*I-4*Ip*Ip*Ip*I3-Ip*I5+4*Ip*Ip*I4-2*Ip*Ip*Ip*(I3-2*Ip*I2)-(I*Ip*gr4-Ip*Ip*Ip*Ip*I2-4*Ip*Ip*I4-I6+4*Ip*I5-2*Ip*Ip*(I4-2*Ip*I3)    );
    gr7=Ip*Ip*(Ip*(  Ip*Ip*Ip*Ip+4*Ip*Ip*I2+I4-4*Ip*I3+2*Ip*Ip*(I2-2*Ip*I)     )-Ip*Ip*Ip*Ip*I-4*Ip*Ip*I3-I5+4*Ip*I4-2*Ip*Ip*(I3-2*Ip*I2))-2*Ip*(Ip*(Ip*Ip*Ip*Ip*I+4*Ip*Ip*I3+I5-4*Ip*I4+2*Ip*Ip*(I3-2*Ip*I2)) -Ip*Ip*Ip*Ip*I2-4*Ip*Ip*I4-I6+4*Ip*I5-2*Ip*Ip*(I4-2*Ip*I3))+Ip*(Ip*Ip*Ip*Ip*I2+4*Ip*Ip*I4+I6-4*Ip*I5+2*Ip*Ip*(I4-2*Ip*I3)) -Ip*Ip*Ip*Ip*I3-4*Ip*Ip*I5-I7+4*Ip*I6-2*Ip*Ip*(I5-2*Ip*I4);
    
    // Hardcoded coefficients of the polynomial of degree 9 that allows to approximate the neighborhood averaging as a sum of convolutions
    return(( -7.7456e+00)*gr7+ (3.1255e-16)*gr6+(1.5836e+01)*gr5+(-1.8371e-15)*gr4+(-1.1013e+01)*gr3+(4.4531e-16)*gr2+(3.7891e+00)*gr1+ 1.2391e-15 ) ;
}



/*
 *  This Quickselect routine is based on the algorithm described in
 *  "Numerical recipes in C", Second Edition,
 *  Cambridge University Press, 1992, Section 8.5, ISBN 0-521-43108-5
 *  This code by Nicolas Devillard - 1998. Public domain.
 */


#define ELEM_SWAP(a,b) { register float t=(a);(a)=(b);(b)=t; }
//#define ELEM_SWAP(a,b) { register float t=a;a=b;b=t; }

double quick_select(float arr[], int n)
{
    int low, high ;
    int median;
    int middle, ll, hh;
    
    low = 0 ; high = n-1 ; median = (low + high) / 2;
    for (;;) {
        if (high <= low) /* One element only */
            return arr[median] ;
        
        if (high == low + 1) {  /* Two elements only */
            if (arr[low] > arr[high])
                ELEM_SWAP(arr[low], arr[high]) ;
            return arr[median] ;
        }
        
        /* Find median of low, middle and high items; swap into position low */
        middle = (low + high) / 2;
        if (arr[middle] > arr[high])    ELEM_SWAP(arr[middle], arr[high]) ;
        if (arr[low] > arr[high])       ELEM_SWAP(arr[low], arr[high]) ;
        if (arr[middle] > arr[low])     ELEM_SWAP(arr[middle], arr[low]) ;
        
        /* Swap low item (now in position middle) into position (low+1) */
        ELEM_SWAP(arr[middle], arr[low+1]) ;
        
        /* Nibble from each end towards middle, swapping items when stuck */
        ll = low + 1;
        hh = high;
        for (;;) {
            do ll++; while (arr[low] > arr[ll]) ;
            do hh--; while (arr[hh]  > arr[low]) ;
            
            if (hh < ll)
                break;
            
            ELEM_SWAP(arr[ll], arr[hh]) ;
        }
        
        /* Swap middle item (in position low) back into correct position */
        ELEM_SWAP(arr[low], arr[hh]) ;
        
        /* Re-set active partition */
        if (hh <= median)
            low = ll;
        if (hh >= median)
            high = hh - 1;
    }
}

#undef ELEM_SWAP

void tmo_ferradans11(int col,int fil,float* imR, float* imG, float* imB, float rho, float invalpha, pfs::Progress &ph){
#ifdef TIMER_PROFILING
    msec_timer stop_watch;
    stop_watch.start();
#endif

    int length=fil*col;
    int colors=3;
    float dt=0.2;//1e-1;//
    float threshold_diff=dt/20.0;//1e-5;//
    
    
    Imagen RGBorig[3],*pIm;
    
    pIm = new Imagen(fil,col);
    for (int c =0;c<3;c++){
        RGBorig[c] = *pIm;
        
    }
    
    ph.setValue(2);
    if (ph.canceled()) return;

    for(int i=0;i<length;i++){
        RGBorig[0].datos[i] = (double)max(imR[i],0);
        RGBorig[1].datos[i] = (double)max(imG[i],0);
        RGBorig[2].datos[i] = (double)max(imB[i],0);
    }
    
    delete(pIm);
    
    ///////////////////////////////////////////
    
    Imagen RGB[3];
    
    int iteration=0;
    float difference=1000.0;
    
    float med[3];
    
    Imagen aux;
    float median,mu[3];
    
    for(int k=0;k<3;k++)
    {
        RGBorig[k]+=1e-6;
        aux=RGBorig[k];
        median=quick_select(aux.datos,length);
        mu[k]=pow(aux.medval(),0.5)*pow(median,0.5);
        
        RGB[k]=RGBorig[k] ;
    }
    
    ph.setValue(5);
    if (ph.canceled()) return;

    // SEMISATURATION CONSTANT SIGMA DEPENDS ON THE ILUMINATION
    // OF THE BACKGROUND. DATA IN TABLA1 FROM VALETON+VAN NORREN.
    // TAKE AS REFERENCE THE CHANNEL WITH MAXIMUM ILUMINATION.
    for(int k=0;k<3;k++)
    {
        //MOVE log(mu) EQUALLY FOR THE 3 COLOR CHANNELS: rho DOES NOT CHANGE
        //PARAMETER OF OUR ALGORITHM
        float z= - 0.37*(log10(mu[k])+4-rho) + 1.9;
        mu[k]*=pow(10,z);
    }
    
    ph.setValue(10);
    if (ph.canceled()) return;

    
    // VALETON + VAN NORREN:
    // range = 4 orders; r is half the range
    // n=0.74 : EXPONENT in NAKA-RUSHTON formula 
    float r=2;
    float n=0.74;

    for(int k=0;k<3;k++)
    {
        //find ctes. for WEBER-FECHNER from NAKA-RUSHTON
        float logs=log10(mu[k]);
        float I0=mu[k]/pow(10,1.2);
        float sigma_n=pow(mu[k],n);
        
        // WYSZECKI-STILES, PÁG. 530: FECHNER FRACTION
        float K_=100.0/1.85;
        if(k==2)
            K_= 100.0/8.7;
        float Ir=pow(10,logs+r);
        float mKlogc=pow(Ir,n)/(pow(Ir,n)+pow(mu[k],n))-K_*log10(Ir+I0);
    
        //mix W-F and N-R
        for(int d=0;d<length;d++)
      	{
            float x=log10(RGBorig[k].datos[d]);
            float In= pow(RGBorig[k].datos[d],n);
            //float srn=pow(pow(10,logs+r),n);
            // before logs+r apply W-F, after N-R
            if(x<=logs+r)
                RGB[k].datos[d]=K_*log10( RGBorig[k].datos[d] + I0)+mKlogc;
            else
                RGB[k].datos[d]= In/(In+sigma_n);
            
      	}
        
        float minmez=RGB[k].minval();
        RGB[k]+= -minmez;
        float escalamez=1.0/(RGB[k].maxval()+1e-12);
        RGB[k]*=escalamez;
        
    }
    
    ph.setValue(15);
    if (ph.canceled()) return;

    for(int color=0;color<colors;color++)
    {
        RGBorig[color]=RGB[color];
        med[color]=RGB[color].medval();
    }
    
    Imagen RGB0=RGB[0];
    Imagen u=RGB[0];
    Imagen u0=RGB[0];
    Imagen u2=RGB[0];
    Imagen u3=RGB[0];
    Imagen u4=RGB[0];
    Imagen u5=RGB[0];
    Imagen u6=RGB[0];
    Imagen u7=RGB[0];
    Imagen I0= RGB0;
    Imagen ut= RGB0;
    
    
    fftwf_complex* U = (fftwf_complex*) fftwf_malloc(sizeof(fftwf_complex) * length);
    fftwf_plan pU = fftwf_plan_dft_r2c_2d(fil, col, u0.datos, U,FFTW_ESTIMATE);
    fftwf_complex* U2 = (fftwf_complex*) fftwf_malloc(sizeof(fftwf_complex) * length);
    fftwf_plan pU2 = fftwf_plan_dft_r2c_2d(fil, col, u2.datos, U2,FFTW_ESTIMATE);
    fftwf_complex* U3 = (fftwf_complex*) fftwf_malloc(sizeof(fftwf_complex) * length);
    fftwf_plan pU3 = fftwf_plan_dft_r2c_2d(fil, col, u3.datos, U3,FFTW_ESTIMATE);
    fftwf_complex* U4 = (fftwf_complex*) fftwf_malloc(sizeof(fftwf_complex) * length);
    fftwf_plan pU4 = fftwf_plan_dft_r2c_2d(fil, col, u4.datos, U4,FFTW_ESTIMATE);
    
    fftwf_complex* U5 = (fftwf_complex*) fftwf_malloc(sizeof(fftwf_complex) * length);
    fftwf_plan pU5 = fftwf_plan_dft_r2c_2d(fil, col, u5.datos, U5,FFTW_ESTIMATE);
    fftwf_complex* U6 = (fftwf_complex*) fftwf_malloc(sizeof(fftwf_complex) * length);
    fftwf_plan pU6 = fftwf_plan_dft_r2c_2d(fil, col, u6.datos, U6,FFTW_ESTIMATE);
    fftwf_complex* U7 = (fftwf_complex*) fftwf_malloc(sizeof(fftwf_complex) * length);
    fftwf_plan pU7 = fftwf_plan_dft_r2c_2d(fil, col, u7.datos, U7,FFTW_ESTIMATE);
    
    fftwf_complex* UG = new fftwf_complex[fil*col];
    fftwf_complex* U2G= new fftwf_complex[fil*col];
    fftwf_complex* U3G= new fftwf_complex[fil*col];
    fftwf_complex* U4G= new fftwf_complex[fil*col];
    fftwf_complex* U5G= new fftwf_complex[fil*col];
    fftwf_complex* U6G= new fftwf_complex[fil*col];
    fftwf_complex* U7G= new fftwf_complex[fil*col];
    
    Imagen iu(fil,col,0);
    fftwf_plan pinvU = fftwf_plan_dft_c2r_2d(fil, col, UG,iu.datos, FFTW_ESTIMATE);
    Imagen  iu2(fil,col,0);
    fftwf_plan pinvU2 = fftwf_plan_dft_c2r_2d(fil, col, U2G,iu2.datos, FFTW_ESTIMATE);
    Imagen  iu3(fil,col,0);
    fftwf_plan pinvU3 = fftwf_plan_dft_c2r_2d(fil, col, U3G,iu3.datos, FFTW_ESTIMATE);
    Imagen  iu4(fil,col,0);
    fftwf_plan pinvU4 = fftwf_plan_dft_c2r_2d(fil, col, U4G,iu4.datos, FFTW_ESTIMATE);
    Imagen iu5(fil,col,0);
    fftwf_plan pinvU5 = fftwf_plan_dft_c2r_2d(fil, col, U5G,iu5.datos, FFTW_ESTIMATE);
    Imagen iu6(fil,col,0);
    fftwf_plan pinvU6 = fftwf_plan_dft_c2r_2d(fil, col, U6G,iu6.datos, FFTW_ESTIMATE);
    Imagen iu7(fil,col,0);
    fftwf_plan pinvU7 = fftwf_plan_dft_c2r_2d(fil, col, U7G,iu7.datos, FFTW_ESTIMATE);
    
    float alpha=min(fil,col)/invalpha;
    Imagen g;
    g=nucleo_gaussiano(fil,col,alpha);
    g.escala(1.0,0.0);
    
    g.fftshift();
    
    float suma=0, norm=1.0/(fil*col);
    for(int i=0;i<length;i++)
        suma+=g.datos[i];
    
    g*=(1.0/suma); //now integral of the weight sums 1
    
    fftwf_complex* G = (fftwf_complex*) fftwf_malloc(sizeof(fftwf_complex) * length);
    fftwf_plan pG = fftwf_plan_dft_r2c_2d(fil, col, g.datos, G,FFTW_ESTIMATE);
    fftwf_execute(pG);
    
    ph.setValue(25);
    if (ph.canceled()) return;

    while(difference>threshold_diff)
    {
        iteration++;
        difference=0.0;

        for(int color=0;color<colors;color++)
        {
            u0=RGB[color];
            RGB0=RGB[color];
            
            u=u0;
            u2=u; u2*=u;
            u3=u2;u3*=u;
            u4=u3;u4*=u;
            u5=u4;u5*=u;
            u6=u5;u6*=u;
            u7=u6;u7*=u;
            
            fftwf_execute(pU);
            fftwf_execute(pU2);
            fftwf_execute(pU3);
            fftwf_execute(pU4);
            fftwf_execute(pU5);
            fftwf_execute(pU6);
            fftwf_execute(pU7);
            
            producto(U,G,UG,fil,col);
            producto(U2,G,U2G,fil,col);
            producto(U3,G,U3G,fil,col);
            producto(U4,G,U4G,fil,col);
            producto(U5,G,U5G,fil,col);
            producto(U6,G,U6G,fil,col);
            producto(U7,G,U7G,fil,col);
            
            fftwf_execute(pinvU);
            iu *= norm;
            
            fftwf_execute(pinvU2);
            iu2 *= norm;
            fftwf_execute(pinvU3);
            iu3 *= norm;
            fftwf_execute(pinvU4);
            iu4 *= norm;
            fftwf_execute(pinvU5);
            iu5 *= norm;
            fftwf_execute(pinvU6);
            iu6 *= norm;
            fftwf_execute(pinvU7);
            iu7 *= norm;

            for(int i=0;i<length;i++)
            {
                // compute contrast component
                 u0.datos[i]=apply_arctg_slope10(u0.datos[i],iu.datos[i],iu2.datos[i],iu3.datos[i],iu4.datos[i],iu5.datos[i],iu6.datos[i],iu7.datos[i]);
                
                //project onto the interval [-1,1]
                 u0.datos[i] = max( min( u0.datos[i],1) , -1 );
            }
            
            // normalizing R term to estandarize results
            u0 *= (1.0/u0.maxabsval());
            
            double norm = (1.0 + dt*(1.0+255.0/253.0));// assuming alpha=255/253,beta=1 
            for(int i=0;i<length;i++)
            {
                RGB[color].datos[i] = (RGB[color].datos[i] + dt*(RGBorig[color].datos[i] + 0.5*u0.datos[i]+255.0/253.0 *med[color])) / norm;
                //project onto the interval [0,1]
                RGB[color].datos[i] = max( min( RGB[color].datos[i],1) , 0 );
            }
            
            difference+=RGB0.MSE(RGB[color],1.0);
            
        }
    }
    delete[] U;
    delete[] U2;
    delete[] U3;
    delete[] U4;
    delete[] U5;
    delete[] U6;
    delete[] U7;
    
    delete[] UG;
    delete[] U2G;
    delete[] U3G;
    delete[] U4G;
    delete[] U5G;
    delete[] U6G;
    delete[] U7G;
    
    ph.setValue(90);
    if (ph.canceled()) return;

    for(int c=0;c<3;c++)
        RGB[c].escala(1,0);
    
    //range between (0,1)
    for (int i =0;i<length;i++){
        imR[i] = (float)RGB[0].datos[i];
        imG[i] = (float)RGB[1].datos[i];
        imB[i] = (float)RGB[2].datos[i];
    }
 
    delete[] G;
    ph.setValue(100);
#ifdef TIMER_PROFILING
    stop_watch.stop_and_update();
    std::cout << "tmo_ferradans11 = " << stop_watch.get_time() << " msec" << std::endl;
#endif
}
