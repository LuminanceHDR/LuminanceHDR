
*************************
Command Line HDR Creation
*************************

Below are shown the command line switches used to create an HDR image and tonemap it.
The relevant ones to just create an HDR are: -a to select the automatic alignment engine,
-e to enter the EV values (one per image) if the images do not contain Exif data, --hdrModel
to select the creation model and -s to save the resulting HDR

Usage: ./luminance-hdr-cli [OPTIONS]... [INPUTFILES]...:


  -h [ --help ]                 Display this help.
  -V [ --version ]              Display program version.
  -v [ --verbose ]              Print more messages during execution.
  -c [ --cameras ]              Print a list of all supported cameras.
  -a [ --align ] arg            [AIS|MTB]   Align Engine to use during HDR creation (default: no alignment).
  -e [ --ev ] arg               EV1,EV2,... Specify numerical EV values (as many as INPUTFILES).
  -d [ --savealigned ] arg      prefix Save aligned images to files which names start with prefix
  -l [ --load ] arg             HDR_FILE Load an HDR instead of creating a new one.
  -s [ --save ] arg             HDR_FILE Save to a HDR file format. (default: don't save)
  -g [ --gamma ] arg            VALUE Gamma value to use during tone mapping. (default: 1)
  -r [ --resize ] arg           VALUE Width you want to resize your HDR to (resized before gamma and tone mapping)
  -o [ --output ] arg           LDR_FILE File name you want to save your tone mapped LDR to.
  -t [ --autoag ] arg           THRESHOLD   Enable auto anti-ghosting with given threshold. (0.0-1.0)
  -b [ --autolevels ]           Apply autolevels correction after tonemapping.
  -w [ --createwebpage ]        Enable generation of a webpage with embedded HDR viewer.

HDR creation parameters  - you must either load an existing HDR file (via the -l option) or specify INPUTFILES to create a new HDR:

  --hdrWeight arg               weight = triangular|gaussian|plateau|flat (Default is triangular)
  --hdrResponseCurve arg        response curve = from_file|linear|gamma|log|srgb (Default is linear)
  --hdrModel arg                model: robertson|robertsonauto|debevec (Default is debevec)
  --hdrCurveFilename arg        curve filename = your_file_here.m

LDR output parameters:

  -q [ --ldrQuality ] arg       VALUE Quality of the saved tone mapped file (1-100).
  --ldrTiff arg                 Tiff format. Legal values are [8b|16b|32b|logluv] (Default is 8b)
  --ldrTiffDeflate arg          Tiff deflate compression. true|false (Default is true)
 
HTML output parameters:

  -k [ --htmlQuality ] arg      VALUE Quality of the interpolated exposures, from the worst (1) to the best(4).
                                Higher quality will introduce less distortions in the brightest
                                and the darkest tones, but will also generate more images.
                                More images means that there is moredata that needs to be
                                transferred to the web-browser, making HDR viewer less responsive.
                                (Default is 2, which is sufficient for most applications)
  --pageName arg                Specifies the file name, of the web page to be generated. If <page_name> is missing,
                                the file name of the first image with .html extension will be used. (Default is first image name)
  --imagesDir arg               Specify where to store the resulting image files
                                Links to images in HTML will be updated accordingly.
                                This must be a relative path and the directory must exist.
                                 Useful to avoid clutter in the current directory. (Default is current working directory)
 

Tone mapping parameters  - no tonemapping is performed unless -o is specified:

  --tmo arg                     Tone mapping operator. Legal values are: [ashikhmin|drago|durand|fattal|ferradans|
                                pattanaik|reinhard02|reinhard05|mai|mantiuk06|mantiuk08] (Default is mantiuk06)
  --tmofile arg                 SETTING_FILE Load an existing setting file containing pre-gamma and all TMO settings

Fattal:

  --tmoFatAlpha arg             alpha FLOAT
  --tmoFatBeta arg              beta FLOAT
  --tmoFatColor arg             color FLOAT
  --tmoFatNoise arg             noise FLOAT
  --tmoFatNew arg               new true|false

Ferradans:

  --tmoFerRho arg               rho FLOAT
  --tmoFerInvAlpha arg          inv_alpha FLOAT

Mantiuk 06:

  --tmoM06Contrast arg          contrast FLOAT
  --tmoM06Saturation arg        saturation FLOAT
  --tmoM06Detail arg            detail FLOAT
  --tmoM06ContrastEqual arg     equalization true|false

Mantiuk 08:

  --tmoM08ColorSaturation arg   color saturation FLOAT
  --tmoM08ConstrastEnh arg      contrast enhancement FLOAT
  --tmoM08LuminanceLvl arg      luminance level FLOAT
  --tmoM08SetLuminance arg      enable luminance level true|false

Durand:

  --tmoDurSigmaS arg            spatial kernel sigma FLOAT
  --tmoDurSigmaR arg            range kernel sigma FLOAT
  --tmoDurBase arg              base contrast FLOAT

Drago:

  --tmoDrgBias arg              bias FLOAT

Reinhard 02:

  --tmoR02Key arg               key value FLOAT
  --tmoR02Phi arg               phi FLOAT
  --tmoR02Scales arg            use scales true|false
  --tmoR02Num arg               range FLOAT
  --tmoR02Low arg               lower scale FLOAT
  --tmoR02High arg              upper scale FLOAT

Reinhard 05:

  --tmoR05Brightness arg        Brightness FLOAT
  --tmoR05Chroma arg            Chroma adaption FLOAT
  --tmoR05Lightness arg         Light adaption FLOAT

Ashikmin:

  --tmoAshEq2 arg               Equation number 2 true|false
  --tmoAshSimple arg            Simple true|false
  --tmoAshLocal arg             Local threshold FLOAT

Pattanaik:

  --tmoPatMultiplier arg        multiplier FLOAT
  --tmoPatLocal arg             Local tone mapping true|false
  --tmoPatAutoLum arg           Auto luminance true|false
  --tmoPatCone arg              cone level FLOAT
  --tmoPatRod arg               rod level FLOAT
