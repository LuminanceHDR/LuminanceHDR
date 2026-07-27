#!/usr/bin/env bash

set -euo pipefail

usage() {
    echo "Usage: $0 APP_BUNDLE [EXPECTED_ARCHITECTURES]" >&2
    echo "Example: $0 'Luminance HDR.app' arm64" >&2
}

if [[ $# -lt 1 || $# -gt 2 ]]; then
    usage
    exit 2
fi

app_path="$1"
expected_architectures="${2:-arm64}"

if [[ ! -d "$app_path" || "${app_path##*.}" != "app" ]]; then
    echo "Not an app bundle: $app_path" >&2
    exit 1
fi

app_parent=$(cd "$(dirname "$app_path")" && pwd -P)
app_path="${app_parent}/$(basename "$app_path")"
main_executable="$app_path/Contents/MacOS/luminance-hdr"
cli_executable="$app_path/Contents/MacOS/luminance-hdr-cli"
align_executable="$app_path/Contents/MacOS/align_image_stack"

for required_path in \
    "$app_path/Contents/Info.plist" \
    "$main_executable" \
    "$cli_executable"; do
    if [[ ! -e "$required_path" ]]; then
        echo "Required bundle file is missing: $required_path" >&2
        exit 1
    fi
done

if [[ "${LHDR_REQUIRE_ALIGN_IMAGE_STACK:-1}" == "1" && ! -x "$align_executable" ]]; then
    echo "The release bundle is missing align_image_stack." >&2
    exit 1
fi

normalize_architectures() {
    printf '%s\n' "$1" | tr ' ' '\n' | sed '/^$/d' | sort | tr '\n' ' ' | sed 's/ $//'
}

expected_normalized=$(normalize_architectures "$expected_architectures")
temporary_dir=$(mktemp -d "${TMPDIR:-/tmp}/lhdr-verify.XXXXXX")
errors_file="$temporary_dir/errors"
touch "$errors_file"
trap 'rm -rf -- "$temporary_dir"' EXIT

mach_o_count=0
while IFS= read -r -d '' candidate; do
    if ! file -b "$candidate" | grep -q 'Mach-O'; then
        continue
    fi
    mach_o_count=$((mach_o_count + 1))
    relative_path="${candidate#"$app_path"/}"
    actual_architectures=$(lipo -archs "$candidate")
    actual_normalized=$(normalize_architectures "$actual_architectures")
    if [[ "$actual_normalized" != "$expected_normalized" ]]; then
        echo "$relative_path: architectures '$actual_architectures', expected '$expected_architectures'" >> "$errors_file"
    fi

    while IFS= read -r dependency; do
        case "$dependency" in
            /System/Library/*|/usr/lib/*)
                ;;
            /*)
                echo "$relative_path: external dependency $dependency" >> "$errors_file"
                ;;
        esac
    done < <(otool -L "$candidate" | sed -n '2,$s/^[[:space:]]*\([^[:space:]]*\).*/\1/p')

    install_id=$(otool -D "$candidate" 2>/dev/null | sed -n '2p' || true)
    case "$install_id" in
        /System/Library/*|/usr/lib/*|"")
            ;;
        /*)
            echo "$relative_path: external install name $install_id" >> "$errors_file"
            ;;
    esac
done < <(find "$app_path/Contents" -type f -print0)

if [[ "$mach_o_count" -eq 0 ]]; then
    echo "No Mach-O files found in the bundle." >&2
    exit 1
fi

if [[ -s "$errors_file" ]]; then
    echo "Bundle verification failed:" >&2
    sed 's/^/  - /' "$errors_file" >&2
    exit 1
fi

plutil -lint "$app_path/Contents/Info.plist" >/dev/null
if [[ "${LHDR_SKIP_CODESIGN_CHECK:-0}" != "1" ]]; then
    codesign --verify --deep --strict --verbose=2 "$app_path"
fi

QT_QPA_PLATFORM=offscreen "$cli_executable" --version >/dev/null
if [[ -x "$align_executable" ]]; then
    "$align_executable" --help >/dev/null
fi

echo "Verified $mach_o_count Mach-O files in $(basename "$app_path"): $expected_architectures, self-contained, signed, and smoke-tested."
