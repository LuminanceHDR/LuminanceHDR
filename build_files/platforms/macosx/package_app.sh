#!/usr/bin/env bash

set -euo pipefail

usage() {
    echo "Usage: $0 SOURCE_APP OUTPUT_APP [EXPECTED_ARCHITECTURES]" >&2
    echo "The output must not exist. macdeployqt is run exactly once." >&2
}

if [[ $# -lt 2 || $# -gt 3 ]]; then
    usage
    exit 2
fi

source_app="$1"
output_app="$2"
expected_architectures="${3:-arm64}"
script_dir=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)

if [[ ! -d "$source_app" || "${source_app##*.}" != "app" ]]; then
    echo "Not an app bundle: $source_app" >&2
    exit 1
fi

source_parent=$(cd "$(dirname "$source_app")" && pwd -P)
source_app="${source_parent}/$(basename "$source_app")"
if [[ -d "$source_app/Contents/Frameworks/QtCore.framework" ||
      -e "$source_app/Contents/Resources/.lhdr-macdeployqt-complete" ]]; then
    echo "Refusing an app that has already been processed by macdeployqt: $source_app" >&2
    exit 1
fi

output_parent=$(dirname "$output_app")
mkdir -p "$output_parent"
output_parent=$(cd "$output_parent" && pwd -P)
output_app="${output_parent}/$(basename "$output_app")"
if [[ -e "$output_app" ]]; then
    echo "Refusing to overwrite existing output: $output_app" >&2
    exit 1
fi
if [[ "$output_app" == "$source_app" || "$output_app" == "$source_app"/* ]]; then
    echo "Output cannot be the source app or a path inside it." >&2
    exit 1
fi

if [[ -n "${QT_PREFIX:-}" ]]; then
    qt_prefix="$QT_PREFIX"
elif brew_prefix=$(brew --prefix qt 2>/dev/null) &&
     [[ -x "$brew_prefix/bin/macdeployqt" ]]; then
    qt_prefix="$brew_prefix"
elif brew_prefix=$(brew --prefix qtbase 2>/dev/null) &&
     [[ -x "$brew_prefix/bin/macdeployqt" ]]; then
    qt_prefix="$brew_prefix"
else
    echo "Set QT_PREFIX to a Qt 6 installation containing macdeployqt." >&2
    exit 1
fi

macdeployqt="$qt_prefix/bin/macdeployqt"
if [[ ! -x "$macdeployqt" ]]; then
    echo "macdeployqt not found at $macdeployqt" >&2
    exit 1
fi

plugin_dir=""
for qtpaths_candidate in "$qt_prefix/bin/qtpaths" "$qt_prefix/bin/qtpaths6"; do
    if [[ -x "$qtpaths_candidate" ]]; then
        plugin_dir=$("$qtpaths_candidate" --query QT_INSTALL_PLUGINS)
        break
    fi
done
if [[ -z "$plugin_dir" || ! -d "$plugin_dir" ]]; then
    echo "Could not locate the Qt plugin directory." >&2
    exit 1
fi

ditto --noextattr --noqtn "$source_app" "$output_app"
touch "$output_app/Contents/Resources/.lhdr-packaging-in-progress"

# Older build trees put documentation directly in Contents, where codesign
# classifies unknown entries as nested code. Normalize them into Resources.
documentation_dir="$output_app/Contents/Resources/Documentation"
mkdir -p "$documentation_dir"
for documentation_file in AUTHORS README.md LICENSE Changelog; do
    if [[ -f "$output_app/Contents/$documentation_file" ]]; then
        mv "$output_app/Contents/$documentation_file" "$documentation_dir/"
    fi
done

plugin_paths=(
    platforms/libqcocoa.dylib
    styles/libqmacstyle.dylib
    imageformats/libqgif.dylib
    imageformats/libqicns.dylib
    imageformats/libqico.dylib
    imageformats/libqjpeg.dylib
    imageformats/libqsvg.dylib
    imageformats/libqtga.dylib
    imageformats/libqtiff.dylib
    imageformats/libqwbmp.dylib
    imageformats/libqwebp.dylib
    iconengines/libqsvgicon.dylib
    sqldrivers/libqsqlite.dylib
    tls/libqcertonlybackend.dylib
    tls/libqsecuretransportbackend.dylib
    networkinformation/libqapplenetworkinformation.dylib
)

deploy_arguments=(
    "-executable=$output_app/Contents/MacOS/luminance-hdr-cli"
)
if [[ -x "$output_app/Contents/MacOS/align_image_stack" ]]; then
    deploy_arguments+=("-executable=$output_app/Contents/MacOS/align_image_stack")
fi

homebrew_prefix=$(brew --prefix)
# Boost 1.90's thread and program_options libraries load Boost.Container as a
# sibling via @loader_path. macdeployqt does not discover that sibling when it
# first copies those libraries, so seed it into the one deployment pass.
boost_container="$homebrew_prefix/lib/libboost_container.dylib"
if [[ -f "$boost_container" ]]; then
    boost_container_destination="$output_app/Contents/Frameworks/libboost_container.dylib"
    mkdir -p "$(dirname "$boost_container_destination")"
    ditto --noextattr --noqtn "$(realpath "$boost_container")" \
        "$boost_container_destination"
    deploy_arguments+=("-executable=$boost_container_destination")
fi

for relative_plugin in "${plugin_paths[@]}"; do
    plugin_source="$plugin_dir/$relative_plugin"
    if [[ ! -f "$plugin_source" ]]; then
        continue
    fi
    plugin_source=$(realpath "$plugin_source")
    plugin_destination="$output_app/Contents/PlugIns/$relative_plugin"
    mkdir -p "$(dirname "$plugin_destination")"
    ditto --noextattr --noqtn "$plugin_source" "$plugin_destination"
    deploy_arguments+=("-executable=$plugin_destination")
done

deploy_arguments+=("-libpath=$homebrew_prefix/lib")
if [[ -n "${LHDR_EXTRA_LIBPATHS:-}" ]]; then
    old_ifs="$IFS"
    IFS=:
    for library_path in $LHDR_EXTRA_LIBPATHS; do
        if [[ -d "$library_path" ]]; then
            deploy_arguments+=("-libpath=$library_path")
        fi
    done
    IFS="$old_ifs"
fi

"$macdeployqt" "$output_app" \
    "${deploy_arguments[@]}" \
    -always-overwrite \
    -no-strip \
    -no-plugins \
    -no-codesign

# Qt 6's WebEngine helper is created during the deployment pass, so repair any
# absolute framework references in all deployed Mach-O files afterwards rather
# than invoking macdeployqt a second time.
while IFS= read -r -d '' candidate; do
    if ! file -b "$candidate" | grep -q 'Mach-O'; then
        continue
    fi
    while IFS= read -r dependency; do
        replacement=""
        case "$dependency" in
            /*.framework/Versions/*/*)
                framework_root="${dependency%%.framework/*}.framework"
                framework_name="${framework_root##*/}"
                framework_suffix="${dependency#"$framework_root"/}"
                if [[ -e "$output_app/Contents/Frameworks/$framework_name/$framework_suffix" ]]; then
                    replacement="@rpath/$framework_name/$framework_suffix"
                fi
                ;;
            /*.dylib)
                library_name="${dependency##*/}"
                if [[ -e "$output_app/Contents/Frameworks/$library_name" ]]; then
                    replacement="@rpath/$library_name"
                fi
                ;;
        esac
        if [[ -n "$replacement" ]]; then
            install_name_tool -change "$dependency" "$replacement" "$candidate"
        fi
    done < <(otool -L "$candidate" | sed -n '2,$s/^[[:space:]]*\([^[:space:]]*\).*/\1/p')

    install_id=$(otool -D "$candidate" 2>/dev/null | sed -n '2p' || true)
    if [[ "$install_id" == /* && "$candidate" == "$output_app/Contents/Frameworks/"* ]]; then
        framework_relative="${candidate#"$output_app/Contents/Frameworks/"}"
        install_name_tool -id "@rpath/$framework_relative" "$candidate"
    fi
done < <(find "$output_app/Contents" -type f -print0)

# macdeployqt rewrites the helper's libraries into Contents/Frameworks but does
# not add a search path when the executable does not use Qt itself. Ensure every
# top-level executable can also resolve transitive @rpath dependencies there.
bundle_framework_rpath="@loader_path/../Frameworks"
for candidate in "$output_app/Contents/MacOS/"*; do
    if [[ ! -f "$candidate" ]] ||
       ! file -b "$candidate" | grep -q 'Mach-O'; then
        continue
    fi
    if ! otool -l "$candidate" | awk '
        /cmd LC_RPATH/ { in_rpath = 1; next }
        in_rpath && /path / {
            sub(/^[[:space:]]*path /, "")
            sub(/ \(offset.*/, "")
            print
            in_rpath = 0
        }
    ' | grep -Fxq "$bundle_framework_rpath"; then
        install_name_tool -add_rpath "$bundle_framework_rpath" "$candidate"
    fi
done

webengine_helper="$output_app/Contents/Frameworks/QtWebEngineCore.framework/Versions/A/Helpers/QtWebEngineProcess.app/Contents/MacOS/QtWebEngineProcess"
if [[ -x "$webengine_helper" ]]; then
    helper_framework_rpath="@executable_path/../../../../../../.."
    if ! otool -l "$webengine_helper" | grep -Fq "$helper_framework_rpath"; then
        install_name_tool -add_rpath "$helper_framework_rpath" "$webengine_helper"
    fi
fi

rm -f "$output_app/Contents/Resources/.lhdr-packaging-in-progress"
touch "$output_app/Contents/Resources/.lhdr-macdeployqt-complete"
xattr -cr "$output_app"

codesign_identity="${LHDR_CODESIGN_IDENTITY:--}"
codesign_arguments=(--force --sign "$codesign_identity")
if [[ "$codesign_identity" == "-" ]]; then
    codesign_arguments+=(--timestamp=none)
elif [[ "${LHDR_HARDENED_RUNTIME:-0}" == "1" ]]; then
    codesign_arguments+=(--options runtime --timestamp)
else
    codesign_arguments+=(--timestamp)
fi

while IFS= read -r -d '' candidate; do
    if ! file -b "$candidate" | grep -q 'Mach-O'; then
        continue
    fi
    case "$candidate" in
        "$output_app/Contents/MacOS/luminance-hdr"|"$webengine_helper"|*.framework/*)
            continue
            ;;
    esac
    codesign "${codesign_arguments[@]}" "$candidate"
done < <(find "$output_app/Contents" -type f -print0)

while IFS= read -r nested_app; do
    codesign "${codesign_arguments[@]}" "$nested_app"
done < <(find "$output_app/Contents" -depth -type d -name '*.app' -print)
if [[ -d "$output_app/Contents/Frameworks" ]]; then
    while IFS= read -r framework; do
        codesign "${codesign_arguments[@]}" "$framework"
    done < <(find "$output_app/Contents/Frameworks" -depth -type d -name '*.framework' -print)
fi
codesign "${codesign_arguments[@]}" "$output_app"

"$script_dir/verify_bundle.sh" "$output_app" "$expected_architectures"

if [[ "${LHDR_CREATE_DMG:-0}" == "1" ]]; then
    dmg_path="${LHDR_DMG_PATH:-${output_app%.app}.dmg}"
    if [[ -e "$dmg_path" ]]; then
        echo "Refusing to overwrite existing DMG: $dmg_path" >&2
        exit 1
    fi
    hdiutil create -fs HFS+ -format UDZO -srcfolder "$output_app" "$dmg_path"
    echo "Created $dmg_path"
fi

echo "Packaged $output_app"
