/* fmpath.h */
/* Get all useful paths in one place with the hope that */
/* finally we will not need resource.qrc anymore */
#ifndef FMPATHS_H
#define FMPATHS_H

#include <QString>
#include <QMap>
#include <QLocale>
#include <QDir>


class FMPaths
{
		QMap<QString,QString> FMPathsDB;
		FMPaths() {}
		static FMPaths *instance;
		static FMPaths *getThis();
		
	public:


//		static QString TranslationsDir();
		
//		static QString ResourcesDir();
		
		static QString HelpDir();
		
//		static QString ScriptsDir();

//		static QString SamplesDir();
		
		static QString LocalizedDirPath(const QString& base, const QString& fallback = QString("en"));
		
//		static QString LocalizedFilePath(const QString& base, const QString& ext, const QString& fallback = QString("en"));
};
#endif
