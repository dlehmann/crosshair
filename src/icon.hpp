// Sets the application's taskbar/window-list icon directly, so it always
// shows the crosshair regardless of whether the program is installed (i.e.
// regardless of any .desktop file / icon theme lookup by the window manager).
#pragma once

#include <X11/Xlib.h>

// Renders the built-in crosshair icon at several sizes and sets it as the
// window's _NET_WM_ICON property.
void set_taskbar_icon(Display* display, Window window);
