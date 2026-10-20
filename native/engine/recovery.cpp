// Copyright (c) 2026 Eng. Ahmed Yasir — MIT
#include "recovery.hpp"

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

namespace ay::engine {
namespace {

constexpr char kJournal[] = "ay-engine-state-v1";
constexpr char kLock[] = "ay-engine-state-v1.lock";
constexpr std::size_t kRecordSize = 16;

std::uint32_t read32(const unsigned char* b) noexcept {
    return static_cast<std::uint32_t>(b[0]) |
           (static_cast<std::uint32_t>(b[1]) << 8u) |
           (static_cast<std::uint32_t>(b[2]) << 16u) |
           (static_cast<std::uint32_t>(b[3]) << 24u);
}

void write32(unsigned char* b, std::uint32_t value) noexcept {
    for (unsigned i = 0; i < 4; ++i) {
        b[i] = static_cast<unsigned char>((value >> (8u * i)) & 0xffu);
    }
}

// An integrity check for accidental disk corruption, NOT authentication.
std::uint32_t checksum(const unsigned char* b, std::size_t size) noexcept {
    std::uint32_t hash = 2166136261u;
    for (std::size_t i = 0; i < size; ++i) {
        hash ^= b[i];
        hash *= 16777619u;
    }
    return hash;
}

bool read_exact(int fd, unsigned char* bytes, std::size_t size) noexcept {
    std::size_t received = 0;
    while (received < size) {
        const ssize_t n = read(fd, bytes + received, size - received);
        if (n < 0 && errno == EINTR) continue;
        if (n <= 0) return false;
        received += static_cast<std::size_t>(n);
    }
    return true;
}

bool write_exact(int fd, const unsigned char* bytes,
                 std::size_t size) noexcept {
    std::size_t written = 0;
    while (written < size) {
        const ssize_t n = write(fd, bytes + written, size - written);
        if (n < 0 && errno == EINTR) continue;
        if (n <= 0) return false;
        written += static_cast<std::size_t>(n);
    }
    return true;
}

bool secure_regular(int fd) noexcept {
    struct stat st {};
    return fstat(fd, &st) == 0 && S_ISREG(st.st_mode) &&
           st.st_uid == geteuid() && st.st_nlink == 1 &&
           (st.st_mode & 0077) == 0;
}

}  // namespace

RecoveryGuard::RecoveryGuard(const std::string& trusted_directory,
                             std::uint32_t max_failures) noexcept
    : max_failures_(max_failures == 0 ? 1 : max_failures) {
    directory_fd_ = open(trusted_directory.c_str(),
                         O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);
    struct stat st {};
    if (directory_fd_ < 0 || fstat(directory_fd_, &st) != 0 ||
        !S_ISDIR(st.st_mode) || st.st_uid != geteuid() ||
        (st.st_mode & 0022) != 0) {
        status_ = GuardStatus::UnsafeStorage;
        return;
    }
    lock_fd_ = openat(directory_fd_, kLock,
                      O_RDWR | O_CREAT | O_NOFOLLOW | O_CLOEXEC | O_NONBLOCK,
                      0600);
    if (lock_fd_ < 0 || !secure_regular(lock_fd_)) {
        status_ = GuardStatus::UnsafeStorage;
        return;
    }
    if (flock(lock_fd_, LOCK_EX | LOCK_NB) != 0) {
        status_ = (errno == EWOULDBLOCK || errno == EAGAIN) ?
            GuardStatus::Busy : GuardStatus::IoFailure;
        return;
    }
    load();
}

RecoveryGuard::~RecoveryGuard() {
    if (lock_fd_ >= 0) close(lock_fd_);
    if (directory_fd_ >= 0) close(directory_fd_);
}

void RecoveryGuard::fail_closed(GuardStatus status) noexcept {
    status_ = status;
}

bool RecoveryGuard::load() noexcept {
    const int fd = openat(directory_fd_, kJournal,
                          O_RDONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK);
    if (fd < 0) {
        status_ = errno == ENOENT ? GuardStatus::Ready : GuardStatus::IoFailure;
        return status_ == GuardStatus::Ready;
    }
    if (!secure_regular(fd)) {
        close(fd);
        fail_closed(GuardStatus::UnsafeStorage);
        return false;
    }
    unsigned char b[kRecordSize]{};
    const bool complete = read_exact(fd, b, sizeof(b));
    unsigned char trailing{};
    ssize_t extra = -1;
    if (complete) {
        do { extra = read(fd, &trailing, 1); } while (extra < 0 && errno == EINTR);
    }
    close(fd);
    if (!complete || extra != 0 ||
        std::memcmp(b, "AYR1", 4) != 0 || b[4] != 1 ||
        (b[5] & ~3u) != 0 || b[6] != 0 || b[7] != 0 ||
        read32(b + 12) != checksum(b, 12)) {
        fail_closed(GuardStatus::CorruptRecord);
        return false;
    }
    failures_ = read32(b + 8);
    in_progress_ = (b[5] & 1u) != 0;
    const bool disabled = (b[5] & 2u) != 0;
    if (disabled && in_progress_) {
        fail_closed(GuardStatus::CorruptRecord);
        return false;
    }
    status_ = disabled || failures_ >= max_failures_ ?
        GuardStatus::Disabled : GuardStatus::Ready;
    return status_ == GuardStatus::Ready;
}

bool RecoveryGuard::persist(std::uint32_t failures, bool in_progress,
                            bool disabled) noexcept {
    if (directory_fd_ < 0 || lock_fd_ < 0) return false;
    unsigned char b[kRecordSize]{'A', 'Y', 'R', '1', 1,
                                  static_cast<unsigned char>((in_progress ? 1 : 0) |
                                                             (disabled ? 2 : 0))};
    write32(b + 8, failures);
    write32(b + 12, checksum(b, 12));

    char temporary[80]{};
    const int name_length = std::snprintf(temporary, sizeof(temporary),
                                          "ay-engine-state-v1.tmp.%ld",
                                          static_cast<long>(getpid()));
    if (name_length <= 0 || static_cast<std::size_t>(name_length) >= sizeof(temporary)) {
        fail_closed(GuardStatus::IoFailure);
        return false;
    }
    // A stale temporary from an interrupted atomic replacement may be removed
    // only if it is a regular file owned by our process user and not linked.
    int fd = openat(directory_fd_, temporary,
                    O_WRONLY | O_CREAT | O_EXCL | O_NOFOLLOW | O_CLOEXEC, 0600);
    if (fd < 0 && errno == EEXIST) {
        const int stale = openat(directory_fd_, temporary,
                                 O_RDONLY | O_NOFOLLOW | O_CLOEXEC | O_NONBLOCK);
        if (stale >= 0 && secure_regular(stale)) {
            close(stale);
            if (unlinkat(directory_fd_, temporary, 0) == 0) {
                fd = openat(directory_fd_, temporary,
                            O_WRONLY | O_CREAT | O_EXCL | O_NOFOLLOW | O_CLOEXEC,
                            0600);
            }
        } else if (stale >= 0) {
            close(stale);
        }
    }
    if (fd < 0) {
        fail_closed(GuardStatus::IoFailure);
        return false;
    }
    const bool ok = secure_regular(fd) && write_exact(fd, b, sizeof(b)) &&
                    fsync(fd) == 0;
    if (close(fd) != 0 || !ok) {
        unlinkat(directory_fd_, temporary, 0);
        fail_closed(GuardStatus::IoFailure);
        return false;
    }
    if (renameat(directory_fd_, temporary, directory_fd_, kJournal) != 0) {
        unlinkat(directory_fd_, temporary, 0);
        fail_closed(GuardStatus::IoFailure);
        return false;
    }
    if (fsync(directory_fd_) != 0) {
        fail_closed(GuardStatus::IoFailure);
        return false;
    }
    failures_ = failures;
    in_progress_ = in_progress;
    status_ = disabled ? GuardStatus::Disabled : GuardStatus::Ready;
    return true;
}

bool RecoveryGuard::begin_attempt() noexcept {
    if (status_ != GuardStatus::Ready || started_here_) return false;
    const auto failures = failures_ + (in_progress_ ? 1u : 0u);
    if (failures >= max_failures_) {
        // Persist the disabled state before refusing another unsafe start.
        persist(failures, false, true);
        return false;
    }
    if (!persist(failures, true, false)) return false;
    started_here_ = true;
    return true;
}

bool RecoveryGuard::mark_healthy() noexcept {
    if (status_ != GuardStatus::Ready || !started_here_ || !in_progress_) return false;
    return persist(0, true, false);
}

bool RecoveryGuard::mark_clean_stop() noexcept {
    if (status_ != GuardStatus::Ready || !started_here_ || !in_progress_) return false;
    if (!persist(failures_, false, false)) return false;
    started_here_ = false;
    return true;
}

bool RecoveryGuard::reset_by_operator() noexcept {
    if (status_ != GuardStatus::Disabled &&
        status_ != GuardStatus::CorruptRecord) return false;
    if (!persist(0, false, false)) return false;
    started_here_ = false;
    return true;
}

GuardSnapshot RecoveryGuard::snapshot() const noexcept {
    return {status_, failures_, in_progress_};
}

}  // namespace ay::engine
