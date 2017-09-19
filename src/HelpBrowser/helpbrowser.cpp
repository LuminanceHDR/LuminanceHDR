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

#include "HelpBrowser/helpbrowser.h"
#include "HelpBrowser/ui_HelpBrowser.h"
#include "HelpBrowser/ui_HelpSideBar.h"

#include <QAction>
#include <QBuffer>
#include <QDebug>
#include <QDesktopServices>
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
#include <QStandardItem>
#include <QString>
#include <QTextEdit>
#include <QTreeView>
#include <QXmlDefaultHandler>

#include <QtPrintSupport/QPrintDialog>
#include <QtPrintSupport/QPrintPreviewDialog>
#include <QtPrintSupport/QPrinter>

#include "Common/LuminanceOptions.h"
#include "Common/global.h"
#include "HelpBrowser/LuminancePaths.h"
#include "HelpBrowser/schelptreemodel.h"

/*! \brief XML parsef for documantation history.
This is small helper class which reads saved bookmarks configuration
from ~/.scribus/doc/history.xml file.
The reference to historyBrowser is a reference to the dialog.
\author Petr Vanek <petr@yarpen.cz>
*/
class HistoryParser2 : public QXmlDefaultHandler {
   public:
    HelpBrowser *helpBrowser;

    bool startDocument() { return true; }

    bool startElement(const QString &, const QString &, const QString &qName,
                      const QXmlAttributes &attrs) {
        if (qName == QLatin1String("item")) {
            struct histd2 his;
            his.title = attrs.value(0);
            his.url = attrs.value(1);
            helpBrowser->mHistory[helpBrowser->histMenu->addAction(his.title)] =
                his;
        }
        return true;
    }

    bool endElement(const QString &, const QString &, const QString &) {
        return true;
    }
};

/*! \brief XML parsef for documantation bookmarks.
This is small helper class which reads saved bookmarks configuration
from ~/.scribus/doc/bookmarks.xml file.
The reference to QListView *view is a reference to the list view with bookmarks
\author Petr Vanek <petr@yarpen.cz>
*/
class BookmarkParser2 : public QXmlDefaultHandler {
   public:
    QTreeWidget *view;
    QMap<QString, QString> *quickHelpIndex;
    QMap<QString, QPair<QString, QString>> *bookmarkIndex;

    bool startDocument() { return true; }

    bool startElement(const QString &, const QString &, const QString &qName,
                      const QXmlAttributes &attrs) {
        if (qName == QLatin1String("item")) {
            // TODO : This will dump items if bookmarks get loaded into a
            // different
            // GUI language
            if (quickHelpIndex->contains(attrs.value(1))) {
                bookmarkIndex->insert(
                    attrs.value(0), qMakePair(attrs.value(1), attrs.value(2)));
                view->addTopLevelItem(
                    new QTreeWidgetItem(view, QStringList() << attrs.value(0)));
            }
        }
        return true;
    }

    bool endElement(const QString &, const QString &, const QString &) {
        return true;
    }
};

HelpBrowser::HelpBrowser(QWidget *parent)
    : QMainWindow(parent), m_Ui(new Ui::HelpBrowser) {
    m_Ui->setupUi(this);
}

HelpBrowser::HelpBrowser(QWidget *parent, const QString & /*caption*/,
                         const QString &guiLanguage,
                         const QString &jumpToSection,
                         const QString &jumpToFile)
    : QMainWindow(parent),
      zoomFactor(1.0),
      // m_textBrowser(new QTextDocument),
      m_textBrowser(new QTextBrowser),
      m_Ui(new Ui::HelpBrowser) {
    m_Ui->setupUi(this);

    restoreGeometry(LuminanceOptions()
                        .value(QStringLiteral("HelpBrowserGeometry"))
                        .toByteArray());
    setupLocalUI();

    // m_Ui->htmlPage->page()->setLinkDelegationPolicy(QWebPage::DelegateExternalLinks);
    // connect(m_Ui->htmlPage, SIGNAL(linkClicked(const QUrl &)), this,
    // SLOT(handleExternalLink(const QUrl &)));
    connect(m_Ui->htmlPage, &QWebEngineView::loadStarted, this,
            &HelpBrowser::loadStarted);
    connect(m_Ui->htmlPage, &QWebEngineView::loadFinished, this,
            &HelpBrowser::loadFinished);
    // connect(m_Ui->htmlPage->page(), SIGNAL(linkHovered(const QString &, const
    // QString &, const QString & )), this, SLOT(linkHovered(const QString &,
    // const QString &, const QString & )));
    connect(m_Ui->htmlPage->page(), &QWebEnginePage::linkHovered, this,
            &HelpBrowser::linkHovered);

    language =
        guiLanguage.isEmpty() ? QStringLiteral("en") : guiLanguage.left(2);
    finalBaseDir = LuminancePaths::HelpDir();

    qDebug() << finalBaseDir;

    m_Ui->htmlPage->setHome(QUrl::fromLocalFile(finalBaseDir + "index.html"));
    menuModel = NULL;
    loadMenu();

    if (menuModel != NULL) {
        readBookmarks();
        jumpToHelpSection(jumpToSection, jumpToFile);
        languageChange();
    } else {
        qDebug("menuModel == NULL");
        displayNoHelp();
    }
}

HelpBrowser::~HelpBrowser() {
    LuminanceOptions().setValue(QStringLiteral("HelpBrowserGeometry"),
                                saveGeometry());
}

void HelpBrowser::closeEvent(QCloseEvent *) {
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
        while (*it) {
            if (bookmarkIndex.contains((*it)->text(0))) {
                QString pagetitle(bookmarkIndex.value((*it)->text(0)).first);
                QString filename(bookmarkIndex.value((*it)->text(0)).second);
                stream << "\t<item title=\"" << (*it)->text(0)
                       << "\" pagetitle=\"" << pagetitle << "\" url=\""
                       << filename << "\" />\n";
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
        for (QMap<QAction *, histd2>::Iterator it = mHistory.begin();
             it != mHistory.end(); ++it)
            stream << "\t<item title=\"" << it.value().title << "\" url=\""
                   << it.value().url << "\" />\n";
        stream << "</history>\n";
        stream.flush();
        options.setValue(KEY_HELP_HISTORY, ba);
    }
    // size
    //     prefs->set("xsize", width());
    //     prefs->set("ysize", height());

    emit closed();
}

void HelpBrowser::setupLocalUI() {
    helpSideBar = new HelpSideBar(tr("Help SideBar"), this);
    helpSideBar->setFeatures(QDockWidget::AllDockWidgetFeatures);
    addDockWidget(Qt::LeftDockWidgetArea, helpSideBar);

    histMenu = new QMenu(this);
    m_Ui->goBack->setMenu(histMenu);

    helpSideBar->m_Ui->listView->header()->hide();
    helpSideBar->m_Ui->searchingView->header()->hide();
    helpSideBar->m_Ui->bookmarksView->header()->hide();

    // basic ui
    connect(m_Ui->filePrint, &QAction::triggered, this, &HelpBrowser::print);
    connect(m_Ui->filePrintPreview, &QAction::triggered, this,
            &HelpBrowser::printPreview);
    connect(m_Ui->fileExit, &QAction::triggered, this, &QWidget::close);
    connect(m_Ui->editFind, SIGNAL(triggered()), this, SLOT(find()));
    connect(m_Ui->editFindNext, &QAction::triggered, this,
            &HelpBrowser::findNext);
    connect(m_Ui->editFindPrev, &QAction::triggered, this,
            &HelpBrowser::findPrevious);
    connect(m_Ui->viewContents, &QAction::triggered, this,
            &HelpBrowser::viewContents_clicked);
    connect(m_Ui->viewSearch, &QAction::triggered, this,
            &HelpBrowser::viewSearch_clicked);
    connect(m_Ui->viewBookmarks, &QAction::triggered, this,
            &HelpBrowser::viewBookmarks_clicked);
    connect(m_Ui->bookAdd, &QAction::triggered, this,
            &HelpBrowser::bookmarkButton_clicked);
    connect(m_Ui->bookDel, &QAction::triggered, this,
            &HelpBrowser::deleteBookmarkButton_clicked);
    connect(m_Ui->bookDelAll, &QAction::triggered, this,
            &HelpBrowser::deleteAllBookmarkButton_clicked);
    connect(m_Ui->goHome, &QAction::triggered, m_Ui->htmlPage,
            &ScTextBrowser::home);
    connect(m_Ui->goBack, &QAction::triggered, m_Ui->htmlPage,
            &QWebEngineView::back);
    connect(m_Ui->goFwd, &QAction::triggered, m_Ui->htmlPage,
            &QWebEngineView::forward);
    connect(m_Ui->zoomIn, &QAction::triggered, this,
            &HelpBrowser::zoomIn_clicked);
    connect(m_Ui->zoomOriginal, &QAction::triggered, this,
            &HelpBrowser::zoomOriginal_clicked);
    connect(m_Ui->zoomOut, &QAction::triggered, this,
            &HelpBrowser::zoomOut_clicked);
    connect(histMenu, &QMenu::triggered, this, &HelpBrowser::histChosen);
    // searching
    connect(helpSideBar->m_Ui->searchingEdit, &QLineEdit::returnPressed, this,
            &HelpBrowser::searchingButton_clicked);
    connect(helpSideBar->m_Ui->searchingButton, &QAbstractButton::clicked, this,
            &HelpBrowser::searchingButton_clicked);
    connect(helpSideBar->m_Ui->searchingView, &QTreeWidget::itemClicked, this,
            &HelpBrowser::itemSearchSelected);
    // bookmarks
    connect(helpSideBar->m_Ui->bookmarkButton, &QAbstractButton::clicked, this,
            &HelpBrowser::bookmarkButton_clicked);
    connect(helpSideBar->m_Ui->deleteBookmarkButton, &QAbstractButton::clicked,
            this, &HelpBrowser::deleteBookmarkButton_clicked);
    connect(helpSideBar->m_Ui->deleteAllBookmarkButton,
            &QAbstractButton::clicked, this,
            &HelpBrowser::deleteAllBookmarkButton_clicked);
    connect(helpSideBar->m_Ui->bookmarksView, &QTreeWidget::itemClicked, this,
            &HelpBrowser::itemBookmarkSelected);
    // links hoover
    connect(m_Ui->htmlPage, &ScTextBrowser::overLink, this,
            &HelpBrowser::showLinkContents);

    languageChange();
}

void HelpBrowser::showLinkContents(const QString &link) {
    statusBar()->showMessage(link);
}

void HelpBrowser::changeEvent(QEvent *e) {
    if (e->type() == QEvent::LanguageChange) {
        languageChange();
    }
    QWidget::changeEvent(e);
}

void HelpBrowser::languageChange() {
    setWindowTitle(tr("LuminanceHDR Online Help"));

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

void HelpBrowser::print() {
    /* TODO With this method images aren't loaded so I'm passing the html page
    to
    a QTextBrowser
    m_Ui->htmlPage->page()->toHtml([this](const QString &result){
            this->m_textBrowser->setHtml(result);
            this->printAvailable();
            });
    */
    m_textBrowser->setSource(m_Ui->htmlPage->page()->url());
    this->printAvailable();
}

void HelpBrowser::printAvailable() {
    QPrinter printer;
    printer.setFullPage(true);
    QPrintDialog dialog(&printer, this);
    if (dialog.exec()) {
        m_textBrowser->print(&printer);
    }
}

void HelpBrowser::printPreview() {
    /* TODO With this method images aren't loaded so I'm passing the html page
    to
    a QTextBrowser
    m_Ui->htmlPage->page()->toHtml([this](const QString &result){
            this->m_textBrowser->setHtml(result);
            this->printPreviewAvailable();
            });
    */
    m_textBrowser->setSource(m_Ui->htmlPage->page()->url());
    this->printPreviewAvailable();
}

void HelpBrowser::printPreviewAvailable() {
    QPrinter printer;
    printer.setFullPage(true);
    QPrintPreviewDialog dialog(&printer, this);
    connect(&dialog, &QPrintPreviewDialog::paintRequested, this,
            &HelpBrowser::paintRequested);
    dialog.exec();
}

void HelpBrowser::paintRequested(QPrinter *printer) {
    m_textBrowser->print(printer);
}

void HelpBrowser::searchingButton_clicked() {
    // root files
    QApplication::changeOverrideCursor(QCursor(Qt::WaitCursor));
    searchingInDirectory(finalBaseDir);
    QApplication::restoreOverrideCursor();
}

void HelpBrowser::searchingInDirectory(const QString &aDir) {
    QDir dir(QDir::toNativeSeparators(aDir + "/"));
    QStringList in;
    in.append(QStringLiteral("*.html"));
    QStringList lst = dir.entryList(in);
    for (QStringList::Iterator it = lst.begin(); it != lst.end(); ++it) {
        QString fname(aDir + "/" + (*it));
        QFile f(fname);
        if (f.open(QIODevice::ReadOnly)) {
            QTextStream stream(&f);
            QString str = stream.readAll();
            int cnt = str.count(helpSideBar->m_Ui->searchingEdit->text(),
                                Qt::CaseInsensitive);
            if (cnt > 0) {
                // QString fullname = fname;
                QString toFind(fname.remove(finalBaseDir + "/"));
                QMapIterator<QString, QString> i(quickHelpIndex);
                while (i.hasNext()) {
                    i.next();
                    if (i.value() == toFind)
                        helpSideBar->m_Ui->searchingView->addTopLevelItem(
                            new QTreeWidgetItem(
                                helpSideBar->m_Ui->searchingView,
                                QStringList() << i.key()));
                }
            }
            f.close();
        }
    }
    // get dirs - ugly recursion
    in.clear();
    in.append(QStringLiteral("*"));
    QStringList dst = dir.entryList(in, QDir::Dirs);
    for (QStringList::Iterator it = dst.begin(); it != dst.end(); ++it)
        if ((*it) != QLatin1String(".") && (*it) != QLatin1String(".."))
            searchingInDirectory(
                QDir::toNativeSeparators(aDir + QString((*it)) + "/"));
}

void HelpBrowser::find() {
    findText = QInputDialog::getText(this, tr("Find"), tr("Search Term:"),
                                     QLineEdit::Normal, findText, 0);
    if (findText.isNull()) return;
    findNext();
}

void HelpBrowser::findNext() {
    if (findText.isNull()) {
        find();
        return;
    }
    // find it. finally
    m_Ui->htmlPage->findText(findText, 0);
}

void HelpBrowser::findPrevious() {
    if (findText.isNull()) {
        find();
        return;
    }
    // find it. finally
    m_Ui->htmlPage->findText(findText);
}

void HelpBrowser::bookmarkButton_clicked() {
    QString title = m_Ui->htmlPage->title();
    QString fname(QDir::cleanPath(m_Ui->htmlPage->url().toLocalFile()));
    title = QInputDialog::getText(this, tr("New Bookmark"),
                                  tr("New Bookmark's Title:"),
                                  QLineEdit::Normal, title, 0);
    // user cancel
    if (title.isNull()) return;
    // TODO: start storing full paths
    QString toFind(fname.remove(finalBaseDir));
    toFind = toFind.mid(1, toFind.length() - 1);
    QMapIterator<QString, QString> i(quickHelpIndex);

    while (i.hasNext()) {
        i.next();
        if (i.value() == toFind) {
            bookmarkIndex.insert(title, qMakePair(i.key(), i.value()));
            helpSideBar->m_Ui->bookmarksView->addTopLevelItem(
                new QTreeWidgetItem(helpSideBar->m_Ui->bookmarksView,
                                    QStringList() << title));
        }
    }
}

void HelpBrowser::deleteBookmarkButton_clicked() {
    QTreeWidgetItem *twi = helpSideBar->m_Ui->bookmarksView->currentItem();
    if (twi != NULL) {
        if (bookmarkIndex.contains(twi->text(0)))
            bookmarkIndex.remove(twi->text(0));
        delete twi;
    }
}

void HelpBrowser::deleteAllBookmarkButton_clicked() {
    bookmarkIndex.clear();
    helpSideBar->m_Ui->bookmarksView->clear();
}

void HelpBrowser::histChosen(QAction *i) {
    if (mHistory.contains(i))
        m_Ui->htmlPage->load(QUrl::fromLocalFile(mHistory[i].url));
}

void HelpBrowser::jumpToHelpSection(const QString &jumpToSection,
                                    const QString &jumpToFile) {
    QString toLoad;
    bool noDocs = false;

    if (jumpToFile.isEmpty()) {
        toLoad =
            finalBaseDir + "/";  // clean this later to handle 5 char locales
        if (jumpToSection.isEmpty()) {
            QModelIndex index = menuModel->index(0, 1);
            if (index.isValid()) {
                helpSideBar->m_Ui->listView->selectionModel()->select(
                    index, QItemSelectionModel::ClearAndSelect);
                toLoad += menuModel->data(index, Qt::DisplayRole).toString();
                // qDebug("jumpToHelpSection %c / %c", finalBaseDir,
                // menuModel->data(index, Qt::DisplayRole).toString());
            } else
                noDocs = true;
        }
    } else
        toLoad = jumpToFile;

    if (!noDocs)
        loadHelp(toLoad);
    else
        displayNoHelp();
}

void HelpBrowser::loadHelp(const QString &filename) {
    struct histd2 his;
    bool Avail = true;
    QString toLoad;
    QFileInfo fi;
    fi = QFileInfo(filename);
    if (fi.fileName().length() > 0) {
        if (fi.exists())
            toLoad = filename;
        else {
            toLoad = LuminancePaths::HelpDir() + "index.html";
            language = QStringLiteral("en");
            // qDebug("Help index: %c", toLoad);
            fi = QFileInfo(toLoad);
            if (!fi.exists()) {
                displayNoHelp();
                Avail = false;
            }
        }
    } else
        Avail = false;
    if (Avail) {
        m_Ui->htmlPage->load(QUrl::fromLocalFile(toLoad));
        // m_Ui->htmlPage->page()->setBackgroundColor(QColor(25, 25, 25));

        his.title = m_Ui->htmlPage->title();
        if (his.title.isEmpty()) his.title = toLoad;
        his.url = toLoad;
        mHistory[histMenu->addAction(his.title)] = his;
    }
    if (mHistory.count() > 15) {
        QAction *first = histMenu->actions().constFirst();
        mHistory.remove(first);
        histMenu->removeAction(first);
    }
}

void HelpBrowser::loadMenu() {
    QString baseHelpDir(LuminancePaths::HelpDir());
    QString baseHelpMenuFile = baseHelpDir + "menu.xml";
    QFileInfo baseFi = QFileInfo(baseHelpMenuFile);
    QString toLoad = baseHelpMenuFile;
    finalBaseDir = baseFi.path();
    if (baseFi.exists()) {
        if (menuModel != NULL) delete menuModel;
        menuModel =
            new ScHelpTreeModel(toLoad, QStringLiteral("Topic"),
                                QStringLiteral("Location"), &quickHelpIndex);

        helpSideBar->m_Ui->listView->setModel(menuModel);
        helpSideBar->m_Ui->listView->setSelectionMode(
            QAbstractItemView::SingleSelection);
        QItemSelectionModel *selectionModel =
            new QItemSelectionModel(menuModel);
        helpSideBar->m_Ui->listView->setSelectionModel(selectionModel);
        connect(helpSideBar->m_Ui->listView->selectionModel(),
                &QItemSelectionModel::selectionChanged, this,
                &HelpBrowser::itemSelected);

        helpSideBar->m_Ui->listView->setColumnHidden(1, true);
    } else {
        // qDebug("Help menu does not exist: %c", baseHelpMenuFile);
        menuModel = NULL;
    }
}

void HelpBrowser::readBookmarks() {
    LuminanceOptions options;
    QByteArray ba(options.value(KEY_HELP_BOOKMARK).toByteArray());
    QBuffer buffer(&ba);
    BookmarkParser2 handler;
    handler.view = helpSideBar->m_Ui->bookmarksView;
    handler.quickHelpIndex = &quickHelpIndex;
    handler.bookmarkIndex = &bookmarkIndex;
    QXmlInputSource source(&buffer);
    QXmlSimpleReader reader;
    reader.setContentHandler(&handler);
    reader.parse(source);
}

void HelpBrowser::setText(const QString &str) { m_Ui->htmlPage->setHtml(str); }

void HelpBrowser::itemSelected(const QItemSelection &selected,
                               const QItemSelection &deselected) {
    Q_UNUSED(deselected);

    QModelIndexList items = selected.indexes();
    int i = 0;
    foreach (const QModelIndex &index, items) {
        if (i == 1)  // skip 0, as this is always the rootitem, even if we are
                     // selecting the rootitem. hmm
        {
            QString filename(
                menuModel->data(index, Qt::DisplayRole).toString());
            if (!filename.isEmpty()) {
                loadHelp(finalBaseDir + "/" + filename);
            }
        }
        ++i;
    }
}

void HelpBrowser::itemSearchSelected(QTreeWidgetItem *twi, int i) {
    Q_UNUSED(i);
    if (!twi) return;
    if (quickHelpIndex.contains(twi->text(0))) {
        QString filename(quickHelpIndex.value(twi->text(0)));
        if (!filename.isEmpty()) {
            loadHelp(finalBaseDir + "/" + filename);
            findText = helpSideBar->m_Ui->searchingEdit->text();
            findNext();
        }
    }
}

void HelpBrowser::itemBookmarkSelected(QTreeWidgetItem *twi, int i) {
    Q_UNUSED(i);
    if (!twi) return;
    if (bookmarkIndex.contains(twi->text(0))) {
        QString filename(bookmarkIndex.value(twi->text(0)).second);
        if (!filename.isEmpty()) loadHelp(finalBaseDir + "/" + filename);
    }
}

void HelpBrowser::displayNoHelp() {
    QString noHelpMsg =
        tr("<h2><p>Sorry, no manual is installed!</p><p>Please contact your "
           "package provider or LuminanceHDR team if you built the application "
           "yourself</p></h2>",
           "HTML message for no documentation available to show");

    m_Ui->htmlPage->setHtml(noHelpMsg);

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
    m_Ui->htmlPage->disconnect();
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
    if (zoomFactor > 46) zoomFactor = 46.005;
    m_Ui->htmlPage->setZoomFactor(zoomFactor);
}

void HelpBrowser::zoomOriginal_clicked() {
    zoomFactor = 1.0;
    m_Ui->htmlPage->setZoomFactor(1.0);
}

void HelpBrowser::zoomOut_clicked() {
    zoomFactor /= 1.2;
    if (zoomFactor < .02) zoomFactor = .0217;
    m_Ui->htmlPage->setZoomFactor(zoomFactor);
}

void HelpBrowser::handleExternalLink(const QUrl &url) {
    // TODO: Check whether handling these protocol internally has now been fixed
    // in Windows
    if ((url.scheme() == QLatin1String("http")) ||
        url.scheme() == QLatin1String("https")) {
        /*
        #ifdef WIN32
                QDesktopServices::openUrl(url);
        #else
        */
        m_Ui->htmlPage->load(url);
        //#endif
    } else {
        QApplication::restoreOverrideCursor();
        if (QMessageBox::warning(
                this, tr("LuminanceHDR - Help Browser"),
                tr("This protocol is not handled by the help browser.\n"
                   "Do you want to open the link with the default application "
                   "\n"
                   "associated with the protocol?"),
                QMessageBox::Yes | QMessageBox::No,
                QMessageBox::No) == QMessageBox::Yes) {
            QDesktopServices::openUrl(url);
        }
    }
}

void HelpBrowser::loadStarted() {
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
}

void HelpBrowser::loadFinished(bool) {
    QApplication::restoreOverrideCursor();
    statusBar()->showMessage(QLatin1String(""));
}

void HelpBrowser::linkHovered(const QString &url) {
    statusBar()->showMessage(url);
}
