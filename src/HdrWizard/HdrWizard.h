/**
 * This file is a part of Luminance HDR package.
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
 * Original Work
 * @author Giuseppe Rota <grota@users.sourceforge.net>
 * Improvements, bugfixing
 * @author Franco Comida <fcomida@users.sourceforge.net>
 *
 */

#ifndef HDRWIZARD_IMPL_H
#define HDRWIZARD_IMPL_H

#include <QDialog>
#include <QString>

#include "ui_HdrWizard.h"
#include "Common/options.h"
#include "Common/Gang.h"
#include "Common/global.h"
#include "Libpfs/pfs.h"
#include "arch/freebsd/math.h"
#include "HdrCreation/HdrCreationManager.h"

class HdrWizard : public QDialog, private Ui::HdrWizard
{
Q_OBJECT

public:
	HdrWizard(QWidget *parent, QStringList files);
	~HdrWizard();
	pfs::Frame* getPfsFrameHDR() {return PfsFrameHDR;}
	QString getCaptionTEXT();

protected:
	void resizeEvent(QResizeEvent *);
	void keyPressEvent(QKeyEvent *);
	virtual void dragEnterEvent(QDragEnterEvent *);
	virtual void dropEvent(QDropEvent *);

private:
	QString getQStringFromConfig( int type );
	void loadInputFiles(QStringList files, int count);
	LuminanceOptions *luminance_options;

	Gang *EVgang;

	HdrCreationManager *hdrCreationManager;

	//the new hdr, returned by the HdrCreationManager class
	pfs::Frame* PfsFrameHDR;
	QString loadcurvefilename,savecurvefilename;

	//hdr creation parameters
	TResponse responses_in_gui[4];
	TModel models_in_gui[2];
	TWeight weights_in_gui[3];

private slots:

	void fileLoaded(int index, QString fname, float expotime);
	void finishedLoadingInputFiles(QStringList NoExifFiles);
	void errorWhileLoading(QString errormessage);

	void updateGraphicalEVvalue(float expotime, int index_in_table);
	void finishedAligning();

	void loadImagesButtonClicked();
	void removeImageButtonClicked();
	void clearListButtonClicked();
	void inputHdrFileSelected(int);
	void predefConfigsComboBoxActivated(int);
	void antighostRespCurveComboboxActivated(int);
	void customConfigCheckBoxToggled(bool);
	void triGaussPlateauComboBoxActivated(int);
	void predefRespCurveRadioButtonToggled(bool);
	void gammaLinLogComboBoxActivated(int);
	void loadRespCurveFromFileCheckboxToggled(bool);
	void loadRespCurveFileButtonClicked();
	void saveRespCurveToFileCheckboxToggled(bool);
	void saveRespCurveFileButtonClicked();
	void modelComboBoxActivated(int);
	void NextFinishButtonClicked();
	void currentPageChangedInto(int);
	void loadRespCurveFilename(const QString&);
	void editingEVfinished();
	void reject();
	void ais_failed(QProcess::ProcessError);
	void writeAisData(QByteArray data);
	void setupConnections();
};
#endif
