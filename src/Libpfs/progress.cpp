/*
 * This file is a part of LuminanceHDR package.
 * ----------------------------------------------------------------------
 * Copyright (C) 2013 Davide Anastasia
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
 */

//! @author Davide Anastasia <davideanastasia@users.sourceforge.net>

#include "progress.h"

namespace pfs {

Progress::Progress()
    : m_maximum(0), m_minimum(0), m_value(0), m_canceled(false) {}

void Progress::setMaximum(int maximum) { m_maximum = maximum; }

void Progress::setMinimum(int minimum) { m_minimum = minimum; }

void Progress::setRange(int minimum, int maximum) {
    setMinimum(minimum);
    setMaximum(maximum);
}

int Progress::maximum() const { return m_maximum; }
int Progress::minimum() const { return m_minimum; }

void Progress::setValue(int value) { m_value = value; }

// int Progress::next();

int Progress::value() const { return m_value; }

void Progress::cancel(bool b) { m_canceled = b; }
bool Progress::canceled() const { return m_canceled; }
}
