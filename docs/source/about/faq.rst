
*********************
FAQ for Luminance HDR
*********************

Q: What is the meaning of the name Luminance HDR?
   A: The name can be decomposed in 3 parts: Qt-pfs-gui.

   :Qt: the program uses Qt4 (www.trolltech.com) to show its graphical widgets.
   :pfs: the main backend library and original sourcecode base.
   :gui: this stands simply for graphical user interface.

Q: What is the meaning of the various settings for tone mapping operator X?
   A: To answer precisely this question one would have to explain the inner workings of the tone mapping operator X, in terms of the original research paper.
   At the end of the day all that matters (to some people, at least) is to fiddle with the settings until you obtain a nice result. 
Q: Why can't Luminance HDR transfer the exif tags to TIFF files?
   A: Because the library Luminance HDR uses to perform this task (exiv2) doesn't support writing to tiff files yet. It's in the working, though.
Q: Should I store JPEG or RAW files for HDR?
   A: Both give the same result, provided you capture all the dynamic range in the scene. This means that with RAW files you may need to capture less files than with JPEGs. On the other hand creating an HDR with JPEG files is more "lightweight" process (reduced memory footprint). 
Q: Where can I get information about all the various HDR formats?
   A: An overview (rather technical) can be found at http://www.anyhere.com/gward/hdrenc/hdr_encodings.html.

   - OpenEXR: Industrial Light and Magic format, widespread use, best compression ratios. www.openexr.com
   - LogLuv TIFF: see http://en.wikipedia.org/wiki/Logluv_TIFF and http://www.anyhere.com/gward/pixformat/tiffluv.html
   - Radiance RGBE: see http://en.wikipedia.org/wiki/Radiance_%28software%29#HDR_image_format
   - PFS: This format stores the binary internal (float) representation of images.
     It is very size demanding, and it supported only by pfstools/pfscalibration/pfstmo,
     Luminance HDR and a few other applications. On the other hand it is a lossless format and supports metadata (tags).
   - Float TIFF (aka 32 bit TIFF): very size demanding (similar to pfs above).


Q: Can we have the tonemapping dialog apply its settings as soon as the user changes a value? In other words, can we avoid having an "apply" button?
   A: Given how the tone mapping panel is implemented right now, this possibility has been discarded.

