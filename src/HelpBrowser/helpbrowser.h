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

#ifndef HELPBROWSER_H
#define HELPBROWSER_H

#include <QAction>
#include <QEvent>
#include <QItemSelection>
#include <QList>
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QPair>
#include <QString>
#include <QToolBar>
#include <QTreeWidgetItem>
#include <QUrl>
#include <QVariant>
#include <QWidget>
#include <QXmlInputSource>
//#include <QTextEdit>
#include <QTextBrowser>

class ScHelpTreeModel;
class QPrinter;

#include "HelpSideBar.h"

#define KEY_HELP_BOOKMARK "help/Bookmark"
#define KEY_HELP_HISTORY "help/History"

//! \brief A structure holding title/file url reference.
struct histd2 {
    QString url;
    QString title;
};

namespace Ui {
class HelpBrowser;
}

class HelpBrowser : public QMainWindow {
    Q_OBJECT

   public:
    explicit HelpBrowser(QWidget *parent);
    HelpBrowser(QWidget *parent, const QString &caption,
                const QString &guiLangage = QStringLiteral("en"),
                const QString &jumpToSection = QLatin1String(""),
                const QString &jumpToFile = QLatin1String(""));
    ~HelpBrowser();

    /*! \brief History menu. It's public because of history reader - separate
     * class */
    QMenu *histMenu;
    /*! \brief Mapping the documents for history. */
    QMap<QAction *, histd2> mHistory;
    /*! \brief Set text to the browser
    \param str a QString with text (html) */
    void setText(const QString &str);

   protected:
    virtual void changeEvent(QEvent *e);
    void closeEvent(QCloseEvent *event);

    void setupLocalUI();
    /*! \brief Reads saved bookmarks from external file */
    void readBookmarks();
    /*! \brief Search in doc files in spec. dir.
    It uses directory-recursion. I hope that the documentation will have
    only 2-3 level dir structure so it doesn't matter.
    \author Petr Vanek <petr@yarpen.cz> */
    void searchingInDirectory(const QString &);

    /*! \brief Tell the user there is no help available */
    void displayNoHelp();

    HelpSideBar *helpSideBar;
    qreal zoomFactor;
    //! \brief Selected language is here. If there is no docs for this language,
    //! "en" is used.
    QString language;
    //! \brief QString holding location of menu.xml we are using, we load the
    //! help
    //! files from here
    QString finalBaseDir;
    /*! \brief Text to be found in document */
    QString findText;

    ScHelpTreeModel *menuModel;
    QMap<QString, QString> quickHelpIndex;
    QMap<QString, QPair<QString, QString>> bookmarkIndex;

    // I need to keep this around because page()->toHtml( <callback> ) is
    // asynchronous
    // QSharedPointer<QTextDocument> m_textDocument;
    QSharedPointer<QTextBrowser> m_textBrowser;

   protected slots:
    virtual void languageChange();
    void histChosen(QAction *i);
    void jumpToHelpSection(const QString &jumpToSection,
                           const QString &jumpToFile = QLatin1String(""));
    void loadHelp(const QString &filename);
    void loadMenu();
    void showLinkContents(const QString &link);

    /*! \brief Load doc file when user select filename in content view. */
    void itemSelected(const QItemSelection &selected,
                      const QItemSelection &deselected);

    /*! \brief Load doc file when user select filename in search view. */
    void itemSearchSelected(QTreeWidgetItem *, int);

    /*! \brief Load doc file when user select filename in bookmark view. */
    void itemBookmarkSelected(QTreeWidgetItem *, int);

    /*! \brief Performs searching in documentation.
    It walks through installed documentation and searching in all text files
    \author Petr Vanek <petr@yarpen.cz> */
    void searchingButton_clicked();

    /*! \brief Find text in one document.
    Classical ctrl+f searching.
    \author Petr Vanek <petr@yarpen.cz> */
    void find();

    /*! \brief Find next occurences of the text in one document.
    \author Petr Vanek <petr@yarpen.cz> */
    void findNext();

    /*! \brief Find previous occurences of the text in one document.
    \author Petr Vanek <petr@yarpen.cz> */
    void findPrevious();

    /*! \brief Print the documentation.    */
    void print();
    void printAvailable();

    /*! \brief Preview the documentation before printing. */
    void printPreview();
    void printPreviewAvailable();
    void paintRequested(QPrinter *printer);

    /*! \brief Add document into bookmarks. */
    void bookmarkButton_clicked();

    /*! \brief Delete selected document from bookmarks. */
    void deleteBookmarkButton_clicked();

    /*! \brief Delete all bookmarks */
    void deleteAllBookmarkButton_clicked();

    /*! \brief Show helpSideBar Contents Tab */
    void viewContents_clicked();

    /*! \brief Show helpSideBar Search Tab */
    void viewSearch_clicked();

    /*! \brief Show helpSideBar Bookmarks */
    void viewBookmarks_clicked();

    /*! \brief zoom In */
    void zoomIn_clicked();

    /*! \brief zoom Original Size */
    void zoomOriginal_clicked();

    /*! \brief zoom Out*/
    void zoomOut_clicked();

    /*! \brief User clicked on an external link */
    void handleExternalLink(const QUrl &);

    /*! \brief Show Wait Cursor while loading a page*/
    void loadStarted();

    /*! \brief Restore Default Cursor */
    void loadFinished(bool);
    void linkHovered(const QString &);
   signals:
    void closed();

   protected:
    QScopedPointer<Ui::HelpBrowser> m_Ui;
};

#endif  // HELPBROWSER_H
