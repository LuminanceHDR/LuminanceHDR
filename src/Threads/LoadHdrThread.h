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

#ifndef IOTHREAD_H
#define IOTHREAD_H

#include <QThread>
#include "Common/options.h"
#include "Libpfs/pfs.h"

class LoadHdrThread : public QThread {
Q_OBJECT

public:
	LoadHdrThread(QString filename, QString RecentDirHDRSetting);
  LoadHdrThread(QString filename);
	virtual ~LoadHdrThread();
	QString getHdrFileName() { return fname; }
signals:
	void updateRecentDirHDRSetting(QString);
	void hdr_ready(pfs::Frame*, QString fname);
	void load_failed(QString error_message);
	void maximumValue(int value);
	void nextstep(int step);
protected:
	void run();
private:
	QString fname;
  QString RecentDirHDRSetting;
	LuminanceOptions *luminance_options;
};
#endif
