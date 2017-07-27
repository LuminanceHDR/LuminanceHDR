
*********
Ferradans
*********

The implementation of a tone mapping operator as described in:

   S. Ferradans, M. Bertalmio, E. Provenzi and V. Caselles 
   An analysis of visual adaptation and contrast perception for tone mapping. 
   In: Trans. on Pattern Analysis and Machine Intelligence, 2011. 

.. note:: Note that this operator does NOT usually require gamma correction.

Options
=======

Rho
   Controls over all lightness(related to the adaptation level),
   the greater the value the brighter the final image. Default value: -2 
Inverse Alpha
   Controls the level of detail, the higher the value the bigger the level of detail enhancement. Default value: 5
