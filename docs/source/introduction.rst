
************
Introduction
************

Luminance HDR is an open source graphical user interface application that provides a workflow for HDR imaging. 

Summary of features
===================

Current supported features include:

#. Create an HDR from a set of files.
#. Tone map an HDR image to get a LDR image.
#. Save and load HDR images.
#. Rotate and resize HDR images.
#. Apply projective transformations to HDR images.
#. Copy exif data between sets of images.

The **first feature** is accessible via the "File -> New Hdr..." wizard: in order to create an HDR the user can either load a set of JPEG files, a set of RAW files, or a set of TIFF files (8bit or 16bit).
Raw files are processed with LibRaw in order to obtain a (8 or 16 bit) tiff file. For more information read :doc:`this page </>`.
The pictures must have been taken at the same scene, with different exposure settings (change the exposure time and/or aperture,
and use a tripod if you have one). The newly created HDR will be available in the workspace as soon as the HDR creation wizard has ended.
The input files can be aligned via two alignment engines: align_image_stack and MTB.
The set of images can contain moving objects. This can result in an (unwanted) effect called ghosting. Luminance HDR provides an interactive anti-ghosting tool that can help avoid such artifacts.
Read the chapter :doc:`about the creation of an hdr </>` for more information about the alignment engines and the interactive anti-ghosting tool.
To tone map an HDR file to get an LDR image (**second feature**) you can press the "Tonemap the HDR" button.
Via the "File -> Open Hdr..." wizard you can choose to load in the workspace an HDR image image file, and the "File->Save Hdr as..."
item lets you save the currently selected hdr image to a HDR image file format (**third feature**).
Users can also rotate and resize (**fourth feature**) the currently selected hdr image via the "Image" menu item, see below.
It is also possible to apply panoramic (projective) transformation to a Hdr image via the "Image" menu item (**fifth feature**).
In order to create an HDR image Luminance HDR requires to have a set of images with exif data in it.
Luminance HDR requires this information to get the exposure settings for an image in the set.
When Luminance HDR doesn't find this information in an image it warns the user and aborts the hdr creation process.
To cope with this requirement Luminance HDR provides a panel that performs a one-to-one copy of the exif data between two sets of files (**sixth feature**).
