#include "exit_dialog_window.hpp"
#include "icon.hpp"
#include "x11_utils.hpp"

#include <X11/Xutil.h>
#include <cairo/cairo-xlib.h>

#include <sstream>
#include <vector>

namespace {

constexpr int kWidth = 420;
constexpr int kHeight = 190;
constexpr int kMargin = 24;
constexpr int kButtonGap = 16;

// Greedy word-wrap for the info text, sized against the currently selected
// cairo font face.
std::vector<std::string> wrap_text(cairo_t* cr, const std::string& text, double max_width) {
    std::vector<std::string> lines;
    std::istringstream words(text);
    std::string word;
    std::string current_line;

    while (words >> word) {
        const std::string candidate = current_line.empty() ? word : current_line + " " + word;
        cairo_text_extents_t extents;
        cairo_text_extents(cr, candidate.c_str(), &extents);
        if (extents.width > max_width && !current_line.empty()) {
            lines.push_back(current_line);
            current_line = word;
        } else {
            current_line = candidate;
        }
    }
    if (!current_line.empty()) {
        lines.push_back(current_line);
    }
    return lines;
}

void draw_button(cairo_t* cr, int x, int y, int w, int h, const std::string& label, bool primary) {
    if (primary) {
        cairo_set_source_rgb(cr, 0.16, 0.42, 0.86);
    } else {
        cairo_set_source_rgb(cr, 0.85, 0.86, 0.88);
    }
    cairo_rectangle(cr, x, y, w, h);
    cairo_fill(cr);

    cairo_select_font_face(cr, "sans-serif", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, 14);
    cairo_text_extents_t extents;
    cairo_text_extents(cr, label.c_str(), &extents);
    const double text_x = x + (w - extents.width) / 2.0 - extents.x_bearing;
    const double text_y = y + (h - extents.height) / 2.0 - extents.y_bearing;
    if (primary) {
        cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
    } else {
        cairo_set_source_rgb(cr, 0.10, 0.10, 0.12);
    }
    cairo_move_to(cr, text_x, text_y);
    cairo_show_text(cr, label.c_str());
}

} // namespace

ExitDialogWindow::ExitDialogWindow(Display* display, int screen, const DialogStrings& strings)
    : display_(display),
      screen_(screen),
      window_(0),
      surface_(nullptr),
      wm_delete_window_(0),
      strings_(strings),
      exit_requested_(false),
      width_(kWidth),
      height_(kHeight),
      cancel_button_x_(0),
      exit_button_x_(0),
      button_y_(0),
      button_w_(96),
      button_h_(34) {
    const Rect monitor = get_primary_monitor_geometry(display_, screen);
    const int x = monitor.x + static_cast<int>(monitor.width) / 2 - width_ / 2;
    const int y = monitor.y + static_cast<int>(monitor.height) / 2 - height_ / 2;

    XSetWindowAttributes attrs{};
    attrs.background_pixel = WhitePixel(display_, screen);
    attrs.event_mask = ExposureMask | ButtonPressMask | StructureNotifyMask;

    window_ = XCreateWindow(display_, RootWindow(display_, screen), x, y, width_, height_, 0,
                             CopyFromParent, InputOutput, CopyFromParent,
                             CWBackPixel | CWEventMask, &attrs);

    XStoreName(display_, window_, strings_.title.c_str());
    wm_delete_window_ = enable_wm_delete_window(display_, window_);
    set_wm_class(display_, window_, "crosshair", "Crosshair");
    set_taskbar_icon(display_, window_);

    // A regular, normal-layer, taskbar-visible window (unlike the
    // click-through crosshair overlay) - deliberately NOT given the
    // always-on-top/skip-taskbar hints used for the overlay.

    XSizeHints size_hints{};
    size_hints.flags = PMinSize | PMaxSize;
    size_hints.min_width = size_hints.max_width = width_;
    size_hints.min_height = size_hints.max_height = height_;
    XSetWMNormalHints(display_, window_, &size_hints);

    // Start minimized: a compliant window manager creates a taskbar entry
    // for the window without showing it on screen, per ICCCM. Clicking that
    // taskbar entry (or a second process instance requesting activation)
    // un-minimizes it, at which point redraw() paints the dialog content.
    XWMHints wm_hints{};
    wm_hints.flags = StateHint;
    wm_hints.initial_state = IconicState;
    XSetWMHints(display_, window_, &wm_hints);

    // Cancel (left, neutral) and Exit (right, primary) side by side, centered
    // as a pair.
    const int pair_width = 2 * button_w_ + kButtonGap;
    cancel_button_x_ = (width_ - pair_width) / 2;
    exit_button_x_ = cancel_button_x_ + button_w_ + kButtonGap;
    button_y_ = height_ - kMargin - button_h_;

    surface_ = cairo_xlib_surface_create(display_, window_, DefaultVisual(display_, screen), width_,
                                          height_);

    XMapWindow(display_, window_);
    XFlush(display_);
}

ExitDialogWindow::~ExitDialogWindow() {
    if (surface_ != nullptr) {
        cairo_surface_destroy(surface_);
    }
    if (window_ != 0) {
        XDestroyWindow(display_, window_);
    }
}

void ExitDialogWindow::redraw() {
    if (surface_ == nullptr) {
        return;
    }
    cairo_t* cr = cairo_create(surface_);

    // Background
    cairo_set_source_rgb(cr, 0.97, 0.97, 0.98);
    cairo_paint(cr);

    // Title
    cairo_select_font_face(cr, "sans-serif", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, 18);
    cairo_set_source_rgb(cr, 0.10, 0.10, 0.12);
    cairo_move_to(cr, kMargin, kMargin + 12);
    cairo_show_text(cr, strings_.title.c_str());

    // Info text (word-wrapped)
    cairo_select_font_face(cr, "sans-serif", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, 14);
    cairo_set_source_rgb(cr, 0.25, 0.25, 0.28);
    const std::vector<std::string> lines = wrap_text(cr, strings_.info, width_ - 2 * kMargin);
    double text_y = kMargin + 48;
    for (const auto& line : lines) {
        cairo_move_to(cr, kMargin, text_y);
        cairo_show_text(cr, line.c_str());
        text_y += 22;
    }

    draw_button(cr, cancel_button_x_, button_y_, button_w_, button_h_, strings_.cancel_button, false);
    draw_button(cr, exit_button_x_, button_y_, button_w_, button_h_, strings_.exit_button, true);

    cairo_destroy(cr);
    cairo_surface_flush(surface_);
    XFlush(display_);
}

void ExitDialogWindow::handle_button_press(int x, int y) {
    const bool in_y_range = y >= button_y_ && y <= button_y_ + button_h_;
    if (!in_y_range) {
        return;
    }
    if (x >= exit_button_x_ && x <= exit_button_x_ + button_w_) {
        exit_requested_ = true;
    } else if (x >= cancel_button_x_ && x <= cancel_button_x_ + button_w_) {
        iconify();
    }
}

void ExitDialogWindow::iconify() {
    XIconifyWindow(display_, window_, screen_);
    XFlush(display_);
}

bool ExitDialogWindow::is_visible() const {
    XWindowAttributes attrs;
    if (XGetWindowAttributes(display_, window_, &attrs) == 0) {
        return false;
    }
    return attrs.map_state == IsViewable;
}
