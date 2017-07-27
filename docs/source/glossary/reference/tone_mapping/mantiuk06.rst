
**********
Mantiuk'06
**********

The Mantiuk'06 tone mapping operator is an implementation of the "A Perceptual Framework for
Contrast Processing of High Dynamic Range Images" scientific paper written by Rafał Mantiuk,
Karol Myszkowski and Hans-Peter Seidel. The original purpose of the framework is to provide
means to adjust contrast locally without creating visible artifacts. Two ways to handle contrast are available:

#. Contrast mapping. In previous sections we introduce our framework for converting images
   to perceptually linearized contrast response and then restoring images from the modified response.
   In this section we show that one potential application of this framework is to compress the dynamic range
   of HDR images to fit into the contrast reproduction capabilities of display devices.
   We call this method contrast mapping instead of tone mapping because it operates
   on contrast response rather than luminance. We try to fit to the dynamic range
   of the display so that no information is lost due to saturation of luminance values and at the same time,
   small contrast details, such as textures, are preserved.
   Within our framework such non-trivial contrast compression operation is reduced
   to a linear scaling in the visual response space. Since the response Rk j is perceptually linearized,
   contrast reduction can be achieved by multiplying the response values by a constant l.
#. Contrast equalization. Sometimes part of an image has a large contrast than other parts of the same image.
   By equalizing the histogram of contrast Luminance HDR allocate dynamic range for each contrast level relative
   to the space it occupies in an image. The resulted LDR image is usually very sharp and quite often looks unnatural.

Options
=======

Contrast Equalization
   By enabling this checkbox you will switch from contrast mapping to contrast equalization.
   The Contrast Factor option below refers to contrast mapping algorithm only and doesn't affect equalization. 
Contrast Factor
   Contrast scaling factor (values 0-1) determines how much contrast magnitudes should be reduced.
   The lower value results in a sharper image. Default value: 0.1 
Saturation Factor
   Saturation correction (values 0-2). The lower value results in stronger desaturation. Default value: 0.8 
Detail Factor
   Currently disabled.
   
   .. note::
   
      Please note that resulted LDR image always requires gamma correction which you can do using *Levels* tool.
