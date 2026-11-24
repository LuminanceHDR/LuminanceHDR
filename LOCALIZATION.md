# Translating Luminance HDR

## Table of contents

1. [Overview](#overview)
2. [English source strings](#source)
3. [Updating translations](#updating)
4. [Adding a language](#adding)
5. [Validation](#validation)
6. [Translation rules](#rules)
7. [Transifex](#transifex)
8. [Help documentation](#help)

## Overview <a name="overview"></a>

Luminance HDR uses Qt 6 Linguist. User interface strings are stored in
`i18n/lang_*.ts` files. Qt compiles them to `.qm` files during the build and
installs them into `share/luminance-hdr/i18n` on Unix or `Resources/i18n`
inside the macOS bundle.

English requires no `.qm` file: `src/Common/TranslatorManager.cpp` treats `en`
as the untranslated source language.

Qt 6 LinguistTools is required (`lupdate`, `lrelease`, and the CMake
`Qt6LinguistTools` package).

## English source strings <a name="source"></a>

There is no tracked English `.ts` file. `i18n/.gitignore` ignores
`i18n/source.ts`, which is generated from source files by `lupdate`.

To regenerate it and refresh all translation files:

```bash
cmake -S . -B build -DUPDATE_TRANSLATIONS=ON
cmake --build build --target translations_target
cmake -S . -B build -DUPDATE_TRANSLATIONS=OFF
```

`LUPDATE_NOOBSOLETE=ON` passes `-noobsolete` to `lupdate`, removing messages
that no longer exist in the source. Leave it `OFF` when reviewing obsolete
strings with translators.

For a report-only validation without changing `.ts` files, generate a
temporary English reference:

```bash
mkdir -p /tmp/lhdr-i18n
git ls-files 'src/*.cpp' 'src/*.h' 'src/*.hxx' 'src/*.ui' \
  > /tmp/lhdr-i18n/sources.lst
lupdate -no-obsolete @/tmp/lhdr-i18n/sources.lst \
  -ts /tmp/lhdr-i18n/source.ts
```

## Updating translations <a name="updating"></a>

1. Regenerate `source.ts` as above.
2. Open the target `i18n/lang_XX.ts` in Qt Linguist.
3. Translate only `unfinished` messages.
4. Preserve placeholders, mnemonics, and rich-text tags (see below).
5. Run the validator.
6. Build with `UPDATE_TRANSLATIONS=OFF` to confirm `.qm` generation.

Do not edit generated `.qm` files.

## Adding a language <a name="adding"></a>

1. Create `i18n/lang_XX.ts` using the same TS format as existing files, with
   `language="XX"` on the root `TS` element.
2. Add the language mapping to `i18n/.tx/config`.
3. Confirm `src/Common/TranslatorManager.cpp` can resolve the language code.
4. Check plural rules for the locale; `lrelease` cannot infer them.
5. Run the validator and include the language in release testing.

## Validation <a name="validation"></a>

`build_files/validate_translations.py` is report-only: it does not modify
translation files. It checks coverage, unfinished/vanished messages, Qt
placeholders, keyboard mnemonics, rich-text tags, whitespace, and source
location drift.

```bash
python3 build_files/validate_translations.py \
  --source-ts /tmp/lhdr-i18n/source.ts \
  --output i18n/TRANSLATION_STATUS.md
```

Use a fresh `source.ts` as the English reference whenever source strings
change. Without `--source-ts`, the script uses the consensus `<source>` text
from the committed translations, which may be stale.

The generated report is `i18n/TRANSLATION_STATUS.md`. Errors must be fixed
before release; warnings require human review.

## Translation rules <a name="rules"></a>

- Keep every Qt placeholder unchanged: `%1`, `%2`, `%L1`, `%n`, and `%%`.
- Preserve keyboard mnemonics (`&`). Use a unique letter in each menu where
  possible. A literal `%` is not a mnemonic.
- Preserve HTML and rich-text tags such as `<b>`, `</b>`, `<br>`, `<p>`,
  `</p>`, `<html>`, and `</html>` unless a change is deliberate and reviewed.
- Do not translate syntax tokens such as `<page_name>`.
- Keep leading and trailing newlines that carry meaning in console output.
- Do not add trailing whitespace.
- Do not translate product names, file extensions, or documented command-line
  tokens unless the project has an established translation.
- Plural forms must be complete for the target locale.

## Transifex <a name="transifex"></a>

`i18n/.tx/config` maps `source.ts` to `lang_<lang>.ts`. Regenerate and push the
English source before pushing translations:

```bash
tx push --source
tx push --translations
tx pull --all
```

The old client commands `tx push -s -t` and `tx pull -a` still work with
legacy configurations but are not recommended.

## Help documentation <a name="help"></a>

Translated user documentation lives in `help/<language>/`. The English source
is `help/en/`. When translating documentation, preserve image paths and HTML
structure. Desktop-file translations belong in
`net.sourceforge.qtpfsgui.LuminanceHDR.desktop`.

See `help/en/contributing_translating.html` for a short introduction.
