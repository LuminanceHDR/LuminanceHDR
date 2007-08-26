/**
 * This file is a part of Qtpfsgui package.
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
 * @author Giuseppe Rota <grota@users.sourceforge.net>
 */

#ifndef hdrwizardform_impl_h
#define hdrwizardform_impl_h

#include <QDialog>
#include <QString>
#include <QSettings>
#include <image.hpp>
#include <exif.hpp>
#include "../generated_uic/ui_hdrwizardform.h"
#include "hdrcreation/createhdr.h"
#include "../options.h"
#include "../ToneMappingDialog/gang.h"
#include <QProcess>

class HdrWizardForm : public QDialog, private Ui::HdrWizardForm
{
Q_OBJECT

public:
	HdrWizardForm(QWidget *parent, qtpfsgui_opts *options);
	~HdrWizardForm();
	pfs::Frame* getPfsFrameHDR();
	QString getCaptionTEXT();

protected:
	void resizeEvent ( QResizeEvent * );
private:
	QString curvefilename;
	config_triple chosen_config;
	TResponse responses_in_gui[4];
	TModel models_in_gui[2];
	TWeight weights_in_gui[3];
	QString getQStringFromConfig( int type );
	void clearlists();
	int numberinputfiles; //it is also the lenght of the array below
	float *expotimes;
	bool need_to_transform_indices;
	Gang *EVgang;

	QList<QImage*> ImagePtrList;  //ldr input
	QList<bool> ldr_tiff_input;  //ldr input
	Array2DList listhdrR,listhdrG,listhdrB; //hdr input
	qtpfsgui_opts *opts;
	QSettings settings;
	QStringList fileStringList;
	QProcess *ais;
	pfs::Frame* PfsFrameHDR;

	bool check_same_size(int&,int&,int,int);

private slots:
	void nextpressed();
	void backpressed();
	void currentPageChangedInto(int);
	void update_current_config_file_or_notfile(bool);
	void update_current_config_model(int);
	void update_current_config_gamma_lin_log(int);
	void update_current_config_weights(int);
	void update_currentconfig(int);
	void update_current_antighost_curve(int);
	void load_response_curve_from_file();
	void loadfiles();
	void fix_gui_custom_config();
	void custom_toggled(bool);
	void update_current_config_calibrate();
	void setLoadFilename(const QString&);
	void updateEVvalue();
	void fileselected(int);
	void ais_failed(QProcess::ProcessError);
	void ais_finished(int,QProcess::ExitStatus);
};
#endif
