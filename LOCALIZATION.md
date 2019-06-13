## This is the README file for contributing translations (i18n) to Luminance HDR
-------------------------------------------------------------------

# Table of Contents
---------
1. [Introduction](#intro)
2. [Operations](#operations)


# Introduction <a name="intro"></a>
---------------------------------------------------------------------
Translations can be managed either using the classical Qt approach or
the [Transifex](https://www.transifex.com/luminance-hdr/) online platform.

The Qt approach is implemented using a CMake option (UPDATE_TRANSLATIONS). If this
is set to true, then the translations in the i18n directory are updated (using Qt's lupdate).

In the following section the operations using the online platform are described.


# Operations <a name="operations"></a>
---------------------------------------------------------------------
First, download the Transifex Client, available for Windows, Linux, Mac.

Open a command shell and navigate (CD) into the i18n directory of Luminance HDR.

The first time you use the client you need to initialize it with your credentials.
Therefore type:

```bash
tx init
```

If after an lupdate you want to update the strings online, you do

```bash
tx push -s -t
```

In order to download the translated strings do

```bash
tx pull -a
```

More options for each operation is available in the help


```bash
tx help
```

or

```bash
tx help push
```

