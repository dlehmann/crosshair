// Small helpers shared by the overlay and tooltip windows: EWMH atom
// lookups, "always on top" state, and primary-monitor geometry via XRandR.
#pragma once

#include <X11/Xlib.h>

struct Rect {
    int x;
    int y;
    unsigned int width;
    unsigned int height;
};

// Returns the geometry of the primary monitor (via XRandR) so the crosshair
// can be centered on the screen the user actually plays on. Falls back to
// the full default-screen geometry if XRandR is unavailable or reports no
// primary output.
Rect get_primary_monitor_geometry(Display* display, int screen);

// Applies the EWMH hints that make a window behave like a permanent overlay:
// always-on-top, skip taskbar/pager, and (optionally) a window type suited
// for on-screen decorations rather than a regular application window.
void set_always_on_top_hints(Display* display, Window window, bool skip_taskbar);

// Requests window-manager close via WM_DELETE_WINDOW and returns the atom so
// callers can recognize the corresponding ClientMessage event.
Atom enable_wm_delete_window(Display* display, Window window);

// Sets WM_CLASS so window managers/taskbars can group and (via a matching
// .desktop file's StartupWMClass) icon the window correctly.
void set_wm_class(Display* display, Window window, const char* instance_name, const char* class_name);

// Publishes this process's control/taskbar window on the root window, so a
// second process instance can find and activate it (see single_instance.hpp).
void publish_control_window(Display* display, Window root, Window control_window);

// Reads back the control window published by publish_control_window(), or 0
// if none is currently set.
Window find_running_control_window(Display* display, Window root);

// Asks the window manager to map (un-minimizing if necessary), raise, and
// focus the given window, via the standard EWMH _NET_ACTIVE_WINDOW request
// (the same mechanism tools like `wmctrl -a` and taskbar clicks use).
void request_window_activation(Display* display, Window root, Window target);
