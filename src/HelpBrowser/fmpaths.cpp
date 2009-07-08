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
//
#include "fmpaths.h"

#include <QApplication>
// #include <QDebug>

FMPaths *FMPaths::instance = 0;
FMPaths * FMPaths::getThis()
{
	if(!instance)
		instance = new FMPaths;
	return instance;
}


/*
QString FMPaths::TranslationsDir()
{
	if(getThis()->FMPathsDB.contains("TranslationsDir"))
		return getThis()->FMPathsDB["TranslationsDir"];
	QString dirsep(QDir::separator());
#ifdef PLATFORM_APPLE
	QString QMDirPath = QApplication::applicationDirPath();
	QMDirPath +=  dirsep + ".." + dirsep + "Resources" + dirsep + "Locales" + dirsep;	
#elif _WIN32
	QString QMDirPath = QApplication::applicationDirPath();
	QMDirPath +=  dirsep + "share" + dirsep + "qm" + dirsep;
#else
	QString QMDirPath = PREFIX;
	QMDirPath +=  dirsep + "share" + dirsep + "fontmatrix" + dirsep + "qm" + dirsep;
#endif
	getThis()->FMPathsDB["TranslationsDir"] = QMDirPath;
	return QMDirPath;
}
*/

QString FMPaths::HelpDir()
{
	if(getThis()->FMPathsDB.contains("HelpDir"))
		return getThis()->FMPathsDB["HelpDir"];
	QString hf;
	QString dirsep(QDir::separator());
//#ifdef PLATFORM_APPLE
//	hf = LocalizedDirPath( QApplication::applicationDirPath() + dirsep + "help" + dirsep );
//#elif _WIN32
//	hf = LocalizedDirPath(QApplication::applicationDirPath() + dirsep + "help" + dirsep );
//#else
//	hf = LocalizedDirPath( PREFIX + dirsep + "share" + dirsep + "fontmatrix" + dirsep + "help" + dirsep );
//#endif
	hf = LocalizedDirPath( "/usr" + dirsep + "share" + dirsep + "qtpfsgui" + dirsep + "help" + dirsep );
	getThis()->FMPathsDB["HelpDir"] = hf;

	//getThis()->FMPathsDB["HelpDir"] = "/usr/share/qtpfsgui/html/";
	return getThis()->FMPathsDB["HelpDir"];
}

/*
QString FMPaths::ResourcesDir()
{
	if(getThis()->FMPathsDB.contains("ResourcesDir"))
		return getThis()->FMPathsDB["ResourcesDir"];
	QString dirsep(QDir::separator());
#ifdef PLATFORM_APPLE
	QString QMDirPath = QApplication::applicationDirPath();
	QMDirPath +=  dirsep + ".." + dirsep + "Resources" + dirsep ;	
#elif _WIN32
	QString QMDirPath = QApplication::applicationDirPath();
	QMDirPath +=  dirsep + "share" + dirsep + "resources" + dirsep;
#else
	QString QMDirPath = PREFIX;
	QMDirPath +=  dirsep + "share" + dirsep + "fontmatrix" + dirsep + "resources" + dirsep;
#endif
	getThis()->FMPathsDB["ResourcesDir"] = QMDirPath;
	return getThis()->FMPathsDB["ResourcesDir"];
}

QString FMPaths::ScriptsDir()
{
	QString sep(QDir::separator());
	return typotek::getInstance()->getOwnDir().absolutePath() + sep + "Scripts"+ sep;
}

QString FMPaths::SamplesDir()
{
	QString sep(QDir::separator());
	return typotek::getInstance()->getOwnDir().absolutePath() + sep + "Samples"+ sep;
}
*/
QString FMPaths::LocalizedDirPath(const QString & base, const QString& fallback )
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
		{
			return d.absolutePath() + QString(QDir::separator()) ;
		}
	}
	
	return QString();
}
/*
QString FMPaths::LocalizedFilePath(const QString & base, const QString & ext, const QString& fallback)
{
	QString sep("_");
	QStringList l_c(QLocale::system().name().split(sep));
	QString langcode( l_c.first() );
	QString countrycode(l_c.last());

	QStringList names;
	if((!langcode.isEmpty()) || (!countrycode.isEmpty()))
	{
		names << base + langcode + sep + countrycode + ext ;
		names << base + langcode + ext ;
	}
	names << base + fallback + ext ;
	names << base + ext ;
	
	foreach(QString t, names)
	{
		if( QFile::exists(t) )
		{
			return t;
		}
	}
	
	return QString();
}
*/

