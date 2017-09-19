/**
 * This file is a part of LuminanceHDR package.
 * ----------------------------------------------------------------------
 * Copyright (C) 2006,2007 Giuseppe Rota
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
 * based on previous GPL code from qpfstmo
 * Original Work
 * @author Giuseppe Rota <grota@users.sourceforge.net>
 * Improvements, bugfixing
 * @author Franco Comida <fcomida@users.sourceforge.net>
 *
 */

#ifndef GANG_H
#define GANG_H

#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QList>
#include <QRadioButton>
#include <QSlider>

class TmoSettingsList;

class Gang : public QObject {
    Q_OBJECT
   public:
    friend class TmoSettings;

    Gang(QSlider *s = 0, QDoubleSpinBox *dsb = 0, QCheckBox *cbx1 = 0,
         QCheckBox *cbx2 = 0, QRadioButton *rb1 = 0, QRadioButton *rb2 = 0,
         const float minv = 0.0, const float maxv = 0.0, const float vv = 0.0,
         const bool logs = false);
    ~Gang();

    float v() const;
    bool isCheckBox1Checked() const;
    bool isCheckBox2Checked() const;
    bool isRadioButton1Checked() const;
    bool isRadioButton2Checked() const;
    float p2v(const int p) const;
    int v2p(const float x) const;
    void setDefault();
    bool changed() const;
    QString flag(const QString &f) const;
    QString fname(const QString &f) const;
    void setupUndo();
    void undo();
    void redo();
    void updateUndoState();

   protected slots:
    void sliderMoved(int p);
    void sliderValueChanged(int p);
    void spinboxValueChanged(double);
    void checkBox1Checked(bool);
    void checkBox2Checked(bool);
    void radioButton1Checked(bool);
    void radioButton2Checked(bool);

   signals:
    void finished();
    void enableUndo(bool);
    void enableRedo(bool);

   private:
    QSlider *s;
    QDoubleSpinBox *dsb;
    QCheckBox *cbx1;
    QCheckBox *cbx2;
    QRadioButton *rb1;
    QRadioButton *rb2;
    bool isCbx1Checked;
    bool isCbx2Checked;
    bool isRb1Checked;
    bool isRb2Checked;
    bool isCbx1Checked_default;
    bool isCbx2Checked_default;
    bool isRb1Checked_default;
    bool isRb2Checked_default;
    float minv;
    float maxv;
    float defaultv;
    bool logscaling;
    float value;
    bool value_from_text;
    bool value_from_slider;
    bool graphics_only;
    bool changed_;
    bool undoState;
    bool redoState;
    TmoSettingsList *tmoSettingsList;
};

inline float Gang::v() const { return value; }

inline bool Gang::isCheckBox1Checked() const { return isCbx1Checked; }

inline bool Gang::isCheckBox2Checked() const { return isCbx2Checked; }

inline bool Gang::isRadioButton1Checked() const { return isRb1Checked; }

inline bool Gang::isRadioButton2Checked() const { return isRb2Checked; }

inline bool Gang::changed() const { return changed_; }

//
//==================================== Undo/Redo
//===================================================
//
// TmoSettings stores current applied settings
//
class TmoSettings {
   public:
    TmoSettings(Gang *gangPtr, float, bool, bool, bool, bool);
    void apply() const;

   protected:
    Gang *gangPtr;
    bool isCbx1Checked;
    bool isCbx2Checked;
    bool isRb1Checked;
    bool isRb2Checked;
    float value;
};

class TmoSettingsList : public QList<TmoSettings> {
   public:
    TmoSettingsList();
    ~TmoSettingsList();
    void previous();
    void next();
    int index();
    void append(const TmoSettings &value);

   private:
    int m_index;
};

#endif
