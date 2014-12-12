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

#include "arch/getopt.h"

#include <QTimer>
#include <QDebug>
#include <iostream>

#include "commandline.h"

#include "Common/LuminanceOptions.h"

#include "Exif/ExifOperations.h"

#include "Core/IOWorker.h"
#include "Core/TMWorker.h"

#include "Libpfs/tm/TonemapOperator.h"

#include <boost/program_options.hpp>

#if defined(_MSC_VER)
#include <fcntl.h>
#include <io.h>
#endif


using namespace libhdr::fusion;

namespace
{
void printErrorAndExit(const QString& error_str)
{
	#if defined(_MSC_VER)
		// if the filemode isn't restored afterwards, a normal std::cout segfaults
		int oldMode = _setmode(_fileno(stderr), _O_U16TEXT);
		std::wcerr << qPrintable(error_str) << std::endl;
		if (oldMode >= 0)
			_setmode(_fileno(stderr), oldMode);
	#else
		std::cerr << qPrintable(error_str) << std::endl;
	#endif
    exit(-1);
}

void printIfVerbose(const QString& str, bool verbose)
{
    if ( verbose )
    {
		#if defined(_MSC_VER)
    		// if the filemode isn't restored afterwards, a normal std::cout segfaults
			int oldMode = _setmode(_fileno(stdout), _O_U16TEXT);
			std::wcout << qPrintable(str) << std::endl;
			if (oldMode >= 0)
				_setmode(_fileno(stdout), oldMode);
		#else
    		std::cout << qPrintable(str) << std::endl;
		#endif
    }
}

float toFloatWithErrMsg(const QString &str)
{
    bool ok;
    float ret = str.toFloat(&ok);
    if (!ok)
    {
        printErrorAndExit(QObject::tr("Cannot convert %1 to a float").arg(str));
    }
    return ret;
}

}

CommandLineInterfaceManager::CommandLineInterfaceManager(const int argc, char **argv):
    argc(argc),
    argv(argv),
    operationMode(UNKNOWN_MODE),
    alignMode(NO_ALIGN),
    tmopts(TMOptionsOperations::getDefaultTMOptions()),
    verbose(false),
	quality(100),
    threshold(-1.0f),
    saveAlignedImagesPrefix("")
{

    hdrcreationconfig.weightFunction = WEIGHT_TRIANGULAR;
    hdrcreationconfig.responseCurve = RESPONSE_LINEAR;
    hdrcreationconfig.fusionOperator = DEBEVEC;
}

int CommandLineInterfaceManager::execCommandLineParams()
{
    // Declare the supported options.
    namespace po = boost::program_options;
    po::variables_map vm;

    bool error = false;

    po::options_description desc(tr("Usage: %1 [OPTIONS]... [INPUTFILES]... ").arg(argv[0]).toUtf8().constData());
    desc.add_options()
        ("help,h", tr("Display this help.").toUtf8().constData())
        ("verbose,v", po::value<bool>(&verbose), tr("Print more messages during execution.").toUtf8().constData())
        ("align,a", po::value<std::string>(),    tr("[AIS|MTB]   Align Engine to use during HDR creation (default: no alignment).").toUtf8().constData())
        ("ev,e", po::value<std::string>(),       tr("EV1,EV2,... Specify numerical EV values (as many as INPUTFILES).").toUtf8().constData())
        ("savealigned,d", po::value<std::string>(),       tr("prefix Save aligned images to files which names start with prefix").toUtf8().constData())
        //
        ("load,l", po::value<std::string>(),       tr("HDR_FILE Load an HDR instead of creating a new one.").toUtf8().constData())
        ("save,s", po::value<std::string>(),       tr("HDR_FILE Save to a HDR file format. (default: don't save)").toUtf8().constData())
        ("gamma,g", po::value<float>(&tmopts->pregamma),       tr("VALUE        Gamma value to use during tone mapping. (default: 1) ").toUtf8().constData())
        ("resize,r", po::value<int>(&tmopts->xsize),       tr("VALUE       Width you want to resize your HDR to (resized before gamma and tone mapping)").toUtf8().constData())

        ("output,o", po::value<std::string>(),       tr("LDR_FILE    File name you want to save your tone mapped LDR to.").toUtf8().constData())
            
        ("quality,q", po::value<int>(&quality),       tr("VALUE      Quality of the saved tone mapped file (0-100).").toUtf8().constData())
        ("autoag,ag", po::value<float>(&threshold),       tr("THRESHOLD   Enable auto anti-ghosting with given threshold. (0.0-1.0)").toUtf8().constData())
    ;

    po::options_description hdr_desc(tr("HDR creation parameters").toUtf8().constData());
    hdr_desc.add_options()
        ("hdrWeight", po::value<std::string>(),       tr("weight = triangular|gaussian|plateau|flat").toUtf8().constData())
        ("hdrResponseCurve", po::value<std::string>(),       tr("response curve = from_file|linear|gamma|log|srgb").toUtf8().constData())
        ("hdrModel", po::value<std::string>(),       tr("model: robertson|robertsonauto|debevec").toUtf8().constData())
        ("hdrCurveFilename", po::value<std::string>(),       tr("curve filename = your_file_here.m").toUtf8().constData())
    ;

    po::options_description tmo_desc(tr("Tone mapping parameters").toUtf8().constData());
    tmo_desc.add_options()
        ("tmo", po::value<std::string>(),       tr("Tone mapping operator. Legal values are: [ashikhmin|drago|durand|fattal|pattanaik|reinhard02|reinhard05|mantiuk06|mantiuk08] (Default is mantiuk06)").toUtf8().constData())
    ;

    po::options_description tmo_fattal(tr(" Fattal").toUtf8().constData());
    tmo_fattal.add_options()
		("tmoFatAlpha", po::value<float>(&tmopts->operator_options.fattaloptions.alpha),  tr("alpha FLOAT").toUtf8().constData())
		("tmoFatBeta", po::value<float>(&tmopts->operator_options.fattaloptions.beta),  tr("beta FLOAT").toUtf8().constData())
		("tmoFatColor", po::value<float>(&tmopts->operator_options.fattaloptions.color),  tr("color FLOAT").toUtf8().constData())
		("tmoFatNoise", po::value<float>(&tmopts->operator_options.fattaloptions.noiseredux),  tr("noise FLOAT").toUtf8().constData())
		("tmoFatNew", po::value<bool>(&tmopts->operator_options.fattaloptions.newfattal), tr("new true|false").toUtf8().constData())
    ;
    po::options_description tmo_mantiuk06(tr(" Mantiuk 06").toUtf8().constData());
    tmo_mantiuk06.add_options()
        ("tmoM06Contrast", po::value<float>(&tmopts->operator_options.mantiuk06options.contrastfactor),  tr("contrast FLOAT").toUtf8().constData())
        ("tmoM06Saturation", po::value<float>(&tmopts->operator_options.mantiuk06options.saturationfactor),  tr("saturation FLOAT").toUtf8().constData())
        ("tmoM06Detail", po::value<float>(&tmopts->operator_options.mantiuk06options.detailfactor),  tr("detail FLOAT").toUtf8().constData())
        ("tmoM06Constrast", po::value<bool>(&tmopts->operator_options.mantiuk06options.contrastequalization), tr("equalization true|false").toUtf8().constData())
    ;
    po::options_description tmo_mantiuk08(tr(" Mantiuk 08").toUtf8().constData());
    tmo_mantiuk08.add_options()
        ("tmoM08ColorSaturation", po::value<float>(&tmopts->operator_options.mantiuk08options.colorsaturation),  tr("").toUtf8().constData())
        ("tmoM08ConstrastEnh", po::value<float>(&tmopts->operator_options.mantiuk08options.contrastenhancement),  tr("").toUtf8().constData())
        ("tmoM08LuminanceLvl", po::value<float>(&tmopts->operator_options.mantiuk08options.luminancelevel),  tr("").toUtf8().constData())
        ("tmoM08SetLuminance", po::value<bool>(&tmopts->operator_options.mantiuk08options.setluminance), tr("").toUtf8().constData())
    ;
    po::options_description tmo_durand(tr(" Durand").toUtf8().constData());
    tmo_durand.add_options()
        ("tmoDurSigmaS", po::value<float>(&tmopts->operator_options.durandoptions.spatial),  tr("").toUtf8().constData())
        ("tmoDurSigmaR", po::value<float>(&tmopts->operator_options.durandoptions.range),  tr("").toUtf8().constData())
        ("tmoDurBase", po::value<float>(&tmopts->operator_options.durandoptions.base),  tr("").toUtf8().constData())
    ;
    po::options_description tmo_drago(tr(" Drago").toUtf8().constData());
    tmo_drago.add_options()
        ("tmoDrgBias", po::value<float>(&tmopts->operator_options.dragooptions.bias),  tr("").toUtf8().constData())
    ;
    po::options_description tmo_reinhard02(tr(" Reinhard 02").toUtf8().constData());
    tmo_reinhard02.add_options()
        ("tmoR02Scales", po::value<bool>(&tmopts->operator_options.reinhard02options.scales), tr("").toUtf8().constData())
        ("tmoR02Key", po::value<float>(&tmopts->operator_options.reinhard02options.key),  tr("").toUtf8().constData())
        ("tmoR02Phi", po::value<float>(&tmopts->operator_options.reinhard02options.phi),  tr("").toUtf8().constData())
        ("tmoR02Num", po::value<int>(&tmopts->operator_options.reinhard02options.range),  tr("").toUtf8().constData())
        ("tmoR02Low", po::value<int>(&tmopts->operator_options.reinhard02options.lower),  tr("").toUtf8().constData())
        ("tmoR02High", po::value<int>(&tmopts->operator_options.reinhard02options.upper),  tr("").toUtf8().constData())
    ;
    po::options_description tmo_reinhard05(tr(" Reinhard 05").toUtf8().constData());
    tmo_reinhard05.add_options()
        ("tmoR05Brightness", po::value<float>(&tmopts->operator_options.reinhard05options.brightness),  tr("").toUtf8().constData())
        ("tmoR05Chroma", po::value<float>(&tmopts->operator_options.reinhard05options.chromaticAdaptation),  tr("").toUtf8().constData())
        ("tmoR05Lightness", po::value<float>(&tmopts->operator_options.reinhard05options.lightAdaptation),  tr("").toUtf8().constData())
    ;
    po::options_description tmo_ash(tr(" Ashikmin").toUtf8().constData());
    tmo_ash.add_options()
        ("tmoAshLocal", po::value<float>(&tmopts->operator_options.ashikhminoptions.lct),  tr("").toUtf8().constData())
        ("tmoAshEq2", po::value<bool>(&tmopts->operator_options.ashikhminoptions.eq2), tr("").toUtf8().constData())
        ("tmoAshSimple", po::value<bool>(&tmopts->operator_options.ashikhminoptions.simple), tr("").toUtf8().constData())
    ;
    po::options_description tmo_patt(tr(" Pattanaik").toUtf8().constData());
    tmo_patt.add_options()
        ("tmoPatLocal", po::value<bool>(&tmopts->operator_options.pattanaikoptions.local), tr("").toUtf8().constData())
        ("tmoPatAutoLum", po::value<bool>(&tmopts->operator_options.pattanaikoptions.autolum), tr("").toUtf8().constData())
        ("tmoPatCone", po::value<float>(&tmopts->operator_options.pattanaikoptions.cone),  tr("").toUtf8().constData())
        ("tmoPatRod", po::value<float>(&tmopts->operator_options.pattanaikoptions.rod),  tr("").toUtf8().constData())
        ("tmoPatMultiplier", po::value<float>(&tmopts->operator_options.pattanaikoptions.multiplier),  tr("").toUtf8().constData())
    ;

    tmo_desc.add(tmo_fattal);
    tmo_desc.add(tmo_mantiuk06);
    tmo_desc.add(tmo_mantiuk08);
    tmo_desc.add(tmo_durand);
    tmo_desc.add(tmo_drago);
    tmo_desc.add(tmo_reinhard02);
    tmo_desc.add(tmo_reinhard05);
    tmo_desc.add(tmo_ash);
    tmo_desc.add(tmo_patt);


    po::options_description hidden("Hidden options");
    hidden.add_options()
        ("input-file", po::value< vector<string> >(), "input file")
        ;    

    po::positional_options_description p;
    p.add("input-file", -1);

    po::options_description cmdline_options;
    cmdline_options.add(desc).add(hdr_desc).add(tmo_desc).add(hidden);

    po::options_description cmdvisible_options;
    cmdvisible_options.add(desc).add(hdr_desc).add(tmo_desc);

    try 
    {
        po::store(po::command_line_parser(argc, argv).options(cmdline_options).positional(p).run(), vm);
        po::notify(vm);    

        if (vm.count("help")) {
            cout << cmdvisible_options << "\n";
            return 1;
        }
        if (vm.count("align")) {
            const char* value = vm["align"].as<std::string>().c_str();
            if (strcmp(value,"AIS")==0)
                alignMode = AIS_ALIGN;
            else if (strcmp(value,"MTB")==0)
                alignMode = MTB_ALIGN;
            else
                printErrorAndExit(tr("Error: Alignment engine not recognized."));
        }
        if (vm.count("ev")) {
            QStringList evstringlist=QString::fromStdString(vm["ev"].as<std::string>()).split(",");
            for (int i=0; i<evstringlist.count(); i++)
                ev.append(toFloatWithErrMsg(evstringlist.at(i)));
        }
        if (vm.count("hdrWeight")) {
            const char* value = vm["hdrWeight"].as<std::string>().c_str();
            if (strcmp(value,"triangular")==0)
            	hdrcreationconfig.weightFunction = WEIGHT_TRIANGULAR;
            else if (strcmp(value,"gaussian")==0)
            	hdrcreationconfig.weightFunction = WEIGHT_GAUSSIAN;
            else if (strcmp(value,"plateau")==0)
            	hdrcreationconfig.weightFunction = WEIGHT_PLATEAU;
            else if (strcmp(value,"flat")==0)
                hdrcreationconfig.weightFunction = WEIGHT_FLAT;
            else
                printErrorAndExit(tr("Error: Unknown weight function specified."));
        }
        if (vm.count("hdrResponseCurve")) {
            const char* value = vm["hdrResponseCurve"].as<std::string>().c_str();
            if (strcmp(value,"from_file")==0)
                hdrcreationconfig.responseCurve = RESPONSE_CUSTOM;
            else if (strcmp(value,"linear")==0)
                hdrcreationconfig.responseCurve = RESPONSE_LINEAR;
            else if (strcmp(value,"gamma")==0)
                hdrcreationconfig.responseCurve = RESPONSE_GAMMA;
            else if (strcmp(value,"log")==0)
                hdrcreationconfig.responseCurve = RESPONSE_LOG10;
            else if (strcmp(value,"srgb")==0)
                hdrcreationconfig.responseCurve = RESPONSE_SRGB;
            else
                printErrorAndExit(tr("Error: Unknown response curve specified."));
        }
        if (vm.count("hdrModel")) {
            const char* value = vm["hdrModel"].as<std::string>().c_str();
            if (strcmp(value,"robertson")==0)
                hdrcreationconfig.fusionOperator = ROBERTSON;
            else if (strcmp(value,"robertsonauto")==0)
                hdrcreationconfig.fusionOperator = ROBERTSON_AUTO;
            else if (strcmp(value,"debevec")==0)
                hdrcreationconfig.fusionOperator = DEBEVEC;
            else
                printErrorAndExit(tr("Error: Unknown HDR creation model specified."));
        }
        if (vm.count("hdrCurveFilename"))
        	hdrcreationconfig.inputResponseCurveFilename = QString::fromStdString(vm["hdrCurveFilename"].as<std::string>());
        
        if (vm.count("tmo")) {
            const char* value = vm["tmo"].as<std::string>().c_str();
            if (strcmp(value,"ashikhmin")==0)
            	tmopts->tmoperator=ashikhmin;
            else if (strcmp(value,"drago")==0)
            	tmopts->tmoperator=drago;
            else if (strcmp(value,"durand")==0)
            	tmopts->tmoperator=durand;
            else if (strcmp(value,"fattal")==0)
            	tmopts->tmoperator=fattal;
            else if (strcmp(value,"pattanaik")==0)
            	tmopts->tmoperator=pattanaik;
            else if (strcmp(value,"reinhard02")==0)
            	tmopts->tmoperator=reinhard02;
            else if (strcmp(value,"reinhard05")==0)
            	tmopts->tmoperator=reinhard05;
            else if (strcmp(value,"mantiuk06")==0)
            	tmopts->tmoperator=mantiuk06;
            else if (strcmp(value,"mantiuk08")==0)
            	tmopts->tmoperator=mantiuk08;
            else
            	printErrorAndExit(tr("Error: Unknown tone mapping operator specified."));
        }

        if (vm.count("load"))
            loadHdrFilename = QString::fromStdString(vm["load"].as<std::string>());
        if (vm.count("save"))
            saveHdrFilename = QString::fromStdString(vm["save"].as<std::string>());
        if (vm.count("output"))
            saveLdrFilename = QString::fromStdString(vm["output"].as<std::string>());
        if (vm.count("savealigned"))
            saveAlignedImagesPrefix = QString::fromStdString(vm["savealigned"].as<std::string>());
        if (quality < 0 || quality > 100)
            printErrorAndExit(tr("Error: Quality must be in the range [0-100]."));
        if (threshold > 1.0f)
            printErrorAndExit(tr("Error: Threshold must be in the range [0-1]."));

    }
    catch(boost::program_options::required_option& e) 
    { 
      std::cerr << "ERROR: " << e.what() << std::endl << std::endl; 
      return 1;
    } 
    catch(boost::program_options::error& e) 
    { 
      std::cerr << "ERROR: " << e.what() << std::endl << std::endl; 
      return 1;
    }

    if (vm.count("input-file"))
    {
        vector<string> options = vm["input-file"].as< vector<string> >();
        for (int i = 0; i < options.size(); i++)
        {
            inputFiles << QString::fromStdString(options[i]);
            printIfVerbose( QObject::tr("Input file %1").arg(QString::fromStdString(options[i])) , verbose);
            cout << options[i] << "\n";
        }
    }

    if (inputFiles.empty()) {
         cout << cmdvisible_options << "\n";
        return 1;
    }
    
    QTimer::singleShot(0, this, SLOT(execCommandLineParamsSlot()));
    return 0;
}

void CommandLineInterfaceManager::execCommandLineParamsSlot()
{
    if (!ev.isEmpty() && ev.count()!=inputFiles.count())
    {
        printErrorAndExit(tr("Error: The number of EV values specified is different from the number of input files."));
    }
    //now validate operation mode.
    if ( inputFiles.size()!=0 && loadHdrFilename.isEmpty() )
    {
        operationMode = CREATE_HDR_MODE;

        printIfVerbose(QObject::tr("Running in HDR-creation mode."), verbose);
    }
    else if (!loadHdrFilename.isEmpty() && inputFiles.size()==0 )
    {
        operationMode = LOAD_HDR_MODE;

        printIfVerbose(QObject::tr("Running in Load-HDR mode."), verbose);
    }
    else
    {
        printHelp(argv[0]);
        exit(-1);
    }

    if (operationMode == CREATE_HDR_MODE)
    {
        if (verbose)
        {
            LuminanceOptions luminance_options;

            printIfVerbose(QObject::tr("Temporary directory: %1").arg(luminance_options.getTempDir()), verbose);
            printIfVerbose(QObject::tr("Using %1 threads.").arg(luminance_options.getNumThreads()), verbose);
        }
        hdrCreationManager.reset( new HdrCreationManager(true) );
        connect(hdrCreationManager.data(), SIGNAL(finishedLoadingFiles()), this, SLOT(finishedLoadingInputFiles()));
        connect(hdrCreationManager.data(), SIGNAL(finishedAligning(int)), this, SLOT(createHDR(int)));
        connect(hdrCreationManager.data(), SIGNAL(ais_failed(QProcess::ProcessError)), this, SLOT(ais_failed(QProcess::ProcessError)));
		connect(hdrCreationManager.data(), SIGNAL(errorWhileLoading(QString)),this, SLOT(errorWhileLoading(QString)));
		connect(hdrCreationManager.data(), SIGNAL(aisDataReady(QByteArray)),this, SLOT(readData(QByteArray)));
			
        hdrCreationManager->setConfig(hdrcreationconfig);
        hdrCreationManager->loadFiles(inputFiles);
    }
    else
    {
        printIfVerbose(QObject::tr("Loading file %1").arg(loadHdrFilename), verbose);

        HDR.reset( IOWorker().read_hdr_frame(loadHdrFilename) );

        if ( HDR != NULL )
        {
            printIfVerbose(QObject::tr("Successfully loaded file %1.").arg(loadHdrFilename), verbose);
            saveHDR();
        }
        else
        {
            printErrorAndExit(tr("Load file %1 failed").arg(loadHdrFilename));
        }
    }
}

void CommandLineInterfaceManager::finishedLoadingInputFiles()
{
    QStringList filesLackingExif = hdrCreationManager->getFilesWithoutExif();
    if (filesLackingExif.size() !=0 && ev.isEmpty())
    {
        printErrorAndExit(tr("Error: Exif data missing in images and EV values not specified on the commandline, bailing out."));
    }
    if (!ev.isEmpty())
    {
        for (int i = 0; i < ev.size(); i++)
            hdrCreationManager->getFile(i).setEV(ev.at(i));

        printIfVerbose( tr("EV values have been assigned.") , verbose);
    }
    //hdrCreationManager->checkEVvalues();
    if (alignMode == AIS_ALIGN)
    {
        hdrCreationManager->align_with_ais();
    }
    else if (alignMode == MTB_ALIGN)
    {
        hdrCreationManager->align_with_mtb();
    }
    else if (alignMode == NO_ALIGN)
    {
        createHDR(0);
    }
}

void CommandLineInterfaceManager::ais_failed(QProcess::ProcessError)
{
    printErrorAndExit( tr("Failed executing align_image_stack"));
}

void CommandLineInterfaceManager::createHDR(int errorcode)
{
	if (errorcode != 0)
		printIfVerbose( tr("Failed aligning images.") , verbose);

    printIfVerbose( tr("Creating (in memory) the HDR.") , verbose);
    
    if (errorcode == 0 && alignMode != NO_ALIGN && saveAlignedImagesPrefix != "") {
        hdrCreationManager->saveImages(saveAlignedImagesPrefix);
    }

    hdrCreationManager->removeTempFiles();
    if (threshold > 0) {
        QList<QPair<int, int> > dummyOffset;
        QStringList::ConstIterator it = inputFiles.begin();
        while( it != inputFiles.end() ) {
            dummyOffset.append(qMakePair(0,0));
            ++it;
        }
        ProgressHelper ph;
        bool patches[agGridSize][agGridSize];
        float patchesPercent;
        int h0 = hdrCreationManager->computePatches(threshold, patches, patchesPercent, dummyOffset);
        HDR.reset( hdrCreationManager->doAntiGhosting(patches, h0, false, &ph) ); // false means auto anti-ghosting
    }
    else {
        HDR.reset( hdrCreationManager->createHdr() );
    }
    saveHDR();
}

void CommandLineInterfaceManager::saveHDR()
{
    if (!saveHdrFilename.isEmpty())
    {
        printIfVerbose( tr("Saving to file %1.").arg(saveHdrFilename) , verbose);

        // write_hdr_frame by default saves to EXR, if it doesn't find a supported file type
        if ( IOWorker().write_hdr_frame(HDR.data(), saveHdrFilename) )
        {
            printIfVerbose( tr("Image %1 saved successfully").arg(saveHdrFilename) , verbose);
        }
        else
        {
            printIfVerbose( tr("Could not save %1").arg(saveHdrFilename) , verbose);
        }
    }
    else
    {
        printIfVerbose( tr("NOT Saving HDR image to file. %1").arg(saveHdrFilename) , verbose);
    }

    startTonemap();
}

void  CommandLineInterfaceManager::startTonemap()
{
    if (!saveLdrFilename.isEmpty())
    {
        printIfVerbose( tr("Tonemapping requested, saving to file %1.").arg(saveLdrFilename) , verbose);

        //now check if user wants to resize (create thread with either -2 or true original size as first argument in ctor, see options.cpp).
        //TODO
        tmopts->origxsize = HDR->getWidth();
#ifdef QT_DEBUG
        qDebug() << "XSIZE:" << tmopts->xsize;
#endif
        if (tmopts->xsize == -2) tmopts->xsize = HDR->getWidth();

        // Build TMWorker
        TMWorker tm_worker;
		connect(&tm_worker, SIGNAL(tonemapSetMaximum(int)), this, SLOT(setProgressBar(int)));
		connect(&tm_worker, SIGNAL(tonemapSetValue(int)), this, SLOT(updateProgressBar(int)));

        // Build a new TM frame
        // The scoped pointer will free the memory automatically later on
        QScopedPointer<pfs::Frame> tm_frame( tm_worker.computeTonemap(HDR.data(), tmopts.data()) );

        QString inputfname; // to copy EXIF tags from 1st input image to saved LDR
        if (inputFiles.isEmpty())
            inputfname = "";
        else
            inputfname = inputFiles.first();

        // Create an ad-hoc IOWorker to save the file
        if ( IOWorker().write_ldr_frame(tm_frame.data(), saveLdrFilename,
                                        inputfname,
                                        hdrCreationManager.data() ? hdrCreationManager->getExpotimes(): QVector<float>(),
                                        tmopts.data(),
                                        pfs::Params("quality", (size_t)quality) ) )
        {
            // File save successful
            printIfVerbose( tr("Image %1 saved successfully").arg(saveLdrFilename) , verbose);
        }
        else
        {
            // File save failed
            printErrorAndExit( tr("ERROR: Cannot save to file: %1").arg(saveLdrFilename) );
        }
        emit finishedParsing();
    }
    else
    {
        printIfVerbose("Tonemapping NOT requested.", verbose);

        emit finishedParsing();
    }
}

void CommandLineInterfaceManager::errorWhileLoading(QString errormessage) {
	printErrorAndExit( tr("Failed loading images"));
}

void CommandLineInterfaceManager::printHelp(char * progname)
{
    QString help=
            "\t\t" + tr("(Default is weight=triangular:response_curve=linear:model=debevec) ") + "\n" +
            "\t" + tr("-p --tmoptions          Tone mapping operator options. Legal values are: ") + "\n" +
            "\t\t" + tr("colorsaturation=VALUE:contrastenhancement=VALUE:luminancelevel=VALUE:setluminance=true|false (for mantiuk08)") + "\n" +
            "\t\t" + tr("localcontrast=VALUE:eq=2|4:simple=true|false (for ashikhmin)") + "\n" +
            "\t\t" + tr("sigma_s=VALUE:sigma_r=VALUE:base=VALUE (for durand)") + "\n" +
            "\t\t" + tr("bias=VALUE (for drago)") + "\n" +
            "\t\t" + tr("local=true|false:autolum=true|false:cone=VALUE:rod=VALUE:multiplier=VALUE (for pattanaik)") + "\n" +
            "\t\t" + tr("scales=true|false:key=VALUE:phi=VALUE:num=VALUE:low=VALUE:high=VALUE (for reinhard02)") + "\n" +
            "\t\t" + tr("brightness=VALUE:chroma=VALUE:lightness=VALUE (for reinhard05)") + "\n" +
            "\t\t" + tr("(default is contrast=0.3:equalization=false:saturation=1.8, see also -o)") + "\n" +
            "\t" + tr("                        (No tonemapping is performed unless -o is specified).") + "\n\n" +
            tr("You must either load an existing HDR file (via the -l option) or specify INPUTFILES to create a new HDR.\n");
    printErrorAndExit(help);
}

void CommandLineInterfaceManager::setProgressBar(int max)
{
	maximum = max;
	progressBar.reset();
	progressBar.n = max;
	std::cout << std::endl;
	started = true;
}

void CommandLineInterfaceManager::updateProgressBar(int value)
{
	if (verbose) {
		if (value < 0) return;
		if (value < oldValue) {
			//progressBar.reset();
			//progressBar.n = maximum;
			//progressBar.start();
			progressBar.cur = value;
			progressBar.setPct( ((float)value)/maximum );
		}
		if (started) {
			started = false;
			progressBar.start();
		}
		for (int i = 0; i < value - oldValue; i++)
			++progressBar;
		oldValue = value;
		//if (value == progressBar.n) {
		//	std::cout << std::endl;
		//}
	}
}

void CommandLineInterfaceManager::readData(QByteArray data)
{
	if (verbose)
		std::cout << data.constData() << std::endl;
}

