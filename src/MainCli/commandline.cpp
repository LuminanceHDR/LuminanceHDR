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

#include <QDebug>
#include <QTimer>
#include <boost/program_options.hpp>
#include <iostream>

#include <Common/CommonFunctions.h>
#include <Common/GitSHA1.h>
#include <Common/LuminanceOptions.h>
#include <Common/config.h>
#include <Core/IOWorker.h>
#include <Core/TMWorker.h>
#include <Exif/ExifOperations.h>
#include <Fileformat/pfsoutldrimage.h>
#include <HdrHTML/pfsouthdrhtml.h>
#include <Libpfs/manip/gamma_levels.h>
#include <Libpfs/tm/TonemapOperator.h>
#include "commandline.h"

#if defined(_MSC_VER)
#include <fcntl.h>
#include <io.h>
#endif

using namespace libhdr::fusion;

namespace {
void printIfVerbose(const QString &str, bool verbose) {
    if (verbose) {
#if defined(_MSC_VER)
        // if the filemode isn't restored afterwards, a normal std::cout
        // segfaults
        int oldMode = _setmode(_fileno(stdout), _O_U16TEXT);
        std::wcout << qPrintable(str) << std::endl;
        if (oldMode >= 0) _setmode(_fileno(stdout), oldMode);
#else
        std::cout << qPrintable(str) << std::endl;
#endif
    }
}

void printErrorAndExit(const QString &error_str) {
    printIfVerbose(error_str, true);
    exit(-1);
}

float toFloatWithErrMsg(const QString &str) {
    bool ok;
    float ret = str.toFloat(&ok);
    if (!ok) {
        printErrorAndExit(QObject::tr("Cannot convert %1 to a float").arg(str));
    }
    return ret;
}
}

CommandLineInterfaceManager::CommandLineInterfaceManager(const int argc,
                                                         char **argv)
    : argc(argc),
      argv(argv),
      operationMode(UNKNOWN_MODE),
      alignMode(NO_ALIGN),
      tmopts(TMOptionsOperations::getDefaultTMOptions()),
      tmofileparams(new pfs::Params()),
      verbose(false),
      oldValue(0),
      maximum(100),
      started(false),
      threshold(0.0f),
      isAutolevels(false),
      isHtml(false),
      isHtmlDone(false),
      htmlQuality(2),
      isProposedLdrName(false),
      isProposedHdrName(false),
      pageName(),
      imagesDir(),
      saveAlignedImagesPrefix(QLatin1String("")) {
    hdrcreationconfig.weightFunction = WEIGHT_TRIANGULAR;
    hdrcreationconfig.responseCurve = RESPONSE_LINEAR;
    hdrcreationconfig.fusionOperator = DEBEVEC;

    tmofileparams->set("quality", (size_t)100);
    validLdrExtensions << "jpg"
                       << "jpeg"
                       << "png"
                       << "tif"
                       << "tiff"
                       << "pnm"
                       << "bmp"
                       << "pgm"
                       << "xpm"
                       << "xbm";
    validHdrExtensions << "exr"
                       << "hdr"
                       << "tif"
                       << "tiff"
                       << "pfs";
}

int CommandLineInterfaceManager::execCommandLineParams() {
    // Declare the supported options.
    namespace po = boost::program_options;
    po::variables_map vm;

    po::options_description desc(tr("Usage: %1 [OPTIONS]... [INPUTFILES]...")
                                     .arg(argv[0])
                                     .toUtf8()
                                     .constData());
    desc.add_options()("help,h", tr("Display this help.").toUtf8().constData())
        ("version,V", tr("Display program version.").toUtf8().constData())
        ("verbose,v", tr("Print more messages during execution.").toUtf8().constData())
        ("cameras,c", tr("Print a list of all supported cameras.").toUtf8().constData())
        ("align,a", po::value<std::string>(), tr("[AIS|MTB]   Align Engine to use during HDR creation (default: no "
           "alignment).").toUtf8().constData())
        ("ev,e", po::value<std::string>(), tr("EV1,EV2,... Specify numerical EV values (as many as INPUTFILES).")
            .toUtf8().constData())
        ("savealigned,d", po::value<std::string>(), tr("prefix Save aligned images to files which names start with prefix")
            .toUtf8().constData())
        //
        ("load,l", po::value<std::string>(), tr("HDR_FILE Load an HDR instead of creating a new one.")
            .toUtf8().constData())
        ("save,s", po::value<std::string>(), tr("HDR_FILE Save to a HDR file format. (default: don't save)")
            .toUtf8().constData())
        ("gamma,g", po::value<float>(&tmopts->pregamma),
            tr("VALUE        Gamma value to use during tone mapping. (default: 1) ").toUtf8().constData())
        ("saturation,S", po::value<float>(&tmopts->postsaturation),
            tr("VALUE        Saturation value to use after tone mapping. (default: 1) ").toUtf8().constData())
        ("postgamma,G", po::value<float>(&tmopts->postgamma),
            tr("VALUE        Gamma value to use after tone mapping. (default: 1) ").toUtf8().constData())
        ("resize,r", po::value<int>(&tmopts->xsize), tr("VALUE       Width you want to resize your HDR to (resized "
            "before gamma and tone mapping)").toUtf8().constData())

        ("output,o", po::value<std::string>(), tr("LDR_FILE    File name you want to save your tone mapped LDR to.")
            .toUtf8().constData())("autoag,t", po::value<float>(&threshold), tr("THRESHOLD   Enable auto anti-ghosting with "
            "given threshold. (0.0-1.0)").toUtf8().constData())
        ("autolevels,b", tr("Apply autolevels correction after tonemapping.").toUtf8().constData())
        ("createwebpage,w", tr("Enable generation of a webpage with embedded HDR viewer.").toUtf8().constData())
        ("proposedldrname,p", po::value<std::string>(&ldrExtension), tr("FILE_EXTENSION   Save LDR file with a name of the form "
            "first-last_tmparameters.extension.").toUtf8().constData())
        ("proposedhdrname,z", po::value<std::string>(&hdrExtension), tr("FILE_EXTENSION   Save HDR file with a name of the form "
            "first-last_HdrCreationModel.extension.").toUtf8().constData());

    po::options_description hdr_desc(
        tr("HDR creation parameters  - you must either load an existing HDR "
           "file "
           "(via the -l option) "
           "or specify INPUTFILES to create a new HDR")
            .toUtf8()
            .constData());
    hdr_desc.add_options()(
        "hdrWeight", po::value<std::string>(),
        tr("weight = triangular|gaussian|plateau|flat (Default is triangular)")
            .toUtf8()
            .constData())("hdrResponseCurve", po::value<std::string>(),
                          tr("response curve = from_file|linear|gamma|log|srgb "
                             "(Default is linear)")
                              .toUtf8()
                              .constData())(
        "hdrModel", po::value<std::string>(),
        tr("model: robertson|robertsonauto|debevec (Default is debevec)")
            .toUtf8()
            .constData())(
        "hdrCurveFilename", po::value<std::string>(),
        tr("curve filename = your_file_here.m").toUtf8().constData());

    po::options_description ldr_desc(
        tr("LDR output parameters").toUtf8().constData());
    ldr_desc.add_options()(
        "ldrQuality,q", po::value<int>(),
        tr("VALUE      Quality of the saved tone mapped file (1-100).")
            .toUtf8()
            .constData())(
        "ldrTiff", po::value<std::string>(),
        tr("Tiff format. Legal values are [8b|16b|32b|logluv] (Default is 8b)")
            .toUtf8()
            .constData())(
        "ldrTiffDeflate", po::value<bool>(),
        tr("Tiff deflate compression. true|false (Default is true)")
            .toUtf8()
            .constData());

    po::options_description html_desc(
        tr("HTML output parameters").toUtf8().constData());
    html_desc.add_options()(
        "htmlQuality,k", po::value<int>(),
        tr("VALUE      Quality of the interpolated exposures, from the worst (1) to the best\
(4). Higher quality will introduce less distortions in the \
brightest and the darkest tones, but will also generate more \
images. More images means that there is more data that needs to be \
transferred to the web-browser, making HDR viewer less responsive. \
(Default is 2, which is sufficient for most applications)")
            .toUtf8()
            .constData())(
        "pageName", po::value<std::string>(), tr("Specifies the file name, of \
the web page to be generated. If <page_name> is missing, the \
file name of the first image with .html extension will be used. \
(Default is first image name)").toUtf8().constData())(
        "imagesDir", po::value<std::string>(),
        tr("Specify where to store the resulting image files. Links to images in \
HTML will be updated accordingly. This must be a relative path and the \
directory must exist.  Useful to avoid clutter in the current directory. \
(Default is current working directory)")
            .toUtf8()
            .constData());

    po::options_description tmo_desc(tr("Tone mapping parameters  - no "
                                        "tonemapping is performed unless -o is "
                                        "specified")
                                         .toUtf8()
                                         .constData());
    tmo_desc.add_options()(
        "tmo", po::value<std::string>(),
        tr("Tone mapping operator. Legal values are: [ashikhmin|drago|durand|fattal|ferradans|pattanaik|reinhard02|reinhard05|\
                mai|mantiuk06|mantiuk08] (Default is mantiuk06)")
            .toUtf8()
            .constData())("tmofile", po::value<std::string>(),
                          tr("SETTING_FILE Load an existing setting file "
                             "containing pre-gamma and all TMO settings")
                              .toUtf8()
                              .constData());

    po::options_description tmo_fattal(tr(" Fattal").toUtf8().constData());
    tmo_fattal.add_options()(
        "tmoFatAlpha",
        po::value<float>(&tmopts->operator_options.fattaloptions.alpha),
        tr("alpha FLOAT").toUtf8().constData())(
        "tmoFatBeta",
        po::value<float>(&tmopts->operator_options.fattaloptions.beta),
        tr("beta FLOAT").toUtf8().constData())(
        "tmoFatColor",
        po::value<float>(&tmopts->operator_options.fattaloptions.color),
        tr("color FLOAT").toUtf8().constData())(
        "tmoFatNoise",
        po::value<float>(&tmopts->operator_options.fattaloptions.noiseredux),
        tr("noise FLOAT").toUtf8().constData())(
        "tmoFatNew",
        po::value<bool>(&tmopts->operator_options.fattaloptions.fftsolver),
        tr("new true|false").toUtf8().constData());
    po::options_description tmo_ferradans(
        tr(" Ferradans").toUtf8().constData());
    tmo_ferradans.add_options()(
        "tmoFerRho",
        po::value<float>(&tmopts->operator_options.ferradansoptions.rho),
        tr("rho FLOAT").toUtf8().constData())(
        "tmoFerInvAlpha",
        po::value<float>(&tmopts->operator_options.ferradansoptions.inv_alpha),
        tr("inv_alpha FLOAT").toUtf8().constData());
    po::options_description tmo_mantiuk06(
        tr(" Mantiuk 06").toUtf8().constData());
    tmo_mantiuk06.add_options()(
        "tmoM06Contrast",
        po::value<float>(
            &tmopts->operator_options.mantiuk06options.contrastfactor),
        tr("contrast FLOAT").toUtf8().constData())(
        "tmoM06Saturation",
        po::value<float>(
            &tmopts->operator_options.mantiuk06options.saturationfactor),
        tr("saturation FLOAT").toUtf8().constData())(
        "tmoM06Detail",
        po::value<float>(
            &tmopts->operator_options.mantiuk06options.detailfactor),
        tr("detail FLOAT").toUtf8().constData())(
        "tmoM06ContrastEqual",
        po::value<bool>(
            &tmopts->operator_options.mantiuk06options.contrastequalization),
        tr("equalization true|false").toUtf8().constData());
    po::options_description tmo_mantiuk08(
        tr(" Mantiuk 08").toUtf8().constData());
    tmo_mantiuk08.add_options()(
        "tmoM08ColorSaturation",
        po::value<float>(
            &tmopts->operator_options.mantiuk08options.colorsaturation),
        tr("color saturation FLOAT").toUtf8().constData())(
        "tmoM08ConstrastEnh",
        po::value<float>(
            &tmopts->operator_options.mantiuk08options.contrastenhancement),
        tr("contrast enhancement FLOAT").toUtf8().constData())(
        "tmoM08LuminanceLvl",
        po::value<float>(
            &tmopts->operator_options.mantiuk08options.luminancelevel),
        tr("luminance level FLOAT").toUtf8().constData())(
        "tmoM08SetLuminance",
        po::value<bool>(
            &tmopts->operator_options.mantiuk08options.setluminance),
        tr("enable luminance level true|false").toUtf8().constData());
    po::options_description tmo_durand(tr(" Durand").toUtf8().constData());
    tmo_durand.add_options()(
        "tmoDurSigmaS",
        po::value<float>(&tmopts->operator_options.durandoptions.spatial),
        tr("spatial kernel sigma FLOAT").toUtf8().constData())(
        "tmoDurSigmaR",
        po::value<float>(&tmopts->operator_options.durandoptions.range),
        tr("range kernel sigma FLOAT").toUtf8().constData())(
        "tmoDurBase",
        po::value<float>(&tmopts->operator_options.durandoptions.base),
        tr("base contrast FLOAT").toUtf8().constData());
    po::options_description tmo_drago(tr(" Drago").toUtf8().constData());
    tmo_drago.add_options()(
        "tmoDrgBias",
        po::value<float>(&tmopts->operator_options.dragooptions.bias),
        tr("bias FLOAT").toUtf8().constData());
    po::options_description tmo_reinhard02(
        tr(" Reinhard 02").toUtf8().constData());
    tmo_reinhard02.add_options()(
        "tmoR02Key",
        po::value<float>(&tmopts->operator_options.reinhard02options.key),
        tr("key value FLOAT").toUtf8().constData())(
        "tmoR02Phi",
        po::value<float>(&tmopts->operator_options.reinhard02options.phi),
        tr("phi FLOAT").toUtf8().constData())(
        "tmoR02Scales",
        po::value<bool>(&tmopts->operator_options.reinhard02options.scales),
        tr("use scales true|false").toUtf8().constData())(
        "tmoR02Num",
        po::value<int>(&tmopts->operator_options.reinhard02options.range),
        tr("range FLOAT").toUtf8().constData())(
        "tmoR02Low",
        po::value<int>(&tmopts->operator_options.reinhard02options.lower),
        tr("lower scale FLOAT").toUtf8().constData())(
        "tmoR02High",
        po::value<int>(&tmopts->operator_options.reinhard02options.upper),
        tr("upper scale FLOAT").toUtf8().constData());
    po::options_description tmo_reinhard05(
        tr(" Reinhard 05").toUtf8().constData());
    tmo_reinhard05.add_options()(
        "tmoR05Brightness",
        po::value<float>(
            &tmopts->operator_options.reinhard05options.brightness),
        tr("Brightness FLOAT").toUtf8().constData())(
        "tmoR05Chroma",
        po::value<float>(
            &tmopts->operator_options.reinhard05options.chromaticAdaptation),
        tr("Chroma adaption FLOAT").toUtf8().constData())(
        "tmoR05Lightness",
        po::value<float>(
            &tmopts->operator_options.reinhard05options.lightAdaptation),
        tr("Light adaption FLOAT").toUtf8().constData());
    po::options_description tmo_ash(tr(" Ashikmin").toUtf8().constData());
    tmo_ash.add_options()(
        "tmoAshEq2",
        po::value<bool>(&tmopts->operator_options.ashikhminoptions.eq2),
        tr("Equation number 2 true|false").toUtf8().constData())(
        "tmoAshSimple",
        po::value<bool>(&tmopts->operator_options.ashikhminoptions.simple),
        tr("Simple true|false").toUtf8().constData())(
        "tmoAshLocal",
        po::value<float>(&tmopts->operator_options.ashikhminoptions.lct),
        tr("Local threshold FLOAT").toUtf8().constData());
    po::options_description tmo_patt(tr(" Pattanaik").toUtf8().constData());
    tmo_patt.add_options()(
        "tmoPatMultiplier",
        po::value<float>(&tmopts->operator_options.pattanaikoptions.multiplier),
        tr("multiplier FLOAT").toUtf8().constData())(
        "tmoPatLocal",
        po::value<bool>(&tmopts->operator_options.pattanaikoptions.local),
        tr("Local tone mapping true|false").toUtf8().constData())(
        "tmoPatAutoLum",
        po::value<bool>(&tmopts->operator_options.pattanaikoptions.autolum),
        tr("Auto luminance true|false").toUtf8().constData())(
        "tmoPatCone",
        po::value<float>(&tmopts->operator_options.pattanaikoptions.cone),
        tr("cone level FLOAT").toUtf8().constData())(
        "tmoPatRod",
        po::value<float>(&tmopts->operator_options.pattanaikoptions.rod),
        tr("rod level FLOAT").toUtf8().constData());

    tmo_desc.add(tmo_fattal);
    tmo_desc.add(tmo_ferradans);
    tmo_desc.add(tmo_mantiuk06);
    tmo_desc.add(tmo_mantiuk08);
    tmo_desc.add(tmo_durand);
    tmo_desc.add(tmo_drago);
    tmo_desc.add(tmo_reinhard02);
    tmo_desc.add(tmo_reinhard05);
    tmo_desc.add(tmo_ash);
    tmo_desc.add(tmo_patt);

    po::options_description hidden("Hidden options");
    hidden.add_options()("input-file", po::value<vector<string>>(),
                         "input file");

    po::positional_options_description p;
    p.add("input-file", -1);

    po::options_description cmdline_options;
    cmdline_options.add(desc)
        .add(hdr_desc)
        .add(ldr_desc)
        .add(html_desc)
        .add(tmo_desc)
        .add(hidden);

    po::options_description cmdvisible_options;
    cmdvisible_options.add(desc).add(hdr_desc).add(ldr_desc).add(html_desc).add(
        tmo_desc);

    try {
        po::store(po::command_line_parser(argc, argv)
                      .options(cmdline_options)
                      .positional(p)
                      .run(),
                  vm);
        po::notify(vm);

        if (vm.count("help")) {
            cout << cmdvisible_options << "\n";
            return 1;
        }
        if (vm.count("version")) {
            cout << tr("Luminance HDR version ").toStdString() +
                        LUMINANCEVERSION + " [Build " +
                        QString(g_GIT_SHA1).left(6).toStdString() + "]"
                 << endl;
            return 1;
        }
        if (vm.count("verbose")) {
            verbose = true;
        }
        if (vm.count("cameras")) {
            cout << tr("With LibRaw version ").toStdString()
                 << LibRaw::version() << endl;
            cout << LibRaw::cameraCount() << tr(" models listed").toStdString()
                 << endl;
            cout << endl;
            const char **list = LibRaw::cameraList();
            while (*list) cout << *list++ << endl;
            return 1;
        }
        if (vm.count("autolevels")) {
            isAutolevels = true;
        }
        if (vm.count("createwebpage")) {
            isHtml = true;
        }
        if (vm.count("proposedldrname")) {
            isProposedLdrName = true;
            if (!validLdrExtensions.contains(
                    QString::fromStdString(ldrExtension), Qt::CaseInsensitive))
                printErrorAndExit(tr("Error: Unsupported LDR file type."));
        }
        if (vm.count("proposedhdrname")) {
            isProposedHdrName = true;
            if (!validHdrExtensions.contains(
                    QString::fromStdString(hdrExtension), Qt::CaseInsensitive))
                printErrorAndExit(tr("Error: Unsupported HDR file type."));
        }
        if (vm.count("htmlQuality")) {
            htmlQuality = vm["htmlQuality"].as<int>();
            if (htmlQuality < 1 || htmlQuality > 4)
                printErrorAndExit(
                    tr("Error: htmlQuality must be in the range [1..4]."));
        }
        if (vm.count("pageName")) {
            pageName = vm["pageName"].as<std::string>();
        }
        if (vm.count("imagesDir")) {
            imagesDir = vm["imagesDir"].as<std::string>();
        }
        if (vm.count("align")) {
            const char *value = vm["align"].as<std::string>().c_str();
            if (strcmp(value, "AIS") == 0)
                alignMode = AIS_ALIGN;
            else if (strcmp(value, "MTB") == 0)
                alignMode = MTB_ALIGN;
            else
                printErrorAndExit(
                    tr("Error: Alignment engine not recognized."));
        }
        if (vm.count("ev")) {
            QStringList evstringlist =
                QString::fromStdString(vm["ev"].as<std::string>())
                    .split(QStringLiteral(","));
            for (int i = 0; i < evstringlist.count(); i++)
                ev.append(toFloatWithErrMsg(evstringlist.at(i)));
        }
        if (vm.count("hdrWeight")) {
            const char *value = vm["hdrWeight"].as<std::string>().c_str();
            if (strcmp(value, "triangular") == 0)
                hdrcreationconfig.weightFunction = WEIGHT_TRIANGULAR;
            else if (strcmp(value, "gaussian") == 0)
                hdrcreationconfig.weightFunction = WEIGHT_GAUSSIAN;
            else if (strcmp(value, "plateau") == 0)
                hdrcreationconfig.weightFunction = WEIGHT_PLATEAU;
            else if (strcmp(value, "flat") == 0)
                hdrcreationconfig.weightFunction = WEIGHT_FLAT;
            else
                printErrorAndExit(
                    tr("Error: Unknown weight function specified."));
        }
        if (vm.count("hdrResponseCurve")) {
            const char *value =
                vm["hdrResponseCurve"].as<std::string>().c_str();
            if (strcmp(value, "from_file") == 0)
                hdrcreationconfig.responseCurve = RESPONSE_CUSTOM;
            else if (strcmp(value, "linear") == 0)
                hdrcreationconfig.responseCurve = RESPONSE_LINEAR;
            else if (strcmp(value, "gamma") == 0)
                hdrcreationconfig.responseCurve = RESPONSE_GAMMA;
            else if (strcmp(value, "log") == 0)
                hdrcreationconfig.responseCurve = RESPONSE_LOG10;
            else if (strcmp(value, "srgb") == 0)
                hdrcreationconfig.responseCurve = RESPONSE_SRGB;
            else
                printErrorAndExit(
                    tr("Error: Unknown response curve specified."));
        }
        if (vm.count("hdrModel")) {
            const char *value = vm["hdrModel"].as<std::string>().c_str();
            if (strcmp(value, "robertson") == 0)
                hdrcreationconfig.fusionOperator = ROBERTSON;
            else if (strcmp(value, "robertsonauto") == 0)
                hdrcreationconfig.fusionOperator = ROBERTSON_AUTO;
            else if (strcmp(value, "debevec") == 0)
                hdrcreationconfig.fusionOperator = DEBEVEC;
            else
                printErrorAndExit(
                    tr("Error: Unknown HDR creation model specified."));
        }
        if (vm.count("hdrCurveFilename"))
            hdrcreationconfig.inputResponseCurveFilename =
                QString::fromStdString(
                    vm["hdrCurveFilename"].as<std::string>());
        if (vm.count("tmo")) {
            const char *value = vm["tmo"].as<std::string>().c_str();
            if (strcmp(value, "ashikhmin") == 0)
                tmopts->tmoperator = ashikhmin;
            else if (strcmp(value, "drago") == 0)
                tmopts->tmoperator = drago;
            else if (strcmp(value, "durand") == 0)
                tmopts->tmoperator = durand;
            else if (strcmp(value, "fattal") == 0)
                tmopts->tmoperator = fattal;
            else if (strcmp(value, "ferradans") == 0)
                tmopts->tmoperator = ferradans;
            else if (strcmp(value, "mai") == 0)
                tmopts->tmoperator = mai;
            else if (strcmp(value, "pattanaik") == 0)
                tmopts->tmoperator = pattanaik;
            else if (strcmp(value, "reinhard02") == 0)
                tmopts->tmoperator = reinhard02;
            else if (strcmp(value, "reinhard05") == 0)
                tmopts->tmoperator = reinhard05;
            else if (strcmp(value, "mantiuk06") == 0)
                tmopts->tmoperator = mantiuk06;
            else if (strcmp(value, "mantiuk08") == 0)
                tmopts->tmoperator = mantiuk08;
            else
                printErrorAndExit(
                    tr("Error: Unknown tone mapping operator specified."));
        }
        if (vm.count("tmofile")) {
            QString settingFile =
                QString::fromStdString(vm["tmofile"].as<std::string>());
            printIfVerbose(QObject::tr("Loading TMO settings from file: %1")
                               .arg(settingFile),
                           verbose);
            try {
                TonemappingOptions *options =
                    TMOptionsOperations::parseFile(settingFile);
                if (options != NULL)
                    tmopts.reset(options);
                else
                    printErrorAndExit(
                        tr("Error: The specified file with TMO "
                           "settings could not be "
                           "parsed!"));
            } catch (QString &error) {
                printErrorAndExit(
                    tr("Error: The specified file with TMO settings "
                       "could not be parsed!: %1")
                        .arg(error));
            } catch (...) {
                printErrorAndExit(tr(
                    "Error: The specified file with TMO settings could not be "
                    "parsed!"));
            }
        }

        if (vm.count("ldrQuality")) {
            int quality = vm["ldrQuality"].as<int>();
            if (quality < 1 || quality > 100)
                printErrorAndExit(
                    tr("Error: Quality must be in the range [1..100]."));
            else
                tmofileparams->set("quality", (size_t)quality);
        }
        if (vm.count("ldrTiff")) {
            const char *value = vm["ldrTiff"].as<std::string>().c_str();
            if (strcmp(value, "8b") == 0)
                tmofileparams->set("tiff_mode", (int)0);
            else if (strcmp(value, "16b") == 0)
                tmofileparams->set("tiff_mode", (int)1);
            else if (strcmp(value, "32b") == 0)
                tmofileparams->set("tiff_mode", (int)2);
            else if (strcmp(value, "logluv") == 0)
                tmofileparams->set("tiff_mode", (int)3);
            else
                printErrorAndExit(tr("Error: Unknown tiff format."));
        }
        if (vm.count("ldrTiffDeflate"))
            tmofileparams->set("deflateCompression",
                               vm["ldrTiffDeflate"].as<bool>());

        if (vm.count("load"))
            loadHdrFilename =
                QString::fromStdString(vm["load"].as<std::string>());
        if (vm.count("save")) {
            saveHdrFilename =
                QString::fromStdString(vm["save"].as<std::string>());
            // let's determine file extension
            int counter = saveHdrFilename.count(".");
            QString fileExtension = saveHdrFilename.section(".", counter);
            if (!validHdrExtensions.contains(fileExtension,
                                             Qt::CaseInsensitive))
                printErrorAndExit(tr("Error: Unsupported HDR file type."));
        }
        if (vm.count("output")) {
            saveLdrFilename =
                QString::fromStdString(vm["output"].as<std::string>());
            // let's determine file extension
            int counter = saveLdrFilename.count(".");
            QString fileExtension = saveLdrFilename.section(".", counter);
            if (!validLdrExtensions.contains(fileExtension,
                                             Qt::CaseInsensitive))
                printErrorAndExit(tr("Error: Unsupported LDR file type."));
        }
        if (vm.count("savealigned"))
            saveAlignedImagesPrefix =
                QString::fromStdString(vm["savealigned"].as<std::string>());
        if (threshold < 0.0f || threshold > 1.0f)
            printErrorAndExit(
                tr("Error: Threshold must be in the range [0..1]."));

    } catch (boost::program_options::required_option &e) {
        std::cerr << "ERROR: " << e.what() << std::endl << std::endl;
        return 1;
    } catch (boost::program_options::error &e) {
        std::cerr << "ERROR: " << e.what() << std::endl << std::endl;
        return 1;
    }

    if (vm.count("input-file")) {
        vector<string> options = vm["input-file"].as<vector<string>>();
        for (int i = 0, size = options.size(); i < size; i++) {
            inputFiles << QString::fromStdString(options[i]);
            printIfVerbose(QObject::tr("Input file %1")
                               .arg(QString::fromStdString(options[i])),
                           verbose);
            // cout << options[i] << "\n";
        }
    }

    if (loadHdrFilename.isEmpty() && inputFiles.size() == 0) {
        cout << cmdvisible_options << endl;
        exit(0); // Exit here instead of returning to main complicating main code
    }

    QTimer::singleShot(0, this,
                       &CommandLineInterfaceManager::execCommandLineParamsSlot);
    return EXIT_SUCCESS;
}

void CommandLineInterfaceManager::execCommandLineParamsSlot() {
    if (!ev.isEmpty() && ev.count() != inputFiles.count()) {
        printErrorAndExit(
            tr("Error: The number of EV values specified is different from the "
               "number of input files."));
    }
    // now validate operation mode.
    if (inputFiles.size() != 0 && loadHdrFilename.isEmpty()) {
        operationMode = CREATE_HDR_MODE;

        printIfVerbose(QObject::tr("Running in HDR-creation mode."), verbose);
    } else if (!loadHdrFilename.isEmpty() && inputFiles.size() == 0) {
        operationMode = LOAD_HDR_MODE;

        printIfVerbose(QObject::tr("Running in Load-HDR mode."), verbose);
    } else {
        exit(-1);
    }

    if (operationMode == CREATE_HDR_MODE) {
        if (verbose) {
            LuminanceOptions luminance_options;

            printIfVerbose(QObject::tr("Temporary directory: %1")
                               .arg(luminance_options.getTempDir()),
                           verbose);
            printIfVerbose(QObject::tr("Using %n threads.", "",
                                       luminance_options.getNumThreads()),
                           verbose);
        }
        hdrCreationManager.reset(new HdrCreationManager(true));
        connect(hdrCreationManager.data(),
                &HdrCreationManager::finishedLoadingFiles, this,
                &CommandLineInterfaceManager::finishedLoadingInputFiles);
        connect(hdrCreationManager.data(),
                &HdrCreationManager::finishedAligning, this,
                &CommandLineInterfaceManager::createHDR);
        connect(hdrCreationManager.data(), &HdrCreationManager::ais_failed,
                this, &CommandLineInterfaceManager::ais_failed);
        connect(hdrCreationManager.data(),
                &HdrCreationManager::errorWhileLoading, this,
                &CommandLineInterfaceManager::errorWhileLoading);
        connect(hdrCreationManager.data(), &HdrCreationManager::aisDataReady,
                this, &CommandLineInterfaceManager::readData);

        try {
            hdrCreationManager->setConfig(hdrcreationconfig);
            hdrCreationManager->loadFiles(inputFiles);
        } catch (std::runtime_error &e) {
            printErrorAndExit(e.what());
        } catch (...) {
            printErrorAndExit(QStringLiteral("Catched unhandled exception"));
        }
    } else {
        printIfVerbose(QObject::tr("Loading file %1").arg(loadHdrFilename),
                       verbose);

        try {
            HDR.reset(IOWorker().read_hdr_frame(loadHdrFilename));
        } catch (...) {
            printErrorAndExit(QStringLiteral("Catched unhandled exception"));
        }

        if (HDR != NULL) {
            printIfVerbose(QObject::tr("Successfully loaded file %1.")
                               .arg(loadHdrFilename),
                           verbose);
            saveHDR();
        } else {
            printErrorAndExit(tr("Load file %1 failed").arg(loadHdrFilename));
        }
    }
}

void CommandLineInterfaceManager::finishedLoadingInputFiles() {
    QStringList filesLackingExif = hdrCreationManager->getFilesWithoutExif();
    if (filesLackingExif.size() != 0 && ev.isEmpty()) {
        printErrorAndExit(tr(
            "Error: Exif data missing in images and EV values not specified on "
            "the commandline, bailing out."));
    }
    if (!ev.isEmpty()) {
        for (int i = 0; i < ev.size(); i++)
            hdrCreationManager->getFile(i).setEV(ev.at(i));

        printIfVerbose(tr("EV values have been assigned."), verbose);
    }
    // hdrCreationManager->checkEVvalues();
    if (alignMode == AIS_ALIGN) {
        printIfVerbose(tr("Starting aligning..."), verbose);
        hdrCreationManager->align_with_ais();
    } else if (alignMode == MTB_ALIGN) {
        printIfVerbose(tr("Starting aligning..."), verbose);
        hdrCreationManager->align_with_mtb();
    } else if (alignMode == NO_ALIGN) {
        createHDR(0);
    }
}

void CommandLineInterfaceManager::ais_failed(QProcess::ProcessError) {
    printErrorAndExit(tr("Failed executing align_image_stack"));
}

void CommandLineInterfaceManager::createHDR(int errorcode) {
    if (errorcode != 0) printIfVerbose(tr("Failed aligning images."), verbose);

    printIfVerbose(tr("Creating (in memory) the HDR."), verbose);

    if (errorcode == 0 && alignMode != NO_ALIGN &&
        saveAlignedImagesPrefix != QLatin1String("")) {
        hdrCreationManager->saveImages(saveAlignedImagesPrefix);
    }

    if (threshold > 0) {
        QList<QPair<int, int>> dummyOffset;
        QStringList::ConstIterator it = inputFiles.begin();
        while (it != inputFiles.constEnd()) {
            dummyOffset.append(qMakePair(0, 0));
            ++it;
        }
        ProgressHelper ph;
        bool patches[agGridSize][agGridSize];
        float patchesPercent;
        int h0 = hdrCreationManager->computePatches(
            threshold, patches, patchesPercent, dummyOffset);
        HDR.reset(hdrCreationManager->doAntiGhosting(
            patches, h0, false, &ph));  // false means auto anti-ghosting
    } else {
        HDR.reset(hdrCreationManager->createHdr());
    }
    saveHDR();
}

void CommandLineInterfaceManager::saveHDR() {
    if (!saveHdrFilename.isEmpty() || isProposedHdrName) {
        QString fileExtension;
        QString caption;

        if (inputFiles.isEmpty()) {
            if (isProposedHdrName) {
                QFileInfo fi(loadHdrFilename);
                saveHdrFilename = fi.completeBaseName();
                saveHdrFilename.append("." +
                                       QString::fromStdString(hdrExtension));
            }
        } else {
            if (isProposedHdrName) {
                caption = QString(
                    QStringLiteral("Weights= ") +
                    getQString(
                        hdrCreationManager->getWeightFunction().getType()) +
                    QStringLiteral(" - Response curve= ") +
                    getQString(
                        hdrCreationManager->getResponseCurve().getType()) +
                    QStringLiteral(" - Model= ") +
                    getQString(hdrCreationManager->getFusionOperator()));

                QFileInfo fi1(inputFiles.first());
                QFileInfo fi2(inputFiles.last());

                saveHdrFilename =
                    fi1.completeBaseName() + "-" + fi2.completeBaseName();
                saveHdrFilename.append("_" + caption);
                saveHdrFilename.append("." +
                                       QString::fromStdString(hdrExtension));
            }
        }

        printIfVerbose(tr("Saving to file %1.").arg(saveHdrFilename), verbose);

        // write_hdr_frame by default saves to EXR, if it doesn't find a
        // supported
        // file type
        if (IOWorker().write_hdr_frame(HDR.data(), saveHdrFilename)) {
            printIfVerbose(
                tr("Image %1 saved successfully").arg(saveHdrFilename),
                verbose);
        } else {
            printIfVerbose(tr("Could not save %1").arg(saveHdrFilename),
                           verbose);
        }
    } else {
        printIfVerbose(
            tr("NOT Saving HDR image to file. %1").arg(saveHdrFilename),
            verbose);
    }

    if (isHtml && !isHtmlDone) {
        printIfVerbose(tr("Exporting to HTML"), verbose);
        generateHTML();
    }
    startTonemap();
}

void CommandLineInterfaceManager::generateHTML() {
    if (operationMode == LOAD_HDR_MODE) {
        if (pageName.empty()) pageName = loadHdrFilename.toStdString();
    } else {
        if (pageName.empty()) pageName = inputFiles.at(0).toStdString();
    }
    if (!imagesDir.empty()) {
        QFileInfo qfi = QFileInfo(QDir::currentPath() + "/" +
                                  QString::fromStdString(imagesDir));
        if (!qfi.isDir())
            printErrorAndExit(tr("ERROR: directory %1 must exist")
                                  .arg(QString::fromStdString(imagesDir)));
    }
    generate_hdrhtml(HDR.data(), pageName, "", imagesDir, "", "", htmlQuality,
                     verbose);
    isHtmlDone = true;
}

void CommandLineInterfaceManager::startTonemap() {
    if (!saveLdrFilename.isEmpty() || isProposedLdrName) {
        QString
            inputfname;  // to copy EXIF tags from 1st input image to saved LDR
        if (inputFiles.isEmpty()) {
            inputfname = QLatin1String("FromHdrFile");
            if (isProposedLdrName) {
                QFileInfo fi(loadHdrFilename);
                saveLdrFilename = fi.completeBaseName();
                saveLdrFilename.append("_" + tmopts->getPostfix());
                saveLdrFilename.append("." +
                                       QString::fromStdString(ldrExtension));
            }
        } else {
            inputfname = inputFiles.first();
            if (isProposedLdrName) {
                QFileInfo fi1(inputFiles.first());
                QFileInfo fi2(inputFiles.last());

                saveLdrFilename =
                    fi1.completeBaseName() + "-" + fi2.completeBaseName();
                saveLdrFilename.append("_" + tmopts->getPostfix());
                saveLdrFilename.append("." +
                                       QString::fromStdString(ldrExtension));
            }
        }

        printIfVerbose(tr("Tonemapping requested, saving to file %1.")
                           .arg(saveLdrFilename),
                       verbose);

        // now check if user wants to resize (create thread with either -2 or
        // true
        // original size as first argument in ctor,
        // see options.cpp).
        // TODO
        tmopts->origxsize = HDR->getWidth();
#ifdef QT_DEBUG
        qDebug() << "XSIZE:" << tmopts->xsize;
#endif
        if (tmopts->xsize == -2)
            tmopts->xsize = HDR->getWidth();
        else
            printIfVerbose(tr("Resizing to width %1.").arg(tmopts->xsize),
                           verbose);

        if (tmopts->pregamma != 1)
            printIfVerbose(tr("Applying gamma %1.").arg(tmopts->pregamma),
                           verbose);

        // Build TMWorker
        TMWorker tm_worker;
        connect(&tm_worker, &TMWorker::tonemapSetMaximum, this,
                &CommandLineInterfaceManager::setProgressBar);
        connect(&tm_worker, &TMWorker::tonemapSetValue, this,
                &CommandLineInterfaceManager::updateProgressBar);
        connect(&tm_worker, &TMWorker::tonemapFailed, this,
                &CommandLineInterfaceManager::tonemapFailed);

        // Build a new TM frame
        // The scoped pointer will free the memory automatically later on
        QScopedPointer<pfs::Frame> tm_frame(tm_worker.computeTonemap(
            HDR.data(), tmopts.data(), BilinearInterp));

        // Autolevels
        if (isAutolevels) {
            float minL, maxL, gammaL;
            QScopedPointer<QImage> temp_qimage(
                fromLDRPFStoQImage(tm_frame.data()));
            computeAutolevels(temp_qimage.data(), 0.985f, minL, maxL, gammaL);
            pfs::gammaAndLevels(tm_frame.data(), minL, maxL, 0.f, 1.f, gammaL);
        }
        if (tmopts->postsaturation != 1)
            printIfVerbose(tr("\nApplying saturation enhancement %1.").arg(tmopts->postsaturation),
                           verbose);
        if (tmopts->postgamma != 1)
            printIfVerbose(tr("\nApplying post-gamma %1.").arg(tmopts->postgamma),
                           verbose);

        // Create an ad-hoc IOWorker to save the file
        if (IOWorker().write_ldr_frame(
                tm_frame.data(), saveLdrFilename, inputfname,
                hdrCreationManager.data() ? hdrCreationManager->getExpotimes()
                                          : QVector<float>(),
                tmopts.data(), *tmofileparams)) {
            // File save successful
            printIfVerbose(
                tr("\nImage %1 successfully saved").arg(saveLdrFilename),
                verbose);
        } else {
            // File save failed
            printErrorAndExit(
                tr("\nERROR: Cannot save to file: %1").arg(saveLdrFilename));
        }
        if (isHtml && !isHtmlDone) {
            generateHTML();
        }
        emit finishedParsing();
    } else {
        printIfVerbose(tr("Tonemapping NOT requested."), verbose);
        if (isHtml && !isHtmlDone) {
            generateHTML();
        }
        emit finishedParsing();
    }
}

void CommandLineInterfaceManager::errorWhileLoading(
    const QString &errormessage) {
    printErrorAndExit(tr("Failed loading images: %1").arg(errormessage));
}

void CommandLineInterfaceManager::setProgressBar(int max) {
    maximum = max;
    oldValue = 0;
    progressBar.reset();
    progressBar.n = max;
    if (verbose) std::cout << std::endl;
    started = true;
}

void CommandLineInterfaceManager::updateProgressBar(int value) {
    if (verbose) {
        if (value < 0) return;
        if (value < oldValue) {
            // progressBar.reset();
            // progressBar.n = maximum;
            // progressBar.start();
            progressBar.cur = value;
            progressBar.setPct(((float)value) / maximum);
        }
        if (started) {
            started = false;
            progressBar.start();
        }
        for (int i = 0; i < value - oldValue; i++) ++progressBar;
        oldValue = value;
        // if (value == progressBar.n) {
        //  std::cout << std::endl;
        //}
    }
}

void CommandLineInterfaceManager::readData(const QByteArray &data) {
    if (verbose) std::cout << data.constData() << std::endl;
}

void CommandLineInterfaceManager::tonemapFailed(const QString &e) {
    printErrorAndExit(e);
}
