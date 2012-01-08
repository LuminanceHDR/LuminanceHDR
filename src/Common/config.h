/**
 * This file is a part of Luminance HDR package.
 * ----------------------------------------------------------------------
 * Copyright (C) 2006,2007 Giuseppe Rota
 * Copyright (C) 2011 Davide Anastasia
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
 * @author Davide Anastasia <davideanastasia@users.sourceforge.net>
 *  Refactory of the options
 */

#ifndef LUMINANCECONFIG_H
#define LUMINANCECONFIG_H

#define LUMINANCEORGANIZATION "Luminance"
#define LUMINANCEAPPLICATION "Luminance"

#define LUMINANCEVERSION "2.2.0-beta2"
#define TMOSETTINGSVERSION "0.6"

#define KEY_TOOLBAR_MODE "MainWindowToolbarVisualizationMode"
#define KEY_TM_TOOLBAR_MODE "TonemappingWindowToolbarVisualizationMode"
#define KEY_MANUAL_AG_MASK_COLOR "ManualAntiGhostingMaskColor"
#define KEY_GUI_LANG "UserInterfaceLanguage"

#define KEY_EXTERNAL_AIS_OPTIONS "External_Tools_Options/ExternalAlignImageStackOptions"

#define KEY_NANINFCOLOR "HDR_visualization/nan_inf_color"
#define KEY_NEGCOLOR "HDR_visualization/neg_color"

#define KEY_TEMP_RESULT_PATH "Tonemapping_Options/TemporaryFilesPath"

#define KEY_SAVE_LOGLUV "TIFF_Options/TiffSaveLogLuv"

#define KEY_TMOWINDOW_PREVIEWS_WIDTH "TMOWindow_Options/TMOWindow_PreviewsWidth"
#define KEY_TMOWINDOW_MAX "TMOWindow_Options/TMOWindow_Max"
#define KEY_TMOWINDOW_SHOWPROCESSED "TMOWindow_Options/TMOWindow_ShowProcessed"
#define KEY_TMOWINDOW_SHOWPREVIEWPANEL "TMOWindow_Options/TMOWindow_ShowPreviewPanel"
#define KEY_WIZARD_SHOWFIRSTPAGE "HDR_Wizard_Options/Wizard_ShowFirstPage"

#define KEY_TMOWARNING_FATTALSMALL "TMOWarning_Options/TMOWarning_fattalsmall"

#define KEY_ABER_0 "Raw_Conversion_Options/aber_0"
#define KEY_ABER_1 "Raw_Conversion_Options/aber_1"
#define KEY_ABER_2 "Raw_Conversion_Options/aber_2"
#define KEY_ABER_3 "Raw_Conversion_Options/aber_3"
#define KEY_GAMM_0 "Raw_Conversion_Options/gamm_0"
#define KEY_GAMM_1 "Raw_Conversion_Options/gamm_1"
#define KEY_TK "Raw_Conversion_Options/TK"
#define KEY_GREEN "Raw_Conversion_Options/green"
#define KEY_USER_MUL_0 "Raw_Conversion_Options/user_mul_0"
#define KEY_USER_MUL_1 "Raw_Conversion_Options/user_mul_1"
#define KEY_USER_MUL_2 "Raw_Conversion_Options/user_mul_2"
#define KEY_USER_MUL_3 "Raw_Conversion_Options/user_mul_3"
#define KEY_USE_AUTO_BRIGHTNESS "Raw_Conversion_Options/use_auto_brightness"
#define KEY_BRIGHTNESS "Raw_Conversion_Options/brightness"
#define KEY_THRESHOLD "Raw_Conversion_Options/threshold"
#define KEY_HALF_SIZE "Raw_Conversion_Options/half_size"
#define KEY_FOUR_COLOR_RGB "Raw_Conversion_Options/four_color_rgb"
#define KEY_HIGHLIGHTS "Raw_Conversion_Options/highlights"
#define KEY_LEVEL "Raw_Conversion_Options/level"
#define KEY_WB_METHOD "Raw_Conversion_Options/wb_method"
#define KEY_OUTPUT_COLOR "Raw_Conversion_Options/output_color"
#define KEY_OUTPUT_PROFILE "Raw_Conversion_Options/output_profile"
#define KEY_CAMERA_PROFILE "Raw_Conversion_Options/camera_profile"
#define KEY_USER_FLIP "Raw_Conversion_Options/user_flip"
#define KEY_USER_QUAL "Raw_Conversion_Options/user_qual"
#define KEY_USER_BLACK "Raw_Conversion_Options/user_black"
#define KEY_USER_SAT "Raw_Conversion_Options/user_sat"
#define KEY_MED_PASSES "Raw_Conversion_Options/med_passes"
#define KEY_AUTO_BRIGHT "Raw_Conversion_Options/auto_bright"
#define KEY_AUTO_BRIGHT_THR "Raw_Conversion_Options/auto_bright_thr"
#define KEY_ADJUST_MAXIMUM_THR "Raw_Conversion_Options/adjust_maximum_thr"
#define KEY_DO_NOT_USE_FUJI_ROTATE "Raw_Conversion_Options/do_not_use_fuji_rotate"
#define KEY_USE_BLACK "Raw_Conversion_Options/use_black"
#define KEY_USE_SAT "Raw_Conversion_Options/use_sat"
#define KEY_USE_NOISE "Raw_Conversion_Options/use_noise"
#define KEY_USE_CHROMA "Raw_Conversion_Options/use_chroma"

//--------------------PATHS & co. ----------------
#define KEY_RECENT_PATH_LOAD_SAVE_HDR "Recent_path_loadsave_hdr"
#define KEY_RECENT_FILES "Recent_files_list"
#define KEY_RECENT_PATH_LOAD_SAVE_TMO_SETTINGS "Recent_path_TMO_settings"
#define KEY_RECENT_PATH_SAVE_LDR "Recent_path_save_ldr"
#define KEY_RECENT_PATH_LOAD_LDRs_FOR_HDR "Recent_path_input_for_hdr"
// Exif
#define KEY_RECENT_PATH_EXIF_FROM "Exif/Recent_path_exif_from"
#define KEY_RECENT_PATH_EXIF_TO "Exif/Recent_path_exif_to"
// Batch HDR
#define KEY_BATCH_HDR_PATH_INPUT "batch_hdr/path_input"
#define KEY_BATCH_HDR_PATH_OUTPUT "batch_hdr/path_output"
// Batch TM
#define KEY_BATCH_TM_PATH_INPUT "batch_tm/path_hdr_input"
#define KEY_BATCH_TM_PATH_TMO_SETTINGS "batch_tm/path_tmo_settings"
#define KEY_BATCH_TM_PATH_OUTPUT "batch_tm/path_ldr_output"
#define KEY_BATCH_TM_LDR_FORMAT "batch_tm/Batch_LDR_Format"
#define KEY_BATCH_TM_NUM_THREADS "batch_tm/Num_Batch_Threads"

#endif
