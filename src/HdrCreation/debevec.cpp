/**
 * This file is a part of LuminanceHDR package.
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

#include "debevec.h"
#include "generic_applyResponse.h"

inline float debevec_sum(float weight, float /*average_weight*/, float t, float response)
{
	return weight * response/t;
}

inline float debevec_div(float weight, float /*average_weight*/, float /*t*/, float)
{
	return weight;
}

inline float debevec_out(float quotient)
{
	return quotient;
}

void debevec_applyResponse(pfs::Array2D* Rout, pfs::Array2D* Gout, pfs::Array2D* Bout, const float * arrayofexptime, const float* Ir,  const float* Ig, const float* Ib, const float* w, const int M, const bool ldrinput, ...)
{
	QList<QImage*> *listldr = NULL;
	Array2DList* listhdrR = NULL;
	Array2DList* listhdrG = NULL;
	Array2DList* listhdrB = NULL;
	va_list arg_pointer;
	va_start(arg_pointer,ldrinput); /* Initialize the argument list. */

	if (ldrinput)
  {
		listldr=va_arg(arg_pointer, QList<QImage*>*);
	}
  else
  {
		listhdrR=va_arg(arg_pointer, Array2DList*);
		listhdrG=va_arg(arg_pointer, Array2DList*);
		listhdrB=va_arg(arg_pointer, Array2DList*);
	}
	va_end(arg_pointer); /* Clean up. */

	generic_applyResponse(&debevec_sum, &debevec_div, &debevec_out, 
	    Rout, Gout, Bout, arrayofexptime, Ir,  Ig, Ib, w,  M, ldrinput,
	    listldr, listhdrR, listhdrG, listhdrB);
}
