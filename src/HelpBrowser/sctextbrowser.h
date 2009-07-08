/*
For general Scribus (>=1.3.2) copyright and licensing information please refer
to the COPYING file provided with the program. Following this notice may exist
a copyright and/or license notice that predates the release of Scribus 1.3.2
for which a new license (GPL+exception) is in place.
*/
#ifndef SCTEXTBROWSER_H
#define SCTEXTBROWSER_H

// #include <QTextBrowser>
#include <QUrl>
#include  <QWebView>

class ScTextBrowser : public QWebView
{
	Q_OBJECT
	QUrl m_home;
	public:
		ScTextBrowser( QWidget * parent = 0 );
		void setHome(const QUrl& h){m_home = h;}
		
	signals:
		void overLink(const QString &link);
		
	public slots:
		void home();

};

#endif
