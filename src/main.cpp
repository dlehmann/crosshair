// crosshair: a borderless, always-on-top, click-through crosshair overlay
// for Linux (X11 native, works under Wayland via XWayland).
//
// Only one instance runs at a time. The running instance shows up in the
// taskbar; opening it (or launching the program again) reveals a small
// dialog offering to exit the program.

#include <X11/Xlib.h>
#include <sys/select.h>
#include <unistd.h>

#include <csignal>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>

#include "config.hpp"
#include "display_server.hpp"
#include "exit_dialog_window.hpp"
#include "overlay_window.hpp"
#include "single_instance.hpp"
#include "x11_utils.hpp"

namespace {

volatile sig_atomic_t g_shutdown_requested = 0;

void handle_termination_signal(int /*signal_number*/) {
    g_shutdown_requested = 1;
}

// X errors (e.g. a stale window ID from an inter-process activation request,
// or a compositor quirk) must never crash a permanent background overlay -
// log and keep running instead of the default abort-on-error behavior.
int handle_x_error(Display* display, XErrorEvent* error) {
    char message[256];
    XGetErrorText(display, error->error_code, message, sizeof(message));
    std::cerr << "Warning: non-fatal X11 error: " << message << " (request "
              << static_cast<int>(error->request_code) << ")\n";
    return 0;
}

// Resolves the directory the running binary lives in. This is where
// crosshair.svg and crosshair.yml are read from (and auto-generated, if
// missing).
std::string resolve_binary_directory() {
    // When running from inside an AppImage, /proc/self/exe resolves inside
    // the temporary, read-only squashfs mount point (e.g.
    // /tmp/.mount_XXXXXX/usr/bin/crosshair) - not next to the .AppImage
    // file the user actually placed on disk, and not writable. AppImage's
    // runtime sets $APPIMAGE to the absolute path of that outer file, so
    // prefer that when present, to keep "next to the binary" meaningful
    // (and writable) for AppImage users too.
    const char* appimage_path = std::getenv("APPIMAGE");
    if (appimage_path != nullptr && appimage_path[0] != '\0') {
        const std::string full_path(appimage_path);
        const std::size_t slash_pos = full_path.find_last_of('/');
        return (slash_pos == std::string::npos) ? "." : full_path.substr(0, slash_pos);
    }

    char path_buffer[4096];
    const ssize_t length = readlink("/proc/self/exe", path_buffer, sizeof(path_buffer) - 1);
    if (length <= 0) {
        std::cerr << "Warning: could not resolve /proc/self/exe, using current "
                     "working directory instead.\n";
        return ".";
    }
    path_buffer[length] = '\0';
    const std::string full_path(path_buffer);
    const std::size_t slash_pos = full_path.find_last_of('/');
    return (slash_pos == std::string::npos) ? "." : full_path.substr(0, slash_pos);
}

} // namespace

int main() {
    std::signal(SIGINT, handle_termination_signal);
    std::signal(SIGTERM, handle_termination_signal);
    XSetErrorHandler(handle_x_error);

    std::cout << "=== Crosshair ===\n";

    // Single-instance check happens before touching X, since it works
    // independently of the display server.
    SingleInstanceLock instance_lock;

    const DisplayServerInfo server_info = detect_display_server();
    std::cout << server_info.description << "\n";

    Display* display = XOpenDisplay(nullptr);
    if (display == nullptr) {
        std::cerr << "Fatal: could not open connection to the X display. Is "
                     "DISPLAY set, and is an X server (or XWayland) running?\n";
        return EXIT_FAILURE;
    }
    const int screen = DefaultScreen(display);
    const Window root = RootWindow(display, screen);

    if (!instance_lock.is_primary()) {
        std::cout << "Crosshair is already running - asking the running "
                     "instance to show its exit dialog.\n";
        const Window running_window = find_running_control_window(display, root);
        if (running_window != 0) {
            request_window_activation(display, root, running_window);
        } else {
            std::cerr << "Warning: could not locate the running instance's window.\n";
        }
        XCloseDisplay(display);
        return EXIT_SUCCESS;
    }

    const std::string base_dir = resolve_binary_directory();
    std::cout << "Using asset directory: " << base_dir << "\n";

    const Config config(base_dir);
    const DialogStrings strings = config.get_dialog_strings();

    const std::string svg_path = base_dir + "/crosshair.svg";
    auto overlay = std::make_unique<OverlayWindow>(display, screen, svg_path);
    overlay->show();
    overlay->redraw();

    // The control window doubles as the app's taskbar presence and its exit
    // dialog; it starts minimized (see ExitDialogWindow) so only the
    // crosshair itself is visible at startup.
    auto control = std::make_unique<ExitDialogWindow>(display, screen, strings);
    publish_control_window(display, root, control->window());

    std::cout << "Crosshair is running. Open it from the taskbar to exit.\n";

    const int display_fd = ConnectionNumber(display);
    bool running = true;

    while (running && g_shutdown_requested == 0) {
        while (XPending(display) > 0) {
            XEvent event;
            XNextEvent(display, &event);

            switch (event.type) {
                case Expose:
                    if (event.xexpose.window == overlay->window()) {
                        overlay->redraw();
                    } else if (event.xexpose.window == control->window()) {
                        control->redraw();
                    }
                    break;

                case MapNotify:
                    // The window manager just un-minimized the control
                    // window (taskbar click, or a second instance's
                    // activation request) - make sure the dialog is drawn.
                    if (event.xmap.window == control->window()) {
                        control->redraw();
                    }
                    break;

                case ButtonPress:
                    if (event.xbutton.window == control->window()) {
                        control->handle_button_press(event.xbutton.x, event.xbutton.y);
                        if (control->exit_requested()) {
                            std::cout << "Exit requested, shutting down.\n";
                            running = false;
                        }
                    }
                    break;

                case ClientMessage:
                    if (event.xclient.window == control->window() &&
                        static_cast<Atom>(event.xclient.data.l[0]) == control->wm_delete_atom()) {
                        // WM_DELETE_WINDOW is sent both when closing the
                        // visible dialog via its own window decoration AND
                        // for "Close"/"Quit" in the taskbar's right-click
                        // menu while minimized - both look identical on the
                        // wire. Tell them apart by whether the dialog is
                        // currently on screen: closing the visible dialog
                        // should just cancel it, while a close/quit request
                        // while minimized (no dialog to cancel) means exit.
                        if (control->is_visible()) {
                            control->iconify();
                        } else {
                            std::cout << "Close/Quit requested via taskbar, shutting down.\n";
                            running = false;
                        }
                    }
                    break;

                default:
                    break;
            }

            if (!running) {
                break;
            }
        }

        if (!running || g_shutdown_requested != 0) {
            break;
        }

        // Block until the next X event or a periodic tick, whichever comes
        // first. The periodic tick re-asserts always-on-top stacking, since
        // some fullscreen applications can otherwise obscure the overlay.
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(display_fd, &read_fds);
        struct timeval timeout {2, 0};

        const int ready = select(display_fd + 1, &read_fds, nullptr, nullptr, &timeout);
        if (ready == 0) {
            overlay->raise_to_top();
        }
        // ready < 0 (EINTR from our signal handlers) simply loops back
        // around and re-checks g_shutdown_requested.
    }

    std::cout << "Shutting down.\n";
    control.reset();
    overlay.reset();
    XCloseDisplay(display);
    return EXIT_SUCCESS;
}
