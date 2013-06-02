/**
 * This file is a part of Luminance HDR package
 * ---------------------------------------------------------------------- 
 * Copyright (C) 2006,2007 Giuseppe Rota
 * Copyright (C) 2010-2012 Franco Comida
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
 *
 * Manual and auto antighosting, improvements, bugfixing
 * @author Franco Comida <fcomida@users.sourceforge.net>
 *
 */

#include <QDebug>
#include <QApplication>
#include <QFileInfo>
#include <QFile>
#include <QColor>
#include <QtConcurrentMap>
#include <QtConcurrentFilter>
#include <QFutureWatcher>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <vector>
#include <boost/make_shared.hpp>
#include <boost/bind.hpp>
#include <boost/foreach.hpp>

#include <Libpfs/frame.h>
#include <Libpfs/utils/msec_timer.h>
#include <Libpfs/manip/shift.h>
#include <Libpfs/manip/cut.h>
#include <Libpfs/manip/copy.h>
#include <Libpfs/io/tiffwriter.h>
#include <Libpfs/io/tiffreader.h>

#include <Libpfs/io/framereader.h>
#include <Libpfs/io/framereaderfactory.h>
#include <Libpfs/io/framewriter.h>
#include <Libpfs/io/framewriterfactory.h>
#include <Libpfs/utils/transform.h>
#include <Libpfs/colorspace/convert.h>

#include "Fileformat/pfsouthdrimage.h"

#include "Exif/ExifOperations.h"
#include "HdrWizard/HdrInputLoader.h"
#include "HdrCreation/mtb_alignment.h"
#include "HdrCreationManager.h"
#include "arch/math.h"

#include <HdrCreation/fusionoperator.h>

static const float max_rgb = 1.0f;
static const float max_lightness = 1.0f;
static const int gridSize = 40;

using namespace std;
using namespace pfs;
using namespace pfs::io;

const config_triple predef_confs[6]= {
    {TRIANGULAR, LINEAR,DEBEVEC, "", ""},
    {TRIANGULAR, GAMMA, DEBEVEC, "", ""},
    {PLATEAU, LINEAR, DEBEVEC, "", ""},
    {PLATEAU, GAMMA, DEBEVEC, "", ""},
    {GAUSSIAN, LINEAR, DEBEVEC, "", ""},
    {GAUSSIAN, GAMMA, DEBEVEC, "", ""},
};

// --- LEGACY CODE ---
namespace
{
inline
void rgb2hsl(float r, float g, float b, float& h, float& s, float& l)
{
    float v, m, vm, r2, g2, b2;
    h = 0.0f;
    s = 0.0f;
    l = 0.0f;
    v = std::max(r, g);
    v = std::max(v, b);
    m = std::min(r, g);
    m = std::min(m, b);
    l = (m + v) / 2.0f;
    if (l <= 0.0f)
        return;
    vm = v - m;
    s = vm;
    //if (s >= 0.0f)
    if (s > 0.0f)
        s /= (l <= 0.5f) ? (v + m) : (2.0f - v - m);
    else return;
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

inline
void hsl2rgb(float h, float sl, float l, float& r, float& g, float& b)
{
    float v;
    r = l;
    g = l;
    b = l;
    v = (l <= 0.5f) ? (l * (1.0f + sl)) : (l + sl - l * sl);
    if (v > 0.0f)
    {
        float m;
        float sv;
        int sextant;
        float fract, vsf, mid1, mid2;
        m = l + l - v;
        sv = (v - m ) / v;
        h *= 6.0f;
        sextant = (int)h;
        fract = h - sextant;
        vsf = v * sv * fract;
        mid1 = m + vsf;
        mid2 = v - vsf;
        switch (sextant)
        {
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

// r,g,b values are from 0 to 1
// h = [0,360], s = [0,1], v = [0,1]
//		if s == 0, then h = -1 (undefined)

inline
void rgb2hsv( float r, float g, float b, float &h, float &s, float &v )
{
	float min, max, delta;

	//min = MIN( r, g, b );
	//max = MAX( r, g, b );
    max = std::max(r, g);
    max = std::max(max, b);
    min = std::min(r, g);
    min = std::min(min, b);
	v = max;				// v

	delta = max - min;

	if( max != 0 )
		s = delta / max;		// s
	else {
		// r = g = b = 0		// s = 0, v is undefined
		s = 0;
		h = -1;
		return;
	}

	if( r == max )
		h = ( g - b ) / delta;		// between yellow & magenta
	else if( g == max )
		h = 2 + ( b - r ) / delta;	// between cyan & yellow
	else
		h = 4 + ( r - g ) / delta;	// between magenta & cyan

	h *= 60;				// degrees
	if( h < 0 )
		h += 360;

}

inline
void hsv2rgb( float &r, float &g, float &b, float h, float s, float v )
{
	int i;
	float f, p, q, t;

	if( s == 0 ) {
		// achromatic (grey)
		r = g = b = v;
		return;
	}

	h /= 60;			// sector 0 to 5
	i = floor( h );
	f = h - i;			// factorial part of h
	p = v * ( 1 - s );
	q = v * ( 1 - s * f );
	t = v * ( 1 - s * ( 1 - f ) );

	switch( i ) {
		case 0:
			r = v;
			g = t;
			b = p;
			break;
		case 1:
			r = q;
			g = v;
			b = p;
			break;
		case 2:
			r = p;
			g = v;
			b = t;
			break;
		case 3:
			r = p;
			g = q;
			b = v;
			break;
		case 4:
			r = t;
			g = p;
			b = v;
			break;
		default:		// case 5:
			r = v;
			g = p;
			b = q;
			break;
	}

}

int findIndex(float *data, int size)
{
    float max = *std::max_element(data, data + size);
    int i;
    for (i = 0; i < size; i++)
        if (data[i] == max) 
            return i;

    return i;
}

float hueMean(float *hues, int size)
{
    float H = 0.0f;
    for (int k = 0; k < size; k++)
        H += hues[k];

    return H / size;
}

float hueSquaredMean(HdrCreationItemContainer& data, int k)
{
    int width = data[0].frame()->getWidth();
    int height = data[0].frame()->getHeight();
    int size = data.size();
    float hues[size];
    float r, g, b, h, s, l;
    float H, HS = 0.0f;
    Channel *X, *Y, *Z, *Xk, *Yk, *Zk;
    data[k].frame()->getXYZChannels( Xk, Yk, Zk );
    Array2Df& Rk = *Xk;
    Array2Df& Gk = *Yk;
    Array2Df& Bk = *Zk;

    for (int j = 0; j < height; j++) {
        for (int i = 0; i < width; i++) {
            for (int w = 0; w < size; w++) {
                data[w].frame()->getXYZChannels( X, Y, Z );
                Array2Df& R = *X;
                Array2Df& G = *Y;
                Array2Df& B = *Z;
                r = R(i, j);
                g = G(i, j);
                b = B(i, j);
                rgb2hsl(r, g, b, h, s, l);
                hues[w] = h;
            }
            r = Rk(i, j);
            g = Gk(i, j);
            b = Bk(i, j);
            rgb2hsl(r, g, b, h, s, l);
            H = hueMean(hues, size) - h;
            HS += H*H;
        }
    }
    return HS / (width*height);
}

qreal averageLightness(const Array2Df& R, const Array2Df& G, const Array2Df& B)
{
    int width = R.getCols();
    int height = R.getRows();
    qreal avgLum = 0.0f;

    float h, s, l;
    for (int i = 0; i < height*width; i++)
    {
        rgb2hsv(R(i), G(i), B(i), h, s, l);
        avgLum += l;
    }
    return avgLum / (width * height);
}
qreal averageLightness(const HdrCreationItem& item)
{
    int width = item.frame()->getWidth();
    int height = item.frame()->getHeight();

    Channel* C_R, *C_G, *C_B;
    item.frame()->getXYZChannels(C_R, C_G, C_B);

    float *R, *G, *B;
    R = C_R->data();
    G = C_G->data();
    B = C_B->data();

    qreal avgLum = 0.0f;

    float h, s, l;
    for (int i = 0; i < height*width; i++)
    {
        rgb2hsv(R[i], G[i], B[i], h, s, l);
        avgLum += l;
    }
    return avgLum / (width * height);
}

bool comparePatches(const HdrCreationItem& item1,
                    const HdrCreationItem& item2,
                    int i, int j, int gridX, int gridY, float threshold, float deltaEV)
{
    vector<float> logRed(gridX*gridY);
    vector<float> logGreen(gridX*gridY);
    vector<float> logBlue(gridX*gridY);

    Channel *X1, *Y1, *Z1, *X2, *Y2, *Z2;
    item1.frame()->getXYZChannels( X1, Y1, Z1 );
    item2.frame()->getXYZChannels( X2, Y2, Z2 );
    Array2Df& R1 = *X1;
    Array2Df& G1 = *Y1;
    Array2Df& B1 = *Z1;
    Array2Df& R2 = *X2;
    Array2Df& G2 = *Y2;
    Array2Df& B2 = *Z2;
    
    int count = 0;
    for (int y = j * gridY; y < (j+1) * gridY; y++) {
        for (int x = i * gridX; x < (i+1) * gridX; x++) {
            if (deltaEV < 0) {
                logRed[count] = logf(R1(x, y)) - logf(R2(x, y)) - deltaEV;
                logGreen[count] = logf(G1(x, y)) - logf(G2(x, y)) - deltaEV;
                logBlue[count++] = logf(B1(x, y)) - logf(B2(x, y)) - deltaEV;
            }
            else {
                logRed[count] = logf(R2(x, y)) - logf(R1(x, y)) + deltaEV;
                logGreen[count] = logf(G2(x, y)) - logf(G1(x, y)) + deltaEV;
                logBlue[count++] = logf(B2(x, y)) - logf(B1(x, y)) + deltaEV;
            }
        }
    }
  
    float threshold1 = 0.7f * std::abs(deltaEV);
    count = 0;
    for (int h = 0; h < gridX*gridY; h++) {
        if (std::abs(logRed[h]) > threshold1 || std::abs(logGreen[h]) > threshold1 || std::abs(logBlue[h]) > threshold1)
            count++;
    }

    if ((static_cast<float>(count) / static_cast<float>(gridX*gridY)) > threshold)
        return true;
    else
        return false;

}

void copyPatch(const pfs::Array2Df& R1, const pfs::Array2Df& G1, const pfs::Array2Df& B1,
               pfs::Array2Df& R2, pfs::Array2Df& G2, pfs::Array2Df& B2,
               int i, int j, int gridX, int gridY, float sf)
{
    float h, s, l, r, g, b;
    float avgL = 0.0f;    
    for (int y = j * gridY; y < (j+1) * gridY; y++) {
        for (int x = i * gridX; x < (i+1) * gridX; x++) {
            rgb2hsl(R1(x, y), G1(x, y), B1(x, y), h, s, l);
            avgL += l;
        }
    }
    avgL /= (gridX*gridY);
    if ( avgL >= max_lightness || avgL <= 0.0f ) return;

    for (int y = j * gridY; y < (j+1) * gridY; y++) {
        for (int x = i * gridX; x < (i+1) * gridX; x++) {
            rgb2hsv(R1(x, y), G1(x, y), B1(x, y), h, s, l);
            l *= sf;
            if (l > max_lightness) l = max_lightness;
            hsv2rgb(h, s, l, r, g, b);

            if (r > max_rgb) r = max_rgb;
            if (g > max_rgb) g = max_rgb;
            if (b > max_rgb) b = max_rgb;

            if (r < 0.0f) r = 0.0f;
            if (g < 0.0f) g = 0.0f;
            if (b < 0.0f) b = 0.0f;

            R2(x, y) = r;
            G2(x, y) = g;
            B2(x, y) = b;
        }
    }
}

void copyPatches(HdrCreationItemContainer& data, 
                 bool patches[gridSize][gridSize],
                 int h0, float* scalefactor, int gridX, int gridY)
{
    const int size = data.size(); 
    for (int h = 0; h < size; h++) {
        if (h == h0) continue;
        for (int j = 0; j < gridSize; j++) {
            for (int i = 0; i < gridSize; i++) {
                if (patches[i][j]) {
                    Channel *X1, *Y1, *Z1, *X2, *Y2, *Z2;
                    data[h0].frame()->getXYZChannels(X1, Y1, Z1);
                    data[h].frame()->getXYZChannels(X2, Y2, Z2);
                    Array2Df& R1 = *X1;
                    Array2Df& G1 = *Y1;
                    Array2Df& B1 = *Z1;
                    Array2Df& R2 = *X2;
                    Array2Df& G2 = *Y2;
                    Array2Df& B2 = *Z2;
                    copyPatch(R1, G1, B1,
                              R2, G2, B2,
                              i, j, gridX, gridY, scalefactor[h]);
                }
            }
        }
    }
}

void blend(pfs::Array2Df& R1, pfs::Array2Df& G1, pfs::Array2Df& B1,
           const pfs::Array2Df& R2, const pfs::Array2Df& G2, const pfs::Array2Df& B2,
           const QImage& mask, const QImage& maskGoodImage)
{
#ifdef TIMER_PROFILING
    msec_timer stop_watch;
    stop_watch.start();
#endif
    int width = R1.getCols();
    int height = R1.getRows();

    float alpha = 0.0f;
    qreal sf = averageLightness(R1, G1, B1) / averageLightness(R2, G2, B2);

    float h, s, l;
    float r1, g1, b1;
    float r2, g2, b2;

    for (int j = 0; j < height; j++)
    {
        for (int i = 0; i < width; i++)
        {
            if (qAlpha(mask.pixel(i,j)) == 0 && qAlpha(maskGoodImage.pixel(i,j)) == 0) continue;
            alpha = (qAlpha(maskGoodImage.pixel(i,j)) == 0) ? static_cast<float>(qAlpha(mask.pixel(i,j))) / 255 : 
                                                              static_cast<float>(qAlpha(maskGoodImage.pixel(i,j))) / 255;

            r1 = R1(i, j);
            g1 = G1(i, j);
            b1 = B1(i, j);

            r2 = R2(i, j);
            g2 = G2(i, j);
            b2 = B2(i, j);

            rgb2hsl(r2, g2, b2, h, s, l);
            l *= sf;
            if (l > max_lightness) l = max_lightness;

            hsl2rgb(h, s, l, r2, g2, b2);

            if (r2 > max_rgb) r2 = max_rgb;
            if (g2 > max_rgb) g2 = max_rgb;
            if (b2 > max_rgb) b2 = max_rgb;

            if (r2 < 0.0f) r2 = 0.0f;
            if (g2 < 0.0f) g2 = 0.0f;
            if (b2 < 0.0f) b2 = 0.0f;

            R1(i, j) = (1.0f - alpha)*r1 + alpha*r2;
            G1(i, j) = (1.0f - alpha)*g1 + alpha*g2;
            B1(i, j) = (1.0f - alpha)*b1 + alpha*b2;
        }
    }
#ifdef TIMER_PROFILING
    stop_watch.stop_and_update();
    std::cout << "blend = " << stop_watch.get_time() << " msec" << std::endl;
#endif
}

void shiftItem(HdrCreationItem& item, int dx, int dy)
{
    Channel *red, *green, *blue;
    item.frame()->getXYZChannels( red, green, blue); 
    Array2Df& Rin = *red;
    Array2Df& Gin = *green;
    Array2Df& Bin = *blue;
    Array2Df *Rout = shift( Rin, dx, dy);
    Array2Df *Gout = shift( Gin, dx, dy);
    Array2Df *Bout = shift( Bin, dx, dy);
    red->swap(*Rout);
    green->swap(*Gout);
    blue->swap(*Bout);
    item.qimage()->swap(*shiftQImage(item.qimage(), dx, dy));
}

} // anonymous namespace

// --- NEW CODE ---
HdrCreationItem::HdrCreationItem(const QString &filename)
    : m_filename(filename)
    , m_averageLuminance(-1.f)
    , m_frame(boost::make_shared<pfs::Frame>())
{
     // qDebug() << QString("Building HdrCreationItem for %1").arg(m_filename);
}

HdrCreationItem::~HdrCreationItem()
{
    // qDebug() << QString("Destroying HdrCreationItem for %1").arg(m_filename);
}

struct ConvertToQRgb {
    void operator()(float r, float g, float b, QRgb& rgb) const {
        uint8_t r8u = colorspace::convertSample<uint8_t>(r);
        uint8_t g8u = colorspace::convertSample<uint8_t>(g);
        uint8_t b8u = colorspace::convertSample<uint8_t>(b);

        rgb = qRgb(r8u, g8u, b8u);
    }
};

struct LoadFile {
    void operator()(HdrCreationItem& currentItem)
    {
        QFileInfo qfi(currentItem.filename());
        qDebug() << QString("Loading data for %1").arg(currentItem.filename());

        // read pfs::Frame
        try
        {
            FrameReaderPtr reader = FrameReaderFactory::open(
                        QFile::encodeName(qfi.filePath()).constData() );
            reader->read( *currentItem.frame(), Params() );

            // read Average Luminance
            currentItem.setAverageLuminance(
                        ExifOperations::getAverageLuminance(
                            QFile::encodeName(qfi.filePath()).constData() )
                        );

            qDebug() << QString("HdrCreationItem: Average Luminance for %1 is %2")
                        .arg(currentItem.filename())
                        .arg(currentItem.getAverageLuminance());

            // build QImage
            QImage* tempImage = new QImage(currentItem.frame()->getWidth(),
                             currentItem.frame()->getHeight(),
                             QImage::Format_ARGB32_Premultiplied);

            QRgb* qimageData = reinterpret_cast<QRgb*>(tempImage->bits());

            Channel* red;
            Channel* green;
            Channel* blue;
            currentItem.frame()->getXYZChannels(red, green, blue);

            utils::transform(red->begin(), red->end(), green->begin(), blue->begin(),
                             qimageData, ConvertToQRgb());

            currentItem.setThumbnail( tempImage );
        }
        catch (std::runtime_error& err)
        {
            qDebug() << QString("Cannot load %1: %2")
                        .arg(currentItem.filename())
                        .arg(QString::fromStdString(err.what()));
        }
    }
};

struct SaveFile {
    void operator()(HdrCreationItem& currentItem)
    {
        QString inputFilename = currentItem.filename();
        QFileInfo qfi(inputFilename);
        QString base = qfi.completeBaseName(); 
        QString filename = base + ".tif";
        QString tempdir = LuminanceOptions().getTempDir();
        qDebug() << QString("Saving data for %1 on %2").arg(filename).arg(tempdir);
        

        QString completeFilename = tempdir + "/" + filename;

        // save pfs::Frame as tiff 16bits
        try
        {
            Params p;
            p.set("tiff_mode", 1); // 16bits
            FrameWriterPtr writer = FrameWriterFactory::open(
                        QFile::encodeName(completeFilename).constData());
            writer->write( *currentItem.frame(), p );

        }
        catch (std::runtime_error& err)
        {
            qDebug() << QString("Cannot save %1: %2")
                        .arg(currentItem.filename())
                        .arg(QString::fromStdString(err.what()));
        }
    }
};

static
bool checkFileName(const HdrCreationItem& item, const QString& str) {
    return (item.filename().compare(str) == 0);
}

void HdrCreationManager::loadFiles(const QStringList &filenames)
{
    std::vector<HdrCreationItem> tempItems;
#ifndef LHDR_CXX11_ENABLED
    for (const QString& i, filenames) {
#else
    BOOST_FOREACH(const QString& i, filenames) {
#endif
        qDebug() << QString("Checking %1").arg(i);
        HdrCreationItemContainer::iterator it = find_if(m_data.begin(), m_data.end(),
                                                        boost::bind(&checkFileName, _1, i));
        // has the file been inserted already?
        if ( it == m_data.end() ) {
            qDebug() << QString("Schedule loading for %1").arg(i);
            tempItems.push_back( HdrCreationItem(i) );
        }
    }

    // parallel load of the data...
    // Create a QFutureWatcher and connect signals and slots.
    QFutureWatcher<void> futureWatcher;
    connect(&futureWatcher, SIGNAL(started()), this, SIGNAL(progressStarted()), Qt::DirectConnection);
    connect(&futureWatcher, SIGNAL(finished()), this, SIGNAL(progressFinished()), Qt::DirectConnection);
    connect(this, SIGNAL(progressCancel()), &futureWatcher, SLOT(cancel()), Qt::DirectConnection);
    connect(&futureWatcher, SIGNAL(progressRangeChanged(int,int)), this, SIGNAL(progressRangeChanged(int,int)), Qt::DirectConnection);
    connect(&futureWatcher, SIGNAL(progressValueChanged(int)), this, SIGNAL(progressValueChanged(int)), Qt::DirectConnection);

    // Start the computation.
    futureWatcher.setFuture( QtConcurrent::map(tempItems.begin(), tempItems.end(), LoadFile()) );
    futureWatcher.waitForFinished();

    if (futureWatcher.isCanceled()) return;

    qDebug() << "Data loaded ... move to internal structure!";
    BOOST_FOREACH(const HdrCreationItem& i, tempItems) {
        if ( i.isValid() ) {
            qDebug() << QString("Insert data for %1").arg(i.filename());
            m_data.push_back(i);
            if (!fromCommandLine) {
                QImage *img = new QImage(i.frame()->getWidth(), i.frame()->getHeight(), QImage::Format_ARGB32);
                img->fill(qRgba(0,0,0,0));
                antiGhostingMasksList.append(img);
            }
        }
    }
    qDebug() << QString("Read %1 out of %2").arg(tempItems.size()).arg(filenames.size());

    if (!framesHaveSameSize()) {
        emit errorWhileLoading(tr("The images have different size."));
        m_data.clear();
    }
    else
        emit finishedLoadingFiles();
}

QStringList HdrCreationManager::getFilesWithoutExif() const
{
    QStringList invalidFiles;
    foreach (const HdrCreationItem& fileData, m_data) {
        if ( !fileData.hasAverageLuminance() ) {
            invalidFiles.push_back( fileData.filename() );
        }
    }
    return invalidFiles;
}

size_t HdrCreationManager::numFilesWithoutExif() const {
    size_t counter = 0;
    foreach (const HdrCreationItem& fileData, m_data) {
        if ( !fileData.hasAverageLuminance() ) {
            ++counter;
        }
    }
    return counter;
}

void HdrCreationManager::removeFile(int idx)
{
    Q_ASSERT(idx >= 0);
    Q_ASSERT(idx < m_data.size());

    m_data.erase(m_data.begin() + idx);
}

HdrCreationManager::HdrCreationManager(bool fromCommandLine)
    : chosen_config( predef_confs[0] )
    , ais( NULL )
    , m_ais_crop_flag(false)
    , fromCommandLine( fromCommandLine )
{}

void HdrCreationManager::setConfig(const config_triple &c)
{
    chosen_config = c;
}

const QVector<float> HdrCreationManager::getExpotimes() const
{
    QVector<float> expotimes;
    for ( HdrCreationItemContainer::const_iterator it = m_data.begin(), 
          itEnd = m_data.end(); it != itEnd; ++it) {
        expotimes.push_back(it->getEV());
    }
    return expotimes;
}

bool HdrCreationManager::framesHaveSameSize()
{
    size_t width = m_data[0].frame()->getWidth();
    size_t height = m_data[0].frame()->getHeight();
    for ( HdrCreationItemContainer::const_iterator it = m_data.begin() + 1, 
          itEnd = m_data.end(); it != itEnd; ++it) {
        if (it->frame()->getWidth() != width || it->frame()->getHeight() != height)
            return false; 
    }
    return true;
}

void HdrCreationManager::align_with_mtb()
{
    //mtb_alignment(ldrImagesList);
    emit finishedAligning(0);
}

void HdrCreationManager::set_ais_crop_flag(bool flag)
{
    m_ais_crop_flag = flag;
}

void HdrCreationManager::align_with_ais()
{
    ais = new QProcess(this);
    if (ais == NULL) exit(1);       //TODO: exit gracefully
    if (!fromCommandLine) {
        ais->setWorkingDirectory(m_luminance_options.getTempDir());
    }
    QStringList env = QProcess::systemEnvironment();
#ifdef WIN32
    QString separator(";");
#else
    QString separator(":");
#endif
    env.replaceInStrings(QRegExp("^PATH=(.*)", Qt::CaseInsensitive), "PATH=\\1"+separator+QCoreApplication::applicationDirPath());
    ais->setEnvironment(env);
    connect(ais, SIGNAL(finished(int,QProcess::ExitStatus)), this, SLOT(ais_finished(int,QProcess::ExitStatus)));
    connect(ais, SIGNAL(error(QProcess::ProcessError)), this, SIGNAL(ais_failed(QProcess::ProcessError)));
    connect(ais, SIGNAL(error(QProcess::ProcessError)), this, SLOT(ais_failed_slot(QProcess::ProcessError)));
    connect(ais, SIGNAL(readyRead()), this, SLOT(readData()));
    
    QStringList ais_parameters = m_luminance_options.getAlignImageStackOptions();

    if (m_ais_crop_flag) { ais_parameters << "-C"; }

    QFutureWatcher<void> futureWatcher;

    // Start the computation.
    futureWatcher.setFuture( QtConcurrent::map(m_data.begin(), m_data.end(), SaveFile()) );
    futureWatcher.waitForFinished();

    if (futureWatcher.isCanceled()) return;

    for ( HdrCreationItemContainer::const_iterator it = m_data.begin(), 
          itEnd = m_data.end(); it != itEnd; ++it) {
        QFileInfo qfi(it->filename());
        QString base = qfi.completeBaseName(); 
        QString filename = base + ".tif";
        QString tempdir = m_luminance_options.getTempDir();
        QString completeFilename = tempdir + "/" + filename;
        ais_parameters << completeFilename; 
    }
    qDebug() << "ais_parameters " << ais_parameters;
#ifdef Q_WS_MAC
    qDebug() << QCoreApplication::applicationDirPath()+"/align_image_stack";
    ais->start(QCoreApplication::applicationDirPath()+"/align_image_stack", ais_parameters );
#else
    ais->start("align_image_stack", ais_parameters );
#endif
    qDebug() << "ais started";
}

void HdrCreationManager::ais_finished(int exitcode, QProcess::ExitStatus exitstatus)
{
    if (exitstatus != QProcess::NormalExit)
    {
        qDebug() << "ais failed";
        //emit ais_failed(QProcess::Crashed);
        return;
    }
    if (exitcode == 0)
    {
        // TODO: try-catch
        // DAVIDE _ HDR CREATION
        std::vector<HdrCreationItem> tempItems;
        int i = 0;
        for ( HdrCreationItemContainer::iterator it = m_data.begin(), 
              itEnd = m_data.end(); it != itEnd; ++it) {
            QString inputFilename = it->filename(), filename;
            if (!fromCommandLine)
                filename = QString(m_luminance_options.getTempDir() + "/aligned_" + QString("%1").arg(i,4,10,QChar('0'))+".tif");
            else
                filename = QString("aligned_" + QString("%1").arg(i,4,10,QChar('0'))+".tif");
            
            tempItems.push_back( HdrCreationItem(filename) );
            ExifOperations::copyExifData(inputFilename.toStdString(), filename.toStdString(), false, "", false, true); 
            i++;
        }

        // parallel load of the data...
        // Create a QFutureWatcher and connect signals and slots.
        QFutureWatcher<void> futureWatcher;

        // Start the computation.
        futureWatcher.setFuture( QtConcurrent::map(tempItems.begin(), tempItems.end(), LoadFile()) );
        futureWatcher.waitForFinished();

        if (futureWatcher.isCanceled()) return;
        
        int width = m_data[0].frame()->getWidth();
        int height = m_data[0].frame()->getHeight();
        
        for ( HdrCreationItemContainer::iterator it = m_data.begin(), 
              itEnd = m_data.end(); it != itEnd; ++it) {
            if (!fromCommandLine) {
                QImage *img = new QImage(width, height, QImage::Format_ARGB32);
                img->fill(qRgba(0,0,0,0));
                antiGhostingMasksList.append(img);
            }
            QFileInfo qfi(it->filename());
            QString base = qfi.completeBaseName(); 
            QString filename = base + ".tif";
            QString tempdir = m_luminance_options.getTempDir();
            QString completeFilename = tempdir + "/" + filename;
            QFile::remove(QFile::encodeName(completeFilename).constData());
            qDebug() << "void HdrCreationManager::ais_finished: remove " << filename;
        }

        m_data.clear();
        m_data = tempItems;
        QFile::remove(m_luminance_options.getTempDir() + "/hugin_debug_optim_results.txt");
        emit finishedAligning(exitcode);
    }
    else
    {
        qDebug() << "align_image_stack exited with exit code " << exitcode;
        emit finishedAligning(exitcode);
    }
}

void HdrCreationManager::ais_failed_slot(QProcess::ProcessError error)
{
    qDebug() << "align_image_stack failed";
}

void HdrCreationManager::removeTempFiles()
{
    int i = 0;
    for ( HdrCreationItemContainer::iterator it = m_data.begin(), 
          itEnd = m_data.end(); it != itEnd; ++it) {
        QString filename;
        if (!fromCommandLine) {
            filename = QString(m_luminance_options.getTempDir() + "/aligned_" + QString("%1").arg(i,4,10,QChar('0'))+".tif");
        }
        else {
            filename = QString("aligned_" + QString("%1").arg(i,4,10,QChar('0'))+".tif");
        }
        QFile::remove(filename);
        qDebug() << "void HdrCreationManager::ais_finished: remove " << filename;
        ++i;
    }
}

/*
void HdrCreationManager::removeTempFiles()
{
    foreach (QString tempfname, filesToRemove) {
        qDebug() << "void HdrCreationManager::removeTempFiles(): "
                 << qPrintable(tempfname);
        if (!tempfname.isEmpty()) {
            QFile::remove(tempfname);
        }
    }
    filesToRemove.clear();
}

void HdrCreationManager::checkEVvalues()
{
    float max=-20, min=+20;
    for (int i = 0; i < fileList.size(); i++) {
        float ev_val = log2f(expotimes[i]);
        if (ev_val > max)
            max = ev_val;
        if (ev_val < min)
            min = ev_val;
    }
    //now if values are out of bounds, add an offset to them.
    if (max > 10) {
        for (int i = 0; i < fileList.size(); i++) {
            float new_ev = log2f(expotimes[i]) - (max - 10);
            expotimes[i] = exp2f(new_ev);
            emit expotimeValueChanged(exp2f(new_ev), i);
        }
    } else if (min < -10) {
        for (int i = 0; i < fileList.size(); i++) {
            float new_ev = log2f(expotimes[i]) - (min + 10);
            expotimes[i] = exp2f(new_ev);
            emit expotimeValueChanged(exp2f(new_ev), i);
        }
    }
    //qDebug("HCM::END checkEVvalues");
}

void HdrCreationManager::setEV(float new_ev, int image_idx)
{
    if (expotimes[image_idx] == -1) {
        //remove always the first one
        //after the initial printing we only need to know the size of the list
        filesLackingExif.removeAt(0);
    }
    expotimes[image_idx] = exp2f(new_ev);
    emit expotimeValueChanged(exp2f(new_ev), image_idx);
}
*/

using namespace libhdr::fusion;

pfs::Frame* HdrCreationManager::createHdr(bool ag, int iterations)
{
    FusionOperatorPtr fusionOperator = IFusionOperator::build(DEBEVEC_NEW);

    std::vector< FrameEnhanced > frames;
    for ( size_t idx = 0; idx < m_data.size(); ++idx ) {
        frames.push_back(
                    FrameEnhanced(m_data[idx].frame(),
                                  m_data[idx].getAverageLuminance())
                    );
    }

    return fusionOperator->computeFusion( frames );


//    //CREATE THE HDR
//    if (inputType == LDR_INPUT_TYPE)
//        return createHDR(expotimes.data(), &chosen_config, ag, iterations, true, &ldrImagesList );
//    else
//        return createHDR(expotimes.data(), &chosen_config, ag, iterations, false, &listmdrR, &listmdrG, &listmdrB );
}

void HdrCreationManager::applyShiftsToItems(const QList<QPair<int,int> >& hvOffsets)
{
    int size = m_data.size();
    //shift the frames and images
    for (int i = 0; i < size; i++)
    {
        if ( hvOffsets[i].first == hvOffsets[i].second &&
             hvOffsets[i].first == 0 )
        {
            continue;
        }
        shiftItem(m_data[i],
                  hvOffsets[i].first,
                  hvOffsets[i].second);
    }
}

void HdrCreationManager::cropItems(const QRect& ca)
{
    //crop all frames and images
    int size = m_data.size();
    for (int idx = 0; idx < size; idx++) {
        QImage *newimage = new QImage(m_data[idx].qimage()->copy(ca));
        if (newimage == NULL)
            exit(1); // TODO: exit gracefully
        m_data[idx].qimage()->swap(*newimage);
        int x_ul, y_ur, x_bl, y_br;
        ca.getCoords(&x_ul, &y_ur, &x_bl, &y_br);
        Frame* cropped = cut(m_data[idx].frame().get(), static_cast<size_t>(x_ul), static_cast<size_t>(y_ur), 
                                                        static_cast<size_t>(x_bl), static_cast<size_t>(y_br));
        FramePtr shared(cropped);
        m_data[idx].frame().swap(shared);
    
    }
    cropAgMasks(ca);
}

HdrCreationManager::~HdrCreationManager()
{
    if (ais != NULL && ais->state() != QProcess::NotRunning) {
        ais->kill();
    }
    //qDeleteAll(antiGhostingMasksList);
}

/*
void HdrCreationManager::clearlists(bool deleteExpotimeAsWell)
{
    startedProcessing.clear();
    filesLackingExif.clear();

    if (deleteExpotimeAsWell)
    {
        fileList.clear();
        expotimes.clear();
    }
    if (ldrImagesList.size() != 0)
    {
        qDeleteAll(ldrImagesList);
        ldrImagesList.clear();
        qDeleteAll(antiGhostingMasksList);
        antiGhostingMasksList.clear();
    }
    if (listmdrR.size()!=0 && listmdrG.size()!=0 && listmdrB.size()!=0)
    {
        Array2DfList::iterator itR=listmdrR.begin(), itG=listmdrG.begin(), itB=listmdrB.begin();
        for (; itR!=listmdrR.end(); itR++,itG++,itB++ )
        {
            delete *itR;
            delete *itG;
            delete *itB;
        }
        listmdrR.clear();
        listmdrG.clear();
        listmdrB.clear();
        qDeleteAll(mdrImagesList);
        mdrImagesList.clear();
        qDeleteAll(mdrImagesToRemove);
        mdrImagesToRemove.clear();
        qDeleteAll(antiGhostingMasksList);
        antiGhostingMasksList.clear();
    }
}

void HdrCreationManager::makeSureLDRsHaveAlpha()
{
    if (ldrImagesList.at(0)->format()==QImage::Format_RGB32) {
        int origlistsize = ldrImagesList.size();
        for (int image_idx = 0; image_idx < origlistsize; image_idx++) {
            QImage *newimage = new QImage(ldrImagesList.at(0)->convertToFormat(QImage::Format_ARGB32));
            if (newimage == NULL)
                exit(1); // TODO: exit gracefully;
            ldrImagesList.append(newimage);
            delete ldrImagesList.takeAt(0);
        }
    }
}

void HdrCreationManager::applyShiftsToImageStack(const QList<QPair<int,int> >& hvOffsets)
{
    int originalsize = ldrImagesList.count();
    //shift the images
    for (int i = 0; i < originalsize; i++)
    {
        if ( hvOffsets[i].first == hvOffsets[i].second &&
             hvOffsets[i].first == 0 )
        {
            continue;
        }
        QImage *shifted = shiftQImage(ldrImagesList[i],
                                      hvOffsets[i].first,
                                      hvOffsets[i].second);
        delete ldrImagesList.takeAt(i);
        ldrImagesList.insert(i, shifted);
    }
}

void HdrCreationManager::applyShiftsToMdrImageStack(const QList<QPair<int,int> >& hvOffsets)
{
    qDebug() << "HdrCreationManager::applyShiftsToMdrImageStack";
    int originalsize = mdrImagesList.count();
    for (int i = 0; i < originalsize; i++)
    {
        if ( hvOffsets[i].first == hvOffsets[i].second &&
             hvOffsets[i].first == 0 )
        {
            continue;
        }
        pfs::Array2Df *shiftedR = shift(*listmdrR[i],
                                        hvOffsets[i].first,
                                        hvOffsets[i].second);
        pfs::Array2Df *shiftedG = shift(*listmdrG[i],
                                        hvOffsets[i].first,
                                        hvOffsets[i].second);
        pfs::Array2Df *shiftedB = shift(*listmdrB[i],
                                        hvOffsets[i].first,
                                        hvOffsets[i].second);
        delete listmdrR[i];
        delete listmdrG[i];
        delete listmdrB[i];
        listmdrR[i] = shiftedR;
        listmdrG[i] = shiftedG;
        listmdrB[i] = shiftedB;
    }
}


void HdrCreationManager::cropLDR(const QRect& ca)
{
    //crop all the images
    int origlistsize = ldrImagesList.size();
    for (int image_idx = 0; image_idx < origlistsize; image_idx++) {
        QImage *newimage = new QImage(ldrImagesList.at(0)->copy(ca));
        if (newimage == NULL)
            exit(1); // TODO: exit gracefully
        ldrImagesList.append(newimage);
        delete ldrImagesList.takeAt(0);
    }
    cropAgMasks(ca);
}

void HdrCreationManager::cropMDR(const QRect& ca)
{
    int x_ul, y_ul, x_br, y_br;
    ca.getCoords(&x_ul, &y_ul, &x_br, &y_br);

    int newWidth = x_br-x_ul;
    int newHeight = y_br-y_ul;

    
    // crop all the images
    //int origlistsize = listmdrR.size();
    //pfs::Frame *frame;
    //pfs::Channel *Xc, *Yc, *Zc;
    //pfs::Frame *cropped_frame;
    

    // all R channels
    for ( size_t idx = 0; idx < listmdrR.size(); ++idx )
    {
        pfs::Array2Df tmp( newWidth, newHeight );
        pfs::cut(listmdrR[idx], &tmp, x_ul, y_ul, x_br, y_br);
        listmdrR[idx]->swap( tmp );
    }

    // all G channels
    for ( size_t idx = 0; idx < listmdrG.size(); ++idx )
    {
        pfs::Array2Df tmp( newWidth, newHeight );
        pfs::cut(listmdrG[idx], &tmp, x_ul, y_ul, x_br, y_br);
        listmdrG[idx]->swap( tmp );
    }

    // all B channel
    for ( size_t idx = 0; idx < listmdrB.size(); ++idx )
    {
        pfs::Array2Df tmp( newWidth, newHeight );
        pfs::cut(listmdrB[idx], &tmp, x_ul, y_ul, x_br, y_br);
        listmdrB[idx]->swap( tmp );
    }

    for ( int idx = 0; idx < mdrImagesList.size(); ++idx )
    {

        QImage *newimage = new QImage(mdrImagesList.at(0)->copy(ca));
        if (newimage == NULL)
            exit(1); // TODO: exit gracefully
        mdrImagesList.append(newimage);
        mdrImagesToRemove.append(mdrImagesList.takeAt(0));
        QImage *img = new QImage(newWidth, newHeight, QImage::Format_ARGB32);
        img->fill(qRgba(0,0,0,0));
        antiGhostingMasksList.append(img);
        antiGhostingMasksList.takeAt(0);
    }
    m_mdrWidth = newWidth;
    m_mdrHeight = newHeight;
    cropAgMasks(ca);
}

void HdrCreationManager::reset()
{
    ais = NULL;
    m_shift = 0;
    chosen_config = predef_confs[0];
    inputType = UNKNOWN_INPUT_TYPE;
    fileList.clear();
    // DAVIDE _ HDR CREATION
    // clearlists(true);
    removeTempFiles();
}
*/

/*
void HdrCreationManager::remove(int index)
{
    switch (inputType) {
    case LDR_INPUT_TYPE:
    {
        ldrImagesList.removeAt(index);          
    }
        break;
    case MDR_INPUT_TYPE:
    {
            Array2DfList::iterator itR = listmdrR.begin() + index;
            delete *itR;
            listmdrR.erase(itR);

            Array2DfList::iterator itG = listmdrG.begin() + index;
            delete *itG;
            listmdrG.erase(itG);

            Array2DfList::iterator itB = listmdrB.begin() + index;
            delete *itB;
            listmdrB.erase(itB);
            
            delete mdrImagesList[index];            
            mdrImagesList.removeAt(index);          
            
            QString fname = filesToRemove.at(index);
            qDebug() << "void HdrCreationManager::remove(int index): filename " << fname;
            QFile::remove(fname);
    }
        break;
        // ...in this case, do nothing!
    case UNKNOWN_INPUT_TYPE:
    default:{}
        break;
    }
    fileList.removeAt(index);
    filesToRemove.remove(index);
    expotimes.remove(index);
    startedProcessing.removeAt(index);
}
*/

void HdrCreationManager::readData()
{
    QByteArray data = ais->readAll();
    emit aisDataReady(data);
}

namespace {

inline float toFloat(int value) {
    return (static_cast<float>(value)/255.f);
}

void interleavedToPlanar(const QImage* image,
                         pfs::Array2Df* r, pfs::Array2Df* g, pfs::Array2Df* b)
{
    for (int row = 0; row < image->height(); ++row)
    {
        const QRgb* data = reinterpret_cast<const QRgb*>(image->scanLine(row));
        pfs::Array2Df::iterator itRed = r->row_begin(row);
        pfs::Array2Df::iterator itGreen = g->row_begin(row);
        pfs::Array2Df::iterator itBlue = g->row_begin(row);

        for ( int col = 0; col < image->width(); ++col )
        {
            *itRed++ = toFloat(qRed(*data));
            *itGreen++ = toFloat(qGreen(*data));
            *itBlue++ = toFloat(qBlue(*data));

            ++data;
        }
    }
}
} // anonymous namespace

void HdrCreationManager::saveImages(const QString& prefix)
{
    int idx = 0;
    for ( HdrCreationItemContainer::const_iterator it = m_data.begin(), 
          itEnd = m_data.end(); it != itEnd; ++it) {

        QString filename = prefix + QString("_%1").arg(idx) + ".tiff";
        pfs::io::TiffWriter writer(QFile::encodeName(filename).constData());
        writer.write( *it->frame(), pfs::Params("tiff_mode", 1) );

        QFileInfo qfi(filename);
        QString absoluteFileName = qfi.absoluteFilePath();
        QByteArray encodedName = QFile::encodeName(absoluteFileName);
        ExifOperations::copyExifData(QFile::encodeName(it->filename()).constData(), encodedName.constData(), false);
        ++idx;
    }
    emit imagesSaved();
}
/*
void HdrCreationManager::saveLDRs(const QString& filename)
{
#ifdef QT_DEBUG
    qDebug() << "HdrCreationManager::saveLDRs";
#endif

    for (int idx = 0, origlistsize = ldrImagesList.size(); idx < origlistsize;
         ++idx)
    {
        QImage* currentImage = ldrImagesList[idx];

        QString fname = filename + QString("_%1").arg(idx) + ".tiff";

        pfs::Frame frame(currentImage->width(), currentImage->height());
        pfs::Channel* R;
        pfs::Channel* G;
        pfs::Channel* B;
        frame.createXYZChannels(R, G, B);
        interleavedToPlanar(currentImage, R, G, B);

        pfs::io::TiffWriter writer(QFile::encodeName(fname).constData());
        writer.write( frame, pfs::Params("tiff_mode", 1) );

        // DAVIDE_TIFF
        // TiffWriter writer(QFile::encodeName(fname).constData(), ldrImagesList[idx]);
        // writer.write8bitTiff();

        QFileInfo qfi(filename);
        QString absoluteFileName = qfi.absoluteFilePath();
        QByteArray encodedName = QFile::encodeName(absoluteFileName + QString("_%1").arg(idx) + ".tiff");
        ExifOperations::copyExifData(QFile::encodeName(fileList[idx]).constData(), encodedName.constData(), false);
    }
    emit imagesSaved();
}

void HdrCreationManager::saveMDRs(const QString& filename)
{
#ifdef QT_DEBUG
    qDebug() << "HdrCreationManager::saveMDRs";
#endif

    int origlistsize = listmdrR.size();
    for (int idx = 0; idx < origlistsize; idx++)
    {
        QString fname = filename + QString("_%1").arg(idx) + ".tiff";

//        pfs::Frame *frame = pfs::DOMIO::createFrame( m_mdrWidth, m_mdrHeight );
//        pfs::Channel *Xc, *Yc, *Zc;
//        frame->createXYZChannels( Xc, Yc, Zc );
//        Xc->setChannelData(listmdrR[idx]);
//        Yc->setChannelData(listmdrG[idx]);
//        Zc->setChannelData(listmdrB[idx]);

//        TiffWriter writer(, frame);
//        writer.writePFSFrame16bitTiff();

        pfs::Frame frame( m_mdrWidth, m_mdrHeight );
        pfs::Channel* R;
        pfs::Channel* G;
        pfs::Channel* B;
        frame.createXYZChannels(R, G, B);

        pfs::copy(listmdrR[idx], R);
        pfs::copy(listmdrG[idx], G);
        pfs::copy(listmdrB[idx], B);

        pfs::io::TiffWriter writer( QFile::encodeName(fname).constData() );
        // tiff_mode = 2 (16 bit tiff)
        // min_luminance = 0
        // max_luminance = 2^16 - 1
        // note: this is due to the fact the reader do read the native
        // data into float, without doing any conversion into the [0, 1] range
        // (definitely something to think about when we touch the readers)
        writer.write(frame,
                     pfs::Params("tiff_mode", 2)
                        ("min_luminance", (float)0)
                        ("max_luminance", (float)65535) );

        QFileInfo qfi(filename);
        QString absoluteFileName = qfi.absoluteFilePath();
        QByteArray encodedName = QFile::encodeName(
                    absoluteFileName + QString("_%1").arg(idx) + ".tiff");
        ExifOperations::copyExifData(
                    QFile::encodeName(fileList[idx]).constData(),
                    encodedName.constData(), false);
    }
    emit imagesSaved();
}
*/
void HdrCreationManager::doAntiGhosting(int goodImageIndex)
{
    Channel *red_goodImage, *green_goodImage, *blue_goodImage;
    m_data[goodImageIndex].frame()->getXYZChannels( red_goodImage, green_goodImage, blue_goodImage);
    Array2Df& R_goodImage = *red_goodImage;
    Array2Df& G_goodImage = *green_goodImage;
    Array2Df& B_goodImage = *blue_goodImage;

    int size = m_data.size();
    for (int idx = 0; idx < size; idx++) {
        if (idx == goodImageIndex) continue;
        Channel *red, *green, *blue;
        m_data[idx].frame()->getXYZChannels( red, green, blue);
        Array2Df& R = *red;
        Array2Df& G = *green;
        Array2Df& B = *blue;
        blend( R, G, B, 
               R_goodImage, G_goodImage, B_goodImage,
               *antiGhostingMasksList[idx],
               *antiGhostingMasksList[goodImageIndex] );
    }
}

void HdrCreationManager::cropAgMasks(const QRect& ca) {
    int origlistsize = antiGhostingMasksList.size();
    for (int image_idx = 0; image_idx < origlistsize; image_idx++) {
        QImage *newimage = new QImage(antiGhostingMasksList.at(0)->copy(ca));
        if (newimage == NULL)
            exit(1); // TODO: exit gracefully
        antiGhostingMasksList.append(newimage);
        delete antiGhostingMasksList.takeAt(0);
    }
}

void HdrCreationManager::doAutoAntiGhostingMDR(float threshold)
{
    const int size = m_data.size(); 
    assert(size >= 2);
    float HE[size];
    const int width = m_data[0].frame()->getWidth();
    const int height = m_data[0].frame()->getHeight();
    const int gridX = width / gridSize;
    const int gridY = height / gridSize;

    float avgLightness[size];
    bool patches[gridSize][gridSize];
    
    for (int i = 0; i < gridSize; i++)
        for (int j = 0; j < gridSize; j++)
            patches[i][j] = false;

    for (int i = 0; i < size; i++) {
        avgLightness[i] = averageLightness(m_data[i]); 
        qDebug() << "avgLightness[" << i << "] = " << avgLightness[i];
    }

    for (int i = 0; i < size; i++) { 
        HE[i] = hueSquaredMean(m_data, i);
        qDebug() << "HE[" << i << "]: " << HE[i];
    }

    int h0 = findIndex(HE, size);
    qDebug() << "h0: " << h0;

    float scaleFactor[size];

    for (int i = 0; i < size; i++) {
        scaleFactor[i] = avgLightness[i] / avgLightness[h0];        
    }

    for (int h = 0; h < size; h++) {
        if (h == h0) 
            continue;
        for (int j = 0; j < gridSize; j++) {
            for (int i = 0; i < gridSize; i++) {
                    float deltaEV = m_data[h0].getAverageLuminance() - m_data[h].getAverageLuminance();
                    if (comparePatches(m_data[h0],
                                       m_data[h],
                                       i, j, gridX, gridY, threshold, deltaEV)) {
                        patches[i][j] = true;
                    }
            }                      
        }
    }

    int count = 0;
    for (int i = 0; i < gridSize; i++)
        for (int j = 0; j < gridSize; j++)
            if (patches[i][j] == true)
                count++;
    qDebug() << "Copied patches: " << static_cast<float>(count) / static_cast<float>(gridSize*gridSize) * 100.0f << "%";
    copyPatches(m_data, patches, h0, scaleFactor, gridX, gridY);
}

void HdrCreationManager::doAutoAntiGhosting(float threshold)
{
    qDebug() << "HdrCreationManager::doAutoAntiGhosting";
#ifdef TIMER_PROFILING
    msec_timer stop_watch;
    stop_watch.start();
#endif

//    (inputType == LDR_INPUT_TYPE) ? doAutoAntiGhostingLDR(threshold) : doAutoAntiGhostingMDR(threshold);
    doAutoAntiGhostingMDR(threshold);

#ifdef TIMER_PROFILING
    stop_watch.stop_and_update();
    std::cout << "doAutoAntiGhosting = " << stop_watch.get_time() << " msec" << std::endl;
#endif
}
