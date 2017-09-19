/**
 * This file is a part of LuminanceHDR package.
 * ----------------------------------------------------------------------
 * Copyright (C) 2007 Giuseppe Rota
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
#ifndef EXIFOPS_H
#define EXIFOPS_H

#include <string>

namespace ExifOperations {
//!
//!
void copyExifData(const std::string &from, const std::string &to,
                  bool dont_overwrite,
                  const std::string &comment = std::string(),
                  bool destIsLDR = false, bool keepRotation = true);

//!
//!
// float obtain_avg_lum(const std::string& filename);

// //! \brief retrieve exposure bias from \c filename
// float getExposureBias(const std::string& filename);

//! \brief compute average luminance from exposure bias
//! \return Average Luminance (as 2^EV) or -1 if the value cannot be calculated
float getAverageLuminance(const std::string &filename);

float getExposureTime(const std::string &filename);
}

#endif
