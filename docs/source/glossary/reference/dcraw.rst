
***************
DCRaw Reference
***************

Here's an excerpt from `DCRaw's man page <http://cybercom.net/~dcoffin/dcraw/dcraw.1.html>`__
(Retrieved: October 30, 2007).

-v
    Print verbose messages, not just warnings and errors.
-c
    Write decoded images or thumbnails to standard output.
-e
    Extract the camera-generated thumbnail, not the raw image.
    You'll get either a JPEG or a PPM file, depending on the camera. 
-z
    Change the access and modification times of an AVI, JPEG, TIFF,
    or raw file to when the photo was taken, assuming that the camera clock was set to Universal Time. 
-i
    Identify files but don't decode them. Exit status is 0 if dcraw can decode the last file,
    1 if it can't. -i -v shows metadata. dcraw cannot decode JPEG files!! 
-d
    Show the raw data as a grayscale image with no interpolation.
    Good for photographing black-and-white documents. 
-D
    Same as -d, but totally raw (no color scaling). 
-h
    Output a half-size color image. Twice as fast as -q 0. 
-q 0
    Use high-speed, low-quality bilinear interpolation. 
-q 1
    Use Variable Number of Gradients (VNG) interpolation. 
-q 2
    Use Patterned Pixel Grouping (PPG) interpolation. 
-q 3
    Use Adaptive Homogeneity-Directed (AHD) interpolation. 
-f
    Interpolate RGB as four colors. Use this if the output shows false 2x2 meshes with VNG or mazes with AHD. 
-m number_of_passes
    After interpolation, clean up color artifacts by repeatedly applying a 3x3 median filter to the R-G and B-G channels. 
-n noise_threshold
    Use wavelets to erase noise while preserving real detail.
    The best threshold should be somewhere between 100 and 1000. 
-b brightness
    By default, dcraw writes 8-bit PGM/PPM/PAM with a BT.709 gamma curve and a 99th-percentile white point.
    If the result is too light or too dark, -b lets you adjust it. Default is 1.0. 
-4
    Write 16-bit linear pseudo-PGM/PPM/PAM with no gamma curve, no white point, and no -b option. 
-T
    Write TIFF output (with metadata) instead of PGM/PPM/PAM. 
-k black
    Set the black point. Default depends on the camera. 
-K darkframe.pgm
    Subtract a dark frame from the raw data.
    To generate a dark frame, shoot a raw photo with no light and do dcraw -D -4 -j -t 0. 
-w
    Use the white balance specified by the camera. If this is not found, print a warning and use another method. 
-a
    Calculate the white balance by averaging the entire image. 
-A left top width height
    Calculate the white balance by averaging a rectangular area.
    First do dcraw -j -t 0 and select an area of neutral grey color. 
-r mul0 mul1 mul2 mul3
    Specify your own raw white balance. These multipliers can be cut and pasted from the output of dcraw -v. 
no white balance option
    Use a fixed white balance based on a color chart illuminated with a standard D65 lamp.
+M or -M
    Use (or don't use) any color matrix from the camera metadata.
    The default is +M if -w is set, -M otherwise. This option only affects Olympus, Leaf, and Phase One cameras. 
-C red_mag blue_mag
    Enlarge the raw red and blue layers by the given factors,
    typically 0.999 to 1.001, to correct chromatic aberration. 
-H 0
    Clip all highlights to solid white (default). 
-H 1
    Leave highlights unclipped in various shades of pink. 
-H 2
    Blend clipped and unclipped values together for a gradual fade to white. 
-H 3-9
    Reconstruct highlights. Low numbers favor whites; high numbers favor colors.
    Try -H 5 as a compromise. If that's not good enough, do -H 9,
    cut out the non-white highlights, and paste them into an image generated with -H 3. 
-o [0-5]
    Select the output colorspace when the -p option is not used:

        0   Raw color (unique to each camera)
        1   sRGB D65 (default)
        2   Adobe RGB (1998) D65
        3   Wide Gamut RGB D65
        4   Kodak ProPhoto RGB D65
        5   XYZ 

-p camera.icm [ -o output.icm ]
    Use ICC profiles to define the camera's raw colorspace and the desired output colorspace (sRGB by default).
-p embed
    Use the ICC profile embedded in the raw photo. 
-t [0-7,90,180,270]
    Flip the output image. By default, dcraw applies the flip specified by the camera. -t 0 disables all flipping. 
-s [0..N-1] or -s all
    If a file contains N raw images, choose one or "all" to decode.
    For example, Fuji Super CCD SR cameras generate a second image underexposed four stops to show detail in the highlights. 
-j
    For Fuji Super CCD cameras, show the image tilted 45 degrees.
    For cameras with non-square pixels, do not stretch the image to its correct aspect ratio.
    In any case, this option guarantees that each output pixel corresponds to one raw pixel. 
