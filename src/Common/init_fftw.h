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
* @author Davide Anastasia <francocomida@gmail.com>
*/

#ifndef INIT_FFTW_H
#define INIT_FFTW_H

#include <boost/thread/mutex.hpp>

class FFTW_MUTEX {
   public:
    static boost::mutex fftw_mutex_global;
    static boost::mutex fftw_mutex_plan;
    static boost::mutex fftw_mutex_destroy_plan;
    static boost::mutex fftw_mutex_alloc;
    static boost::mutex fftw_mutex_free;
};

void init_fftw();

#endif
