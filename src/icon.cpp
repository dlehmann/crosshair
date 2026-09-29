#include "icon.hpp"

#include <X11/Xatom.h>
#include <cairo/cairo.h>

#include <cmath>
#include <cstdint>
#include <vector>

namespace {

// Draws the crosshair app icon (dark circle badge + black/white cross +
// center ring) into a size x size ARGB32 image, matching assets/icon.svg.
cairo_surface_t* render_icon_surface(int size) {
    cairo_surface_t* surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, size, size);
    cairo_t* cr = cairo_create(surface);

    const double s = size;
    const double cx = s / 2.0;
    const double cy = s / 2.0;

    // Badge background.
    cairo_set_source_rgb(cr, 0x1b / 255.0, 0x1f / 255.0, 0x27 / 255.0);
    cairo_arc(cr, cx, cy, s * 0.469, 0, 2 * M_PI);
    cairo_fill(cr);

    auto draw_cross = [&](double line_width, double gray) {
        cairo_set_line_width(cr, line_width);
        cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
        cairo_set_source_rgb(cr, gray, gray, gray);

        cairo_move_to(cr, cx, s * 0.125);
        cairo_line_to(cr, cx, s * 0.344);
        cairo_move_to(cr, cx, s * 0.656);
        cairo_line_to(cr, cx, s * 0.875);
        cairo_move_to(cr, s * 0.125, cy);
        cairo_line_to(cr, s * 0.344, cy);
        cairo_move_to(cr, s * 0.656, cy);
        cairo_line_to(cr, s * 0.875, cy);
        cairo_stroke(cr);
    };

    draw_cross(s * 0.109, 0.0);
    draw_cross(s * 0.047, 1.0);

    cairo_set_line_width(cr, s * 0.063);
    cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
    cairo_arc(cr, cx, cy, s * 0.094, 0, 2 * M_PI);
    cairo_stroke(cr);

    cairo_set_line_width(cr, s * 0.031);
    cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
    cairo_arc(cr, cx, cy, s * 0.094, 0, 2 * M_PI);
    cairo_stroke(cr);

    cairo_destroy(cr);
    cairo_surface_flush(surface);
    return surface;
}

// Appends one icon's [width, height, pixels...] to the EWMH _NET_WM_ICON
// data buffer. Cairo's ARGB32 format (premultiplied, native-endian 32-bit
// words) is exactly the pixel layout _NET_WM_ICON expects.
void append_icon(std::vector<unsigned long>& data, int size) {
    cairo_surface_t* surface = render_icon_surface(size);
    const int stride = cairo_image_surface_get_stride(surface);
    const unsigned char* pixels = cairo_image_surface_get_data(surface);

    data.push_back(static_cast<unsigned long>(size));
    data.push_back(static_cast<unsigned long>(size));
    for (int y = 0; y < size; ++y) {
        const uint32_t* row = reinterpret_cast<const uint32_t*>(pixels + y * stride);
        for (int x = 0; x < size; ++x) {
            data.push_back(static_cast<unsigned long>(row[x]));
        }
    }
    cairo_surface_destroy(surface);
}

} // namespace

void set_taskbar_icon(Display* display, Window window) {
    std::vector<unsigned long> data;
    for (int size : {16, 32, 48}) {
        append_icon(data, size);
    }

    Atom net_wm_icon = XInternAtom(display, "_NET_WM_ICON", False);
    XChangeProperty(display, window, net_wm_icon, XA_CARDINAL, 32, PropModeReplace,
                     reinterpret_cast<unsigned char*>(data.data()), static_cast<int>(data.size()));
}
