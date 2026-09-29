#!/usr/bin/env bash
# Builds crosshair and installs it for the current user:
#   - binary + assets -> ~/.local/share/crosshair/
#   - application icon -> ~/.local/share/icons/hicolor/scalable/apps/
#   - .desktop launcher -> ~/.local/share/applications/
#
# No root privileges are required or used.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build"
INSTALL_DIR="${HOME}/.local/share/crosshair"
ICON_DIR="${HOME}/.local/share/icons/hicolor/scalable/apps"
APPLICATIONS_DIR="${HOME}/.local/share/applications"

"${SCRIPT_DIR}/build.sh" Release

echo "==> Installing binary and assets to ${INSTALL_DIR}"
mkdir -p "${INSTALL_DIR}"
install -m 755 "${BUILD_DIR}/crosshair" "${INSTALL_DIR}/crosshair"
# Ship the example SVG for reference, but never as "crosshair.svg" itself -
# the program auto-generates its own default crosshair.svg on first run.
install -m 644 "${SCRIPT_DIR}/assets/crosshair.example.svg" "${INSTALL_DIR}/crosshair.example.svg"

echo "==> Installing application icon to ${ICON_DIR}"
mkdir -p "${ICON_DIR}"
install -m 644 "${SCRIPT_DIR}/assets/icon.svg" "${ICON_DIR}/crosshair.svg"

echo "==> Generating and installing .desktop launcher to ${APPLICATIONS_DIR}"
mkdir -p "${APPLICATIONS_DIR}"
sed \
    -e "s|@EXEC_PATH@|${INSTALL_DIR}/crosshair|g" \
    -e "s|@ICON_NAME@|crosshair|g" \
    "${SCRIPT_DIR}/desktop/crosshair.desktop.in" \
    > "${APPLICATIONS_DIR}/crosshair.desktop"
chmod 644 "${APPLICATIONS_DIR}/crosshair.desktop"

if command -v update-desktop-database >/dev/null 2>&1; then
    update-desktop-database "${APPLICATIONS_DIR}" >/dev/null 2>&1 || true
fi
if command -v gtk-update-icon-cache >/dev/null 2>&1; then
    gtk-update-icon-cache -f -t "${HOME}/.local/share/icons/hicolor" >/dev/null 2>&1 || true
fi

echo
echo "Installation complete."
echo "  Binary:   ${INSTALL_DIR}/crosshair"
echo "  Launcher: ${APPLICATIONS_DIR}/crosshair.desktop"
echo
echo "Crosshair should now appear in the KDE Plasma application launcher."
echo "To use a custom crosshair, copy an SVG file to:"
echo "  ${INSTALL_DIR}/crosshair.svg"
