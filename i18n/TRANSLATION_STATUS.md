# Translation validation report

English reference: `generated i18n/source.ts`
Translation directory: `i18n`

## Coverage

| Language | Done | Total | Complete | Unfinished | Same as English |
|---|---:|---:|---:|---:|---:|
| cs | 257 | 1192 | 21.6% | 935 | 19 |
| da | 860 | 1192 | 72.1% | 332 | 98 |
| de | 1099 | 1192 | 92.2% | 93 | 87 |
| es | 1074 | 1192 | 90.1% | 118 | 121 |
| fi | 614 | 1192 | 51.5% | 578 | 64 |
| fr | 1192 | 1192 | 100.0% | 0 | 139 |
| hu | 319 | 1192 | 26.8% | 873 | 37 |
| id | 285 | 1192 | 23.9% | 907 | 130 |
| it | 1192 | 1192 | 100.0% | 0 | 152 |
| nl | 264 | 1192 | 22.1% | 928 | 40 |
| pl | 164 | 1192 | 13.8% | 1028 | 32 |
| pt_BR | 1063 | 1192 | 89.2% | 129 | 131 |
| ro | 550 | 1192 | 46.1% | 642 | 52 |
| ru | 1060 | 1192 | 88.9% | 132 | 76 |
| tr | 593 | 1192 | 49.7% | 599 | 53 |
| zh | 677 | 1192 | 56.8% | 515 | 74 |

## Findings

- Errors: 0
- Warnings: 33
- Info: 1553

| Severity | Category | Count |
|---|---|---:|
| info | not-in-english | 289 |
| info | source-not-found | 1264 |
| warning | accelerator-missing | 8 |
| warning | html-tag | 16 |
| warning | newline | 9 |

## Errors and warnings

| Severity | Category | File | Line | Message |
|---|---|---|---:|---|
| warning | html-tag | lang_cs.ts | 4724 | PreferencesDialog: "Brightness" -> rich-text tags differ |
| warning | html-tag | lang_cs.ts | 5182 | QObject: "Color Saturation" -> rich-text tags differ |
| warning | html-tag | lang_cs.ts | 5264 | QObject: "Bias" -> rich-text tags differ |
| warning | html-tag | lang_cs.ts | 5275 | QObject: "Range" -> rich-text tags differ |
| warning | html-tag | lang_cs.ts | 5285 | QObject: "Multiplier" -> rich-text tags differ |
| warning | html-tag | lang_cs.ts | 5330 | QObject: "Brightness" -> rich-text tags differ |
| warning | html-tag | lang_cs.ts | 5335 | QObject: "Chromatic Adaptation" -> rich-text tags differ |
| warning | html-tag | lang_cs.ts | 5340 | QObject: "Light Adaptation" -> rich-text tags differ |
| warning | html-tag | lang_cs.ts | 6104 | TonemappingPanel: "Clicking this button you will be able to insert a <i>width</i> value for the size of the resulting LDR image.<br>After pressing Enter (or Return) a height val... |
| warning | html-tag | lang_da.ts | 4582 | PreferencesDialog: "<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.0//EN" "http://www.w3.org/TR/REC-html40/strict.dtd"> <html><head><meta name="qrichtext" content="1" /><style type="... |
| warning | newline | lang_da.ts | 5915 | TonemappingPanel: "Enable Luminace Level" -> newline count differs |
| warning | accelerator-missing | lang_de.ts | 4159 | MainWindow: "&Realtime Previews" -> mnemonic missing |
| warning | newline | lang_de.ts | 5931 | TonemappingPanel: "Enable Luminace Level" -> newline count differs |
| warning | newline | lang_es.ts | 1496 | CommandLineInterfaceManager: " Image %1 successfully saved" -> newline count differs |
| warning | newline | lang_es.ts | 1502 | CommandLineInterfaceManager: " ERROR: Cannot save to file: %1" -> newline count differs |
| warning | accelerator-missing | lang_es.ts | 3565 | MainWindow: "Rotate c&ounter-clockwise" -> mnemonic missing |
| warning | html-tag | lang_fr.ts | 11 | AboutLuminance: "<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.0//EN" "http://www.w3.org/TR/REC-html40/strict.dtd"> <html><head><meta name="qrichtext" content="1" /><style type="tex... |
| warning | newline | lang_fr.ts | 1591 | CommandLineInterfaceManager: " Image %1 successfully saved" -> newline count differs |
| warning | html-tag | lang_fr.ts | 4742 | PreferencesDialog: "<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.0//EN" "http://www.w3.org/TR/REC-html40/strict.dtd"> <html><head><meta name="qrichtext" content="1" /><style type="... |
| warning | newline | lang_fr.ts | 4948 | PreferencesDialog: "<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.0//EN" "http://www.w3.org/TR/REC-html40/strict.dtd"> <html><head><meta name="qrichtext" content="1" /><style type="... |
| warning | newline | lang_fr.ts | 4973 | PreferencesDialog: "<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.0//EN" "http://www.w3.org/TR/REC-html40/strict.dtd"> <html><head><meta name="qrichtext" content="1" /><style type="... |
| warning | html-tag | lang_it.ts | 2905 | HdrWizard: "<font color="#FF0000"><h3><b>WARNING:</b></h3></font> Luminance HDR was not able to find the relevant <b>EXIF</b> tags for the following images:<ul>" -> rich-text ta... |
| warning | html-tag | lang_it.ts | 3027 | HdrWizard: "<center><h3><b>To proceed you need to manually set the exposure values.<br><font color="#FF0000">%1</font> values still required.</b></h3></center>" -> rich-text tag... |
| warning | accelerator-missing | lang_pt_BR.ts | 114 | AboutLuminance: "&Thanks To" -> mnemonic missing |
| warning | html-tag | lang_pt_BR.ts | 1833 | EditingToolsDialog: "<html><head/><body><p>Enable auto anti-ghosting algorithm</p></body></html>" -> rich-text tags differ |
| warning | newline | lang_pt_BR.ts | 2923 | HdrWizard: "<center><font color="#008400"><h3><b>All the EV values have been set.<br>Now click on Next button.</b></h3></font></center>" -> newline count differs |
| warning | html-tag | lang_ru.ts | 5068 | PreferencesDialog: "<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.0//EN" "http://www.w3.org/TR/REC-html40/strict.dtd"> <html><head><meta name="qrichtext" content="1" /><style type="... |
| warning | accelerator-missing | lang_tr.ts | 110 | AboutLuminance: "&Thanks To" -> mnemonic missing |
| warning | accelerator-missing | lang_tr.ts | 144 | AboutLuminance: "&License Agreement" -> mnemonic missing |
| warning | accelerator-missing | lang_tr.ts | 2008 | EditingToolsDialog: "Reset &All" -> mnemonic missing |
| warning | accelerator-missing | lang_tr.ts | 2468 | HdrWizard: "Selected Image &Exposure" -> mnemonic missing |
| warning | newline | lang_tr.ts | 3176 | HelpBrowser: "<h2><p>Sorry, no manual is installed!</p><p>Please contact your package provider or LuminanceHDR team if you built the application yourself</p></h2>" -> newline co... |
| warning | accelerator-missing | lang_tr.ts | 3984 | MainWindow: "Remove &Selection" -> mnemonic missing |

## Informational examples

### not-in-english

- lang_cs.ts:188 — BatchHDRDialog: "Choose one of the creation predefined profiles. See documentation for more information." -> not in fresh English reference
- lang_cs.ts:428 — BatchHDRDialog: "The chosen output directory contains HDR files. Those files might be overwritten.  Continue?" -> not in fresh English reference
- lang_cs.ts:2332 — FitsImporter: "Cannot load FITS image %1. ERROR: %2" -> not in fresh English reference
- lang_cs.ts:2711 — HdrWizard: "Show HDR" -> not in fresh English reference
- lang_cs.ts:2716 — HdrWizard: "Show HDR Preview" -> not in fresh English reference
- lang_cs.ts:2747 — HdrWizard: "All formats (*.jpeg *.jpg *.tiff *.tif *.crw *.cr2 *.nef *.dng *.mrw *.orf *.kdc *.dcr *.arw *.raf *.ptx *.pef *.x3f *.raw *.sr2 *.rw2 *.3fr *.mef *.mos *.erf *.nrw ...
- lang_cs.ts:2752 — HdrWizard: "*.JPEG *.JPG *.TIFF *.TIF *.CRW *.CR2 *.NEF *.DNG *.MRW *.ORF *.KDC *.DCR *.ARW *.RAF *.PTX *.PEF *.X3F *.RAW *.SR2 *.RW2 *.3FR *.MEF *.MOS *.ERF *.NRW *.SRW);;" -> ...
- lang_cs.ts:2767 — HdrWizard: "RAW Images (*.crw *.cr2 *.nef *.dng *.mrw *.orf *.kdc *.dcr *.arw *.raf *.ptx *.pef *.x3f *.raw *.sr2 *.rw2 *.3fr *.mef *.mos *.erf *.nrw *.srw" -> not in fresh Engl...
- lang_cs.ts:2772 — HdrWizard: "*.CRW *.CR2 *.NEF *.DNG *.MRW *.ORF *.KDC *.DCR *.ARW *.RAF *.PTX *.PEF *.X3F *.RAW *.SR2 *.RW2 *.3FR *.MEF *.MOS *.ERF *.NRW *.SRW)" -> not in fresh English reference
- lang_cs.ts:2859 — HdrWizard: "Compute" -> not in fresh English reference

### source-not-found

- lang_cs.ts:386 — BatchHDRDialog: "<html><head/><body><p>Threshold used by auto anti-ghosting algorithm. A lower threshold means a more aggressive processing but that can lower the final dynamic ...
- lang_cs.ts:386 — BatchHDRDialog: "<html><head/><body><p>Threshold used by auto anti-ghosting algorithm. A lower threshold means a more aggressive processing but that can lower the final dynamic ...
- lang_cs.ts:428 — BatchHDRDialog: "The chosen output directory contains HDR files. Those files might be overwritten.  Continue?" -> source not found in current tree
- lang_cs.ts:436 — BatchHDRDialog: "Total number of pictures must be a multiple of number of bracketed images." -> source not found in current tree
- lang_cs.ts:436 — BatchHDRDialog: "Total number of pictures must be a multiple of number of bracketed images." -> source not found in current tree
- lang_cs.ts:759 — BatchTMDialog: "Load tone mapping settings text files..." -> source not found in current tree
- lang_cs.ts:764 — BatchTMDialog: "Luminance HDR tone mapping settings text file (*.txt)" -> source not found in current tree
- lang_cs.ts:774 — BatchTMDialog: "An error occurred reading one or more tonemapping options files." -> source not found in current tree
- lang_cs.ts:875 — CommandLineInterfaceManager: "[AIS\|MTB]   Align Engine to use during HDR creation (default: no alignment)." -> source not found in current tree
- lang_cs.ts:915 — CommandLineInterfaceManager: "VALUE       Width you want to resize your HDR to (resized before gamma and tone mapping)" -> source not found in current tree

