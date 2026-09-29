// Ensures only one instance of the program runs at a time, using a POSIX
// advisory file lock (independent of X11) so the check works reliably
// regardless of window manager or display server quirks.
#pragma once

#include <string>

class SingleInstanceLock {
public:
    // Attempts to acquire the lock immediately (non-blocking).
    SingleInstanceLock();
    ~SingleInstanceLock();

    SingleInstanceLock(const SingleInstanceLock&) = delete;
    SingleInstanceLock& operator=(const SingleInstanceLock&) = delete;

    // True if this process holds the lock (i.e. is the sole/primary
    // instance). False if another instance already holds it.
    bool is_primary() const { return is_primary_; }

private:
    int fd_;
    bool is_primary_;

    static std::string resolve_lock_path();
};
