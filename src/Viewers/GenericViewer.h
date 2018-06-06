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

#include <memory>

#include <QGraphicsPixmapItem>
#include <QGraphicsScene>
#include <QImage>
#include <QString>
#include <QToolBar>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWidget>

#include <Libpfs/colorspace/rgbremapper_fwd.h>

// Forward declaration
namespace pfs {
class Frame;  // #include "Libpfs/frame.h"
}

class PanIconWidget;        // #include "Common/PanIconWidget.h"
class IGraphicsView;        // #include "Viewers/IGraphicsView.h"
class IGraphicsPixmapItem;  // #include "Viewers/IGraphicsPixmapItem.h"
class TonemappingOptions;

class GenericViewer : public QWidget {
    Q_OBJECT
   public:
    //! \brief Enum containing the list of possible view mode
    enum ViewerMode { FIT_WINDOW = 0, FILL_WINDOW = 1, NORMAL_SIZE = 2 };

   public:
    //! \brief GenericViewer constructor
    //! \param[in] frame reference frame
    //! \param[in] ns need saving
    GenericViewer(pfs::Frame *frame, QWidget *parent = 0, bool ns = false);

    //! \brief GenericViewer virtual destructor
    virtual ~GenericViewer();

    //! returns the width of the current frame
    int getWidth();

    //! returns the height of the current frame
    int getHeight();

    /*virtual*/ bool isFittedToWindow();
    /*virtual*/ bool isFilledToWindow();
    /*virtual*/ bool isNormalSize();

    //! \brief Set tonemap options that generated the handled frame
    //! \note GenericViewer holds only an empty implementation of this function.
    //! It is
    //! up to the derived class to override this behaviour
    virtual void setTonemappingOptions(TonemappingOptions *tmopts) {}

    //! \brief Get tonemap options that generated the handled frame
    //! \note GenericViewer holds only an empty implementation of this function.
    //! It is
    //! up to the derived class to override this behaviour
    virtual TonemappingOptions *getTonemappingOptions() { return NULL; }

   public Q_SLOTS:
    //! \brief Update viewer's pixmap when a new frame is set by setFrame() or
    //! AWB
    //! is performed
    virtual void updatePixmap() = 0;

    /*virtual*/ void updateView();  // tells the Viewer to update the View area

    /*virtual*/ void zoomIn();
    /*virtual*/ void zoomOut();

    /*virtual*/ void fitToWindow(
        bool checked = true);  // checked is useless: kept for compatibility
    /*virtual*/ void fillToWindow();
    /*virtual*/ void normalSize();

    //! \brief get viewer mode (Fit, Fill or Normal Size)
    ViewerMode getViewerMode();

    //! \brief set viewer mode (Fit, Fill or Normal Size)
    void setViewerMode(ViewerMode viewer_mode);

    // selection properties!
    bool hasSelection();
    void setSelectionTool(bool);
    QRect getSelectionRect();
    void removeSelection();

    bool needsSaving();
    void setNeedsSaving(bool);

    //! \brief get filename if set, or an empty string
    const QString &getFileName() const;

    //! \brief set filename (overwrite previous status)
    void setFileName(const QString &);

    //! \brief This function syncronizes scrollbars position and zoom mode
    //! \param[src] GenericViewer to syncronize with
    void syncViewer(GenericViewer *src);

    //! \brief tells is the viewer holds an LDR or an HDR frame
    virtual bool isHDR() = 0;

    //! \brief returns max value of the handled frame
    virtual float getMaxLuminanceValue() = 0;

    //! \brief returns min value of the handled frame
    virtual float getMinLuminanceValue() = 0;

    //! \brief returns the mapping method
    virtual RGBMappingType getLuminanceMappingMethod() { return MAP_LINEAR; }

    //! \brief returns a filename postfix based on the viewer's content
    virtual QString getFileNamePostFix() = 0;

    //! \brief
    virtual QString getExifComment() = 0;

    //! \brief returns a QImage that reflects the content of the viewerport
    QImage getQImage() const;

    //! \brief set new QImage
    void setQImage(const QImage &qimage);

    //! \brief
    void setDevicePixelRatio(const float s);

    //! this function returns an handle to the internal data type
    //! it would be better if the return pointer is const
    //! It requires to many changes at this stage and it does not worth the
    //! effort
    //! it will be done during the integration of LibHDR
    pfs::Frame *getFrame() const;

    //! set a new reference frame to be shown in the viewport
    //! previous frame gets DELETED!
    void setFrame(pfs::Frame *new_frame, TonemappingOptions *tmopts = NULL);
    void setFrameShared(std::shared_ptr<pfs::Frame> new_frame);

   protected Q_SLOTS:
    /*virtual*/ void slotPanIconSelectionMoved(QRect);
    /*virtual*/ void slotPanIconHidden();
    /*virtual*/ void slotCornerButtonPressed();
    /*virtual*/ void scrollBarChanged(int /*value*/);

    void startDragging();

   protected:
    virtual void retranslateUi();
    virtual void changeEvent(QEvent *event);

    QToolBar *mToolBar;
    QToolButton *mCornerButton;
    PanIconWidget *mPanIconWidget;

    QVBoxLayout *mVBL;

    QGraphicsScene *mScene;
    IGraphicsView *mView;
    ViewerMode mViewerMode;
    IGraphicsPixmapItem *mPixmap;

    QString mFileName;

    void keyPressEvent(QKeyEvent *event);

   private:
    //! \brief Zoom to the input factor
    //! \param[in] factor zoom factor to match
    //! \attention currently this function has an empty body
    void zoomToFactor(float factor);

    //! \brief Calculate the current zoom factor
    //! \return current zoom factor
    float getScaleFactor();

    bool mNeedsSaving;
    std::shared_ptr<pfs::Frame> mFrame;

    QAction *m_actionClose;

    float m_devicePixelRatio = 1.0;

   Q_SIGNALS:
    void selectionReady(bool isReady);
    void changed(
        GenericViewer *v);  // emitted when zoomed in/out, scrolled ....
    void reparent(GenericViewer *v);     // emitted when exit fullscreen
    void goNext(GenericViewer *v);       // shows next image in fullscreen
    void goPrevious(GenericViewer *v);   // shows previous image in fullscreen
    void syncViewers(GenericViewer *v);  // toggle viewers syncronization
};

inline bool GenericViewer::needsSaving(void) { return mNeedsSaving; }

inline void GenericViewer::setNeedsSaving(bool s) { mNeedsSaving = s; }

inline const QString &GenericViewer::getFileName() const { return mFileName; }

inline void GenericViewer::setFileName(const QString &fn) { mFileName = fn; }

inline void GenericViewer::setDevicePixelRatio(const float s) {
    m_devicePixelRatio = s;
}
#endif
