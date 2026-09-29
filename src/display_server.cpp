#include "display_server.hpp"

#include <cstdlib>

namespace {

std::string env_or_empty(const char* name) {
    const char* value = std::getenv(name);
    return value != nullptr ? std::string(value) : std::string();
}

} // namespace

DisplayServerInfo detect_display_server() {
    const std::string session_type = env_or_empty("XDG_SESSION_TYPE");
    const std::string wayland_display = env_or_empty("WAYLAND_DISPLAY");
    const std::string x11_display = env_or_empty("DISPLAY");

    if (session_type == "wayland" || !wayland_display.empty()) {
        return {DisplayServerType::Wayland,
                "Wayland session detected (compositor-managed). The overlay "
                "will connect through XWayland, since it uses Xlib for "
                "maximum compatibility and minimal footprint."};
    }

    if (session_type == "x11" || !x11_display.empty()) {
        return {DisplayServerType::X11,
                "X11 session detected. Using native Xlib overlay with full "
                "click-through and always-on-top support."};
    }

    return {DisplayServerType::Unknown,
            "Could not determine the display server type from the "
            "environment. Falling back to a direct X11/Xlib connection."};
}
