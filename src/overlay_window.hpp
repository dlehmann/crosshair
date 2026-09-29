// The permanent, click-through, always-on-top crosshair overlay window.
#pragma once

#include <X11/Xlib.h>
#include <cairo/cairo.h>
#include <librsvg/rsvg.h>

#include <string>

class OverlayWindow {
public:
    // Creates the overlay window (centered on the primary monitor) but does
    // NOT map it yet - call show() once it should actually become visible.
    // If svg_path exists and is loadable, it is rendered; otherwise a
    // programmatic high-contrast fallback crosshair is drawn instead.
    OverlayWindow(Display* display, int screen, const std::string& svg_path);
    ~OverlayWindow();

    OverlayWindow(const OverlayWindow&) = delete;
    OverlayWindow& operator=(const OverlayWindow&) = delete;

    Window window() const { return window_; }

    // Maps the window, making the crosshair visible. Intended to be called
    // once the startup notification dialog has been dismissed.
    void show();

    // Redraws the crosshair; call in response to Expose events.
    void redraw();

    // Re-asserts top-of-stack position. Some fullscreen applications can
    // otherwise obscure the overlay over time.
    void raise_to_top();

private:
    Display* display_;
    int screen_;
    Window window_;
    Visual* visual_;
    Colormap colormap_;
    cairo_surface_t* surface_;
    RsvgHandle* svg_handle_; // may be nullptr -> fallback drawing is used
    unsigned int width_;
    unsigned int height_;

    void load_svg_or_fallback(const std::string& svg_path);
    void make_click_through();
    void draw_fallback_crosshair(cairo_t* cr) const;
};
