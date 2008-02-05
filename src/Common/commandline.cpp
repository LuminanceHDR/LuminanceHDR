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

#include <getopt.h>
#include <QSettings>
#include "options.h"
#include "config.h"
#include "../HdrCreation/createhdr.h"
#include "commandline.h"
#include "../Threads/loadHdrThread.h"
#include "../Threads/tonemapperThread.h"
#include "../Exif/exif_operations.h"

//save hdr
#include "../Fileformat/pfstiff.h"
void writeRGBEfile (pfs::Frame* inputpfsframe, const char* outfilename);
void writeEXRfile  (pfs::Frame* inputpfsframe, const char* outfilename);

#ifdef WIN32
#define error(Z) { fprintf(stderr,Z); exit(1); }
#else
#include <error.h>
#define error(Z) error(1,0,Z);
#endif


static struct option cmdLineOptions[] = {
	{ "verbose", no_argument, NULL, 'v' },
	{ "help", no_argument, NULL, 'h' },
	{ "align", required_argument, NULL, 'a' },
	{ "ev", required_argument, NULL, 'e' },
	{ "config", required_argument, NULL, 'c' },
	{ "load", required_argument, NULL, 'l' },
	{ "gamma", required_argument, NULL, 'g' },
	{ "resize", required_argument, NULL, 'r' },
	{ "save", required_argument, NULL, 's' },
	{ "tmo", required_argument, NULL, 't' },
	{ "tmoptions", required_argument, NULL, 'p' },
	{ "output", required_argument, NULL, 'o' },
	{ NULL, 0, NULL, 0 }
};

CommandLineInterfaceManager::CommandLineInterfaceManager(const int argc, char **argv) : argc(argc), argv(argv) {
	hdrCreationManager=NULL;
	align_mode=NO_ALIGN;
	qtpfsgui_options=new qtpfsgui_opts();
	QtPfsGuiOptions::loadOptions(qtpfsgui_options);
	connect(this,SIGNAL(startParsing()),this,SLOT(parseArgs()),Qt::QueuedConnection);
	emit startParsing();
}

void CommandLineInterfaceManager::parseArgs() {
	operation_mode=UNKNOWN_MODE;

	verbose = false;
	config_triple hdrcreationconfig;
	hdrcreationconfig.weights=TRIANGULAR;
	hdrcreationconfig.response_curve=LINEAR;
	hdrcreationconfig.model=DEBEVEC;
	QString loadHdrFilename;
	tmopts=TMOptionsOperations::getDefaultTMOptions();
	QStringList inputFiles;

	int optionIndex = 0, c;
	while( (c=getopt_long (argc, argv, ":hva:e:c:l:s:g:r:t:p:o:", cmdLineOptions, &optionIndex)) != -1 ) {
		switch( c ) {
			case 'h':
				printHelp(argv[0]);
				exit(0);
			case 'v':
				verbose = true;
				break;
			case 'a':
				if (strcmp(optarg,"AIS")==0)
					align_mode=AIS_ALIGN;
				else if (strcmp(optarg,"MTB")==0)
					align_mode=MTB_ALIGN;
				else
					error(qPrintable(tr("Error: Alignment engine not recognized.")));
				break;
			case 'e': {
				QStringList evstringlist=QString(optarg).split(",");
				for (int i=0; i<evstringlist.count(); i++)
					ev.append(toFloatWithErrMsg(evstringlist.at(i)));
				}
				break;
			case 'c': {
				QStringList hdrCreationOptsList=QString(optarg).split(":");
				  for(int i=0; i<hdrCreationOptsList.size(); i++) {
				    QStringList keyandvalue = hdrCreationOptsList[i].split("=");

				    if (keyandvalue.size()!=2)
				      error(qPrintable(tr("Error: Wrong HDR creation format")));

				    if (keyandvalue.at(0)== "weight") {
				      if (keyandvalue.at(1)== "triangular")
				        hdrcreationconfig.weights=TRIANGULAR;
				      else if (keyandvalue.at(1)== "gaussian")
				        hdrcreationconfig.weights=GAUSSIAN;
				      else if (keyandvalue.at(1)== "plateau")
				        hdrcreationconfig.weights=PLATEAU;
				      else
				        error(qPrintable(tr("Error: Unknown weight function specified.")));
				    }

				    else if (keyandvalue.at(0)== "response_curve") {
				      if (keyandvalue.at(1)== "from_file")
				        hdrcreationconfig.response_curve=FROM_FILE;
				      else if (keyandvalue.at(1)== "linear")
				        hdrcreationconfig.response_curve=LINEAR;
				      else if (keyandvalue.at(1)== "gamma")
				        hdrcreationconfig.response_curve=GAMMA;
				      else if (keyandvalue.at(1)== "log")
				        hdrcreationconfig.response_curve=LOG10;
				      else if (keyandvalue.at(1)== "robertson")
				        hdrcreationconfig.response_curve=FROM_ROBERTSON;
				      else
				        error(qPrintable(tr("Error: Unknown response curve specified.")));
				    }

				    else if (keyandvalue.at(0)== "model") {
				      if (keyandvalue.at(1)== "robertson")
				        hdrcreationconfig.model=ROBERTSON;
				      else if (keyandvalue.at(1)== "debevec")
				        hdrcreationconfig.model=DEBEVEC;
				      else
				        error(qPrintable(tr("Error: Unknown HDR creation model specified.")));
				    }

				    else if (keyandvalue.at(0)== "curve_filename")
				      hdrcreationconfig.LoadCurveFromFilename=keyandvalue.at(1);

				    else
				      error(qPrintable(tr("Error: Unknown HDR creation format specified.")));

				  } //end for loop over all "key=values"
				}
				break;
			case 'l':
				loadHdrFilename=QString(optarg);
				break;
			case 's':
				saveHdrFilename=QString(optarg);
				break;
			case 'r':
				tmopts->xsize=toIntWithErrMsg(optarg);
				break;
			case 'g':
				tmopts->pregamma=toFloatWithErrMsg(optarg);
				break;
			case 't': {
				QString tmoperator=QString(optarg);
				if (tmoperator=="ashikhmin")
					tmopts->tmoperator=ashikhmin;
				else if (tmoperator=="drago")
					tmopts->tmoperator=drago;
				else if (tmoperator=="durand")
					tmopts->tmoperator=durand;
				else if (tmoperator=="fattal")
					tmopts->tmoperator=fattal;
				else if (tmoperator=="pattanaik")
					tmopts->tmoperator=pattanaik;
				else if (tmoperator=="reinhard02")
					tmopts->tmoperator=reinhard02;
				else if (tmoperator=="reinhard05")
					tmopts->tmoperator=reinhard05;
				else if (tmoperator=="mantiuk")
					tmopts->tmoperator=mantiuk;
				else
					error(qPrintable(tr("Error: Unknown tone mapping operator specified.")));
				}
				break;
			case 'p': {
				QStringList tmOptsList=QString(optarg).split(":");
				  for(int i=0; i<tmOptsList.size(); i++) {
				    QStringList keyandvalue = tmOptsList[i].split("=");

				    if (keyandvalue.size()!=2)
				      error(qPrintable(tr("Error: Wrong tone mapping option format.")));

				    //fattal options
				    if (keyandvalue.at(0)== "alpha")
				      tmopts->operator_options.fattaloptions.alpha=toFloatWithErrMsg(keyandvalue.at(1));
				    else if (keyandvalue.at(0)== "beta")
				      tmopts->operator_options.fattaloptions.beta=toFloatWithErrMsg(keyandvalue.at(1));
				    else if (keyandvalue.at(0)== "color")
				      tmopts->operator_options.fattaloptions.color=toFloatWithErrMsg(keyandvalue.at(1));
				    else if (keyandvalue.at(0)== "noise")
				      tmopts->operator_options.fattaloptions.noiseredux=toFloatWithErrMsg(keyandvalue.at(1));
				    else if (keyandvalue.at(0)== "new")
				      tmopts->operator_options.fattaloptions.newfattal=(keyandvalue.at(1)=="true");

				    //mantiuk options
				    else if (keyandvalue.at(0)== "contrast")
				      tmopts->operator_options.mantiukoptions.contrastfactor=toFloatWithErrMsg(keyandvalue.at(1));
				    else if (keyandvalue.at(0)== "saturation")
				      tmopts->operator_options.mantiukoptions.saturationfactor=toFloatWithErrMsg(keyandvalue.at(1));
				    else if (keyandvalue.at(0)== "equalization")
				      tmopts->operator_options.mantiukoptions.contrastequalization=(keyandvalue.at(1)=="true");

				    //ashikhmin options
				    else if (keyandvalue.at(0)== "localcontrast")
				      tmopts->operator_options.ashikhminoptions.lct=toFloatWithErrMsg(keyandvalue.at(1));
				    else if (keyandvalue.at(0)== "eq")
				      tmopts->operator_options.ashikhminoptions.eq2=(keyandvalue.at(1)=="2");
				    else if (keyandvalue.at(0)== "simple")
				      tmopts->operator_options.ashikhminoptions.simple=(keyandvalue.at(1)=="true");

				    //durand options
				    else if (keyandvalue.at(0)== "sigma_s")
				      tmopts->operator_options.durandoptions.spatial=toFloatWithErrMsg(keyandvalue.at(1));
				    else if (keyandvalue.at(0)== "sigma_r")
				      tmopts->operator_options.durandoptions.range=toFloatWithErrMsg(keyandvalue.at(1));
				    else if (keyandvalue.at(0)== "base")
				      tmopts->operator_options.durandoptions.base=toFloatWithErrMsg(keyandvalue.at(1));

				    //drago options
				    else if (keyandvalue.at(0)== "bias")
				      tmopts->operator_options.dragooptions.bias=toFloatWithErrMsg(keyandvalue.at(1));

				    //pattanaik options
				    else if (keyandvalue.at(0)== "local")
				      tmopts->operator_options.pattanaikoptions.local=(keyandvalue.at(1)=="true");
				    else if (keyandvalue.at(0)== "autolum")
				      tmopts->operator_options.pattanaikoptions.autolum=(keyandvalue.at(1)=="true");
				    else if (keyandvalue.at(0)== "cone")
				      tmopts->operator_options.pattanaikoptions.cone=toFloatWithErrMsg(keyandvalue.at(1));
				    else if (keyandvalue.at(0)== "rod")
				      tmopts->operator_options.pattanaikoptions.rod=toFloatWithErrMsg(keyandvalue.at(1));
				    else if (keyandvalue.at(0)== "multiplier")
				      tmopts->operator_options.pattanaikoptions.multiplier=toFloatWithErrMsg(keyandvalue.at(1));

				    //reinhard02 options
				    else if (keyandvalue.at(0)== "scales")
				      tmopts->operator_options.reinhard02options.scales=(keyandvalue.at(1)=="true");
				    else if (keyandvalue.at(0)== "key")
				      tmopts->operator_options.reinhard02options.key=toFloatWithErrMsg(keyandvalue.at(1));
				    else if (keyandvalue.at(0)== "phi")
				      tmopts->operator_options.reinhard02options.phi=toFloatWithErrMsg(keyandvalue.at(1));
				    else if (keyandvalue.at(0)== "num")
				      tmopts->operator_options.reinhard02options.range=toIntWithErrMsg(keyandvalue.at(1));
				    else if (keyandvalue.at(0)== "low")
				      tmopts->operator_options.reinhard02options.lower=toIntWithErrMsg(keyandvalue.at(1));
				    else if (keyandvalue.at(0)== "high")
				      tmopts->operator_options.reinhard02options.upper=toIntWithErrMsg(keyandvalue.at(1));

				    //reinhard05 options
				    else if (keyandvalue.at(0)== "brightness")
				      tmopts->operator_options.reinhard05options.brightness=toFloatWithErrMsg(keyandvalue.at(1));
				    else if (keyandvalue.at(0)== "chroma")
				      tmopts->operator_options.reinhard05options.chromaticAdaptation=toFloatWithErrMsg(keyandvalue.at(1));
				    else if (keyandvalue.at(0)== "lightness")
				      tmopts->operator_options.reinhard05options.lightAdaptation=toFloatWithErrMsg(keyandvalue.at(1));

				    else
					error(qPrintable(tr("Error: Unknown tone mapping option specified.")));
				  } //end for loop over all "key=values"
				}
				break;
			case 'o':
				saveLdrFilename=QString(optarg);
				break;
			case '?':
				error(qPrintable(tr("Error: Unknown option %1.").arg(optopt)));
			case ':':
				error(qPrintable(tr("Error: Missing argument for %1.").arg(optopt)));
		}
	}
	for (int index = optind; index < argc; index++) {
		inputFiles << QString(argv[index]);
		qDebug("Input file %s", argv[index]);
	}

	if (!ev.isEmpty() && ev.count()!=inputFiles.count())
		error(qPrintable(tr("Error: The number of EV values specified is different from the number of input files.")));

	//now validate operation mode.
	if (inputFiles.size()!=0 && loadHdrFilename.isEmpty())
		operation_mode=CREATE_HDR_MODE;
	else if (!loadHdrFilename.isEmpty() && inputFiles.size()==0 )
		operation_mode=LOAD_HDR_MODE;
	else {
		printHelp(argv[0]);
		error("Wrong combination of parameters.");
	}

	if (operation_mode==CREATE_HDR_MODE) {
		QSettings settings("Qtpfsgui", "Qtpfsgui");
		QString temppath = settings.value(KEY_TEMP_RESULT_PATH, QDir::currentPath()).toString();
		int numthreads=settings.value(KEY_NUM_BATCH_THREADS,1).toInt();
		QStringList rawopts=settings.value(KEY_EXTERNAL_DCRAW_OPTIONS,"-T").toStringList();
		hdrCreationManager = new HdrCreationManager(numthreads, temppath, rawopts);
		connect(hdrCreationManager,SIGNAL(finishedLoadingInputFiles(QStringList)),this, SLOT(finishedLoadingInputFiles(QStringList)));
		connect(hdrCreationManager,SIGNAL(errorWhileLoading(QString)),this, SLOT(errorWhileLoading(QString)));
		connect(hdrCreationManager, SIGNAL(finishedAligning()), this, SLOT(createHDR()));
		connect(hdrCreationManager, SIGNAL(ais_failed(QProcess::ProcessError)), this, SLOT(ais_failed(QProcess::ProcessError)));
		hdrCreationManager->setFileList(inputFiles);
		hdrCreationManager->loadInputFiles();
	}
	else {
		LoadHdrThread *loadthread = new LoadHdrThread(loadHdrFilename, "", qtpfsgui_options);
		connect(loadthread, SIGNAL(finished()), loadthread, SLOT(deleteLater()));
		connect(loadthread, SIGNAL(hdr_ready(pfs::Frame*,QString)), this, SLOT(loadFinished(pfs::Frame*,QString)));
		connect(loadthread, SIGNAL(load_failed(QString)), this, SLOT(errorWhileLoading(QString)));
		loadthread->start();
	}
}

void CommandLineInterfaceManager::loadFinished(pfs::Frame* hdr, QString /*fname*/) {
	HDR=hdr;
	saveHDR();
}

void CommandLineInterfaceManager::finishedLoadingInputFiles(QStringList filesLackingExif) {
	if (filesLackingExif.size()!=0) {
		if (!ev.isEmpty())
			for(int i=0; i<ev.size(); i++)
				hdrCreationManager->setEV(ev.at(i),i);
		else
			error(qPrintable(tr("Error: Exif data missing in images and EV values not specifed on the commandline, bailing out.")));
	}
	hdrCreationManager->checkEVvalues();
	if (align_mode==AIS_ALIGN)
		hdrCreationManager->align_with_ais();
	else if (align_mode==MTB_ALIGN)
		hdrCreationManager->align_with_mtb();
	else if (align_mode==NO_ALIGN)
		createHDR();
}

void CommandLineInterfaceManager::errorWhileLoading(QString errormessage) {
	error(qPrintable(errormessage));
	exit(1);
}
void CommandLineInterfaceManager::ais_failed(QProcess::ProcessError) {
	errorWhileLoading("Failed executing align_image_stack");
}

void CommandLineInterfaceManager::createHDR() {
	hdrCreationManager->removeTempFiles();
	HDR=hdrCreationManager->createHdr(false,1);
	saveHDR();
}

void CommandLineInterfaceManager::saveHDR() {
	if (!saveHdrFilename.isEmpty()) {
		QFileInfo qfi(saveHdrFilename);
		if (qfi.suffix().toUpper()=="EXR") {
			writeEXRfile(HDR,qfi.filePath().toUtf8().constData());
		} else if (qfi.suffix().toUpper()=="HDR") {
			writeRGBEfile(HDR, qfi.filePath().toUtf8().constData());
		} else if (qfi.suffix().toUpper().startsWith("TIF")) {
			TiffWriter tiffwriter(qfi.filePath().toUtf8().constData(), HDR);
			if (qtpfsgui_options->saveLogLuvTiff)
				tiffwriter.writeLogLuvTiff();
			else
				tiffwriter.writeFloatTiff();
		} else if (qfi.suffix().toUpper()=="PFS") {
			pfs::DOMIO pfsio;
			HDR->convertRGBChannelsToXYZ();
			pfsio.writeFrame(HDR,qfi.filePath());
			HDR->convertXYZChannelsToRGB();
		} else {
			error("Error, please specify a supported HDR file formats.");
		}
	}

	startTonemap();
}

void  CommandLineInterfaceManager::startTonemap() {
	if (!saveLdrFilename.isEmpty()) {
		//now check if user wants to resize (create thread with either -2 or true original size as first argument in ctor, see options.cpp).
		int origxsize= (tmopts->xsize==-2) ? -2 : HDR->getWidth();
		TonemapperThread *thread = new TonemapperThread(origxsize, *tmopts);
		connect(thread, SIGNAL(ImageComputed(const QImage&,tonemapping_options*)), this, SLOT(tonemapTerminated(const QImage&,tonemapping_options*)));
	} else {
		emit finishedParsing();
	}
}

void CommandLineInterfaceManager::tonemapTerminated(const QImage& newimage,tonemapping_options*) {
	QFileInfo qfi(saveLdrFilename);
	if (!newimage.save(saveLdrFilename, qfi.suffix().toAscii().constData(), 100)) {
		error(qPrintable(tr("ERROR: Cannot save to file: %1").arg(saveLdrFilename)));
	} else {
		TMOptionsOperations operations(tmopts);
		//ExifOperations methods want a std::string, we need to use the QFile::encodeName(QString).constData() trick to cope with utf8 characters.
		ExifOperations::writeExifData(QFile::encodeName(saveLdrFilename).constData(),operations.getExifComment().toStdString());
	}
	emit finishedParsing();
}

float CommandLineInterfaceManager::toFloatWithErrMsg(const QString &str) {
	bool ok;
	float ret = str.toFloat(&ok);
	if (!ok) {
		QString errmessage=tr("Cannot convert %1 to a float").arg(str);
		error(qPrintable(errmessage));
	}
	return ret;
}

int CommandLineInterfaceManager::toIntWithErrMsg(const QString &str) {
	bool ok;
	int ret = str.toInt(&ok);
	if (!ok) {
		QString errmessage=tr("Cannot convert %1 to an integer").arg(str);
		error(qPrintable(errmessage));
	}
	return ret;
}

void CommandLineInterfaceManager::printHelp(char * progname) {
	QString help=tr("Usage: %1 [OPTIONS]... [INPUTFILES]...\n \
	Commandline interface to %2.\n\n \
	-h --help              Display this help.\n \
	-v --verbose           Print more messages during execution.\n\
	-a --align AIS|MTB     Align Engine to use during HDR creation (default: no alignment).\n \
	-e --ev EV1,EV2,...    Specify numerical EV values (as many as INPUTFILES).\n \
	-c --config            HDR creation config. Possible values: \n\
		weight=triangular|gaussian|plateau:response_curve=from_file|linear|gamma|log|robertson:model=robertson|debevec:curve_filename=your_file_here.m \n\
		(Default is triangular,linear,debevec) \n\
	-l --load HDR_FILE     Load an HDR instead of creating a new one. \n \
	-s --save HDR_FILE     Save to a HDR file format. \n \
	-g --gamma VALUE       Gamma value to use during tone mapping. \n \
	-r --resize VALUE      Width want to resize your HDR to (resized before gamma and tone mapping) \n \
	-t --tmo               Tone mapping operator. Possible values: \n\
		ashikhmin|drago|durand|fattal|pattanaik|reinhard02|reinhard05|mantiuk\n \
		(Default is mantiuk)\n \
	-p --tmoptions         Tone mapping operator options. Possible values: \n\
		alpha=VALUE:beta=VALUE:color=VALUE:noise=VALUE:new=true|false (for fattal)\n\
		contrast=VALUE:saturation=VALUE:equalization=true|false (for mantiuk)\n\
		localcontrast=VALUE:eq=2|4:simple=true|false (for ashikhmin)\n\
		sigma_s=VALUE:sigma_r=VALUE:base=VALUE (for durand)\n\
		bias=VALUE (for drago)\n\
		local=true|false:autolum=true|false:cone=VALUE:rod=VALUE:multiplier=VALUE (for pattanaik)\n\
		scales=true|false:key=VALUE:phi=VALUE:num=VALUE:low=VALUE:high=VALUE (for reinhard02)\n\
		brightness=VALUE:chroma=VALUE:lightness=VALUE (for reinhard05)\n\
		(Default is contrast=0.3:equalization=false:saturation=1.8)\n \
	-o --output LDR_FILE   File name you want to save your tone mapped LDR to.\n \
	\n\
You must either load an existing HDR file (via the -l option) or specify INPUTFILES to create a new one.\n").arg(progname).arg(progname);
	fprintf(stderr,qPrintable(help));
}
