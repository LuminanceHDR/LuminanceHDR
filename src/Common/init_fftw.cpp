/*
* This file is a part of LuminanceHDR package, based on pfstmo.
* ----------------------------------------------------------------------
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
* @author Franco Comida <francocomida@gmail.com>
*/

#include <fftw3.h>
#ifdef _OPENMP
#include <omp.h>
#endif

#include <Common/init_fftw.h>

using namespace std;

boost::mutex FFTW_MUTEX::fftw_mutex_global;
boost::mutex FFTW_MUTEX::fftw_mutex_plan;
boost::mutex FFTW_MUTEX::fftw_mutex_destroy_plan;
boost::mutex FFTW_MUTEX::fftw_mutex_alloc;
boost::mutex FFTW_MUTEX::fftw_mutex_free;

void init_fftw() {
    FFTW_MUTEX::fftw_mutex_global.lock();
    static bool is_init_threads = false;
    // activate parallel execution of fft routines
    if (!is_init_threads) {
        fftwf_init_threads();
#ifdef _OPENMP
        fftwf_plan_with_nthreads(omp_get_max_threads());
#else
        fftwf_plan_with_nthreads(2);
#endif
        is_init_threads = true;
    }
    FFTW_MUTEX::fftw_mutex_global.unlock();
}
