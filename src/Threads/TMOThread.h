/*
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
 * Original Work
 * @author Giuseppe Rota <grota@users.sourceforge.net>
 * Improvements, bugfixing 
 * @author Franco Comida <fcomida@users.sourceforge.net>
 *
 */

#ifndef TMOTHREAD_H
#define TMOTHREAD_H

#include <QThread>
#include <QImage>

#include "Core/TonemappingOptions.h"
#include "Common/options.h"
#include "Common/global.h"
#include "Common/ProgressHelper.h"
#include "Libpfs/pfs.h"
#include "Libpfs/frame.h"
#include "Libpfs/colorspace.h"

enum TMOTHREAD_MODE { TMO_INTERACTIVE, TMO_BATCH };

class TMOThread : public QThread {
Q_OBJECT

public:
  TMOThread(pfs::Frame *frame, const TonemappingOptions *opts);
  virtual ~TMOThread();
  virtual void startTonemapping();

  void set_mode(TMOTHREAD_MODE mode) { m_tmo_thread_mode = mode; }
  void set_batch_mode() { m_tmo_thread_mode = TMO_BATCH; }
  
public slots:
	virtual void terminateRequested();
  
signals:
	void imageComputed(QImage*);
        void imageComputed(QImage*, const TonemappingOptions* opts);
	void processedFrame(pfs::Frame *);
	void setMaximumSteps(int);
	void setValue(int);
	void setValue();
	void finished();
	void deleteMe(TMOThread *);
	void tmo_error(const char *);
  
protected:
	virtual void run() = 0;
	void finalize();
	pfs::Frame *workingframe;
        const TonemappingOptions *opts;
  TMOTHREAD_MODE m_tmo_thread_mode;
	ProgressHelper *ph;
  
  // Different output color spaces can be selected by a specific operator
  // in order to make its output compliant to the one defined by qtpfsgui 1.9.3
  // In the future, this value can change accordingly to the TM operator's changes
  pfs::ColorSpace out_CS;  
};

#endif
