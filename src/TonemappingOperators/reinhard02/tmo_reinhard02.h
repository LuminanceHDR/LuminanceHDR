/**
 * This file is a part of LuminanceHDR package, based on pfstmo.
 * @file tmo_reinhard02.h
 * @brief Tone map luminance channel using Reinhard02 model
 *
 * @author Grzegorz Krawczyk, <krawczyk@mpi-sb.mpg.de>
 *
 * $Id: tmo_reinhard02.h,v 1.2 2008/09/04 12:46:49 julians37 Exp $
 */

#ifndef _tmo_reinhard02_h_
#define _tmo_reinhard02_h_

#include "Common/ProgressHelper.h"

/**
 * Used to achieve temporal coherence
 */
template<class T>
class TemporalSmoothVariable
{
//  const int hist_length = 100;
  T value;

  T getThreshold( T luminance )
  {
    return 0.01 * luminance;
  }
  
public:
  TemporalSmoothVariable() : value( -1 )
  {
  }
  
  void set( T new_value )
  {
    if( value == -1 )
      value = new_value;
    else {
      T delta = new_value - value;
      const T threshold = getThreshold( (new_value + value)/2 );
      if( delta > threshold ) delta = threshold;
      else if( delta < -threshold ) delta = -threshold;
      value += delta;
    }
  }
  
  T get() const
  {
    return value;
  }  
};

//--- from defines.h
typedef struct {
  int     xmax, ymax;     /* image dimensions */
} CVTS;

typedef double  COLOR[3];       /* red, green, blue (or X,Y,Z) */
//--- end of defines.h


//static double    key              = 0.18;
//static double    threshold        = 0.05;
//static double    phi              = 8.;
//static double    white            = 1e20;
//static int       scale_low        = 1;
//static int       scale_high       = 43;  // 1.6^8 = 43
//static int       range            = 8;
//static int       use_scales       = 0;
//static int       use_border       = 0;



/*
 * @brief Photographic tone-reproduction
 *
 * @param width image width
 * @param height image height
 * @param Y input luminance
 * @param L output tonemapped intensities
 * @param use_scales true: local version, false: global version of TMO
 * @param key maps log average luminance to this value (default: 0.18)
 * @param phi sharpening parameter (defaults to 1 - no sharpening)
 * @param num number of scales to use in computation (default: 8)
 * @param low size in pixels of smallest scale (should be kept at 1)
 * @param high size in pixels of largest scale (default 1.6^8 = 43)
 */
class Reinhard02
{
public:
	Reinhard02(unsigned int width, unsigned int height,
	const float *Y, float *L,
	bool use_scales, float key, float phi,
	int num, int low, int high, bool temporal_coherent, ProgressHelper *ph );
	~Reinhard02() { delete m_Y; delete m_L; };
	void tmo_reinhard02(); 

private:
	TemporalSmoothVariable<double> m_avg_luminance, m_max_luminance;
	CVTS m_cvts;
	COLOR   **m_image;
	double m_sigma_0, m_sigma_1;
	double **m_luminance;
	unsigned int m_width, m_height;
	const pfs::Array2D*  m_Y;
	pfs::Array2D* m_L;
	bool m_use_scales;
	bool m_use_border;
	double m_key, m_phi, m_white;
	int m_range, m_scale_low, m_scale_high;
	bool  m_temporal_coherent;
	const double m_alpha;	
	double m_bbeta;
	double m_threshold;
	ProgressHelper *m_ph;

	double ***Pyramid;
	int       PyramidHeight;
	int       PyramidWidth0;

	double bessel(double);
	void compute_bessel();
	double kaiserbessel(double, double, double);
	double get_maxvalue();
	void tonemap_image();
	void clamp_image();
	double log_average();
	void scale_to_midtone();
	void copy_luminance();
	void allocate_memory();
	void deallocate_memory();
	void dynamic_range();
	double V1(int, int, int);
	double pyramid_lookup(int, int, int);
	void build_pyramid(double **, int, int);
	void clean_pyramid();
};
#endif /* _tmo_reinhard02_h_ */
