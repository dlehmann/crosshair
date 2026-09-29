// The application's control window: a small, regular (window-manager
// managed) dialog that offers a way to exit the program. It starts mapped
// in the WM's iconic/minimized state, so it shows up as a taskbar entry
// without a dialog box popping up on screen. Clicking that taskbar entry (or
// a second process instance asking the window manager to activate it)
// un-minimizes it, revealing the "Exit Crosshair?" dialog.
#pragma once

#include <X11/Xlib.h>
#include <cairo/cairo.h>

#include <string>

#include "config.hpp"

class ExitDialogWindow {
public:
    ExitDialogWindow(Display* display, int screen, const DialogStrings& strings);
    ~ExitDialogWindow();

    ExitDialogWindow(const ExitDialogWindow&) = delete;
    ExitDialogWindow& operator=(const ExitDialogWindow&) = delete;

    Window window() const { return window_; }
    Atom wm_delete_atom() const { return wm_delete_window_; }

    // True once the Exit button has been clicked - the whole application
    // should shut down.
    bool exit_requested() const { return exit_requested_; }

    void redraw();

    // Handles a ButtonPress at the given coordinates: requests application
    // exit if it landed on the Exit button, or just dismisses the dialog
    // (re-minimizing it) if it landed on the Cancel button.
    void handle_button_press(int x, int y);

    // Re-minimizes the window back to a taskbar-only entry. The application
    // keeps running.
    void iconify();

    // True if the dialog is currently mapped/on-screen. Used to tell apart
    // "the visible dialog's own close control was used" (should cancel)
    // from "the taskbar's Close/Quit action was used while minimized"
    // (should exit) - both arrive as the same WM_DELETE_WINDOW message.
    bool is_visible() const;

private:
    Display* display_;
    int screen_;
    Window window_;
    cairo_surface_t* surface_;
    Atom wm_delete_window_;
    DialogStrings strings_;
    bool exit_requested_;

    int width_;
    int height_;
    int cancel_button_x_;
    int exit_button_x_;
    int button_y_;
    int button_w_;
    int button_h_;
};
