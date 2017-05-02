
***********
HDR Options
***********

.. figure:: images/prefs-hdr.png

Here you can choose which TIFF-HDR format Luminance HDR will use to save an HDR image.

HDR TIFF Default File Format
   LogLuv TIFF
      The appropriate format for most cases because it stores the hdr data in a reasonable amount of space (i.e. file size).
      Even if it is a 16 bit format, it can store floating point data.
   Float TIFF
      This format has to be used only if you want to load your tiff in another application that cannot open the LogLuv format.

.. rubric:: HDR Visualization

Show negative numbers as
   Color to show negative color values.
Show nan and +/- inf values as
   Color to show nan/inf color values.
