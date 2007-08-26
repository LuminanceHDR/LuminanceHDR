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

#ifndef CONFIG_H
#define CONFIG_H

#define QTPFSGUIVERSION "1.8.13"
#define TMOSETTINGSVERSION "0.3"

#define KEY_TOOLBAR_MODE "MainWindowToolbarVisualizationMode"

#define GROUP_DCRAW "DCRAW_Options"
#define KEY_AUTOWB "auto_white_balance"
#define KEY_CAMERAWB "camera_white_balance"
#define KEY_HIGHLIGHTS "highlights_recovery_mode"
#define KEY_QUALITY "quality_mode"
#define KEY_4COLORS "use_4colors_interpolation"
#define KEY_OUTCOLOR "output_color_space"

#define GROUP_HDRVISUALIZATION "HDR_visualization"
#define KEY_NANINFCOLOR "nan_inf_color"
#define KEY_NEGCOLOR "neg_color"

#define GROUP_TONEMAPPING "Tonemapping_Options"
#define KEY_TEMP_RESULT_PATH "TemporaryFilesPath"
#define KEY_BATCH_LDR_FORMAT "Batch_LDR_Format"
#define KEY_NUM_BATCH_THREADS "Num_Batch_Threads"

#define GROUP_TIFF "TIFF_Options"
#define KEY_SAVE_LOGLUV "TiffSaveLogLuv"

//--------------------PATHS & co. ----------------
#define KEY_RECENT_PATH_LOAD_SAVE_HDR "RecentDirHDRSetting"
#define KEY_RECENT_FILES "recentFileList"
#define KEY_RECENT_PATH_LOAD_SAVE_TMO_SETTINGS "RecentDirTMOSetting"
#define KEY_RECENT_PATH_SAVE_LDR "RecentDirLDRSetting"
#define KEY_RECENT_PATH_LOAD_LDRs_FOR_HDR "RecentDirInputLDRs"
#define KEY_RECENT_PATH_EXIF_FROM "recent_path_exif_from"
#define KEY_RECENT_PATH_EXIF_TO "recent_path_exif_to"

#endif
