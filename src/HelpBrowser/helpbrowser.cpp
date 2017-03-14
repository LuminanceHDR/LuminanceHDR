/**
** This file is a part of Luminance HDR package.
** ----------------------------------------------------------------------
** Copyright (C) 2009-2016 Davide Anastasia, Franco Comida, Daniel Kaneider
**
**  This program is free software; you can redistribute it and/or modify
**  it under the terms of the GNU General Public License as published by
**  the Free Software Foundation; either version 2 of the License, or
**  (at your option) any later version.
**
**  This program is distributed in the hope that it will be useful,
**  but WITHOUT ANY WARRANTY; without even the implied warranty of
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**  GNU General Public License for more details.
**
**  You should have received a copy of the GNU General Public License
**  along with this program; if not, write to the Free Software
**  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
** ----------------------------------------------------------------------
**
** Copied from fontmatrix.
**
** Adapted to Luminance HDR
**
**
**
***************************************************************************
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

#include <QDebug>
#include <QAction>
#include <QBuffer>
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
#include <QProcess>
#include <QPushButton>
#include <QString>
#include <QStandardItem>
#include <QTextEdit>
#include <QTreeView>
#include <QXmlDefaultHandler>
#include <QDesktopServices>

#include <QtPrintSupport/QPrinter>
#include <QtPrintSupport/QPrintDialog>
#include <QtPrintSupport/QPrintPreviewDialog>

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

HelpBrowser::HelpBrowser(QWidget* parent):
    QMainWindow( parent ),
    m_Ui(new Ui::HelpBrowser)
{
    m_Ui->setupUi(this);
}

HelpBrowser::HelpBrowser( QWidget* parent, const QString& /*caption*/, const QString& guiLanguage, const QString& jumpToSection, const QString& jumpToFile):
    QMainWindow( parent ),
    zoomFactor(1.0),
    m_textDocument(new QTextDocument),
    m_Ui(new Ui::HelpBrowser)
{
    m_Ui->setupUi(this);

    restoreGeometry(LuminanceOptions().value("HelpBrowserGeometry").toByteArray());
	setupLocalUI();

    //m_Ui->textBrowser->page()->setLinkDelegationPolicy(QWebPage::DelegateExternalLinks);
    //connect(m_Ui->textBrowser, SIGNAL(linkClicked(const QUrl &)), this, SLOT(handleExternalLink(const QUrl &)));
    connect(m_Ui->textBrowser, SIGNAL(loadStarted()), this, SLOT(loadStarted()));
    connect(m_Ui->textBrowser, SIGNAL(loadFinished(bool)), this, SLOT(loadFinished(bool)));
    //connect(m_Ui->textBrowser->page(), SIGNAL(linkHovered(const QString &, const QString &, const QString & )), this, SLOT(linkHovered(const QString &, const QString &, const QString & )));
    connect(m_Ui->textBrowser->page(), SIGNAL(linkHovered(const QString &)), this, SLOT(linkHovered(const QString &)));

	language = guiLanguage.isEmpty() ? QString("en") : guiLanguage.left(2);
   	finalBaseDir = LuminancePaths::HelpDir();

    qDebug() << finalBaseDir;

    m_Ui->textBrowser->setHome( QUrl::fromLocalFile( finalBaseDir + "index.html" ));
	menuModel=NULL;
    loadMenu();

    if (menuModel!=NULL)
	{
		readBookmarks();
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
    LuminanceOptions().setValue("HelpBrowserGeometry", saveGeometry());
}

void HelpBrowser::closeEvent(QCloseEvent *)
{
	delete menuModel;

	// no need to delete child widgets, Qt does it all for us
	// bookmarks
    LuminanceOptions options;
    {
        QByteArray ba;
	    QTextStream stream(&ba);
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
        stream.flush();

        options.setValue(KEY_HELP_BOOKMARK, ba);
    }

    // history
	{
        QByteArray ba;
		QTextStream stream(&ba);
		stream.setCodec("UTF-8");
		stream << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
		stream << "<history>\n";
		for (QMap<QAction*,histd2>::Iterator it = mHistory.begin() ; it != mHistory.end(); ++it)
			stream << "\t<item title=\"" << it.value().title << "\" url=\"" << it.value().url << "\" />\n";
		stream << "</history>\n";
        stream.flush();
        options.setValue(KEY_HELP_HISTORY, ba);
	}
	// size
// 	prefs->set("xsize", width());
// 	prefs->set("ysize", height());

	emit closed();
}

void HelpBrowser::setupLocalUI()
{
    //icons
    m_Ui->filePrint->setIcon(QIcon::fromTheme("document-print", QIcon(":/help/document-print.png")));
    m_Ui->filePrintPreview->setIcon(QIcon::fromTheme("document-print-preview", QIcon(":/help/document-print-preview.png")));
    m_Ui->fileExit->setIcon(QIcon::fromTheme("application-exit", QIcon(":/help/exit.png")));
    m_Ui->editFind->setIcon(QIcon::fromTheme("edit-find", QIcon(":/help/edit-find.png")));
    m_Ui->goHome->setIcon(QIcon::fromTheme("go-home", QIcon(":/help/go-home.png")));
    m_Ui->goBack->setIcon(QIcon::fromTheme("go-previous", QIcon(":/help/go-previous.png")));
    m_Ui->goFwd->setIcon(QIcon::fromTheme("go-next", QIcon(":/help/go-next.png")));
    m_Ui->zoomIn->setIcon(QIcon::fromTheme("zoom-in", QIcon(":/icons/images/zoom-in.png")));
    m_Ui->zoomOut->setIcon(QIcon::fromTheme("zoom-out", QIcon(":/icons/images/zoom-out.png")));
    m_Ui->zoomOriginal->setIcon(QIcon::fromTheme("zoom-original", QIcon(":/icons/images/zoom-original.png")));
    // end setting icons

	helpSideBar = new HelpSideBar(tr("Help SideBar"), this);
	helpSideBar->setFeatures(QDockWidget::AllDockWidgetFeatures);
	addDockWidget(Qt::LeftDockWidgetArea, helpSideBar);

	histMenu=new QMenu(this);
	m_Ui->goBack->setMenu(histMenu);

    helpSideBar->m_Ui->listView->header()->hide();
    helpSideBar->m_Ui->searchingView->header()->hide();
    helpSideBar->m_Ui->bookmarksView->header()->hide();

	//basic ui
	connect(m_Ui->filePrint, SIGNAL(triggered()), this, SLOT(print()));
	connect(m_Ui->filePrintPreview, SIGNAL(triggered()), this, SLOT(printPreview()));
	connect(m_Ui->fileExit, SIGNAL(triggered()), this, SLOT(close()));
	connect(m_Ui->editFind, SIGNAL(triggered()), this, SLOT(find()));
	connect(m_Ui->editFindNext, SIGNAL(triggered()), this, SLOT(findNext()));
	connect(m_Ui->editFindPrev, SIGNAL(triggered()), this, SLOT(findPrevious()));
	connect(m_Ui->viewContents, SIGNAL(triggered()), this, SLOT(viewContents_clicked()));
	connect(m_Ui->viewSearch, SIGNAL(triggered()), this, SLOT(viewSearch_clicked()));
	connect(m_Ui->viewBookmarks, SIGNAL(triggered()), this, SLOT(viewBookmarks_clicked()));
	connect(m_Ui->bookAdd, SIGNAL(triggered()), this, SLOT(bookmarkButton_clicked()));
	connect(m_Ui->bookDel, SIGNAL(triggered()), this, SLOT(deleteBookmarkButton_clicked()));
	connect(m_Ui->bookDelAll, SIGNAL(triggered()), this, SLOT(deleteAllBookmarkButton_clicked()));
	connect(m_Ui->goHome, SIGNAL(triggered()), m_Ui->textBrowser, SLOT(home()));
	connect(m_Ui->goBack, SIGNAL(triggered()), m_Ui->textBrowser, SLOT(back()));
	connect(m_Ui->goFwd, SIGNAL(triggered()), m_Ui->textBrowser, SLOT(forward()));
	connect(m_Ui->zoomIn, SIGNAL(triggered()), this, SLOT(zoomIn_clicked()));
	connect(m_Ui->zoomOriginal, SIGNAL(triggered()), this, SLOT(zoomOriginal_clicked()));
	connect(m_Ui->zoomOut, SIGNAL(triggered()), this, SLOT(zoomOut_clicked()));
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
	
	m_Ui->fileMenu->setTitle(tr("&File"));
	m_Ui->editMenu->setTitle(tr("&Edit"));
	m_Ui->viewMenu->setTitle(tr("&View"));
	m_Ui->bookMenu->setTitle(tr("&Bookmarks"));
	
	m_Ui->filePrint->setText(tr("&Print..."));
	m_Ui->fileExit->setText(tr("&Quit"));
	m_Ui->editFind->setText(tr("&Find..."));
	m_Ui->editFindNext->setText(tr("Find &Next"));
	m_Ui->editFindPrev->setText(tr("Find &Previous"));
	m_Ui->viewContents->setText(tr("&Contents"));
	m_Ui->viewSearch->setText(tr("&Search"));
	m_Ui->viewBookmarks->setText(tr("&Bookmarks"));
	m_Ui->bookAdd->setText(tr("&Add Bookmark"));
	m_Ui->bookDel->setText(tr("&Delete"));
	m_Ui->bookDelAll->setText(tr("D&elete All"));

    m_Ui->retranslateUi(this);
}

void HelpBrowser::print()
{
    m_Ui->textBrowser->page()->toHtml([this](const QString &result){
            this->m_textDocument->setHtml(result);
            this->printAvailable();
            });
}

void HelpBrowser::printAvailable()
{
	QPrinter printer;
	printer.setFullPage(true);
	QPrintDialog dialog(&printer, this);
	if (dialog.exec())
    {
        m_textDocument->print(&printer);
    }
}

void HelpBrowser::printPreview()
{
    m_Ui->textBrowser->page()->toHtml([this](const QString &result){
            this->m_textDocument->setHtml(result);
            this->printPreviewAvailable();
            });
}

void HelpBrowser::printPreviewAvailable()
{
	QPrinter printer;
	printer.setFullPage(true);
	QPrintPreviewDialog dialog(&printer, this);
    connect(&dialog, SIGNAL(paintRequested(QPrinter *)), this, SLOT(paintRequested(QPrinter *)));
	dialog.exec();
}

void HelpBrowser::paintRequested(QPrinter *printer)
{
    m_textDocument->print(printer);
}

void HelpBrowser::searchingButton_clicked()
{
	// root files
	QApplication::changeOverrideCursor(QCursor(Qt::WaitCursor));
	searchingInDirectory(finalBaseDir);
	QApplication::restoreOverrideCursor();
}

void HelpBrowser::searchingInDirectory(const QString& aDir)
{
	QDir dir(QDir::toNativeSeparators(aDir + "/"));
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
			searchingInDirectory(QDir::toNativeSeparators(aDir + QString((*it)) + "/"));
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
 	QString toFind(fname.remove(finalBaseDir));
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
   			toLoad = LuminancePaths::HelpDir() + "index.html";
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
    QString baseHelpDir(LuminancePaths::HelpDir());
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
    LuminanceOptions options;
    QByteArray ba(options.value(KEY_HELP_BOOKMARK).toByteArray());
    QBuffer buffer(&ba);
	BookmarkParser2 handler;
    handler.view = helpSideBar->m_Ui->bookmarksView;
	handler.quickHelpIndex=&quickHelpIndex;
	handler.bookmarkIndex=&bookmarkIndex;
	QXmlInputSource source(&buffer);
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

void HelpBrowser::displayNoHelp()
{
	QString noHelpMsg=tr("<h2><p>Sorry, no manual is installed!</p><p>Please contact your package provider or LuminanceHDR team if you built the application yourself</p></h2>", "HTML message for no documentation available to show");

    m_Ui->textBrowser->setHtml(noHelpMsg);
	
	m_Ui->filePrint->setEnabled(false);
	m_Ui->editFind->setEnabled(false);
	m_Ui->editFindNext->setEnabled(false);
	m_Ui->editFindPrev->setEnabled(false);
	m_Ui->bookAdd->setEnabled(false);
	m_Ui->bookDel->setEnabled(false);
	m_Ui->bookDelAll->setEnabled(false);
	m_Ui->goHome->setEnabled(false);
	m_Ui->goBack->setEnabled(false);
	m_Ui->goFwd->setEnabled(false);
	
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
    m_Ui->textBrowser->setZoomFactor(zoomFactor);
}

void HelpBrowser::zoomOriginal_clicked() {
	zoomFactor = 1.0;
    m_Ui->textBrowser->setZoomFactor(1.0);
}

void HelpBrowser::zoomOut_clicked() {
	zoomFactor /= 1.2;
	if (zoomFactor < .02)
		zoomFactor = .0217;
    m_Ui->textBrowser->setZoomFactor(zoomFactor);
}

void HelpBrowser::handleExternalLink(const QUrl &url) {
//TODO: Check whether handling these protocol internally has now been fixed in Windows
	if ((url.scheme() == "http") || url.scheme() == "https") {
/*
#ifdef WIN32
		QDesktopServices::openUrl(url);
#else
*/
        m_Ui->textBrowser->load(url);
//#endif
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

void HelpBrowser::linkHovered (const QString &url) {
	statusBar()->showMessage(url);
}
