/**
 * This file is a part of LuminanceHDR package.
 * ----------------------------------------------------------------------
 * Copyright (C) 2013 Franco Comida
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
 * @author Franco Comida <fcomida@users.sourceforge.net>
 */

#ifndef COMMONFUNCTIONS_H
#define COMMONFUNCTIONS_H

#include <QImage>
#include <QString>

#include <HdrCreation/fusionoperator.h>
#include <HdrWizard/HdrCreationItem.h>
#include <Libpfs/utils/minmax.h>

void computeAutolevels(const QImage *data, const float threshold, float &minL,
                       float &maxL, float &gammaL);

inline void rgb2hsl(float r, float g, float b, float &h, float &s, float &l) {
    float v, m, vm, r2, g2, b2;
    h = 0.0f;
    s = 0.0f;
    l = 0.0f;

    pfs::utils::minmax(r, g, b, m, v);

    l = (m + v) / 2.0f;
    if (l <= 0.0f) return;
    vm = v - m;
    s = vm;
    // if (s >= 0.0f)
    if (s > 0.0f) {
        float div = (l <= 0.5f) ? (v + m) : (2.0f - v - m);

        //assert(div != 0.f);

        if(fabs(div) < 1.e-15)
            div = 1.e-15;

        s /= div;
    } else {
        return;
    }
    r2 = (v - r) / vm;
    g2 = (v - g) / vm;
    b2 = (v - b) / vm;
    if (r == v)
        h = (g == m ? 5.0f + b2 : 1.0f - g2);
    else if (g == v)
        h = (b == m ? 1.0f + r2 : 3.0f - b2);
    else
        h = (r == m ? 3.0f + g2 : 5.0f - r2);
    h /= 6.0f;
}

inline void hsl2rgb(float h, float sl, float l, float &r, float &g, float &b) {
    float v;
    r = l;
    g = l;
    b = l;
    v = (l <= 0.5f) ? (l * (1.0f + sl)) : (l + sl - l * sl);
    if (v > 0.0f) {
        float m;
        float sv;
        int sextant;
        float fract, vsf, mid1, mid2;
        m = l + l - v;
        sv = (v - m) / v;
        h *= 6.0f;
        sextant = (int)h;
        fract = h - sextant;
        vsf = v * sv * fract;
        mid1 = m + vsf;
        mid2 = v - vsf;
        switch (sextant) {
            case 0:
                r = v;
                g = mid1;
                b = m;
                break;
            case 1:
                r = mid2;
                g = v;
                b = m;
                break;
            case 2:
                r = m;
                g = v;
                b = mid1;
                break;
            case 3:
                r = m;
                g = mid2;
                b = v;
                break;
            case 4:
                r = mid1;
                g = m;
                b = v;
                break;
            case 5:
                r = v;
                g = m;
                b = mid2;
                break;
        }
    }
}

struct ConvertToQRgb {
    float gamma;
    explicit ConvertToQRgb(float gamma = 1.0f);
    void operator()(float r, float g, float b, QRgb &rgb) const;
};

struct LoadFile {
    explicit LoadFile(bool fromFITS = false) : m_datamax(0.f), m_datamin(0.f) {
        m_fromFITS = fromFITS;
    }
    void operator()(HdrCreationItem &currentItem);
    float normalize(float);
    float m_datamax;
    float m_datamin;
    bool m_fromFITS;
};

struct SaveFile {
    int m_mode;
    float m_minLum;
    float m_maxLum;
    bool m_deflateCompression;
    SaveFile(int mode, float minLum = 0.0f, float maxLum = 1.0f,
             bool deflateCompression = true);
    void operator()(HdrCreationItem &currentItem);
};

struct RefreshPreview {
    void operator()(HdrCreationItem &currentItem);
};

QString getQString(libhdr::fusion::FusionOperator fo);
QString getQString(libhdr::fusion::WeightFunctionType wf);
QString getQString(libhdr::fusion::ResponseCurveType rf);
#endif
