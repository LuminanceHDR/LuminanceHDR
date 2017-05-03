
***********
Translating
***********

Translating desktop entry
=========================

On Linux systems .desktop files are used to build system menus that list applications available to users.
Here is what it looks like in GNOME desktop environment:

.. figure:: /images/not-translated-menu-item.png

Luminance HDR ships with such a file as well.
It is located in root directory with source code and gets installed to
/usr/share/applications or /usr/local/share/applications, depending on your preferences.

To get a localized menu entry you need to do a very simple thing:

#. Open this file in your preferred text editor and make sure you opened it as a UTF-8 encoded text file.
#. Create a new entry which looks like "Comment[LANG]=Create and tonemap HDR images",
   where LANG is a two-letter code for your language
   (as referenced by `ISO-639-1 <https://en.wikipedia.org/wiki/List_of_ISO_639-1_codes>`)
   and everything after "=" is translated.
#. Create a new entry which looks like "GenericName[LANG]=HDR imaging".
#. Save.
#. Test by running 'sudo make install' (if Luminance HDR is already installed or just
   'sudo cp luminance.desktop /usr/share/applications/' and look in the menu.

You should see something like this:

.. figure:: /images/translated-menu-item.png

Send the updated file to us.


Translating user interface
==========================

The very first thing you need to translate Luminance HDR into your native language
is to get source code from the current development branch.
To do this, you need a Subversion client (svn being the regular command line client). Then type::

   svn co https://qtpfsgui.svn.sourceforge.net/svnroot/qtpfsgui/trunk/qtpfsgui qtpfsgui

somewhere in your home directory to fetch source code, so that you always have access to it.

Then you will need to install Qt development package that contains Linguist --
the application to assist you with translating.
On Linux start your package manager and look for a package named something like qt4-dev, install it.

The next steps are as follows:

#. Go to the top level directory of Luminance HDR's source code.
#. Open the file called project.pro.
#. Find a section that starts with "TRANSLATIONS =" and add a new line that looks like "i18n/lang_LOCALE.ts \",
   where LOCALE is a two-letter code for your language (as referenced by ISO-639-1), lowercase, e.g. pt for Portuguese.
#. Save the project.pro file.
#. Open console in that directory and run "$ lupdate-qt4 project.pro" command.
   This will create a new translation file (and update existing translation files).
#. Open your lang_LOCALE.ts file in Linguist and start translating.
#. To test your translation, use File > Release menu item in Linguist to create a binary version of translation
   and then run sudo make install from console to quickly install created translation in the right place.
   When your work is done, compress this lang_LOCALE.ts file to a ZIP, GZ or BZ2 archive and submit it via patches tracker.

Here are some tips to help you make translation better.

Translating Luminance HDR takes a while, so it's best to translate those parts of user interface that you use most of the time.
This will give you a false, but useful feeling of accomplishment and motivation to finish the whole work.

Test your translation as frequently as possible. This is especially important for dialogs that you rarely use.

Make sure you find a good balance between short and easy to understand phrases and words.
English language is known to have relatively shorter words, so in most cases your translation
will make user interface a bit larger. But if you start using abbreviations or shorter synonyms
that don't quite fit the context, users won't appreciate that either.

Some translatable messages use variables like %1. Those are substituted by some values.
For example, in "Using %1 thread(s)" (Batch Tonemapping dialog) this variable is substituted
with amount of threads used to process HDR images into LDR images. When you type these variables manually,
you can make a mistake and the trick with a variable won't work. So it's better to paste
original text to translation entry field by pressing Ctrl+B in Linguist and then replace
this original text with translation, leaving all present variables intact.
