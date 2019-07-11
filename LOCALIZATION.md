## This is the README file for contributing translations (i18n) to Luminance HDR
-------------------------------------------------------------------

# Table of Contents
---------
1. [Contributing Translations](#contrib)
2. [Managing Translations](#manage)


# Contributing Translations <a name="contrib"></a>
---------------------------------------------------------------------
Contributing translations and, doing so, helping improving Luminance HDR is very easy.

The translation process is managed through the web-based [Transifex](https://www.transifex.com/luminance-hdr/) platform.

You can spend few minutes of your time, whenever you want, just translating few strings of the UI in your native language or helping reviewing the work of other translators.
If you are not a developer and still willing to help, please read the guide [Getting Started as a Translator|Transifex Documentation](https://docs.transifex.com/getting-started-1/translators)

If instead you are a developer contributing code to the Luminance HDR project, you might be interested in the information given below.


# Managing Translations <a name="manage"></a>
---------------------------------------------------------------------
Managing the translations is implemented using a CMake option (UPDATE_TRANSLATIONS).
When changes are made to the UI, setting this option to TRUE will force *make* to update the files containing the translations in the *i18n* directory (Qt's *lupdate* will be called).
You can then rerun CMAKE with the option set to OFF.

To synchronize the local content with the Transifex online platform, you first install the Transifex Client, available for Windows, Linux, Mac.

Open a command shell and navigate (CD) into the i18n directory of Luminance HDR source tree.

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

