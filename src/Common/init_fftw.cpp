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
boost::mutex FFTW_MUTEX::fftw_mutex_ag;
boost::mutex FFTW_MUTEX::fftw_mutex_fattal_1;
boost::mutex FFTW_MUTEX::fftw_mutex_fattal_2;
boost::mutex FFTW_MUTEX::fftw_mutex_fattal_3;
boost::mutex FFTW_MUTEX::fftw_mutex_fattal_4;
boost::mutex FFTW_MUTEX::fftw_mutex_ferradans_1;
boost::mutex FFTW_MUTEX::fftw_mutex_ferradans_2;
boost::mutex FFTW_MUTEX::fftw_mutex_ferradans_3;
boost::mutex FFTW_MUTEX::fftw_mutex_ferradans_4;
boost::mutex FFTW_MUTEX::fftw_mutex_ferradans_5;
boost::mutex FFTW_MUTEX::fftw_mutex_ferradans_6;
boost::mutex FFTW_MUTEX::fftw_mutex_ferradans_7;
boost::mutex FFTW_MUTEX::fftw_mutex_durand_1;
boost::mutex FFTW_MUTEX::fftw_mutex_durand_2;

void init_fftw() {
    static bool is_init_threads = false;
    // activate parallel execution of fft routines
    FFTW_MUTEX::fftw_mutex_global.lock();
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
