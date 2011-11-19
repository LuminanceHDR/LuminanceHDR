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
 * Original Work
 * @author Giuseppe Rota <grota@users.sourceforge.net>
 * Improvements, bugfixing
 * @author Franco Comida <fcomida@users.sourceforge.net>
 *
 */

#if defined(_MSC_VER)
	#include "getopt_win32.h"
#else
	#include <getopt.h>
#endif

#include <QTimer>

#include "Common/global.h"
#include "Common/config.h"
#include "Common/LuminanceOptions.h"
#include "HdrCreation/createhdr.h"
#include "Threads/LoadHdrThread.h"
#include "Exif/ExifOperations.h"
#include "Common/commandline.h"

#include "Libpfs/array2d.h"
#include "Libpfs/colorspace.h"
#include "Libpfs/domio.h"

#include "Fileformat/pfstiff.h"
#include "Fileformat/pfs_file_format.h"

#include "Threads/TMOThread.h"

#if defined(__FreeBSD__) || defined(WIN32) || defined(Q_WS_MAC) || defined(__APPLE__)
#define error(Z) { fprintf(stderr,"%s", Z); exit(1); }

#else
#include <error.h>
#define error(Z) error(1,0,"%s", Z);
#endif

///string is a QString with a %1 in it
#define VERBOSEPRINT( string, argument ) \
if (verbose) { \
	fprintf(stdout, "%s", qPrintable(tr( string "\n" ).arg( argument )) ); \
}

#define NONCLIOPTIONS 31

static struct option cmdLineOptions[] = {
  // Qt options
  { "nograb", no_argument, NULL, 0 },
  { "dograb", no_argument, NULL, 0 },
  { "sync", no_argument, NULL, 0 },
  { "style", required_argument, NULL, 0 },
  { "stylesheet", required_argument, NULL, 0 },
  { "session", required_argument, NULL, 0 },
  { "widgetcount", no_argument, NULL, 0 },
  { "reverse", no_argument, NULL, 0 },
  { "graphicssystem", required_argument, NULL, 0 },
  { "qmljsdebugger", required_argument, NULL, 0 },
  { "display", required_argument, NULL, 0 },
  { "geometry", required_argument, NULL, 0 },
  { "fn", required_argument, NULL, 0 },
  { "font", required_argument, NULL, 0 },
  { "bg", required_argument, NULL, 0 },
  { "background", required_argument, NULL, 0 },
  { "fg", required_argument, NULL, 0 },
  { "foreground", required_argument, NULL, 0 },
  { "btn", required_argument, NULL, 0 },
  { "button", required_argument, NULL, 0 },
  { "name", required_argument, NULL, 0 },
  { "title", required_argument, NULL, 0 },
  { "visual", required_argument, NULL, 0 },
  { "ncols", required_argument, NULL, 0 },
  { "cmap", no_argument, NULL, 0 },
  { "im", no_argument, NULL, 0 },
  { "inputstyle", required_argument, NULL, 0 },
  // Luminance HDR options
	{ "verbose", no_argument, NULL, 'v' },
	{ "help", no_argument, NULL, 'h' },
	{ "gui", no_argument, NULL, 'u' },
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

CommandLineInterfaceManager::CommandLineInterfaceManager(const int argc, char **argv)
    :argc(argc), argv(argv)
{
        hdrCreationManager = NULL;
        align_mode = NO_ALIGN;

	parseArgs();
}

void CommandLineInterfaceManager::parseArgs() {
	operation_mode=UNKNOWN_MODE;

	verbose = false;
	hdrcreationconfig.weights=TRIANGULAR;
	hdrcreationconfig.response_curve=LINEAR;
	hdrcreationconfig.model=DEBEVEC;
	hdrcreationconfig.SaveCurveToFilename="";
        tmopts = TMOptionsOperations::getDefaultTMOptions();
  cliApp = false;
  bool forceGui = false;
	int optionIndex = 0, c;
  QString cliOptions;
  for (int i = NONCLIOPTIONS; cmdLineOptions[i].name != NULL; ++i) {
      cliOptions += cmdLineOptions[i].val;
  }
	while( (c=getopt_long_only (argc, argv, ":hva:e:c:l:s:g:r:t:p:o:u", cmdLineOptions, &optionIndex)) != -1 ) {
    if (cliOptions.indexOf(c) > -1) {
        cliApp = true;
    }
		switch( c ) {
			case 'h':
				printHelp(argv[0]);
				exit(0);
			case 'v':
				verbose = true;
				break;
			case 'u':
				forceGui = true;
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
				      hdrcreationconfig.LoadCurveFromFilename=strdup(QFile::encodeName(keyandvalue.at(1)).constData());

				    else
				      error(qPrintable(tr("Error: Unknown HDR creation format specified.")));

				  } //end for loop over all "key=values"
				}
				break;
			case 'l':
        loadHdrFilename = QString(optarg);
        //loadHdrFilename=QDir::currentPath () + QDir::separator () + QString(optarg);
				break;
			case 's':
				saveHdrFilename = QString(optarg);
				break;
			case 'r':
				tmopts->xsize = toIntWithErrMsg(optarg);
				break;
			case 'g':
				tmopts->pregamma = toFloatWithErrMsg(optarg);
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
				else if (tmoperator=="mantiuk06")
					tmopts->tmoperator=mantiuk06;
				else if (tmoperator=="mantiuk08")
					tmopts->tmoperator=mantiuk08;
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

				    //mantiuk06 options
				    else if (keyandvalue.at(0)== "contrast")
				      tmopts->operator_options.mantiuk06options.contrastfactor=toFloatWithErrMsg(keyandvalue.at(1));
				    else if (keyandvalue.at(0)== "saturation")
				      tmopts->operator_options.mantiuk06options.saturationfactor=toFloatWithErrMsg(keyandvalue.at(1));
            else if (keyandvalue.at(0)== "detail")
              tmopts->operator_options.mantiuk06options.detailfactor=toFloatWithErrMsg(keyandvalue.at(1));
				    else if (keyandvalue.at(0)== "equalization")
				      tmopts->operator_options.mantiuk06options.contrastequalization=(keyandvalue.at(1)=="true");
            
				    //mantiuk08 options
				    else if (keyandvalue.at(0)== "colorsaturation")
				      tmopts->operator_options.mantiuk08options.colorsaturation=toFloatWithErrMsg(keyandvalue.at(1));
				    else if (keyandvalue.at(0)== "contrastenhancement")
				      tmopts->operator_options.mantiuk08options.contrastenhancement=toFloatWithErrMsg(keyandvalue.at(1));
				    else if (keyandvalue.at(0)== "luminancelevel")
				      tmopts->operator_options.mantiuk08options.luminancelevel=toFloatWithErrMsg(keyandvalue.at(1));
				    else if (keyandvalue.at(0)== "setluminance")
				      tmopts->operator_options.mantiuk08options.setluminance=(keyandvalue.at(1)=="true");

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
		cliApp = true;
		inputFiles << QString(argv[index]);
		VERBOSEPRINT("Input file %1" , argv[index]);
	}
	if (forceGui) {
		cliApp = false;
	}
}

void CommandLineInterfaceManager::execCommandLineParams()
{
    QTimer::singleShot(0, this, SLOT(execCommandLineParamsSlot()));
}

void CommandLineInterfaceManager::execCommandLineParamsSlot()
{
	if (!ev.isEmpty() && ev.count()!=inputFiles.count())
		error(qPrintable(tr("Error: The number of EV values specified is different from the number of input files.")));

	//now validate operation mode.
	if (inputFiles.size()!=0 && loadHdrFilename.isEmpty()) {
		operation_mode=CREATE_HDR_MODE;
		VERBOSEPRINT("Running in HDR-creation mode. %1" , "");
	} else if (!loadHdrFilename.isEmpty() && inputFiles.size()==0 ) {
		operation_mode=LOAD_HDR_MODE;
		VERBOSEPRINT("Running in Load-HDR mode. %1" , "");
	} else {
		printHelp(argv[0]);
		error("Wrong combination of parameters.");
	}

	if (operation_mode==CREATE_HDR_MODE) {
		if (verbose) {
                        VERBOSEPRINT("Temporary directory: %1",luminance_options.getTempDir());
                        VERBOSEPRINT("Using %1 threads.", luminance_options.getNumThreads());
		}
		hdrCreationManager = new HdrCreationManager();
		connect(hdrCreationManager,SIGNAL(finishedLoadingInputFiles(QStringList)),this, SLOT(finishedLoadingInputFiles(QStringList)));
		connect(hdrCreationManager,SIGNAL(errorWhileLoading(QString)),this, SLOT(errorWhileLoading(QString)));
		connect(hdrCreationManager, SIGNAL(finishedAligning()), this, SLOT(createHDR()));
		connect(hdrCreationManager, SIGNAL(ais_failed(QProcess::ProcessError)), this, SLOT(ais_failed(QProcess::ProcessError)));
        hdrCreationManager->setConfig(hdrcreationconfig);
		hdrCreationManager->setFileList(inputFiles);
		hdrCreationManager->loadInputFiles();
	}
	else {
		LoadHdrThread *loadthread = new LoadHdrThread(loadHdrFilename, "");
		connect(loadthread, SIGNAL(finished()), loadthread, SLOT(deleteLater()));
		connect(loadthread, SIGNAL(hdr_ready(pfs::Frame*,QString)), this, SLOT(loadFinished(pfs::Frame*,QString)));
		connect(loadthread, SIGNAL(load_failed(QString)), this, SLOT(errorWhileLoading(QString)));
		loadthread->start();
		VERBOSEPRINT("Loading file %1",loadHdrFilename);
	}
}

void CommandLineInterfaceManager::loadFinished(pfs::Frame* hdr, QString fname) {
	VERBOSEPRINT("Successfully loaded file %1.",fname);
	HDR=hdr;
	saveHDR();
}

void CommandLineInterfaceManager::finishedLoadingInputFiles(QStringList filesLackingExif) {
        if (filesLackingExif.size()!=0 && ev.isEmpty()) {//else
		error(qPrintable(tr("Error: Exif data missing in images and EV values not specifed on the commandline, bailing out.")));
	}	
	if (!ev.isEmpty()) {
		for(int i=0; i<ev.size(); i++)
			hdrCreationManager->setEV(ev.at(i),i);
		VERBOSEPRINT("EV values have been assigned. %1","");
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
	errorWhileLoading(tr("Failed executing align_image_stack"));
}

void CommandLineInterfaceManager::createHDR() {
	hdrCreationManager->removeTempFiles();
	VERBOSEPRINT("Creating (in memory) the HDR. %1","");
	HDR=hdrCreationManager->createHdr(false,1);
	saveHDR();
}

void CommandLineInterfaceManager::saveHDR()
{
    if (!saveHdrFilename.isEmpty())
    {
        VERBOSEPRINT("Saving to file %1.",saveHdrFilename);
        QFileInfo qfi(saveHdrFilename);
        QByteArray encodedName(QFile::encodeName(qfi.filePath()));
        if (qfi.suffix().toUpper()=="EXR")
        {
            writeEXRfile(HDR, encodedName);
        }
        else if (qfi.suffix().toUpper()=="HDR")
        {
            writeRGBEfile(HDR, encodedName);
        }
        else if (qfi.suffix().toUpper().startsWith("TIF"))
        {
            TiffWriter tiffwriter(encodedName, HDR);
            if (luminance_options.isSaveLogLuvTiff())
            {
                tiffwriter.writeLogLuvTiff();
            }
            else
            {
                tiffwriter.writeFloatTiff();
            }
        }
        else if (qfi.suffix().toUpper()=="PFS")
        {
            // Convert to CS_XYZ:
            pfs::Channel *X, *Y, *Z;
            HDR->getXYZChannels( X, Y, Z );
            pfs::transformColorSpace(pfs::CS_RGB, X->getChannelData(), Y->getChannelData(), Z->getChannelData(),
                                     pfs::CS_XYZ, X->getChannelData(), Y->getChannelData(), Z->getChannelData());
            FILE *fd = fopen(encodedName, "wb");
            pfs::DOMIO pfsio;
            pfsio.writeFrame(HDR, fd);
            fclose(fd);
        }
        else
        {
            error("Error, please specify a supported HDR file format.");
        }
    }
    else
    {
        VERBOSEPRINT("NOT Saving HDR image to file. %1","");
    }

    startTonemap();
}

void  CommandLineInterfaceManager::startTonemap()
{
	if (!saveLdrFilename.isEmpty())
  {
		VERBOSEPRINT("Tonemapping requested, saving to file %1.",saveLdrFilename);
		//now check if user wants to resize (create thread with either -2 or true original size as first argument in ctor, see options.cpp).
		//TODO
		tmopts->origxsize = HDR->getWidth();
		
		std::cout << "XSIZE: " << tmopts->xsize << std::endl;
    
		if (tmopts->xsize == -2)	
			tmopts->xsize = HDR->getWidth();
    
                TMOThread *thread = TMOThread::getTMOThread(tmopts->tmoperator, HDR, tmopts);
		connect(thread, SIGNAL(imageComputed(QImage*, quint16*)), this, SLOT(tonemapTerminated(QImage*, quint16*)));
    
    thread->startTonemapping();
	}
  else
  {
		VERBOSEPRINT("Tonemapping NOT requested. %1","");
		emit finishedParsing();
	}
}

void CommandLineInterfaceManager::tonemapTerminated(QImage* newimage, quint16* pixmap)
{
    delete pixmap;

	QFileInfo qfi(saveLdrFilename);
        if (!newimage->save(saveLdrFilename, qfi.suffix().toLocal8Bit().constData(), 100))
  {
		error(qPrintable(tr("ERROR: Cannot save to file: %1").arg(saveLdrFilename)));
	}
  else
  {
		TMOptionsOperations operations(tmopts);
		//ExifOperations methods want a std::string, we need to use the QFile::encodeName(QString).constData() trick to cope with local 8-bit encoding determined by the user's locale.
		ExifOperations::writeExifData(QFile::encodeName(saveLdrFilename).constData(),operations.getExifComment().toStdString());
	}
	emit finishedParsing();
}

float CommandLineInterfaceManager::toFloatWithErrMsg(const QString &str)
{
	bool ok;
	float ret = str.toFloat(&ok);
	if (!ok)
  {
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

void CommandLineInterfaceManager::printHelp(char * progname)
{
	QString help=
  tr("Usage: %1 [OPTIONS]... [INPUTFILES]...").arg(progname) + "\n" +
  "\t" + tr("Commandline interface to %1.").arg(progname) + "\n\n" +
  "\t" + tr("-h --help              Display this help.") + "\n" +
  "\t" + tr("-v --verbose           Print more messages during execution.") + "\n" +
  "\t" + tr("-u --gui               Start in gui mode with input files.") + "\n" +
  "\t" + tr("-a --align AIS|MTB     Align Engine to use during HDR creation (default: no alignment).") + "\n" +
  "\t" + tr("-e --ev EV1,EV2,...    Specify numerical EV values (as many as INPUTFILES).") + "\n" +
  "\t" + tr("-c --config            HDR creation config. Possible values: ") + "\n" +
  "\t\t" + tr("weight=triangular|gaussian|plateau:response_curve=from_file|linear|gamma|log|robertson:model=robertson|debevec:curve_filename=your_file_here.m") + "\n" +
  "\t\t" + tr("(Default is weight=triangular:response_curve=linear:model=debevec) ") + "\n" +
  "\t" + tr("-l --load HDR_FILE     Load an HDR instead of creating a new one. ") + "\n" +
  "\t" + tr("-s --save HDR_FILE     Save to a HDR file format. (default: don't save) ") + "\n" +
  "\t" + tr("-g --gamma VALUE       Gamma value to use during tone mapping. (default: 1) ") + "\n" +
  "\t" + tr("-r --resize VALUE      Width you want to resize your HDR to (resized before gamma and tone mapping) ") + "\n" +
  "\t" + tr("-t --tmo               Tone mapping operator. Legal values are: ") + "\n" +
  "\t\t" + tr("ashikhmin|drago|durand|fattal|pattanaik|reinhard02|reinhard05|mantiuk06|mantiuk08") + "\n" +
  "\t\t" + tr("(Default is mantiuk06)") + "\n" +
  "\t" + tr("-p --tmoptions         Tone mapping operator options. Legal values are: ") + "\n" +
  "\t\t" + tr("alpha=VALUE:beta=VALUE:color=VALUE:noise=VALUE:new=true|false (for fattal)") + "\n" +
  "\t\t" + tr("contrast=VALUE:saturation=VALUE:detail=VALUE:equalization=true|false (for mantiuk06)") + "\n" +
  "\t\t" + tr("colorsaturation=VALUE:contrastenhancement=VALUE:luminancelevel=VALUE:setluminance=true|false (for mantiuk08)") + "\n" +
  "\t\t" + tr("localcontrast=VALUE:eq=2|4:simple=true|false (for ashikhmin)") + "\n" +
  "\t\t" + tr("sigma_s=VALUE:sigma_r=VALUE:base=VALUE (for durand)") + "\n" +
  "\t\t" + tr("bias=VALUE (for drago)") + "\n" +
  "\t\t" + tr("local=true|false:autolum=true|false:cone=VALUE:rod=VALUE:multiplier=VALUE (for pattanaik)") + "\n" +
  "\t\t" + tr("scales=true|false:key=VALUE:phi=VALUE:num=VALUE:low=VALUE:high=VALUE (for reinhard02)") + "\n" +
  "\t\t" + tr("brightness=VALUE:chroma=VALUE:lightness=VALUE (for reinhard05)") + "\n" +
  "\t\t" + tr("(default is contrast=0.3:equalization=false:saturation=1.8, see also -o)") + "\n" +
  "\t" + tr("-o --output LDR_FILE   File name you want to save your tone mapped LDR to.") + "\n" +
  "\t" + tr("                       (No tonemapping is performed unless -o is specified).") + "\n\n" +
  tr("You must either load an existing HDR file (via the -l option) or specify INPUTFILES to create a new HDR.\n");
	fprintf(stderr,"%s", qPrintable(help));
}
