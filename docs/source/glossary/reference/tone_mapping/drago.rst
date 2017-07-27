
*****
Drago
*****

The Drago tone mapping operator is an implementation of the "Adaptive Logarithmic Mapping
For Displaying High Contrast Scenes" scientific paper written by F. Drago, K. Myszkowski, T. Annen and N. Chiba.
The original purpose of the algorithm is to provide a high quality tone mapping technique to display
high contrast images on devices with limited dynamic range of luminance values.

To achieve this goal the operator calculates luminance of every pixel and maximum luminance of the whole image,
then divides them by average luminance and finally multiplies the result by user defined exposure factor
that is called bias. The bias power function is perfomed to the whole image splitted
to a set of 3×3px tiles (for efficiency sake).


Options
=======

Bias
   The only user defined option is bias. Smaller bias values produce significantly brighter pictures.
   Values between 0.7 and 0.9 seem to be most useful, but 0.85 is reported to be optimal according
   to a small survey held by developers of the algorithm. Thus 0.85 is used by default.
