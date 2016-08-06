#ifndef IMAGEN_H
#define IMAGEN_H

#include <config.h>
#include <fftw3.h>
#include <math.h>
#define MINDOUBLE 1.0e-2

#include <iostream>

using namespace std;

inline int signo(double a)
{ 
  if(a!=0.0)
    if(a>0)
      return(1);
    else
      return(-1);
  else 
    return(0);
} ;


inline double max(double a, double b)
{
  if(a<=b)
    return(b);
  else
    return(a);
};


inline double min(double a, double b)
{
  if(a>=b)
    return(b);
  else
    return(a);
};


inline float minmod(float x,float y)
{
  return(signo(x)*max(0,min(fabs(x),y*signo(x))));
};


inline double mediana(float a, float b, float c)
{
  if(a <= b)
    if(c<=a)
      return(a);
    else
      if(c<=b)
	return(c);
      else
	return(b);
  else // b < a
    if(c<=b)
      return(b);
    else
      if(c<=a)
	return(c);
      else
	return(a);
}


class Imagen
{
 public:
  int dim[2];
  float * datos;
  Imagen();
  Imagen(Imagen & im2);
  Imagen(int fil, int col);
  Imagen(int fil, int col, float val);
  ~Imagen();
   void  operator*=(float dt);
  void  operator+=(float val);
  void  operator+=(Imagen & im2);
  void  operator-=(Imagen & im2);
  void  operator*=(Imagen & im2);
  void  operator/=(Imagen & im2);
  void  operator=(Imagen & im2);
  int fils(){return dim[0];}
  int cols(){return dim[1];} 
  int area(){return dim[0]*dim[1];}
  inline float & operator()(int i, int j)
    {
      return(datos[i*dim[1]+j]);
    };
  void setvalues(int fil, int col, float* val);
    
    float maxval();
    float maxabsval();
  float minval();
  float medval();

  void recorta(float M,float m);

  void escala(float p, float q);
 // fftw_complex * fft(rfftwnd_plan p);
  void fftshift();
  float MSE(Imagen & Im2, float escala);
  Imagen & minmod(Imagen & c);
    
   
};

void producto(fftwf_complex* A, fftwf_complex* B,fftwf_complex* AB, int fil, int col);
//fftw_complex * producto(fftw_complex* A, fftw_complex* B, int fil, int col);
Imagen & nucleo_gaussiano(int fil, int col, float sigma);
int compara_dims(Imagen & im1, Imagen & im2);


#endif

