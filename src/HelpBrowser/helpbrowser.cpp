/*
For general Scribus (>=1.3.2) copyright and licensing information please refer
to the COPYING file provided with the program. Following this notice may exist
a copyright and/or license notice that predates the release of Scribus 1.3.2
for which a new license (GPL+exception) is in place.
*/
/***************************************************************************
*   Copyright (C) 2004 by Craig Bradney                                   *
*   cbradney@zip.com.au                                                   *
*   Copyright (C) 2005 by Petr Vanek                                      *
*   petr@yarpen.cz                                                        *
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
*   This program is distributed in the hope that it will be useful,       *
*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
*   GNU General Public License for more details.                          *
*                                                                         *
*   You should have received a copy of the GNU General Public License     *
*   along with this program; if not, write to the                         *
*   Free Software Foundation, Inc.,                                       *
*   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
***************************************************************************/

#include "helpbrowser.h"
#include "ui_HelpBrowser.h"
#include "ui_HelpSideBar.h"

#include <QAction>
#include <QDir>
#include <QDomDocument>
#include <QEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QHeaderView>
#include <QInputDialog>
#include <QItemSelectionModel>
#include <QList>
#include <QMessageBox>
#include <QModelIndex>
#include <QModelIndexList>
#include <QPainter>
#include <QPrinter>
#include <QPrintDialog>
#include <QProcess>
#include <QPushButton>
#include <QString>
#include <QStandardItem>
#include <QTextEdit>
#include <QTreeView>
#include <QXmlDefaultHandler>
#include <QDesktopServices>

#include "Common/global.h"
#include "HelpBrowser/schelptreemodel.h"
#include "HelpBrowser/LuminancePaths.h"
#include "Common/LuminanceOptions.h"

/*! \brief XML parsef for documantation history.
This is small helper class which reads saved bookmarks configuration
from ~/.scribus/doc/history.xml file.
The reference to historyBrowser is a reference to the dialog.
\author Petr Vanek <petr@yarpen.cz>
*/
class HistoryParser2 : public QXmlDefaultHandler
{
	public:
		HelpBrowser *helpBrowser;

		bool startDocument()
		{
			return true;
		}

		bool startElement(const QString&, const QString&, const QString& qName, const QXmlAttributes& attrs)
		{
			if (qName == "item")
			{
				struct histd2 his;
				his.title = attrs.value(0);
				his.url = attrs.value(1);
				helpBrowser->mHistory[helpBrowser->histMenu->addAction(his.title)] = his;
			}
			return true;
		}

		bool endElement(const QString&, const QString&, const QString&)
		{
			return true;
		}
};

/*! \brief XML parsef for documantation bookmarks.
This is small helper class which reads saved bookmarks configuration
from ~/.scribus/doc/bookmarks.xml file.
The reference to QListView *view is a reference to the list view with bookmarks
\author Petr Vanek <petr@yarpen.cz>
*/
class BookmarkParser2 : public QXmlDefaultHandler
{
	public:
		QTreeWidget* view;
		QMap<QString, QString>* quickHelpIndex;
		QMap<QString, QPair<QString, QString> >* bookmarkIndex;

		bool startDocument()
		{
			return true;
		}

		bool startElement(const QString&, const QString&, const QString& qName, const QXmlAttributes& attrs)
		{
			if (qName == "item")
			{
				//TODO : This will dump items if bookmarks get loaded into a different GUI language
				if (quickHelpIndex->contains(attrs.value(1)))
				{
					bookmarkIndex->insert(attrs.value(0), qMakePair(attrs.value(1), attrs.value(2)));
					view->addTopLevelItem(new QTreeWidgetItem(view, QStringList() << attrs.value(0)));
				}
			}
			return true;
		}

		bool endElement(const QString&, const QString&, const QString&)
		{
			return true;
		}
};

bool HelpBrowser::firstRun=true;

HelpBrowser::HelpBrowser(QWidget* parent):
    QMainWindow( parent ),
    m_Ui(new Ui::HelpBrowser)
{
    m_Ui->setupUi(this);
}

HelpBrowser::HelpBrowser( QWidget* parent, const QString& /*caption*/, const QString& guiLanguage, const QString& jumpToSection, const QString& jumpToFile):
    QMainWindow( parent ),
    zoomFactor(1.0),
    m_Ui(new Ui::HelpBrowser)
{
    firstRun=true;
    m_Ui->setupUi(this);

    restoreGeometry(LuminanceOptions().value("HelpBrowserGeometry").toByteArray());
	setupLocalUI();

    m_Ui->textBrowser->page()->setLinkDelegationPolicy(QWebPage::DelegateExternalLinks);
    connect(m_Ui->textBrowser, SIGNAL(linkClicked(const QUrl &)), this, SLOT(handleExternalLink(const QUrl &)));
    connect(m_Ui->textBrowser, SIGNAL(loadStarted()), this, SLOT(loadStarted()));
    connect(m_Ui->textBrowser, SIGNAL(loadFinished(bool)), this, SLOT(loadFinished(bool)));
    connect(m_Ui->textBrowser->page(), SIGNAL(linkHovered(const QString &, const QString &, const QString & )), this, SLOT(linkHovered(const QString &, const QString &, const QString & )));

	language = guiLanguage.isEmpty() ? QString("en") : guiLanguage.left(2);
	finalBaseDir = LuminancePaths::HelpDir();
    m_Ui->textBrowser->setHome( QUrl::fromLocalFile( finalBaseDir + "index.html" ));
	menuModel=NULL;
    loadMenu();

    if (menuModel!=NULL)
	{
		readBookmarks();
		readHistory();
		jumpToHelpSection(jumpToSection, jumpToFile );
		languageChange();
	}
	else
	{
		qDebug("menuModel == NULL");
		displayNoHelp();
	}
}

HelpBrowser::~HelpBrowser()
{
	firstRun=true;
        LuminanceOptions().setValue("HelpBrowserGeometry", saveGeometry());
}

void HelpBrowser::closeEvent(QCloseEvent *)
{
	delete menuModel;

	// no need to delete child widgets, Qt does it all for us
	// bookmarks
	QFile bookFile(bookmarkFile());
	if (bookFile.open(QIODevice::WriteOnly))
	{
		QTextStream stream(&bookFile);
		stream.setCodec("UTF-8");
		stream << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
		stream << "<bookmarks>\n";
        QTreeWidgetItemIterator it(helpSideBar->m_Ui->bookmarksView);
		while (*it) 
		{
			if (bookmarkIndex.contains((*it)->text(0)))
			{
				QString pagetitle(bookmarkIndex.value((*it)->text(0)).first);
				QString filename(bookmarkIndex.value((*it)->text(0)).second);
				stream << "\t<item title=\"" << (*it)->text(0) << "\" pagetitle=\"" << pagetitle << "\" url=\"" << filename << "\" />\n";
			}
			++it;
		}
		stream << "</bookmarks>\n";
		bookFile.close();
	}
	// history
  	QFile histFile(historyFile());
	if (histFile.open(QIODevice::WriteOnly))
	{
		QTextStream stream(&histFile);
		stream.setCodec("UTF-8");
		stream << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
		stream << "<history>\n";
		for (QMap<QAction*,histd2>::Iterator it = mHistory.begin() ; it != mHistory.end(); ++it)
			stream << "\t<item title=\"" << it.value().title << "\" url=\"" << it.value().url << "\" />\n";
		stream << "</history>\n";
		histFile.close();
	}
	// size
// 	prefs->set("xsize", width());
// 	prefs->set("ysize", height());

	emit closed();
}

void HelpBrowser::setupLocalUI()
{
	helpSideBar = new HelpSideBar(tr("Help SideBar"), this);
	helpSideBar->setFeatures(QDockWidget::AllDockWidgetFeatures);
	addDockWidget(Qt::LeftDockWidgetArea, helpSideBar);
// 	setWindowIcon(loadIcon("AppIcon.png"));
	//Add Menus
	fileMenu=menuBar()->addMenu("");
	editMenu=menuBar()->addMenu("");
	viewMenu=menuBar()->addMenu("");
	bookMenu=menuBar()->addMenu("");
	histMenu=new QMenu(this);

	//Add Menu items
	filePrint=fileMenu->addAction(QIcon(":/help/document-print.png"), "", this, SLOT(print()), Qt::CTRL+Qt::Key_P);
	fileMenu->addSeparator();
	fileExit=fileMenu->addAction(QIcon(":/help/exit.png"), "", this, SLOT(close()));
	editFind=editMenu->addAction(QIcon(":/help/find.png"), "", this, SLOT(find()), Qt::CTRL+Qt::Key_F);
	editFindNext=editMenu->addAction( "", this, SLOT(findNext()), Qt::Key_F3);
	editFindPrev=editMenu->addAction( "", this, SLOT(findPrevious()), Qt::SHIFT+Qt::Key_F3);
	viewContents=viewMenu->addAction( "", this, SLOT(viewContents_clicked()), Qt::CTRL+Qt::Key_C);
	viewSearch=viewMenu->addAction( "", this, SLOT(viewSearch_clicked()), Qt::CTRL+Qt::Key_S);
	viewBookmarks=viewMenu->addAction( "", this, SLOT(viewBookmarks_clicked()), Qt::CTRL+Qt::Key_B);
	bookAdd=bookMenu->addAction( "", this, SLOT(bookmarkButton_clicked()), Qt::CTRL+Qt::Key_D);
	bookDel=bookMenu->addAction( "", this, SLOT(deleteBookmarkButton_clicked()));
	bookDelAll=bookMenu->addAction( "", this, SLOT(deleteAllBookmarkButton_clicked()));

	//Add Toolbar items
    goHome=m_Ui->toolBar->addAction(QIcon(":/help/go-home.png"), "", m_Ui->textBrowser, SLOT(home()));
    goBack=m_Ui->toolBar->addAction(QIcon(":/help/go-previous.png"), "", m_Ui->textBrowser, SLOT(back()));
    goFwd=m_Ui->toolBar->addAction(QIcon(":/help/go-next.png"), "", m_Ui->textBrowser, SLOT(forward()));
    zoomIn=m_Ui->toolBar->addAction(QIcon(":/new/prefix1/images/zoom-in.png"), "", this, SLOT(zoomIn_clicked()));
    zoomOriginal=m_Ui->toolBar->addAction(QIcon(":/new/prefix1/images/zoom-original.png"), "", this, SLOT(zoomOriginal_clicked()));
    zoomOut=m_Ui->toolBar->addAction(QIcon(":/new/prefix1/images/zoom-out.png"), "", this, SLOT(zoomOut_clicked()));
	goBack->setMenu(histMenu);
	
    helpSideBar->m_Ui->listView->header()->hide();
    helpSideBar->m_Ui->searchingView->header()->hide();
    helpSideBar->m_Ui->bookmarksView->header()->hide();

	//splitter->setStretchFactor(splitter->indexOf(tabWidget), 0);
	//splitter->setStretchFactor(splitter->indexOf(textBrowser), 1);
	// reset previous size
// 	prefs = PrefsManager::instance()->prefsFile->getPluginContext("helpbrowser");
// 	int xsize = prefs->getUInt("xsize", 640);
// 	int ysize = prefs->getUInt("ysize", 480);
// 	resize(QSize(xsize, ysize).expandedTo(minimumSizeHint()) );

	//basic ui
	connect(histMenu, SIGNAL(triggered(QAction*)), this, SLOT(histChosen(QAction*)));
	// searching
    connect(helpSideBar->m_Ui->searchingEdit, SIGNAL(returnPressed()), this, SLOT(searchingButton_clicked()));
    connect(helpSideBar->m_Ui->searchingButton, SIGNAL(clicked()), this, SLOT(searchingButton_clicked()));
    connect(helpSideBar->m_Ui->searchingView, SIGNAL(itemClicked( QTreeWidgetItem *, int)), this, SLOT(itemSearchSelected(QTreeWidgetItem *, int)));
	// bookmarks
    connect(helpSideBar->m_Ui->bookmarkButton, SIGNAL(clicked()), this, SLOT(bookmarkButton_clicked()));
    connect(helpSideBar->m_Ui->deleteBookmarkButton, SIGNAL(clicked()), this, SLOT(deleteBookmarkButton_clicked()));
    connect(helpSideBar->m_Ui->deleteAllBookmarkButton, SIGNAL(clicked()), this, SLOT(deleteAllBookmarkButton_clicked()));
    connect(helpSideBar->m_Ui->bookmarksView, SIGNAL(itemClicked( QTreeWidgetItem *, int)), this, SLOT(itemBookmarkSelected(QTreeWidgetItem *, int)));
	// links hoover
    connect(m_Ui->textBrowser, SIGNAL(overLink(const QString &)), this, SLOT(showLinkContents(const QString &)));
	
	languageChange();
}

void HelpBrowser::showLinkContents(const QString &link)
{
	statusBar()->showMessage(link);
}

void HelpBrowser::changeEvent(QEvent *e)
{
	if (e->type() == QEvent::LanguageChange)
	{
		languageChange();
	}
	QWidget::changeEvent(e);
}

void HelpBrowser::languageChange()
{
	setWindowTitle( tr( "LuminanceHDR Online Help" ) );
	
	fileMenu->setTitle(tr("&File"));
	editMenu->setTitle(tr("&Edit"));
	viewMenu->setTitle(tr("&View"));
	bookMenu->setTitle(tr("&Bookmarks"));
	
	filePrint->setText(tr("&Print..."));
	fileExit->setText(tr("&Quit"));
	editFind->setText(tr("&Find..."));
	editFindNext->setText(tr("Find &Next"));
	editFindPrev->setText(tr("Find &Previous"));
	viewContents->setText(tr("&Contents"));
	viewSearch->setText(tr("&Search"));
	viewBookmarks->setText(tr("&Bookmarks"));
	bookAdd->setText(tr("&Add Bookmark"));
	bookDel->setText(tr("&Delete"));
	bookDelAll->setText(tr("D&elete All"));

    m_Ui->retranslateUi(this);
// 	if (!firstRun)
// 	{
// 		QString fname(QDir::cleanPath(textBrowser->source().toString()));
// 		QFileInfo fi(fname);
// 		QString filename(fi.fileName());
// 		if (ScCore->getGuiLanguage().isEmpty())
// 			language="en";
// 		else
// 			language=ScCore->getGuiLanguage();
// 		loadMenu();
// 		if (menuModel!=NULL)
// 			loadHelp(finalBaseDir + "/" + filename);
// 	}
// 	else
// 		firstRun=false;
}

void HelpBrowser::print()
{
	QPrinter printer;
	printer.setFullPage(true);
	QPrintDialog dialog(&printer, this);
	if (dialog.exec())
        m_Ui->textBrowser->print(&printer);
}

void HelpBrowser::searchingButton_clicked()
{
	// root files
	QApplication::changeOverrideCursor(QCursor(Qt::WaitCursor));
	searchingInDirectory(finalBaseDir);
	QApplication::changeOverrideCursor(Qt::ArrowCursor);
}

void HelpBrowser::searchingInDirectory(const QString& aDir)
{
	QDir dir(QDir::convertSeparators(aDir + "/"));
	QStringList in;
	in.append("*.html");
	QStringList lst = dir.entryList(in);
	for (QStringList::Iterator it = lst.begin(); it != lst.end(); ++it)
	{
		QString fname(aDir + "/" + (*it));
		QFile f(fname);
		if (f.open(QIODevice::ReadOnly))
		{
			QTextStream stream(&f);
			QString str = stream.readAll();
            int cnt = str.count(helpSideBar->m_Ui->searchingEdit->text(), Qt::CaseInsensitive);
			if (cnt > 0)
			{
				QString fullname = fname;
				QString toFind(fname.remove(finalBaseDir + "/"));
				QMapIterator<QString, QString> i(quickHelpIndex);
				while (i.hasNext())
				{
					i.next();
					if (i.value()==toFind)
                        helpSideBar->m_Ui->searchingView->addTopLevelItem(new QTreeWidgetItem(helpSideBar->m_Ui->searchingView, QStringList() << i.key()));
				}
			}
			f.close();
		}
	}
	// get dirs - ugly recursion
	in.clear();
	in.append("*");
	QStringList dst = dir.entryList(in, QDir::Dirs);
	for (QStringList::Iterator it = dst.begin(); it != dst.end(); ++it)
		if ((*it)!="." && (*it)!="..")
			searchingInDirectory(QDir::convertSeparators(aDir + QString((*it)) + "/"));
}

void HelpBrowser::find()
{
	findText = QInputDialog::getText( this, tr("Find"), tr("Search Term:"), QLineEdit::Normal, findText, 0);
	if (findText.isNull())
		return;
	findNext();
}

void HelpBrowser::findNext()
{
	if (findText.isNull())
	{
		find();
		return;
	}
	// find it. finally
    m_Ui->textBrowser->findText(findText, 0);
}

void HelpBrowser::findPrevious()
{
	if (findText.isNull())
	{
		find();
		return;
	}
	// find it. finally
    m_Ui->textBrowser->findText(findText);
}

void HelpBrowser::bookmarkButton_clicked()
{
    QString title = m_Ui->textBrowser->title();
    QString fname(QDir::cleanPath(m_Ui->textBrowser->url().toLocalFile()));
 	title = QInputDialog::getText(this, tr("New Bookmark"), tr("New Bookmark's Title:"), QLineEdit::Normal, title, 0);
	// user cancel
 	if (title.isNull())
 		return;
	//TODO: start storing full paths
 	QString toFind(fname.remove(QDir::convertSeparators(finalBaseDir)));
	toFind=toFind.mid(1, toFind.length()-1);
	QMapIterator<QString, QString> i(quickHelpIndex);
	while (i.hasNext())
	{
		i.next();
		if (i.value()==toFind)
		{
			bookmarkIndex.insert(title, qMakePair(i.key(), i.value()));
            helpSideBar->m_Ui->bookmarksView->addTopLevelItem(new QTreeWidgetItem(helpSideBar->m_Ui->bookmarksView, QStringList() << title));
		}
	}
}

void HelpBrowser::deleteBookmarkButton_clicked()
{
    QTreeWidgetItem *twi = helpSideBar->m_Ui->bookmarksView->currentItem();
	if (twi!=NULL)
	{
		if (bookmarkIndex.contains(twi->text(0)))
			bookmarkIndex.remove(twi->text(0));
		delete twi;
	}
}

void HelpBrowser::deleteAllBookmarkButton_clicked()
{
	bookmarkIndex.clear();
    helpSideBar->m_Ui->bookmarksView->clear();
}

void HelpBrowser::histChosen(QAction* i)
{
	if (mHistory.contains(i))
        m_Ui->textBrowser->load( QUrl::fromLocalFile(mHistory[i].url) );
}

void HelpBrowser::jumpToHelpSection(const QString& jumpToSection, const QString& jumpToFile)
{
	QString toLoad;
	bool noDocs=false;

	if (jumpToFile.isEmpty())
	{
		toLoad = finalBaseDir + "/"; //clean this later to handle 5 char locales
		if (jumpToSection.isEmpty())
		{
			QModelIndex index=menuModel->index(0,1);
			if (index.isValid())
			{
                helpSideBar->m_Ui->listView->selectionModel()->select(index, QItemSelectionModel::ClearAndSelect);
				toLoad += menuModel->data(index, Qt::DisplayRole).toString();
                //qDebug("jumpToHelpSection %c / %c", finalBaseDir,  menuModel->data(index, Qt::DisplayRole).toString());
			}
			else
				noDocs=true;
		}
	}
	else
		toLoad=jumpToFile;

	if (!noDocs)
		loadHelp(toLoad);
	else
		displayNoHelp();
}

void HelpBrowser::loadHelp(const QString& filename)
{
	struct histd2 his;
	bool Avail = true;
	QString toLoad;
	QFileInfo fi;
	fi = QFileInfo(filename);
	if (fi.fileName().length()>0)
	{
		if (fi.exists())
			toLoad=filename;
		else
		{
			toLoad = LuminancePaths::HelpDir() +"index.html";
 			language="en";
            //qDebug("Help index: %c", toLoad);
			fi = QFileInfo(toLoad);
			if (!fi.exists())
			{
				displayNoHelp();
				Avail = false;
			}
		}
	}
	else
		Avail=false;
	if (Avail)
	{
        m_Ui->textBrowser->load( QUrl::fromLocalFile(toLoad) );
		
        his.title = m_Ui->textBrowser->title();
		if (his.title.isEmpty())
			his.title = toLoad;
		his.url = toLoad;
		mHistory[histMenu->addAction(his.title)] = his;
	}
	if (mHistory.count() > 15)
	{
		QAction* first=histMenu->actions().first();
		mHistory.remove(first);
		histMenu->removeAction(first);
	}
}

void HelpBrowser::loadMenu()
{
	QString baseHelpDir = LuminancePaths::HelpDir();
	QString baseHelpMenuFile = baseHelpDir + "menu.xml";
	QFileInfo baseFi = QFileInfo(baseHelpMenuFile);
	QString toLoad = baseHelpMenuFile;
	finalBaseDir=baseFi.path();
        if (baseFi.exists())
	{
                if (menuModel!=NULL)
			delete menuModel;
		menuModel=new ScHelpTreeModel(toLoad, "Topic", "Location", &quickHelpIndex);
	
        helpSideBar->m_Ui->listView->setModel(menuModel);
        helpSideBar->m_Ui->listView->setSelectionMode(QAbstractItemView::SingleSelection);
		QItemSelectionModel *selectionModel = new QItemSelectionModel(menuModel);
        helpSideBar->m_Ui->listView->setSelectionModel(selectionModel);
        connect(helpSideBar->m_Ui->listView->selectionModel(), SIGNAL(selectionChanged( const QItemSelection &, const QItemSelection &)), this, SLOT(itemSelected( const QItemSelection &, const QItemSelection &)));
	
        helpSideBar->m_Ui->listView->setColumnHidden(1,true);
	}
	else
	{
        //qDebug("Help menu does not exist: %c", baseHelpMenuFile);
		menuModel=NULL;
	}
}

void HelpBrowser::readBookmarks()
{
	BookmarkParser2 handler;
    handler.view = helpSideBar->m_Ui->bookmarksView;
	handler.quickHelpIndex=&quickHelpIndex;
	handler.bookmarkIndex=&bookmarkIndex;
	QFile xmlFile(bookmarkFile());
	QXmlInputSource source(&xmlFile);
	QXmlSimpleReader reader;
	reader.setContentHandler(&handler);
	reader.parse(source);
}

void HelpBrowser::readHistory()
{
 	HistoryParser2 handler;
 	handler.helpBrowser = this;
 	QFile xmlFile(historyFile());
 	QXmlInputSource source(&xmlFile);
 	QXmlSimpleReader reader;
 	reader.setContentHandler(&handler);
 	reader.parse(source);
}

void HelpBrowser::setText(const QString& str)
{
    m_Ui->textBrowser->setHtml(str);
}

void HelpBrowser::itemSelected(const QItemSelection & selected, const QItemSelection & deselected)
{
	Q_UNUSED(deselected);

	QModelIndex index;
	QModelIndexList items = selected.indexes();
	int i=0;
	foreach (index, items)
	{
		if (i==1) // skip 0, as this is always the rootitem, even if we are selecting the rootitem. hmm
		{
			QString filename(menuModel->data(index, Qt::DisplayRole).toString());
			if (!filename.isEmpty())
			{
				loadHelp(finalBaseDir + "/" + filename);
			}
		}
		++i;
	}
}

void HelpBrowser::itemSearchSelected(QTreeWidgetItem *twi, int i)
{
	Q_UNUSED(i);
	if (!twi)
		return;
	if (quickHelpIndex.contains(twi->text(0)))
	{
		QString filename(quickHelpIndex.value(twi->text(0)));
		if (!filename.isEmpty())
		{
			loadHelp(finalBaseDir + "/" + filename);
            findText = helpSideBar->m_Ui->searchingEdit->text();
			findNext();
		}
	}
}

void HelpBrowser::itemBookmarkSelected(QTreeWidgetItem *twi, int i)
{
	Q_UNUSED(i);
	if (!twi)
		return;
	if (bookmarkIndex.contains(twi->text(0)))
	{
		QString filename(bookmarkIndex.value(twi->text(0)).second);
		if (!filename.isEmpty())
			loadHelp(finalBaseDir + "/" + filename);
	}
}

/*! \brief Returns the name of the cfg file for bookmarks.
A helper function.
\author Petr Vanek <petr@yarpen.cz>
*/
QString HelpBrowser::bookmarkFile()
{
	//TODO   MAC OSX
	//QString appDataDir(typotek::getInstance()->getOwnDir().path() + "/");
	QString sep(QDir::separator());
#ifdef Q_WS_MAC	
	QString appDataDir(QDir::homePath() + sep + "Library" + sep + "LuminanceHDR" + sep);
#elif WIN32
	QString appDataDir(QDir::homePath() + sep + "LuminanceHDR" + sep);
#else
	QString appDataDir(QDir::homePath() + sep + ".LuminanceHDR" + sep);
#endif
	QString fname(appDataDir + "HelpBookmarks.xml");
// 	if (!QFile::exists(fname))
// 	{
// 		QDir d(QDir::convertSeparators(appDataDir));
// 		d.mkdir("doc");
// 	}
	return fname;
}


/*! \brief Returns the name of the cfg file for persistent history.
A helper function.
\author Petr Vanek <petr@yarpen.cz>
*/
QString HelpBrowser::historyFile()
{
	//QString appDataDir(typotek::getInstance()->getOwnDir().path() + "/");
	QString sep(QDir::separator());
#ifdef Q_WS_MAC	
	QString appDataDir(QDir::homePath() + sep + "Library" + sep + "LuminanceHDR" + sep);
#elif WIN32
	QString appDataDir(QDir::homePath() + sep + "LuminanceHDR" + sep);
#else
	QString appDataDir(QDir::homePath() + sep + ".LuminanceHDR" + sep);
	//QString fname(appDataDir + "HelpHistory.xml");
#endif	
	QString fname(appDataDir + "HelpHistory.xml");
// 	if (!QFile::exists(fname))
// 	{
// 		QDir d(QDir::convertSeparators(appDataDir));
// 		d.mkdir("doc");
// 	}
	return fname;
}

void HelpBrowser::displayNoHelp()
{
	QString noHelpMsg=tr("<h2><p>Sorry, no manual is installed!</p><p>Please contact your package provider or LuminanceHDR team if you built the application yourself</p></h2>", "HTML message for no documentation available to show");

    m_Ui->textBrowser->setHtml(noHelpMsg);
	
	filePrint->setEnabled(false);
	editFind->setEnabled(false);
	editFindNext->setEnabled(false);
	editFindPrev->setEnabled(false);
	bookAdd->setEnabled(false);
	bookDel->setEnabled(false);
	bookDelAll->setEnabled(false);
	goHome->setEnabled(false);
	goBack->setEnabled(false);
	goFwd->setEnabled(false);
	
	histMenu->disconnect();
    helpSideBar->m_Ui->searchingEdit->disconnect();
    helpSideBar->m_Ui->searchingButton->disconnect();
    helpSideBar->m_Ui->searchingView->disconnect();
    helpSideBar->m_Ui->bookmarkButton->disconnect();
    helpSideBar->m_Ui->deleteBookmarkButton->disconnect();
    helpSideBar->m_Ui->deleteAllBookmarkButton->disconnect();
    helpSideBar->m_Ui->bookmarksView->disconnect();
    m_Ui->textBrowser->disconnect();
}

void HelpBrowser::viewContents_clicked() {
    helpSideBar->show();
    helpSideBar->m_Ui->tabWidget->setCurrentIndex(0);
}

void HelpBrowser::viewSearch_clicked() {
    helpSideBar->show();
    helpSideBar->m_Ui->tabWidget->setCurrentIndex(1);
}

void HelpBrowser::viewBookmarks_clicked() {
    helpSideBar->show();
    helpSideBar->m_Ui->tabWidget->setCurrentIndex(2);
}

void HelpBrowser::zoomIn_clicked() {
	zoomFactor *= 1.2;
	if (zoomFactor > 46)
		zoomFactor = 46.005;
    m_Ui->textBrowser->setTextSizeMultiplier(zoomFactor);
}

void HelpBrowser::zoomOriginal_clicked() {
	zoomFactor = 1.0;
    m_Ui->textBrowser->setTextSizeMultiplier(1.0);
}

void HelpBrowser::zoomOut_clicked() {
	zoomFactor /= 1.2;
	if (zoomFactor < .02)
		zoomFactor = .0217;
    m_Ui->textBrowser->setTextSizeMultiplier(zoomFactor);
}

void HelpBrowser::handleExternalLink(const QUrl &url) {
	if ((url.scheme() == "http") || url.scheme() == "https") {
#ifdef WIN32
		QDesktopServices::openUrl(url);
#else
        m_Ui->textBrowser->load(url);
#endif
	}
	else {
		QApplication::restoreOverrideCursor();
		if ( QMessageBox::warning(this, tr("LuminanceHDR - Help Browser"),
					tr("This protocol is not handled by the help browser.\n"
						"Do you want to open the link with the default application \n"
						"associated with the protocol?"),
					QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::Yes
			) {

			QDesktopServices::openUrl(url);
		}
	}
}

void HelpBrowser::loadStarted() {
	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
}

void HelpBrowser::loadFinished(bool) {
	QApplication::restoreOverrideCursor();
	statusBar()->showMessage("");
}

void HelpBrowser::linkHovered (const QString &link, const QString &, const QString &) {
	statusBar()->showMessage(link);
}
