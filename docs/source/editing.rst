
*******
Editing
*******

After creating an new HDR file or opening an existing one you can do several things to them except tonemapping.


Resizing
========

You can access this feature via the Edit > Resize... menu item.

Luminance HDR can resize an HDR image to a given pixel size of percentage value counting from the original.
If you use percentage, thi final size in pixels will be calculated and displayed to the right from *Height* entry field.

.. figure:: /images/resize.png

Clicking **Scale** button will resize the HDR image.


Cropping
========

To crop an HDR file to some area first you need to select this are.
Click somewhere on an image, drag the mouse pointer to a side and release it.
You will see something like this:

.. figure:: /images/cropping_frame.png

You can further edit the frame you created by dragging its edges or corners.
You can also move the frame around by clicking inside it and dragging mouse pointer
(that will change from an arrow to a hand icon).

When the frame is placed correctly, choose Edit > Crop to Selection in menu or use the relevant button in the toolbar.
Luminance HDR will create a new unsaved HDR image that contains cropped version of the original image.

.. figure:: /images/edit_menu.png

To get rid of the selection frame simply single-click anywhere outside the frame or use the Edite > Remove Selection menu item.


Rotating
========

You can rotate an HDR image to 90 degrees a step, using Edit > Rotate Counter-Clockwise and Edit > Rotate Clockwise commands or
:kbd:`<` and :kbd:`>` shortcuts respectively. Unlike setting an Exif orientation tag this will physically modify the HDR image.


Projective Transformation
=========================

You can access this feature via the Edit > Projective Transformation... menu item.

With this tool the user is able to apply projective transformations to an HDR image.
This is useful if you shoot mirrorball Hdrs and you want to unwrap them, for example.

The Angular projection accepts an Angle parameter which defines how many degrees from the viewing direction the projection should cover.

.. figure:: /images/projectiveTransformationDialog.png


Adjust Levels
=============

TODO.
