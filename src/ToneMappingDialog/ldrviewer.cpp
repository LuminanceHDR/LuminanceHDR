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
 *
 */
#include <QVBoxLayout>
#include "ldrviewer.h"
#include "gamma_and_levels.h"
#include "../config.h"

LdrViewer::LdrViewer(QWidget *parent, const QImage& o, tonemapping_options *opts) : QWidget(parent),origimage(o) {
	currentimage=&origimage;
	setAttribute(Qt::WA_DeleteOnClose);

	QVBoxLayout *VBL_L = new QVBoxLayout(this);
	VBL_L->setSpacing(0);
	VBL_L->setMargin(0);

	imageLabel = new QLabel;
	imageLabel->setPixmap(QPixmap::fromImage(origimage));
	scrollArea = new SmartScrollArea(this,imageLabel);
	VBL_L->addWidget(scrollArea);

	parseOptions(opts);
	setWindowTitle(caption);
	setToolTip(caption);
}

LdrViewer::~LdrViewer() {
	//TODO evaluate possible delete QImage*
	delete imageLabel;
	delete scrollArea;
}

void LdrViewer::parseOptions(tonemapping_options *opts) {
	postfix=QString("pregamma_%1_").arg(opts->pregamma);
	caption=QString("PreGamma=%1 ~ ").arg(opts->pregamma);
	exif_comment="Qtpfsgui "QTPFSGUIVERSION" tonemapping parameters:\n";
	exif_comment+="Operator: ";
	
	switch (opts->tmoperator) {
	case mantiuk: {
		float contrastfactor=opts->operator_options.mantiukoptions.contrastfactor;
		float saturationfactor=opts->operator_options.mantiukoptions.saturationfactor;
		bool contrast_eq=opts->operator_options.mantiukoptions.contrastequalization;
		caption+="Mantiuk: ~ ";
		postfix+="mantiuk_";
		exif_comment+="Mantiuk\nParameters:\n";
		if (contrast_eq) {
			postfix+="contrast_equalization_";
			caption+="Contrast Equalization ~ ";
			exif_comment+="Contrast Equalization\n";
		} else {
			postfix+=QString("contrast_mapping_%1_").arg(contrastfactor);
			caption+=QString("Contrast=%1 ~ ").arg(contrastfactor);
			exif_comment+=QString("Contrast Mapping factor: %1\n").arg(contrastfactor);
		}
		postfix+=QString("saturation_factor_%1").arg(saturationfactor);
		caption+=QString("Saturation=%1").arg(saturationfactor);
		exif_comment+=QString("Saturation Factor: %1 \n").arg(saturationfactor);
		}
		break;
	case fattal: {
		float alpha=opts->operator_options.fattaloptions.alpha;
		float beta=opts->operator_options.fattaloptions.beta;
		float saturation2=opts->operator_options.fattaloptions.color;
		float noiseredux=opts->operator_options.fattaloptions.noiseredux;
		if (!opts->operator_options.fattaloptions.newfattal) {
			caption+="V1_";
			postfix+="v1_";
			exif_comment+="V1_";
		}
		caption+="Fattal: ~ ";
		postfix+="fattal_";
		exif_comment+="Fattal\nParameters:\n";
		postfix+=QString("alpha_%1_").arg(alpha);
		caption+=QString("Alpha=%1 ~ ").arg(alpha);
		exif_comment+=QString("Alpha: %1\n").arg(alpha);
		postfix+=QString("beta_%1_").arg(beta);
		caption+=QString("Beta=%1 ~ ").arg(beta);
		exif_comment+=QString("Beta: %1\n").arg(beta);
		postfix+=QString("saturation_%1_").arg(saturation2);
		caption+=QString("Saturation=%1 ~ ").arg(saturation2);
		exif_comment+=QString("Color Saturation: %1 \n").arg(saturation2);
		postfix+=QString("noiseredux_%1").arg(noiseredux);
		caption+=QString("NoiseRedux=%1").arg(noiseredux);
		exif_comment+=QString("Noise Reduction: %1 \n").arg(noiseredux);
		}
		break;
	case ashikhmin: {
		caption+="Ashikhmin: ~ ";
		postfix+="ashikhmin_";
		exif_comment+="Ashikhmin\nParameters:\n";
		if (opts->operator_options.ashikhminoptions.simple) {
			postfix+="-simple";
			caption+="simple";
			exif_comment+="Simple\n";
		} else {
			if (opts->operator_options.ashikhminoptions.eq2) {
				postfix+="-eq2_";
				caption+="Equation 2 ~ ";
				exif_comment+="Equation 2\n";
			} else {
				postfix+="-eq4_";
				caption+="Equation 4 ~ ";
				exif_comment+="Equation 4\n";
			}
			postfix+=QString("local_%1").arg(opts->operator_options.ashikhminoptions.lct);
			caption+=QString("Local=%1").arg(opts->operator_options.ashikhminoptions.lct);
			exif_comment+=QString("Local Contrast value: %1\n").arg(opts->operator_options.ashikhminoptions.lct);
		}
		}
		break;
	case drago: {
		caption+="Drago: ~ ";
		postfix+="drago_";
		exif_comment+="Drago\nParameters:\n";
		postfix+=QString("bias_%1").arg(opts->operator_options.dragooptions.bias);
		caption+=QString("Bias=%1").arg(opts->operator_options.dragooptions.bias);
		exif_comment+=QString("Bias: %1\n").arg(opts->operator_options.dragooptions.bias);
		}
		break;
	case durand: {
		float spatial=opts->operator_options.durandoptions.spatial;
		float range=opts->operator_options.durandoptions.range;
		float base=opts->operator_options.durandoptions.base;
		caption+="Durand: ~ ";
		postfix+="durand_";
		exif_comment+="Durand\nParameters:\n";
		postfix+=QString("spatial_%1_").arg(spatial);
		caption+=QString("Spatial=%1 ~ ").arg(spatial);
		exif_comment+=QString("Spatial Kernel Sigma: %1\n").arg(spatial);
		postfix+=QString("range_%1_").arg(range);
		caption+=QString("Range=%1 ~ ").arg(range);
		exif_comment+=QString("Range Kernel Sigma: %1\n").arg(range);
		postfix+=QString("base_%1").arg(base);
		caption+=QString("Base=%1").arg(base);
		exif_comment+=QString("Base Contrast: %1\n").arg(base);
		}
		break;
	case pattanaik: {
		float multiplier=opts->operator_options.pattanaikoptions.multiplier;
		float cone=opts->operator_options.pattanaikoptions.cone;
		float rod=opts->operator_options.pattanaikoptions.rod;
		caption+="Pattanaik00: ~ ";
		postfix+="pattanaik00_";
		exif_comment+="Pattanaik\nParameters:\n";
		postfix+=QString("mul_%1_").arg(multiplier);
		caption+=QString("Multiplier=%1 ~ ").arg(multiplier);
		exif_comment+=QString("Multiplier: %1\n").arg(multiplier);
		if (opts->operator_options.pattanaikoptions.local) {
			postfix+="local";
			caption+="Local";
			exif_comment+="Local Tone Mapping\n";
		} else if (opts->operator_options.pattanaikoptions.autolum) {
			postfix+="autolum";
			caption+="AutoLuminance";
			exif_comment+="Con and Rod based on image luminance\n";
		} else {
			postfix+=QString("cone_%1_").arg(cone);
			caption+=QString("Cone=%1 ~ ").arg(cone);
			exif_comment+=QString("Cone Level: %1\n").arg(cone);
			postfix+=QString("rod_%1_").arg(rod);
			caption+=QString("Rod=%1 ~ ").arg(rod);
			exif_comment+=QString("Rod Level: %1\n").arg(rod);
		}
		}
		break;
	case reinhard02: {
		float key=opts->operator_options.reinhard02options.key;
		float phi=opts->operator_options.reinhard02options.phi;
		int range=opts->operator_options.reinhard02options.range;
		int lower=opts->operator_options.reinhard02options.lower;
		int upper=opts->operator_options.reinhard02options.upper;
		caption+="Reinhard02: ~ ";
		postfix+="reinhard02_";
		exif_comment+="Reinhard02\nParameters:\n";
		postfix+=QString("key_%1_").arg(key);
		caption+=QString("Key=%1 ~ ").arg(key);
		exif_comment+=QString("Key: %1\n").arg(key);
		postfix+=QString("phi_%1").arg(phi);
		caption+=QString("Phi=%1").arg(phi);
		exif_comment+=QString("Phi: %1\n").arg(phi);
		if (opts->operator_options.reinhard02options.scales) {
			postfix+=QString("_scales_");
			caption+=QString(" ~ Scales: ~ ");
			exif_comment+=QString("Scales\n");
			postfix+=QString("range_%1_").arg(range);
			caption+=QString("Range=%1 ~ ").arg(range);
			exif_comment+=QString("Range: %1\n").arg(range);
			postfix+=QString("lower%1_").arg(lower);
			caption+=QString("Lower=%1 ~ ").arg(lower);
			exif_comment+=QString("Lower: %1\n").arg(lower);
			postfix+=QString("upper%1").arg(upper);
			caption+=QString("Upper=%1").arg(upper);
			exif_comment+=QString("Upper: %1\n").arg(upper);
		}
		}
		break;
	case reinhard04: {
		float brightness = opts->operator_options.reinhard04options.brightness;
		float chromaticAdaptation = opts->operator_options.reinhard04options.chromaticAdaptation;
		float lightAdaptation = opts->operator_options.reinhard04options.lightAdaptation;
		caption+="Reinhard04: ~ ";
		postfix+="reinhard04_";
		exif_comment+="Reinhard04\nParameters:\n";
		postfix+=QString("brightness_%1_").arg(brightness);
		caption+=QString("Brightness=%1 ~ ").arg(brightness);
		exif_comment+=QString("Brightness: %1\n").arg(brightness);
		postfix+=QString("chromatic_adaptation_%1_").arg(chromaticAdaptation);
		caption+=QString("Chromatic Adaptation=%1 ~ ").arg(chromaticAdaptation);
		exif_comment+=QString("Chromatic Adaptation: %1\n").arg(chromaticAdaptation);
		postfix+=QString("light_adaptation_%1").arg(lightAdaptation);
		caption+=QString("Light Adaptation=%1").arg(lightAdaptation);
		exif_comment+=QString("Light Adaptation: %1\n").arg(lightAdaptation);
		}
		break;
	}
	exif_comment+=QString("------\nPreGamma: %1\n").arg(opts->pregamma);
}

QString LdrViewer::getFilenamePostFix() {
	return postfix;
}
void LdrViewer::fitToWindow(bool checked) {
	scrollArea->fitToWindow(checked);
}
bool LdrViewer::getFittingWin() {
	return scrollArea->isFitting();
}
const QImage* LdrViewer::getQImage() {
	return currentimage;
}
QString LdrViewer::getExifComment() {
	return exif_comment;
}

void LdrViewer::LevelsRequested(bool a) {
	assert(a); //a is always true
	qDebug("accessing currentimage");
	//copy original data
	previewimage=currentimage->copy();
	qDebug("constructing");
	GammaAndLevels *levels=new GammaAndLevels(0,origimage);
	qDebug("constructed");
	levels->setAttribute(Qt::WA_DeleteOnClose);
	//when closing levels, inform the Tone Mapping dialog.
	connect(levels,SIGNAL(closing()),this,SIGNAL(levels_closed()));
	//refresh preview when a values changes
	connect(levels,SIGNAL(LUTrefreshed(unsigned char *)),this,SLOT(updatePreview(unsigned char *)));
	//restore original on "cancel"
	connect(levels,SIGNAL(rejected()),this,SLOT(restoreoriginal()));
	levels->setModal(true);
	levels->show();
}

void LdrViewer::updatePreview(unsigned char *LUT) {
	qDebug("LDRVIEWER::UPDATEPREVIEW\n");
	for (int x=0; x<origimage.width(); x++) {
		for (int y=0; y<origimage.height(); y++) {
			QRgb rgb=origimage.pixel(x,y);
			QRgb withgamma = qRgb(LUT[qRed(rgb)],LUT[qGreen(rgb)],LUT[qBlue(rgb)]);
			previewimage.setPixel(x,y,withgamma);
		}
	}
	imageLabel->setPixmap(QPixmap::fromImage(previewimage));
	currentimage=&previewimage;
}

void LdrViewer::restoreoriginal() {
	imageLabel->setPixmap(QPixmap::fromImage(origimage));
	currentimage=&origimage;
}
