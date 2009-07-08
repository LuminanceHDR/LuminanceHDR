/*
For general Scribus (>=1.3.2) copyright and licensing information please refer
to the COPYING file provided with the program. Following this notice may exist
a copyright and/or license notice that predates the release of Scribus 1.3.2
for which a new license (GPL+exception) is in place.
*/

#include <QApplication>
#include <QCursor>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QProcess>
#include <QString>
#include <QStringList>
#include <QToolTip>
#include <QUrl>

//#if defined(_WIN32)
//#include <windows.h>
//#include <shellapi.h>
//#endif

// #include "prefsmanager.h"
#include "sctextbrowser.h"
// #include "urllauncher.h"

ScTextBrowser::ScTextBrowser( QWidget * parent )
	: QWebView(parent)
{
}

void ScTextBrowser::home()
{
	if(m_home.isValid())
		load(m_home);
}
