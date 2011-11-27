/**
 * This file is a part of Luminance HDR package.
 * ----------------------------------------------------------------------
 * Copyright (C) 2009 Franco Comida
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * ----------------------------------------------------------------------
 *
 * @author Franco Comida <fcomida@users.sourceforge.net>
 */

#ifndef GENERICVIEWER_H
#define GENERICVIEWER_H

#include <QImage>
#include <QWidget>
#include <QVBoxLayout>
#include <QToolBar>
#include <QToolButton>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>

// Forward declaration
namespace pfs {
    class Frame;            // #include "Libpfs/frame.h"
}

class PanIconWidget;        // #include "Common/PanIconWidget.h"
class IGraphicsView;        // #include "Viewers/IGraphicsView.h"
class IGraphicsPixmapItem;  // #include "Viewers/IGraphicsPixmapItem.h"

enum ViewerMode {FIT_WINDOW, FILL_WINDOW, NORMAL_SIZE};

class GenericViewer : public QWidget 
{
    Q_OBJECT
public:
    //! \brief GenericViewer constructor
    //! \param[in] frame reference frame
    //! \param[in] ns need saving
    GenericViewer(pfs::Frame* frame, QWidget *parent = 0, bool ns = false);

    //! \brief GenericViewer virtual destructor
    virtual ~GenericViewer();

    //! returns the width of the current frame
    int getWidth();

    //! returns the height of the current frame
    int getHeight();

public Q_SLOTS:
    /*virtual*/ void updateView();  // tells the Viewer to update the View area

    /*virtual*/ void zoomIn();
    /*virtual*/ void zoomOut();

    /*virtual*/ void fitToWindow(bool checked = true);  // checked is useless: kept for compatibility
    /*virtual*/ bool isFittedToWindow();

    /*virtual*/ void fillToWindow();
    /*virtual*/ bool isFilledToWindow();

    /*virtual*/ void normalSize();
    /*virtual*/ bool isNormalSize();

    // selection properties!
    bool hasSelection();
    void setSelectionTool(bool);
    QRect getSelectionRect();
    void removeSelection();

    bool needsSaving();
    void setNeedsSaving(bool);

    //! \brief get filename if set, or an empty string
    QString getFileName();

    //! \brief set filename (overwrite previous status)
    void setFileName(const QString&);

    //! \brief This function syncronizes scrollbars position and zoom mode
    //! \param[src] GenericViewer to syncronize with
    void syncViewer(GenericViewer *src);

    virtual bool isHDR() = 0;
    virtual void levelsRequested(bool) = 0; // only used by LdrViewer

    //! \brief returns a filename postfix based on the viewer's content
    virtual QString getFileNamePostFix() = 0;

    //! \brief
    virtual QString getExifComment() = 0;

    //! \brief returns a QImage that reflects the content of the viewerport
    QImage getQImage() const;

    //! this function returns an handle to the internal data type
    //! it would be better if the return pointer is const
    //! It requires to many changes at this stage and it does not worth the effort
    //! it will be done during the integration of LibHDR
    pfs::Frame* getFrame() const;

    //! set a new reference frame to be shown in the viewport
    //! previous frame gets DELETED!
    void setFrame(pfs::Frame* new_frame);

protected Q_SLOTS:
    /*virtual*/  void slotPanIconSelectionMoved(QRect);
    /*virtual*/  void slotPanIconHidden();
    /*virtual*/  void slotCornerButtonPressed();
    /*virtual*/  void scrollBarChanged(int /*value*/);

    //! \brief Update viewer's pixmap when a new frame is set by setFrame()
    virtual void updatePixmap() = 0;

//    //! \brief Refresh viewer's pixmap when
//    virtual void refreshPixmap() = 0;

protected:

    QToolBar* mToolBar;
    QToolButton* mCornerButton;
    PanIconWidget* mPanIconWidget;

    QVBoxLayout *mVBL;

    QGraphicsScene* mScene;
    IGraphicsView* mView;
    ViewerMode mViewerMode;
    IGraphicsPixmapItem* mPixmap;

    QString mFileName;

private:
    //! \brief Zoom to the input factor
    //! \param[in] factor zoom factor to match
    //! \attention currently this function has an empty body
    void zoomToFactor(float factor);

    //! \brief Calculate the current zoom factor
    //! \return current zoom factor
    float getScaleFactor();

    bool mNeedsSaving;
    pfs::Frame* mFrame;

Q_SIGNALS:
    void selectionReady(bool isReady);
    void changed(GenericViewer *v);     // emitted when zoomed in/out, scrolled ....
    void levels_closed(bool isReady);   // only used by LdrViewer
};

#endif
