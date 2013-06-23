/**
 * This file is a part of Luminance HDR package
 * ---------------------------------------------------------------------- 
 * Copyright (C) 2013 Franco Comida
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
 * Manual and auto antighosting functions
 * @author Franco Comida <fcomida@users.sourceforge.net>
 *
 */

#include <QDebug>
#include <Libpfs/frame.h>
#include <Libpfs/manip/shift.h>
#include <Libpfs/manip/copy.h>
#include <Libpfs/vex/minmax.h>
#include <fftw3.h>

#include "AutoAntighosting.h"
// --- LEGACY CODE ---
static const float max_rgb = 1.0f;
static const float max_lightness = 1.0f;

inline
void rgb2hsl(float r, float g, float b, float& h, float& s, float& l)
{
    float v, m, vm, r2, g2, b2;
    h = 0.0f;
    s = 0.0f;
    l = 0.0f;

    vex::minmax(r, g, b, m, v);

    l = (m + v) / 2.0f;
    if (l <= 0.0f)
        return;
    vm = v - m;
    s = vm;
    //if (s >= 0.0f)
    if (s > 0.0f)
        s /= (l <= 0.5f) ? (v + m) : (2.0f - v - m);
    else return;
    r2 = (v - r) / vm;
    g2 = (v - g) / vm;
    b2 = (v - b) / vm;
    if (r == v)
        h = (g == m ? 5.0f + b2 : 1.0f - g2);
    else if (g == v)
        h = (b == m ? 1.0f + r2 : 3.0f - b2);
    else
        h = (r == m ? 3.0f + g2 : 5.0f - r2);
    h /= 6.0f;
}

inline
void hsl2rgb(float h, float sl, float l, float& r, float& g, float& b)
{
    float v;
    r = l;
    g = l;
    b = l;
    v = (l <= 0.5f) ? (l * (1.0f + sl)) : (l + sl - l * sl);
    if (v > 0.0f)
    {
        float m;
        float sv;
        int sextant;
        float fract, vsf, mid1, mid2;
        m = l + l - v;
        sv = (v - m ) / v;
        h *= 6.0f;
        sextant = (int)h;
        fract = h - sextant;
        vsf = v * sv * fract;
        mid1 = m + vsf;
        mid2 = v - vsf;
        switch (sextant)
        {
        case 0:
            r = v;
            g = mid1;
            b = m;
            break;
        case 1:
            r = mid2;
            g = v;
            b = m;
            break;
        case 2:
            r = m;
            g = v;
            b = mid1;
            break;
        case 3:
            r = m;
            g = mid2;
            b = v;
            break;
        case 4:
            r = mid1;
            g = m;
            b = v;
            break;
        case 5:
            r = v;
            g = m;
            b = mid2;
            break;
        }
    }
}

// r,g,b values are from 0 to 1
// h = [0,360], s = [0,1], v = [0,1]
//		if s == 0, then h = -1 (undefined)

inline
void rgb2hsv( float r, float g, float b, float &h, float &s, float &v )
{
    float min, max;

	//min = MIN( r, g, b );
	//max = MAX( r, g, b );

    vex::minmax(r, g, b, min, max);

	v = max;				// v

    float delta = max - min;

    if ( max != 0 ) {
		s = delta / max;		// s
    } else {
		// r = g = b = 0		// s = 0, v is undefined
		s = 0;
		h = -1;
		return;
	}

    if ( r == max )
		h = ( g - b ) / delta;		// between yellow & magenta
    else if ( g == max )
		h = 2 + ( b - r ) / delta;	// between cyan & yellow
	else
		h = 4 + ( r - g ) / delta;	// between magenta & cyan

	h *= 60;				// degrees
    if ( h < 0 ) {
		h += 360;
    }
}

inline
void hsv2rgb( float &r, float &g, float &b, float h, float s, float v )
{
	int i;
	float f, p, q, t;

    if ( s == 0 ) {
		// achromatic (grey)
		r = g = b = v;
		return;
	}

	h /= 60;			// sector 0 to 5
	i = floor( h );
	f = h - i;			// factorial part of h
	p = v * ( 1 - s );
	q = v * ( 1 - s * f );
	t = v * ( 1 - s * ( 1 - f ) );

	switch( i ) {
		case 0:
			r = v;
			g = t;
			b = p;
			break;
		case 1:
			r = q;
			g = v;
			b = p;
			break;
		case 2:
			r = p;
			g = v;
			b = t;
			break;
		case 3:
			r = p;
			g = q;
			b = v;
			break;
		case 4:
			r = t;
			g = p;
			b = v;
			break;
		default:		// case 5:
			r = v;
			g = p;
			b = q;
			break;
	}

}

float max(const Array2Df *u)
{
  const int nx = u->getCols();
  const int ny = u->getRows();

  return *std::max_element(u->data(), u->data() + nx*ny);
}

float min(const Array2Df *u)
{
  const int nx = u->getCols();
  const int ny = u->getRows();

  return *std::min_element(u->data(), u->data() + nx*ny);
}

void buildA(Array2Df *A, float b)
{
    const int height = A->getRows();

    for ( int j = 0; j < height; j++ ) {
        for ( int i = 0; i < height; i++ ) {
            (*A)(i, j) = 0.0f;
        }
    }

    for ( int j = 0; j < height; j++ ) {
        for ( int i = 0; i < height; i++ ) {
            if ( i == j ) {
                (*A)(i, j) = b;
                if (i > 0) {
                    (*A)(i-1, j) = 1.0f;
                }
                if (i < height - 1) {
                    (*A)(i+1, j) = 1.0f;
                }
            }
        }
    }
}

float norm(int N, vector<float> &x, vector<float> &y)
{
    float n = 0.0f;
    for ( int i = 0; i < N; i++ ) {
        n += (x[i] - y[i])*(x[i] - y[i]);
    }
    return sqrt(n);
}

void multiply(Array2Df *A, vector<float> &x, vector<float> &y)
{
    const int width = A->getCols();
    const int height = A->getRows();
    
    for ( int i = 0; i < width; i++ ) {
        y[i] = 0.0f;
    }

    for ( int j = 0; j < height; j++ ) {
        for ( int i = 0; i < width; i++ ) {
            y[j] += (*A)(i, j) * x[i];
        }
    }
}

void solve_pde_dft(Array2Df *F, Array2Df *U)
{
    const int width = U->getCols();
    const int height = U->getRows();
    assert((int)F->getCols()==width && (int)F->getRows()==height);

    Array2Df *Ftr = new Array2Df(width, height);
    Array2Df *Utr = new Array2Df(width, height);

    fftwf_plan p = fftwf_plan_r2r_2d(height, width, F->data(), Ftr->data(), FFTW_REDFT00, FFTW_REDFT00, FFTW_ESTIMATE);
    fftwf_execute(p);

    for ( int j = 0; j < height; j++ ) {
        for ( int i = 0; i < width; i++ ) {
            float wi = M_PI*i/width;
            float wj = M_PI*j/height;
            (*Utr)(i, j) = 0.5f*(*Ftr)(i, j)/(cos(wi) + cos(wj) - 2.0f);
        }
    }
    
    (*Utr)(0, 0) = 0.5f*(*Ftr)(0, 0);

    p = fftwf_plan_r2r_2d(height, width, Utr->data(), U->data(), FFTW_REDFT00, FFTW_REDFT00, FFTW_ESTIMATE);
    fftwf_execute(p);
    fftwf_execute(p);
    
    for ( int j = 0; j < height; j++ ) {
        for ( int i = 0; i < width; i++ ) {
            (*U)(i, j) /= 4.0f*((width-1)*(height-1));
        }
    }

    delete Ftr;
    delete Utr;
    fftwf_destroy_plan(p);
}


void solve_pde_dct(Array2Df *F, Array2Df *U)
{
#ifdef TIMER_PROFILING
    msec_timer stop_watch;
    stop_watch.start();
#endif
    const int width = U->getCols();
    const int height = U->getRows();
    assert((int)F->getCols()==width && (int)F->getRows()==height);

    Array2Df *Ftr = new Array2Df(width, height);

    fftwf_plan p;
    #pragma omp parallel for private(p) schedule(static)
    for ( int j = 0; j < height; j++ ) {
        #pragma omp critical (make_plan)
        p = fftwf_plan_r2r_1d(width, F->data()+width*j, Ftr->data()+width*j, FFTW_REDFT00, FFTW_ESTIMATE);
        fftwf_execute(p); 
    }
    
  #pragma omp parallel 
  {
    vector<float> c(height);
    #pragma omp for
    for ( int i = 0; i < width; i++ ) {
        for (int j = 0; j < height; j++) {
            c[j] = 1.0f; 
        }
        float b = 2.0f*(cos(M_PI*i/width) - 2.0f); 
        c[0] /= b;
        (*Ftr)(i, 0) /= b;
        for (int j = 1; j < height - 1; j++ ) {
            float m = (b - c[j-1]);
            c[j] /= m;
            (*Ftr)(i, j) = ((*Ftr)(i, j) - (*Ftr)(i, j-1))/m;   
        }
        (*Ftr)(i, height - 1) = ((*Ftr)(i, height - 1) - (*Ftr)(i, height - 2))/(b - c[height - 2]);
        (*U)(i, height - 1) = (*Ftr)(i, height - 1);
        for (int j = height - 2; j >= 0; j--) {
            (*U)(i, j) = (*Ftr)(i, j) - c[j]*(*U)(i, j+1);
        }
    }
  }
    delete Ftr;

    #pragma omp parallel for private(p) schedule(static)
    for ( int j = 0; j < height; j++ ) {
        #pragma omp critical (make_plan)
        p = fftwf_plan_r2r_1d(width, U->data()+width*j, U->data()+width*j, FFTW_REDFT00, FFTW_ESTIMATE);
        fftwf_execute(p); 
        for ( int i = 0; i < width; i++ ) {
            (*U)(i, j) /= (2.0f*(width-1));
        }
    }

    fftwf_destroy_plan(p);
#ifdef TIMER_PROFILING
    stop_watch.stop_and_update();
    std::cout << "solve_pde_dct = " << stop_watch.get_time() << " msec" << std::endl;
#endif
}

void clampToZero(Array2Df &R, Array2Df &G, Array2Df &B, float m)
{
  const int width = R.getCols();
  const int height = R.getRows();
  
  #pragma omp parallel for
  for ( int i = 0; i < width*height; i++ )
  {
    R(i) -= m;
    G(i) -= m;
    B(i) -= m;
  }
}

float norm_max( int nx, int ny, Array2Df *u)
{
  float norm = 0.0f;
  #pragma omp parallel for
  for ( int i = 0; i < nx*ny; i++ )
  {
      if (abs((*u)(i)) > norm)
          norm = abs((*u)(i));
  }
  return norm;
}

void sweep ( int nx, int ny, Array2Df* f, 
  Array2Df* u, Array2Df* unew )
{
  #pragma omp parallel for
  for ( int j = 0; j < ny; j++ )
  {
    for ( int i = 0; i < nx; i++ )
    {
      if ( i == 0 || j == 0 || i == nx - 1 || j == ny - 1 )
      {
        (*unew)(i, j) = (*f)(i, j);
      }
      else
      { 
        (*unew)(i, j) = 0.25 * ( 
          (*u)(i-1, j) + (*u)(i, j+1) + (*u)(i, j-1) + (*u)(i+1, j) - (*f)(i, j));
      }
    }
  }
}

void solve_poisson(Array2Df* f, Array2Df* u, const Array2Df* uapprox)
{
  bool converged;
  float diff;
  const int it_max = 100000;
  const int nx = u->getCols();
  const int ny = u->getRows();
  float tolerance = 3e-5;
  Array2Df* udiff = new Array2Df(nx, ny);
  Array2Df* unew = new Array2Df(nx, ny);
//
//  Set the initial solution estimate.
//  We are "allowed" to pick up the boundary conditions exactly.
//
  #pragma omp parallel for
  for ( int i = 0; i < nx*ny; i++ )
  {
    (*unew)(i) = (*uapprox)(i);
  }
//
//  Do the iteration.
//
  converged = false;

  for ( int it = 1; it <= it_max; it++ )
  {
    #pragma omp parallel for
    for ( int j = 0; j < ny; j++ )
    {
      for ( int i = 0; i < nx; i++ )
      {
        (*u)(i, j) = (*unew)(i, j);
      }
    }
//
//  UNEW is derived from U by one Jacobi step.
//
    sweep ( nx, ny, f, u, unew );
//
//  Check for convergence.
//
    #pragma omp parallel for
    for ( int i = 0; i < nx*ny; i++ )
    {
      (*udiff)(i) = (*unew)(i) - (*u)(i);
    }
    diff = norm_max ( nx, ny, udiff );
    cout << it << "\t" << diff << endl;
    if ( diff <= tolerance )
    {
      converged = true;
      break;
    }

  }

  if ( converged )
  {
    cout << "  The iteration has converged.\n";
  }
  else
  {
    cout << "  The iteration has NOT converged.\n";
  }
  delete udiff;
  delete unew;
//
//  Terminate.
//
}

int findIndex(const float* data, int size)
{
    assert(size > 0);

    int idx = 0;
    float currentMax = data[0];
    for (int i = 0; i < size; i++) {
        if ( data[i] > currentMax ) {
            currentMax = data[i];
            idx = i;
        }
    }
    return idx;
}

typedef vector<float> BufferF;

inline
float hueMean(const BufferF& data)
{
    return std::accumulate(data.begin(), data.end(), 0.0f)/data.size();
}

void hueSquaredMean(const HdrCreationItemContainer& data,
                    vector<float>& HE)
{
    size_t width = data[0].frame()->getWidth();
    size_t height = data[0].frame()->getHeight();
    size_t numItems = data.size();

    float r, g, b, h, s, l;

    BufferF hues(numItems, 0.f);
    BufferF HS(numItems, 0.f);

    const Channel *X, *Y, *Z;

    for (size_t j = 0; j < height; j++) {
        for (size_t i = 0; i < width; i++) {
            for (size_t w = 0; w < numItems; w++) {
                data[w].frame()->getXYZChannels( X, Y, Z );

                r = (*X)(i, j);
                g = (*Y)(i, j);
                b = (*Z)(i, j);
                rgb2hsl(r, g, b, h, s, l);
                hues[w] = h;
            }

            float hueMean_ = hueMean(hues);

            for (size_t w = 0; w < numItems; w++) {
                float H = hueMean_ - hues[w];
                HS[w] += H*H;
            }
        }
    }

    for (size_t w = 0; w < numItems; w++) {
        HE[w] = HS[w] / (width*height);

        qDebug() << "HE[" << w << "]: " << HE[w];
    }
}

bool comparePatches(const HdrCreationItem& item1,
                    const HdrCreationItem& item2,
                    int i, int j, int gridX, int gridY, float threshold, float deltaEV)
{
    vector<float> logRed(gridX*gridY);
    vector<float> logGreen(gridX*gridY);
    vector<float> logBlue(gridX*gridY);

    Channel *X1, *Y1, *Z1, *X2, *Y2, *Z2;
    item1.frame()->getXYZChannels( X1, Y1, Z1 );
    item2.frame()->getXYZChannels( X2, Y2, Z2 );
    Array2Df& R1 = *X1;
    Array2Df& G1 = *Y1;
    Array2Df& B1 = *Z1;
    Array2Df& R2 = *X2;
    Array2Df& G2 = *Y2;
    Array2Df& B2 = *Z2;
    
    int count = 0;
    for (int y = j * gridY; y < (j+1) * gridY; y++) {
        for (int x = i * gridX; x < (i+1) * gridX; x++) {
            if (deltaEV < 0) {
                logRed[count] = logf(R1(x, y)) - logf(R2(x, y)) - deltaEV;
                logGreen[count] = logf(G1(x, y)) - logf(G2(x, y)) - deltaEV;
                logBlue[count++] = logf(B1(x, y)) - logf(B2(x, y)) - deltaEV;
            }
            else {
                logRed[count] = logf(R2(x, y)) - logf(R1(x, y)) + deltaEV;
                logGreen[count] = logf(G2(x, y)) - logf(G1(x, y)) + deltaEV;
                logBlue[count++] = logf(B2(x, y)) - logf(B1(x, y)) + deltaEV;
            }
        }
    }
  
    float threshold1 = 0.7f * std::abs(deltaEV);
    count = 0;
    for (int h = 0; h < gridX*gridY; h++) {
        if (std::abs(logRed[h]) > threshold1 || std::abs(logGreen[h]) > threshold1 || std::abs(logBlue[h]) > threshold1)
            count++;
    }

    if ((static_cast<float>(count) / static_cast<float>(gridX*gridY)) > threshold)
        return true;
    else
        return false;

}

void computeIrradiance(Array2Df* irradiance, const Array2Df* in)
{
#ifdef TIMER_PROFILING
    msec_timer stop_watch;
    stop_watch.start();
#endif
    const int width = in->getCols();
    const int height = in->getRows();

    #pragma omp parallel for schedule(static)
    for (int j = 0; j < height; j++) {
        for (int i = 0; i < width; i++) {
            (*irradiance)(i, j) = std::exp((*in)(i, j));
        }
    }
#ifdef TIMER_PROFILING
    stop_watch.stop_and_update();
    std::cout << "computeIrradiance = " << stop_watch.get_time() << " msec" << std::endl;
#endif
}

void computeLogIrradiance(Array2Df* &logIrradiance, const Array2Df* u) 
{
#ifdef TIMER_PROFILING
    msec_timer stop_watch;
    stop_watch.start();
#endif
    const int width = u->getCols();
    const int height = u->getRows();
    
    float ir, logIr;
    #pragma omp parallel for private (ir, logIr) schedule(static)
    for (int i = 0; i < width*height; i++) {
            ir = (*u)(i);
            if (ir == 0.0f)
                logIr = -11.09f;
            else
                logIr = std::log(ir);
            (*logIrradiance)(i) = logIr;
    }
 
#ifdef TIMER_PROFILING
    stop_watch.stop_and_update();
    std::cout << "computeLogIrradiance = " << stop_watch.get_time() << " msec" << std::endl;
#endif
}

void computeGradient(Array2Df* &gradientX, Array2Df* &gradientY, const Array2Df* in)
{
#ifdef TIMER_PROFILING
    msec_timer stop_watch;
    stop_watch.start();
#endif
    const int width = in->getCols();
    const int height = in->getRows();

    #pragma omp parallel for schedule(static)
    for (int j = 1; j < height-1; j++) {
        for (int i = 1; i < width-1; i++) {
            (*gradientX)(i, j) = 0.5f*((*in)(i+1, j) - (*in)(i-1, j));
            (*gradientY)(i, j) = 0.5f*((*in)(i, j+1) - (*in)(i, j-1));
        }
    }
    #pragma omp parallel for schedule(static)
    for (int i = 1; i < width-1; i++) {
        (*gradientX)(i, 0) = 0.5f*((*in)(i+1, 0) - (*in)(i-1, 0));
        (*gradientX)(i, height-1) = 0.5f*((*in)(i+1, height-1) - (*in)(i-1, height-1));
        (*gradientY)(i, 0) = 0.0f;
        (*gradientY)(i, height-1) = 0.0f;
    }
    #pragma omp parallel for schedule(static)
    for (int j = 1; j < height-1; j++) {
        (*gradientX)(0, j) = 0.0f;
        (*gradientX)(width-1, j) = 0.0f;
        (*gradientY)(0, j) = 0.5f*((*in)(0, j+1) - (*in)(0, j-1));
        (*gradientY)(width-1, j) = 0.5f*((*in)(width-1, j+1) - (*in)(width-1, j-1));
    }
    (*gradientX)(0, 0) = (*gradientX)(0, height-1) = (*gradientX)(width-1, 0) = (*gradientX)(width-1, height-1) = 0.0f;
    (*gradientY)(0, 0) = (*gradientY)(0, height-1) = (*gradientY)(width-1, 0) = (*gradientY)(width-1, height-1) = 0.0f;
#ifdef TIMER_PROFILING
    stop_watch.stop_and_update();
    std::cout << "computeGradient = " << stop_watch.get_time() << " msec" << std::endl;
#endif
}

void computeDivergence(Array2Df* &divergence, const Array2Df* gradientX, const Array2Df* gradientY)
{
#ifdef TIMER_PROFILING
    msec_timer stop_watch;
    stop_watch.start();
#endif
    const int width = gradientX->getCols();
    const int height = gradientX->getRows();

    (*divergence)(0, 0) = (*gradientX)(0, 0) + (*gradientY)(0, 0);
    #pragma omp parallel for schedule(static)
    for (int j = 1; j < height-1; j++) {
        for (int i = 1; i < width-1; i++) {
            (*divergence)(i, j) = 0.5f*((*gradientX)(i+1, j) - (*gradientX)(i-1, j)) + 
                                  0.5f*((*gradientY)(i, j+1) - (*gradientY)(i, j-1));
        }
    }
    #pragma omp parallel for schedule(static)
    for (int j = 1; j < height-1; j++) {
        (*divergence)(0, j) = (*gradientX)(1, j) - (*gradientX)(0, j) + 0.5f*((*gradientY)(0, j+1) - (*gradientY)(0, j-1));
        (*divergence)(width-1, j) = (*gradientX)(width-1, j) - (*gradientX)(width-2, j) + 
                                    0.5f*((*gradientY)(width-1, j) - (*gradientY)(width-1, j-1));
    }
    #pragma omp parallel for schedule(static)
    for (int i = 1; i < width-1; i++) {
        (*divergence)(i, 0) = 0.5f*((*gradientX)(i, 0) - (*gradientX)(i-1, 0)) + (*gradientY)(i, 0);    
        (*divergence)(i, height-1) = 0.5f*((*gradientX)(i+1, height-1) - (*gradientX)(i-1, height-1)) + 
                                     (*gradientY)(i, height-1) - (*gradientY)(i, height-2);
    }
#ifdef TIMER_PROFILING
    stop_watch.stop_and_update();
    std::cout << "computeDivergence = " << stop_watch.get_time() << " msec" << std::endl;
#endif
}

void blendGradients(Array2Df* &gradientXBlended, Array2Df* &gradientYBlended,
                    Array2Df* &gradientX, Array2Df* &gradientY,
                    Array2Df* &gradientXGood, Array2Df* &gradientYGood,
                    bool patches[gridSize][gridSize], const int gridX, const int gridY)
{
#ifdef TIMER_PROFILING
    msec_timer stop_watch;
    stop_watch.start();
#endif
    int width = gradientX->getCols();
    int height = gradientY->getRows();

    int x, y;
    #pragma omp parallel for schedule(static)
    for (int j = 0; j < height; j++) {
        y = floor(static_cast<float>(j)/gridY);
        for (int i = 0; i < width; i++) {
            x = floor(static_cast<float>(i)/gridX);
            if (patches[x][y] == true) {
                (*gradientXBlended)(i, j) = (*gradientXGood)(i, j);
                (*gradientYBlended)(i, j) = (*gradientYGood)(i, j);
                if (i % gridX == 0) {
                    (*gradientXBlended)(i+1, j) = (*gradientXGood)(i+1, j);
                    (*gradientYBlended)(i+1, j) = (*gradientYGood)(i+1, j);
                }
                if (j % gridY == 0) {
                    (*gradientXBlended)(i, j+1) = (*gradientXGood)(i, j+1);
                    (*gradientYBlended)(i, j+1) = (*gradientYGood)(i, j+1);
                }
            }
            else {
                (*gradientXBlended)(i, j) = (*gradientX)(i, j);
                (*gradientYBlended)(i, j) = (*gradientY)(i, j);
            }
        }
    }
#ifdef TIMER_PROFILING
    stop_watch.stop_and_update();
    std::cout << "blendGradients = " << stop_watch.get_time() << " msec" << std::endl;
#endif
}

void colorBalance(pfs::Array2Df& U, const pfs::Array2Df& F, const int x, const int y, const int gridX, const int gridY)
{
    const int width = U.getCols();
    const int height = U.getRows();
    
    float umean = 0.0f;
    for (int i = x; i < x+gridX; i++) 
        for (int j = y; j < y+gridY; j++) 
            umean += U(i, j);
    umean /= gridX*gridY;

    float fmean = 0.0f;
    for (int i = x; i < x+gridX; i++) 
        for (int j = y; j < y+gridY; j++) 
            fmean += F(i, j);
    fmean /= gridX*gridY;
 
    //float sf = F(x, y) /  U(x, y);
    float sf = fmean /  umean;
    #pragma omp parallel for schedule(static)
    for (int i = 0; i < width*height; i++)
        U(i) = sf * U(i);
} 

qreal averageLightness(const Array2Df& R, const Array2Df& G, const Array2Df& B, const int i, const int j, const int gridX, const int gridY)
{
    float h, s, l;
    float avgL = 0.0f;    
    for (int y = j * gridY; y < (j+1) * gridY; y++) {
        for (int x = i * gridX; x < (i+1) * gridX; x++) {
            rgb2hsv(R(x, y), G(x, y), B(x, y), h, s, l);
            avgL += l;
        }
    }
    return avgL / (gridX*gridY);
}


qreal averageLightness(const Array2Df& R, const Array2Df& G, const Array2Df& B)
{
    int width = R.getCols();
    int height = R.getRows();
    qreal avgLum = 0.0f;

    float h, s, l;
    for (int i = 0; i < height*width; i++)
    {
        rgb2hsv(R(i), G(i), B(i), h, s, l);
        avgLum += l;
    }
    return avgLum / (width * height);
}

qreal averageLightness(const HdrCreationItem& item)
{
    int width = item.frame()->getWidth();
    int height = item.frame()->getHeight();

    Channel* C_R, *C_G, *C_B;
    item.frame()->getXYZChannels(C_R, C_G, C_B);

    float *R, *G, *B;
    R = C_R->data();
    G = C_G->data();
    B = C_B->data();

    qreal avgLum = 0.0f;

    float h, s, l;
    for (int i = 0; i < height*width; i++)
    {
        rgb2hsv(R[i], G[i], B[i], h, s, l);
        avgLum += l;
    }
    return avgLum / (width * height);
}

void blend(pfs::Array2Df& R1, pfs::Array2Df& G1, pfs::Array2Df& B1,
           const pfs::Array2Df& R2, const pfs::Array2Df& G2, const pfs::Array2Df& B2,
           const QImage& mask, const QImage& maskGoodImage)
{
#ifdef TIMER_PROFILING
    msec_timer stop_watch;
    stop_watch.start();
#endif
    const int width = R1.getCols();
    const int height = R1.getRows();

    float alpha = 0.0f;
    qreal sf = averageLightness(R1, G1, B1) / averageLightness(R2, G2, B2);

    float h, s, l;
    float r1, g1, b1;
    float r2, g2, b2;

    for (int j = 0; j < height; j++)
    {
        for (int i = 0; i < width; i++)
        {
            if (qAlpha(mask.pixel(i,j)) == 0 && qAlpha(maskGoodImage.pixel(i,j)) == 0) continue;
            alpha = (qAlpha(maskGoodImage.pixel(i,j)) == 0) ? static_cast<float>(qAlpha(mask.pixel(i,j))) / 255 : 
                                                              static_cast<float>(qAlpha(maskGoodImage.pixel(i,j))) / 255;

            r1 = R1(i, j);
            g1 = G1(i, j);
            b1 = B1(i, j);

            r2 = R2(i, j);
            g2 = G2(i, j);
            b2 = B2(i, j);

            rgb2hsv(r2, g2, b2, h, s, l);
            l *= sf;
            if (l > max_lightness) l = max_lightness;

            hsl2rgb(h, s, l, r2, g2, b2);

            if (r2 > max_rgb) r2 = max_rgb;
            if (g2 > max_rgb) g2 = max_rgb;
            if (b2 > max_rgb) b2 = max_rgb;

            if (r2 < 0.0f) r2 = 0.0f;
            if (g2 < 0.0f) g2 = 0.0f;
            if (b2 < 0.0f) b2 = 0.0f;

            R1(i, j) = (1.0f - alpha)*r1 + alpha*r2;
            G1(i, j) = (1.0f - alpha)*g1 + alpha*g2;
            B1(i, j) = (1.0f - alpha)*b1 + alpha*b2;
        }
    }
#ifdef TIMER_PROFILING
    stop_watch.stop_and_update();
    std::cout << "blend = " << stop_watch.get_time() << " msec" << std::endl;
#endif
}

QImage* shiftQImage(const QImage *in, int dx, int dy)
{
    QImage *out = new QImage(in->size(),QImage::Format_ARGB32);
    assert(out!=NULL);
    out->fill(qRgba(0,0,0,0)); //transparent black
    for(int i = 0; i < in->height(); i++)
    {
        if( (i+dy) < 0 ) continue;
        if( (i+dy) >= in->height()) break;
        QRgb *inp = (QRgb*)in->scanLine(i);
        QRgb *outp = (QRgb*)out->scanLine(i+dy);
        for(int j = 0; j < in->width(); j++)
        {
            if( (j+dx) >= in->width()) break;
            if( (j+dx) >= 0 ) outp[j+dx] = *inp;
            inp++;
        }
    }
    return out;
}

void shiftItem(HdrCreationItem& item, int dx, int dy)
{
    FramePtr shiftedFrame( pfs::shift(*item.frame(), dx, dy) );
    item.frame().swap(shiftedFrame);
    shiftedFrame.reset();       // release memory

    QScopedPointer<QImage> img(shiftQImage(item.qimage(), dx, dy));
    item.qimage()->swap( *img );
    img.reset();    // release memory
}

