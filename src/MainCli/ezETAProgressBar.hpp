/*
This file is part of ezProgressBar.

ezProgressBar is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

ezProgressBar is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with ezProgressBar.  If not, see <http://www.gnu.org/licenses/>.

Copyright (C) 2011 Remik Ziemlinski
*/
/*
CHANGELOG

v0.0.0 20110502 rsz Created.
V1.0.0 20110522 rsz Extended to show eta with growing bar.
v2.0.0 20110525 rsz Added time elapsed.
v2.0.1 20111006 rsz Added default constructor value.
*/

#ifndef EZ_ETAPROGRESSBAR_H
#define EZ_ETAPROGRESSBAR_H

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <iostream>
#include <string>

#ifdef WIN32
#define _WINSOCKAPI_  // stops windows.h including winsock.h
#include <windows.h>
#else
#include <sys/ioctl.h>
#include <sys/time.h>
#endif

namespace ez {
// One-line refreshing progress bar inspired by wget that shows ETA (time
// remaining).
// 90% [##################################################     ] ETA 12d 23h 56s
class ezETAProgressBar {
   public:
    ezETAProgressBar(unsigned int p_n = 0)
        : n(p_n), pct(0), cur(0), width(80), startTime(0), endTime(0) {}
    void reset() {
        pct = 0;
        cur = 0;
    }
    void start() {
#ifdef WIN32
        assert(QueryPerformanceFrequency(&g_llFrequency) != 0);
#endif
        startTime = osQueryPerfomance();
        setPct(0);
    }

    void operator++() {
        if (cur >= n) return;
        ++cur;

        setPct(((float)cur) / n);
    };

    // http://stackoverflow.com/questions/3283804/c-get-milliseconds-since-some-date
    long long osQueryPerfomance() {
#ifdef WIN32
        LARGE_INTEGER llPerf = {0};
        QueryPerformanceCounter(&llPerf);
        return llPerf.QuadPart * 1000ll / (g_llFrequency.QuadPart / 1000ll);
#else
        struct timeval stTimeVal;
        gettimeofday(&stTimeVal, NULL);
        return stTimeVal.tv_sec * 1000000ll + stTimeVal.tv_usec;
#endif
    }

    std::string secondsToString(long long t) {
        int days = t / 86400;
        long long sec = t - days * 86400;
        int hours = sec / 3600;
        sec -= hours * 3600;
        int mins = sec / 60;
        sec -= mins * 60;
        char tmp[8];
        std::string out;

        if (days) {
            sprintf(tmp, "%dd ", days);
            out += tmp;
        }

        if (hours >= 1) {
            sprintf(tmp, "%dh ", hours);
            out += tmp;
        }

        if (mins >= 1) {
            sprintf(tmp, "%dm ", mins);
            out += tmp;
        }

        if (sec >= 1) {
            sprintf(tmp, "%ds", (int)sec);
            out += tmp;
        }

        if (out.empty()) out = "0s";

        return out;
    }

    // Set 0.0-1.0, where 1.0 equals 100%.
    void setPct(float Pct) {
        endTime = osQueryPerfomance();
        char pctstr[5];
        sprintf(pctstr, "%3d%%", (int)(100 * Pct));
        // Compute how many tics we can display.
        int nticsMax = (width - 27);
        int ntics = (int)(nticsMax * Pct);
        std::string out(pctstr);
        out.append(" [");
        out.append(ntics, '#');
        out.append(nticsMax - ntics, ' ');
        out.append("] ");
        out.append((Pct < 1.0) ? "ETA " : "in ");
        // Seconds.
        long long dt = (long long)((endTime - startTime) / 1000000.0);
        std::string tstr;
        if (Pct >= 1.0) {
            // Print overall time and newline.
            tstr = secondsToString(dt);
            out.append(tstr);
            if (out.size() < width) out.append(width - out.size(), ' ');

            out.append("\n");
            std::cout << out;
            return;
        } else {
            float eta = 999999.;
            if (Pct > 0.0) eta = dt * (1.0 - Pct) / Pct;

            if (eta > 604800.0)
                out.append("> 1 week");
            else {
                tstr = secondsToString((long long)eta);
                out.append(tstr);
            }
        }

        // Pad end with spaces to overwrite previous string that may have been
        // longer.
        if (out.size() < width) out.append(width - out.size(), ' ');

        out.append("\r");
        std::cout << out;
        std::cout.flush();
    }

    unsigned int n;
    unsigned short pct;  // Stored as 0-1000, so 2.5% is encoded as 25.
    unsigned int cur;
    unsigned char width;  // How many chars the entire line can be.
    long long startTime, endTime;
#ifdef WIN32
    LARGE_INTEGER g_llFrequency;
#endif
};
}
#endif  // EZ_ETAPROGRESSBAR_H
