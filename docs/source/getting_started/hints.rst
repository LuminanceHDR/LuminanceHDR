
*****
Hints
*****

.. contents::
  

Single Raw file workflow
========================

You can load directly a raw file in LuminanceHDR via the "File->Load Hdr..." menu item.
Doing so you will be able to tone map directly this single raw file.
This means that there is no need to create different exposures in Ufraw from your raw file.

When the user wants to load a single raw file in the main workspace a RAW->TIFF conversion takes place by calling the dcraw executable.
For more information please read the page about :doc:`Raw Conversion </dcraw>`.

If you still don't like what LuminanceHDR does with your raw file and you want to process you raw file directly before loading it in LuminanceHDR
(white balance, color profile, and so on), you can use Ufraw to tweak the color settings of your raw file, and then save the result as a 16-bits tiff file.
Just remember to save the result as a 16-bits file, or you'll lose some dynamic range in the process.


Post-processing of the LDR
==========================

#. If you don't like the result of a specific tone mapping operator,
   please keep in mind that after the tone mapping step you can still use tools like GIMP to post process the resulting image.
   For example, you can still fix the brightness, change the gamma or the levels, and so on.

#. Some users have reported [1] pleasant results combining in GIMP 2 LDRs: one obtained with Fattal and the other one with Drago.
   Drago for the first layer and Fattal for the second in overlay mode (70% can be a good starting point).
   This is not a silver bullet technique, these values can be thought as a starting point,
   you can then go on tweaking the opacity value and the tone mapping parameters, your mileage may vary.

#. You can also put the Fattal image down as the master and then layer the Drago image on top in overlay mode with c.50% opacity [1].
   [1] http://www.flickr.com/groups/luminance/discuss/72157600715644855/


Align_image_stack
=================

align_image_stack is a tool that can be found in the hugin project (http://hugin.sf.net).
It is a standalone tool (an exe in windows) that can perform various functions,
we use it in LuminanceHDR to do what it says, i.e. to align a stack of (LDR) images.

Linux
   As of today (23 Nov 2007) the hugin project has not published a release with align_image_stack
   in it yet (they are working hard for their next release).
   Linux users have to checkout the project's subversion repository and compile the sources.
   On this page ( http://luminance.wiki.sourceforge.net/align_image_stack ) you can find how to do that.
Mac OS X
   The align_image_stack executable has already been included in the dmg file you downloaded.
Windows
   In windows the align_image_stack.exe file has to stay in the same directory of the luminance.exe file
   (or, as an alternative, in one of the directories listed in the PATH environment variable). 
