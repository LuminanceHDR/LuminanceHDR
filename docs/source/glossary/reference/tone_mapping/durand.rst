
******
Durand
******

The Durand tone mapping operator is an implementation of the "Fast Bilateral Filtering for
the Display of High-Dynamic-Range Images" scientific paper written by Frédo Durand and Julie Dorseye.
The original purpose of the algorithm is to adjust local contrast to bring out details of high dynamic range scenes.
To do so the operator decomposes every image into a base layer and a layer with fine details,
then compresses contrast of the base layer.

Options
=======

Spatial Kernel Sigma
   The larger the values is, the larger amount of details is preserved during decomposition. 
Range Kernel Sigma
   Larger values tend to result in slightly darker images with less homogenous distribution
   of pixels across the tonal range and much increased halo around edges. 
Base contrast
   Adjustment of contrast is perfomed only to the base layer to preserve details.
   The lower value is, the higher contrast compression and the darker the resulted picture is.
   Thus increasing this value a bit will render a brighter picture.
