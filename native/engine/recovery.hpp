// Copyright (c) 2026 Eng. Ahmed Yasir — MIT
// Original AY engine persistent crash-loop guard. No Zygote attachment.
#pragma once

#include <cstdint>
#include <string>

namespace ay::engine {

enum class GuardStatus : std::uint8_t {
    Ready,
    Disabled,
    Busy,
    UnsafeStorage,
    CorruptRecord,
    IoFailure
};

struct GuardSnapshot {
    GuardStatus status;
    std::uint32_t consecutive_failures;
    bool attempt_in_progress;
};

// The directory must already exist, be owned by the process and disallow
// group/other writes. No directory or storage is created by this guard.
// A process-exclusive lock prevents two supervisors sharing the same journal.
class RecoveryGuard final {
public:
    explicit RecoveryGuard(const std::string& trusted_directory,
                           std::uint32_t max_failures = 3) noexcept;
    ~RecoveryGuard();
    RecoveryGuard(const RecoveryGuard&) = delete;
    RecoveryGuard& operator=(const RecoveryGuard&) = delete;

    // Atomically mark this startup as unfinished. If the previous attempt
    // never stopped cleanly, it counts as a failure, including across reboots.
    bool begin_attempt() noexcept;
    // A health signal resets the failure count but leaves the session armed:
    // a later unexpected process death must still be detected at next start.
    bool mark_healthy() noexcept;
    bool mark_clean_stop() noexcept;
    // Explicit recovery only, never automatic when a fail-safe has tripped.
    bool reset_by_operator() noexcept;
    GuardSnapshot snapshot() const noexcept;

private:
    bool load() noexcept;
    bool persist(std::uint32_t failures, bool in_progress,
                 bool disabled) noexcept;
    void fail_closed(GuardStatus status) noexcept;

    int directory_fd_ = -1;
    int lock_fd_ = -1;
    GuardStatus status_ = GuardStatus::IoFailure;
    std::uint32_t failures_ = 0;
    std::uint32_t max_failures_ = 3;
    bool in_progress_ = false;
    bool started_here_ = false;
};

}  // namespace ay::engine
