//
// C++ Implementation: fmpaths
//
// Description: 
//
//
// Author: Pierre Marchand <pierremarc@oep-h.com>, (C) 2008
//
// Copyright: See COPYING file that comes with this distribution
//
// Adapted to Luminance HDR
//

#include "LuminancePaths.h"
#include "help-path.hxx"

#include <QApplication>
#include <iostream>

LuminancePaths *LuminancePaths::instance = 0;
LuminancePaths * LuminancePaths::getThis()
{
	if(!instance)
		instance = new LuminancePaths;
	return instance;
}

namespace {
const QString dirsep(QDir::separator());
}

QString LuminancePaths::HelpDir()
{
    if (getThis()->LuminancePathsDB.contains("HelpDir")) {
        QString hf = getThis()->LuminancePathsDB["HelpDir"];
        if ( !hf.isEmpty() && QDir(hf).exists() ) {
            return hf;
        }
    }

#ifdef Q_WS_MAC
    QString hf = LocalizedDirPath(QApplication::applicationDirPath() + dirsep + "../Resources/help/en" + dirsep);
#elif _WIN32
    QString hf = LocalizedDirPath(QApplication::applicationDirPath() + dirsep + "help" + dirsep);
    // no fall-back
#else   // UNIX
    // hf = LocalizedDirPath( PREFIX + dirsep + "share" + dirsep + "fontmatrix" + dirsep + "help" + dirsep );
    // hf = LocalizedDirPath("usr" + dirsep + "share" + dirsep + "luminance-hdr" + dirsep + "help" + dirsep);
    QString hf = LocalizedDirPath(HELPDIR + dirsep);
#endif

	getThis()->LuminancePathsDB["HelpDir"] = hf;

    return hf;
}

QString LuminancePaths::LocalizedDirPath(const QString & base, const QString& fallback )
{	
	QString sep("_");
	QStringList l_c(QLocale::system().name().split(sep));
	QString langcode( l_c.first() );
	QString countrycode(l_c.last());

	QStringList names;
	if((!langcode.isEmpty()) || (!countrycode.isEmpty()))
	{
		names << base + langcode + sep + countrycode ;
		names << base + langcode  ;
	}
	names << base + fallback  ;
	names << base  ;
	
	foreach(QString t, names)
	{
		QDir d(t);
		if( d.exists() )
			return d.absolutePath() + QString(QDir::separator()) ;
	}
	
	return QString();
}
