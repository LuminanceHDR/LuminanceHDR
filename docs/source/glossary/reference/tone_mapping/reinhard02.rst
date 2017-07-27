
***********
Reinhard'02
***********

The implementation of the Reinhard tone mapping operator as described in:

Photographic Tone Reproduction for Digital Images. E. Reinhard, M. Stark, P. Shirley,
and J. Ferwerda. In ACM Transactions on Graphics, 2002.

.. note:: According to the paper, results of this TMO require gamma correction.


Options
=======

Key Value
   Set key value for the image (refer to paper for details). Default value: 0.18, accepted range <0..1>. 
Phi Value
   Set phi value (refer to paper for details). Default value: 1.0, accepted range >=0.0. 
Use Scales
   Use scales to calculate local adaptation.
   That means: use local version of this operator. By default, global version is used. 
Range
   Set range size (refer to paper for details). Default value: 8, accepted range >1. 
Lower Scale
   Set lower scale size (refer to paper for details). Default value: 1, accepted range >=1. 
Upper Scale
   Set upper scale size (refer to paper for details). Default value: 43, accepted range >=1. 
