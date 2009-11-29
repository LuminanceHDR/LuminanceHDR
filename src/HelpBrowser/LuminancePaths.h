/* Get all useful paths in one place with the hope that */
/* finally we will not need resource.qrc anymore */
#ifndef LUMINANCEPATHS_H
#define LUMINANCEPATHS_H

#include <QString>
#include <QMap>
#include <QLocale>
#include <QDir>

class LuminancePaths
{
		QMap<QString,QString> LuminancePathsDB;
		LuminancePaths() {}
		static LuminancePaths *instance;
		static LuminancePaths *getThis();
		
	public:
		static QString HelpDir();
		static QString LocalizedDirPath(const QString& base, const QString& fallback = QString("en"));
};
#endif
