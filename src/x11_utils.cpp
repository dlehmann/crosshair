#include "x11_utils.hpp"

#include <X11/extensions/Xrandr.h>
#include <X11/Xatom.h>

#include <cstring>

Rect get_primary_monitor_geometry(Display* display, int screen) {
    Rect fallback{0, 0,
                  static_cast<unsigned int>(DisplayWidth(display, screen)),
                  static_cast<unsigned int>(DisplayHeight(display, screen))};

    int event_base = 0;
    int error_base = 0;
    if (!XRRQueryExtension(display, &event_base, &error_base)) {
        return fallback;
    }

    Window root = RootWindow(display, screen);
    XRRScreenResources* resources = XRRGetScreenResourcesCurrent(display, root);
    if (resources == nullptr) {
        return fallback;
    }

    RROutput primary = XRRGetOutputPrimary(display, root);
    Rect result = fallback;
    bool found = false;

    auto try_output = [&](RROutput output) -> bool {
        XRROutputInfo* output_info = XRRGetOutputInfo(display, resources, output);
        if (output_info == nullptr) {
            return false;
        }
        bool ok = false;
        if (output_info->connection == RR_Connected && output_info->crtc != 0) {
            XRRCrtcInfo* crtc_info = XRRGetCrtcInfo(display, resources, output_info->crtc);
            if (crtc_info != nullptr) {
                result = Rect{crtc_info->x, crtc_info->y, crtc_info->width, crtc_info->height};
                ok = true;
                XRRFreeCrtcInfo(crtc_info);
            }
        }
        XRRFreeOutputInfo(output_info);
        return ok;
    };

    if (primary != 0) {
        found = try_output(primary);
    }

    if (!found) {
        // No primary output configured - use the first connected output with
        // an active CRTC instead of giving up on per-monitor centering.
        for (int i = 0; i < resources->noutput && !found; ++i) {
            found = try_output(resources->outputs[i]);
        }
    }

    XRRFreeScreenResources(resources);
    return found ? result : fallback;
}

void set_always_on_top_hints(Display* display, Window window, bool skip_taskbar) {
    Atom net_wm_state = XInternAtom(display, "_NET_WM_STATE", False);
    Atom state_above = XInternAtom(display, "_NET_WM_STATE_ABOVE", False);
    Atom state_skip_taskbar = XInternAtom(display, "_NET_WM_STATE_SKIP_TASKBAR", False);
    Atom state_skip_pager = XInternAtom(display, "_NET_WM_STATE_SKIP_PAGER", False);
    Atom net_wm_window_type = XInternAtom(display, "_NET_WM_WINDOW_TYPE", False);
    Atom window_type_utility = XInternAtom(display, "_NET_WM_WINDOW_TYPE_UTILITY", False);

    Atom states[3];
    int count = 0;
    states[count++] = state_above;
    if (skip_taskbar) {
        states[count++] = state_skip_taskbar;
        states[count++] = state_skip_pager;
    }

    XChangeProperty(display, window, net_wm_state, XA_ATOM, 32, PropModeReplace,
                     reinterpret_cast<unsigned char*>(states), count);
    XChangeProperty(display, window, net_wm_window_type, XA_ATOM, 32, PropModeReplace,
                     reinterpret_cast<unsigned char*>(&window_type_utility), 1);
}

Atom enable_wm_delete_window(Display* display, Window window) {
    Atom wm_delete_window = XInternAtom(display, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(display, window, &wm_delete_window, 1);
    return wm_delete_window;
}

void set_wm_class(Display* display, Window window, const char* instance_name, const char* class_name) {
    XClassHint* hint = XAllocClassHint();
    if (hint == nullptr) {
        return;
    }
    hint->res_name = const_cast<char*>(instance_name);
    hint->res_class = const_cast<char*>(class_name);
    XSetClassHint(display, window, hint);
    XFree(hint);
}

namespace {
// Custom, application-private root window property used to advertise the
// currently running instance's control/taskbar window to later launches.
Atom control_window_atom(Display* display) {
    return XInternAtom(display, "_CROSSHAIR_CONTROL_WINDOW", False);
}
} // namespace

void publish_control_window(Display* display, Window root, Window control_window) {
    XChangeProperty(display, root, control_window_atom(display), XA_WINDOW, 32, PropModeReplace,
                     reinterpret_cast<unsigned char*>(&control_window), 1);
}

Window find_running_control_window(Display* display, Window root) {
    Atom actual_type;
    int actual_format = 0;
    unsigned long item_count = 0;
    unsigned long bytes_after = 0;
    unsigned char* data = nullptr;

    Window result = 0;
    const int status = XGetWindowProperty(display, root, control_window_atom(display), 0, 1, False,
                                           XA_WINDOW, &actual_type, &actual_format, &item_count,
                                           &bytes_after, &data);
    if (status == Success && actual_type == XA_WINDOW && item_count == 1 && data != nullptr) {
        result = *reinterpret_cast<Window*>(data);
    }
    if (data != nullptr) {
        XFree(data);
    }
    return result;
}

void request_window_activation(Display* display, Window root, Window target) {
    Atom net_active_window = XInternAtom(display, "_NET_ACTIVE_WINDOW", False);

    XEvent event{};
    event.type = ClientMessage;
    event.xclient.window = target;
    event.xclient.message_type = net_active_window;
    event.xclient.format = 32;
    event.xclient.data.l[0] = 1; // source indication: 1 = normal application
    event.xclient.data.l[1] = CurrentTime;
    event.xclient.data.l[2] = 0;

    XSendEvent(display, root, False, SubstructureRedirectMask | SubstructureNotifyMask, &event);
    XFlush(display);
}
