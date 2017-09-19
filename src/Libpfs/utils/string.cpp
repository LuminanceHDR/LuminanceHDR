/*
 * This file is a part of Luminance HDR package.
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
 */

#include <Libpfs/utils/string.h>

#include <algorithm>
#include <cmath>

using namespace std;

namespace pfs {
namespace utils {

// Utility functions for case insensitive string compare
static int charDiff(char c1, char c2) {
    if (tolower(c1) < tolower(c2)) return -1;
    if (tolower(c1) == tolower(c2)) return 0;
    return 1;
}

static int stringCompare(const string &str1, const string &str2) {
    int diff = 0;
    size_t size = std::min(str1.size(), str2.size());
    for (size_t idx = 0; idx < size && diff == 0; ++idx) {
        diff = charDiff(str1[idx], str2[idx]);
    }
    if (diff != 0) return diff;

    if (str2.length() == str1.length()) return 0;
    if (str2.length() > str1.length()) return 1;
    return -1;
}

bool StringUnsensitiveComp::operator()(const std::string &str1,
                                       const std::string &str2) const {
    return (stringCompare(str1, str2) == -1);
}

string getFormat(const string &filename) {
    size_t i = filename.rfind('.', filename.length());
    if (i != string::npos) {
        return filename.substr(i + 1, filename.length() - i);
    }
    return string();
}

}  // utils
}  // pfs
