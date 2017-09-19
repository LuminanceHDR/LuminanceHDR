/*
 * This file is a part of LuminanceHDR package.
 * ----------------------------------------------------------------------
 * Copyright (C) 2006,2007 Giuseppe Rota
 * Copyright (C) 2011 Davide Anastasia
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
 * Original Work
 * @author Giuseppe Rota <grota@users.sourceforge.net>
 * Improvements, bugfixing
 * @author Franco Comida <fcomida@users.sourceforge.net>
 * Refactory of TMThread.h class to TonemapOperator in order to remove
 * dependency from QObject and QThread
 * @author Davide Anastasia <davideanastasia@users.sourceforge.net>
 *
 */

#ifndef TONEMAPOPERATOR_H
#define TONEMAPOPERATOR_H

#include <stdexcept>

#include "Core/TonemappingOptions.h"

// Forward declaration
namespace pfs {
class Progress;
class Frame;
}

class TonemapOperator {
   public:
    static TonemapOperator *getTonemapOperator(const TMOperator tmo);
    virtual ~TonemapOperator();

    //!
    //! \return return the underlying type of the TonemapOperator
    //!
    virtual TMOperator getType() const = 0;

    //!
    //! Get a Frame in RGB and processes it.
    //! \note input frame is MODIFIED
    //! If you want to keep the original frame, make a copy before
    //!
    virtual void tonemapFrame(pfs::Frame &, TonemappingOptions *,
                              pfs::Progress &ph) = 0;

   protected:
    TonemapOperator();
};

#endif  // TONEMAPOPERATOR_H
