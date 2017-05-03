
********
Glossary
********

.. toctree::

   reference/index.rst

.. glossary::
   :sorted:

   HDR
      Stands for "High Dynamic Range". An HDR image is an image which presents more than 8 bit per color channel.
      Most CRTs, LCDs and printers only have a limited dynamic range, and can display only LDR images (see below).
      Thus various methods of "converting" HDR images into a viewable format have been developed, generally called "tone mapping".

   LDR
      Stands for "Low Dynamic Range". The most common image formats, such as JPEG, PNG, GIF, ...
      have 8 bits per color channel, LDR is just another umbrella definition.

   Tone mapping
      A method of converting an HDR image into a LDR image. Various algorithms exist for this purpose,
      and in this context they are also known as "tone mapping operators", or in this manual simply as "operators".
      You can choose an operator from a list in the top of tone mapping options sidebar.

   TMO
      Shorthand for "Tone Mapping Operator".

   Raw
      Another umbrella definition for several (minimally processed) image formats.
      Raw files can have 12 or 14 bits per color channel, but noise usually cuts down
      the available dynamic range to something like 1000:1, roughly 10 bits.
      For all intents and purposes they are HDR files.

   Anti-ghosting
      The HDR creation process involves merging a stack of images.
      An object changing position in the image set creates a strange effect
      in which the object is partially visible (like a ghost) in the final HDR image.
      This problem can be corrected by automatic or manual procedures. 
