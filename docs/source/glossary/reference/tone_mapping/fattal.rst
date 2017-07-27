
******
Fattal
******

The implementation of a tone mapping operator as described in:

Gradient Domain High Dynamic Range Compression R. Fattal, D. Lischinski, and M. Werman
In ACM Transactions on Graphics (3), p. 249, 2002.

With respect to the original paper, this program provides additional parameter which limits the amplification of noise.
The noise is often starkly amplified because of division by zero in one of the equations in the paper.
Extension contributed by Przemyslaw Bazarnik.

At the core of the program is a Poisson PDE which as suggested in the original paper
is solved using a Full Multigrid Algorithm. However, this is an iterative solver which
seems to lose accuracy when applied to higher resolution images resulting in halo effects
and surreal looking images. For that reason a second solver has been implemented using the
discrete cosine transform as the underlying method and is considerably more accurate
mainly because it is a direct solver. This solver is the preferred method and is used by default.
The old multigrid solver can be selected unchecking the Version 2.3.0 checkbox.  

.. note::

   Please note that resulted LDR image always requires gamma correction which you can do using Levels tool.

Options
=======

Alpha
   Set alpha parameter. This parameter is depreciated as setting a value other than 1.0
   has only the effect of a global gamma adjustment of the luminance channel.
   See the paper for the definition of alpha. 
Beta
   Set beta parameter. It sets the strength of gradient (local contrast) modification.
   Suggested range is 0.8 to 0.96, default is 0.9 (see paper for details).
   Value of 1 does not change contrasts, values above 1 reverse the effect:
   local contrast is stretched and details are attenuated.
   Values below 0.5 lead to very strong amplification of small contrast,
   so consider using Noise Reduction parameter to prevent noise. 
Color Saturation
   Amount of color saturation. Suggested range is 0.4 to 0.8. Default value: 0.8. 
Noise Reduction
   Amount of noise reduction.
 