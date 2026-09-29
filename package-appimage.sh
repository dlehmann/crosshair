#!/usr/bin/env bash
# Builds crosshair and packages it, together with every shared library it
# needs (X11, Cairo, librsvg + its GNOME-stack dependencies, yaml-cpp, ...),
# into a single, self-contained crosshair-x86_64.AppImage.
#
# That AppImage runs on other machines without installing any of this
# project's dependencies first - unlike handing someone the raw binary,
# which only works if their system happens to have matching versions of
# every shared library already installed (see README's Technical Details).
#
# Uses linuxdeploy (+ its appimage plugin) to do the actual dependency
# discovery/bundling; both are downloaded on first run into
# .appimage-tools/ (gitignored, kept outside build/ so `rm -rf build` doesn't
# force re-downloading them) and reused afterwards.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build"
TOOLS_DIR="${SCRIPT_DIR}/.appimage-tools"
APPDIR="${BUILD_DIR}/AppDir"
OUTPUT_FILE="${BUILD_DIR}/crosshair-x86_64.AppImage"

LINUXDEPLOY="${TOOLS_DIR}/linuxdeploy-x86_64.AppImage"
LINUXDEPLOY_PLUGIN="${TOOLS_DIR}/linuxdeploy-plugin-appimage-x86_64.AppImage"

echo "==> Building (Release)"
"${SCRIPT_DIR}/build.sh" Release

mkdir -p "${TOOLS_DIR}"

download_tool() {
    local url="$1"
    local dest="$2"
    if [ ! -x "${dest}" ]; then
        echo "==> Downloading $(basename "${dest}")"
        curl -sL "${url}" -o "${dest}"
        chmod +x "${dest}"
    fi
}

download_tool \
    "https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage" \
    "${LINUXDEPLOY}"
download_tool \
    "https://github.com/linuxdeploy/linuxdeploy-plugin-appimage/releases/download/continuous/linuxdeploy-plugin-appimage-x86_64.AppImage" \
    "${LINUXDEPLOY_PLUGIN}"

echo "==> Preparing AppDir"
rm -rf "${APPDIR}"
mkdir -p "${APPDIR}/usr/bin" \
         "${APPDIR}/usr/share/applications" \
         "${APPDIR}/usr/share/icons/hicolor/scalable/apps" \
         "${APPDIR}/usr/share/icons/hicolor/256x256/apps"

install -m 755 "${BUILD_DIR}/crosshair" "${APPDIR}/usr/bin/crosshair"

# Exec must reference the binary by name (found via AppRun's PATH), not an
# absolute path, since the AppImage can be run from anywhere on the target
# machine.
sed \
    -e "s|@EXEC_PATH@|crosshair|g" \
    -e "s|@ICON_NAME@|crosshair|g" \
    "${SCRIPT_DIR}/desktop/crosshair.desktop.in" \
    > "${APPDIR}/usr/share/applications/crosshair.desktop"

install -m 644 "${SCRIPT_DIR}/assets/icon.svg" \
    "${APPDIR}/usr/share/icons/hicolor/scalable/apps/crosshair.svg"
rsvg-convert -w 256 -h 256 "${SCRIPT_DIR}/assets/icon.svg" \
    -o "${APPDIR}/usr/share/icons/hicolor/256x256/apps/crosshair.png"

echo "==> Running linuxdeploy (bundles shared library dependencies)"
export PATH="${TOOLS_DIR}:${PATH}"
# --appimage-extract-and-run avoids needing FUSE to run these tool AppImages
# themselves - useful in containers/CI where FUSE often isn't available.
export APPIMAGE_EXTRACT_AND_RUN=1
OUTPUT="${OUTPUT_FILE}" \
"${LINUXDEPLOY}" --appimage-extract-and-run \
    --appdir "${APPDIR}" \
    --executable "${APPDIR}/usr/bin/crosshair" \
    --desktop-file "${APPDIR}/usr/share/applications/crosshair.desktop" \
    --icon-file "${SCRIPT_DIR}/assets/icon.svg" \
    --output appimage

echo
echo "Done: ${OUTPUT_FILE}"
echo "Copy that single file to another machine and run it directly:"
echo "  chmod +x $(basename "${OUTPUT_FILE}")"
echo "  ./$(basename "${OUTPUT_FILE}")"
