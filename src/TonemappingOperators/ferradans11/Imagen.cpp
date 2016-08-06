#include "Imagen.h"

Imagen::Imagen()
{
  dim[0]=0;
  dim[1]=0;
  datos=new float[1];
}

Imagen::Imagen(Imagen & im2)
{
  dim[0]=im2.fils();
  dim[1]=im2.cols();
  int largo=dim[0]*dim[1];
  datos=new float[largo];
  for(int i=0; i< largo; i++)
    datos[i]=im2.datos[i];
}

Imagen::Imagen(int fil, int col)
{
  dim[0]=fil;
  dim[1]=col;
  datos=new float[fil*col];
}

Imagen::Imagen(int fil, int col, float val)
{
  dim[0]=fil;
  dim[1]=col;
  datos=new float[fil*col];
  for(int i=0; i<fil*col;i++)
    datos[i]=val;
}


Imagen::~Imagen()
{
  delete[] datos;
}


void Imagen::operator*=(float dt)
{
  int largo=dim[0]*dim[1];
  for(int i=0;i<largo;i++)
    datos[i]*=dt;
}

void Imagen::operator+=(float dt)
{
  int largo=dim[0]*dim[1];
  for(int i=0;i<largo;i++)
    datos[i]+=dt;
}

void Imagen::operator+=(Imagen & im2)
{
  int fil2=im2.fils();
  int col2=im2.cols();
  if(dim[0]!=fil2 || dim[1]!=col2)
    {
      fprintf(stderr,"Diferentes dimensiones al sumar imagenes \n");
      return;
    }

  int largo=dim[0]*dim[1];
  for(int i=0; i<largo; i++)
    datos[i]+=im2.datos[i];

  return;
}

void Imagen::operator-=(Imagen & im2)
{
  int fil2=im2.fils();
  int col2=im2.cols();
  if(dim[0]!=fil2 || dim[1]!=col2)
    {
      fprintf(stderr,"Diferentes dimensiones al restar imagenes \n");
      return;
    }

  int largo=dim[0]*dim[1];
  for(int i=0; i<largo; i++)
    datos[i]-=im2.datos[i];

  return;
}


void Imagen::operator*=(Imagen  & im2)
{
  int fil2=im2.fils();
  int col2=im2.cols();
  if(dim[0]!=fil2 || dim[1]!=col2)
    {
      fprintf(stderr,"Diferentes dimensiones al multiplicar imagenes \n");
      return;
    }

  int largo=dim[0]*dim[1];
  for(int i=0; i<largo; i++)
    this->datos[i] *= (&im2)->datos[i];

  return;
}

void Imagen::operator/=(Imagen  & im2)
{
  // si el divisor es 0, pone 0 en el cociente...
  int fil2=im2.fils();
  int col2=im2.cols();
  if(dim[0]!=fil2 || dim[1]!=col2)
    {
      fprintf(stderr,"Diferentes dimensiones al multiplicar imagenes \n");
      return;
    }

  int largo=dim[0]*dim[1];
  double div;
  for(int i=0; i<largo; i++)
    {
      div=im2.datos[i];
      if(fabs(div)>1e-10)
	datos[i]/=div;
      else
	datos[i]=0.0;
    }
  return;
}

void Imagen::operator=(Imagen  & im2)
{
  // este operador no duplica la memoria //
  int fil2=im2.fils();
  int col2=im2.cols();
  if(dim[0]!=fil2 || dim[1]!=col2)
    {
      //      fprintf(stderr,"Diferentes dimensiones al copiar imagenes \n");
      dim[0]=fil2;
      dim[1]=col2;

      delete[] datos;
      datos=new float[fil2*col2];
    }

  int largo=dim[0]*dim[1];
  for(int i=0; i<largo; i++)
    datos[i]=im2.datos[i];

  return;
}

void Imagen::setvalues(int fil, int col, float* val)
{
    dim[0]=fil;
    dim[1]=col;
    datos=val;
}


float Imagen::minval()
{
  float m=1e20;
  int largo=dim[0]*dim[1];
  for(int i=0; i<largo; i++)
    m=min(m,datos[i]);

  return m;
}


float Imagen::maxval()
{
  float m=-1e20;
  int largo=dim[0]*dim[1];
  for(int i=0; i<largo; i++)
    m=max(m,datos[i]);

  return m;
}

float Imagen::maxabsval()
{
    float m=-1e20;
    int largo=dim[0]*dim[1];
    for(int i=0; i<largo; i++)
        m=max(m,fabs(datos[i]));
    
    return m;
}


float Imagen::medval()
{
  float m=0.0;
  int largo=dim[0]*dim[1];
  for(int i=0; i<largo; i++)
    m+=datos[i];
  m/= (double)largo;
  return m;
}

void Imagen::recorta(float M,float m)
{
    // recorta los valores mayores que M y menores que m
    
    int largo=dim[0]*dim[1];
    for(int i=0;i<largo;i++)
    {
        datos[i]=min(datos[i],M);
        datos[i]=max(datos[i],m);
    }
}



float Imagen::MSE(Imagen & Im2, float escala)
{
    int largo=dim[0]*dim[1];
    float res=0.0;
    float tmp;
    float v=1.0/escala;
    float v2=1.0;
    for(int i=0;i<largo;i++)
    {
        tmp=(datos[i])*v-(Im2.datos[i])*v2;
        tmp=fabs(tmp);
        res+=tmp;
    }
    return(res/largo);
    
}


void Imagen::escala(float maxv, float minv)
{
    int largo=dim[0]*dim[1];
    float M=this->maxval();
    float m=this->minval();
    
    float R;
    float eps=1e-6;
    float s=(maxv-minv)/(M-m);
    for(int i=0;i<largo;i++)
    {
        R=datos[i];
        datos[i]=minv+s*(R-m);
    }
}

// Related to fft
void Imagen::fftshift()
{
    int fil=dim[0];
    int col=dim[1];
    float tmp;
    
    for(int i=0;i<fil/2;i++)
        for(int j=0;j<col/2;j++)
        {
            tmp=(*this)(i,j);
            (*this)(i,j)=(*this)(i+fil/2,j+col/2);
            (*this)(i+fil/2,j+col/2)=tmp;
            tmp=(*this)(i+fil/2,j);
            (*this)(i+fil/2,j)=(*this)(i,j+col/2);
            (*this)(i,j+col/2)=tmp;
        }
}

Imagen & nucleo_gaussiano(int fil, int col, float sigma)
{
    Imagen * res= new Imagen(fil,col,0.0);
    float normaliza=1.0/(sqrt(2*M_PI)*sigma+1e-6);
    int i_;
    int mitfil=fil/2;
    int mitcol=col/2;
    for(int i=0;i<fil;i++)
        for(int j=0;j<col;j++)
            (*res)(i,j)=normaliza*exp( -((i-mitfil)*(i-mitfil)+(j-mitcol)*(j-mitcol) )/(2*sigma*sigma) );
    
    return(*res);
}


void producto(fftwf_complex* A, fftwf_complex* B,fftwf_complex* AB, int fil, int col)
{
  //  int length=fil*(col/2+1);
    //fftw_complex* C= (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * length);
    int length=fil*col;
    
    for (int i = 0; i < length; i++)
    {
        AB[i][0] = (A[i][0] * B[i][0] - A[i][1] * B[i][1]);
        AB[i][1] = (A[i][0] * B[i][1] + A[i][1] * B[i][0]);
    }
    
    
}

