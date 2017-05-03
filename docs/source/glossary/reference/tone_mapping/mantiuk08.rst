
**********
Mantiuk'08
**********

This operator applies the display adaptive tone mapping, which attempts to preserve contrast
of an input (HDR) image as close as possible given the characteristic of an output display.
Use this tone mapping operator if you want to preserve original image appearance,
or slightly enhance contrast while maintaining the natural look of images.
The operator can also compensate for ambient light reflections on a screen,
and for varying dynamic range and brightness of a display.
The operator is suitable for video sequences as it prevents high-frequency changes
in tone-curve between consecutive frames, which would result in flickering.
Note that the temporal filtering is always active and there is no need to specify an argument to switch it on.
	
More details can be found in:	

   Rafal Mantiuk, Scott Daly and Louis Kerofsky.
   Display Adaptive Tone Mapping.
   In: ACM Transactions on Graphics 27 (3), 2008.
   http://www.mpi-inf.mpg.de/resources/hdr/datmo/

If you find this TMO useful in your research project, please cite the paper above.

This operator also employs color correction mechanism from:

   Radoslaw Mantiuk, Rafal Mantiuk, Anna Tomaszewska, Wolfgang Heidrich.
   Color Correction for Tone Mapping.
   In: Computer Graphics Forum (Proc. of EUROGRAPHICS’09), 28(2), 2009.
   http://zgk.wi.ps.pl/color_correction/

.. note:: The result of this TMO does not usually require gamma correction.

Options
=======

Color Saturation
   Decrease or increase color saturation after tone mapping.
   Default value 1 attempts to preserve color appearance of the original image.
   Use values >1 to increase and <1 to decrease color saturation. 
Contrast Enhancement
   By default this tone-mapper attempts to preserve contrast of an input image.
   This parameter controls whether the contrast of an input image should be enhanced before tone-mapping.
   For example 1.15 boosts contrast by 15%. Note that if a target display does not offer sufficient dynamic range,
   contrast may be enhanced only for selected tone-values (those that dominate in an image) or not enhanced at all. 
Luminance Level
    Tells the tone-mapper what luminance level in the input image should be mapped
    to the maximum luminance of a display. Since HDR images contain only relative
    luminance information, tone-mapper does not know how bright should be the scene.
    This option is meant to fix this problem by providing tone-mapper with the information
    what luminance level in an input image should be perceived as a diffuse white surface.
    Default is none, which means that no such mapping will be enforced and tone-mapper
    is free to find an optimal brightness for a given image. This is a recommended setting for HDR images. 
