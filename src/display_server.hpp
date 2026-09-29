// Detects whether the process is running under an X11 or Wayland session.
#pragma once

#include <string>

enum class DisplayServerType {
    X11,
    Wayland,
    Unknown
};

struct DisplayServerInfo {
    DisplayServerType type;
    std::string description;
};

// Inspects XDG_SESSION_TYPE / WAYLAND_DISPLAY / DISPLAY to classify the
// current session. This does not change how the overlay connects to the
// display (it always uses Xlib, relying on XWayland when necessary) - it is
// purely informational and printed to the console at startup.
DisplayServerInfo detect_display_server();
