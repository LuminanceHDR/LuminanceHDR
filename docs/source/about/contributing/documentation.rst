
*************
Documentation
*************

TODO

.. Needs to be re-written

Translations
============

The documentation is inside help directory and consists of menu.xml file
that defines table of contents, HTML files with text and illustrations in PNG or JPEG files.

Every translation is kept in its own directory named with two-letter language code
like ru for Russian or es for Spanish. So download source code,
unpack it and create a copy of help/en directory in help directory.

Start translating. It's best to translate table of contents first and proceed with actual content later.
To translate table of contents open menu.xml file in your editor of choice and translate values of every text attribute.
E.g. for <area text="Setting up Luminance HDR" file="prefs.html"> translatable text will be "Setting up Luminance HDR".

To test your translation open a terminal window, go to the top level directory with source code,
and run sudo make install to reinstall Luminance HDR. All available translations will be automaticaly copied to the right place,
and you will have to restart Luminance HDR to let it pick the added translation.
However, as you progress with your translation, you only need to restart help browser
to see changes in table of contents and you don't even need to restart the help browser
to see changes in separate chapters — just click on some other chapter and go back again.

If user interface is not localized, you might want to do it before translating docs.
Some users might complain and tell you that user interface in English is a de-facto standard
and thus localized documentation should refer only to English UI.
But this is just because they have grown up to use unlocalized software, so don't you worry.

The English (and Russian) translations have screenshots with Dust theme for both GTK+ and Metacity (and GTK+ engine for Qt),
and Droid Sans 9pt font. You don't have to try to reproduce that, but please be visually consistent across your translation.

Please keep all of your illustrations below 800 pixels on the longer side.
The reason is: when an image doesn't fit help browser's window, a nasty horizontal scrollbar appears.
To get rid of it you need to grow width of the window, and that means that text will reflow
and there will be too long barely readable lines of text.

When you are done, archive help/LANGUAGE directory with your translation and send it to us.
