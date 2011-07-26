/**
 * This file is a part of Luminance HDR package.
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

#ifndef LOADHDRINPUT_H
#define LOADHDRINPUT_H

#include <QThread>
#include <QImage>
#include <QDebug>

#ifdef __APPLE__
#include <libraw.h>
#else
#include <libraw/libraw.h>
#endif

#include "Common/options.h"
#include "Libpfs/frame.h"

int prog_callback(void *data,enum LibRaw_progress p,int iteration, int expected);

class HdrInputLoader : public QThread {
    Q_OBJECT

public:
    HdrInputLoader(QString filename, int image_idx);
    ~HdrInputLoader();
signals:
    void ldrReady(QImage *ldrImage, int index, float expotime, QString new_fname, bool ldrtiff);
    void mdrReady(pfs::Frame *mdrImage, int index, float expotime, QString new_fname);
    void thumbReady(QImage *thumb);
    void loadFailed(QString errormessage, int index);
	void maximumValue(int);
	void nextstep(int);
protected:
    void run();
private:
	friend int prog_callback(void *data,enum LibRaw_progress p,int iteration, int expected);
	void emitNextStep(int iteration);
	void emitMaximumValue(int iteration);
    int image_idx;
    QString fname;
    LuminanceOptions *luminance_options;
};
#endif
