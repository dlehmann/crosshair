#include "single_instance.hpp"

#include <fcntl.h>
#include <sys/file.h>
#include <unistd.h>

#include <cstdlib>
#include <iostream>

std::string SingleInstanceLock::resolve_lock_path() {
    const char* runtime_dir = std::getenv("XDG_RUNTIME_DIR");
    if (runtime_dir != nullptr && runtime_dir[0] != '\0') {
        return std::string(runtime_dir) + "/crosshair.lock";
    }
    return "/tmp/crosshair-" + std::to_string(static_cast<unsigned long>(getuid())) + ".lock";
}

SingleInstanceLock::SingleInstanceLock() : fd_(-1), is_primary_(false) {
    const std::string path = resolve_lock_path();
    fd_ = open(path.c_str(), O_CREAT | O_RDWR, 0600);
    if (fd_ < 0) {
        // Could not even open the lock file (e.g. unusual filesystem
        // permissions). Fail open rather than blocking startup entirely on
        // an unrelated filesystem issue.
        std::cerr << "Warning: could not open single-instance lock file at " << path
                  << "; skipping single-instance check.\n";
        is_primary_ = true;
        return;
    }
    // A non-blocking exclusive lock: succeeds only if no other instance
    // currently holds it. The lock is released automatically when this
    // process exits (or explicitly closes fd_), regardless of how it exits.
    is_primary_ = (flock(fd_, LOCK_EX | LOCK_NB) == 0);
}

SingleInstanceLock::~SingleInstanceLock() {
    if (fd_ >= 0) {
        close(fd_);
    }
}
