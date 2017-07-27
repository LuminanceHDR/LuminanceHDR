
*********
Pattanaik
*********

The implementation of a tone mapping operator as described in:

Time-Dependent Visual Adaptation for Realistic Image Display. S.N. Pattanaik, J. Tumblin,
H. Yee, and D.P. Greenberg. In Proceedings of ACM SIGGRAPH 2000.

.. note::

   This operator requires properly calibrated image data (in cd/m2) and its results should be gamma corrected.

The local version of this operator is based on the following paper:

Adaptive Gain Control For High Dynamic Range Image Display. S.N. Pattanaik, H. Yee. In proceedings of SCCG 2002.


Options
=======

Multiplier
   Multiply input values by a multiplier value. Useful if input data is not calibrated. Default value: 1.0 
Local Tone Mapping
   Use local version of the tone mapping. Time-dependent effects are cancelled
   while using local version, global version is used by default. 
Cone Level
   Set the adaptation level for cones. By default, the adaptation level
   is calculated as a logarithmic average of luminance in the input image. 
Rod Level
   Set the adaptation level for rods. By default, the adaptation level
   is calculated as a logarithmic average of luminance in the input image.
