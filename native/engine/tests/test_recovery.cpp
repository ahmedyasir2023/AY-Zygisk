// Copyright (c) 2026 Eng. Ahmed Yasir — MIT
#include "../recovery.hpp"

#include <cstdio>
#include <cstdlib>
#include <fcntl.h>
#include <iostream>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

using ay::engine::GuardStatus;
using ay::engine::RecoveryGuard;

static void check(bool ok, const char* expression, int line) {
    if (!ok) {
        std::cerr << "FAIL at line " << line << ": " << expression << '\n';
        std::abort();
    }
}
#define CHECK(x) check(static_cast<bool>(x), #x, __LINE__)

int main() {
    char pattern[] = "/tmp/ay-recovery-XXXXXX";
    char* created = mkdtemp(pattern);
    CHECK(created != nullptr);
    const std::string dir(created);
    const std::string record = dir + "/ay-engine-state-v1";
    const std::string lock = dir + "/ay-engine-state-v1.lock";

    {
        RecoveryGuard g(dir, 3);
        CHECK(g.snapshot().status == GuardStatus::Ready);
        CHECK(g.begin_attempt());
        CHECK(!g.begin_attempt());
        CHECK(g.snapshot().attempt_in_progress);
        CHECK(g.mark_healthy());
        CHECK(g.mark_clean_stop());
        CHECK(!g.mark_clean_stop());
        CHECK(g.snapshot().consecutive_failures == 0);

        RecoveryGuard another(dir, 3);
        CHECK(another.snapshot().status == GuardStatus::Busy);
        CHECK(!another.begin_attempt());
    }

    // Three successive unclean process exits must persist across restarts.
    for (unsigned failure = 0; failure < 3; ++failure) {
        RecoveryGuard g(dir, 3);
        CHECK(g.snapshot().status == GuardStatus::Ready);
        CHECK(g.begin_attempt());
        CHECK(g.snapshot().consecutive_failures == failure);
    }
    {
        RecoveryGuard g(dir, 3);
        CHECK(g.snapshot().attempt_in_progress);
        CHECK(!g.begin_attempt());
        CHECK(g.snapshot().status == GuardStatus::Disabled);
        CHECK(g.snapshot().consecutive_failures == 3);
    }
    {
        RecoveryGuard g(dir, 3);
        CHECK(g.snapshot().status == GuardStatus::Disabled);
        CHECK(!g.begin_attempt());
        CHECK(g.reset_by_operator());
        CHECK(g.begin_attempt());
        CHECK(g.mark_healthy());
        CHECK(g.mark_clean_stop());
    }

    // Reject corrupted or truncated journal data; only explicit reset repairs.
    {
        const int fd = open(record.c_str(), O_WRONLY | O_TRUNC | O_CLOEXEC);
        CHECK(fd >= 0);
        CHECK(write(fd, "bad", 3) == 3);
        CHECK(close(fd) == 0);
        RecoveryGuard g(dir);
        CHECK(g.snapshot().status == GuardStatus::CorruptRecord);
        CHECK(!g.begin_attempt());
        CHECK(g.reset_by_operator());
        CHECK(g.snapshot().status == GuardStatus::Ready);
    }

    // A journal symlink must not be followed, even when it has a valid target.
    CHECK(unlink(record.c_str()) == 0);
    CHECK(symlink("ay-engine-state-v1.lock", record.c_str()) == 0);
    {
        RecoveryGuard g(dir);
        CHECK(g.snapshot().status != GuardStatus::Ready);
        CHECK(!g.begin_attempt());
    }
    CHECK(unlink(record.c_str()) == 0);
    {
        RecoveryGuard g(dir);
        CHECK(g.snapshot().status == GuardStatus::Ready);
        CHECK(g.begin_attempt());
        CHECK(g.mark_clean_stop());
    }

    CHECK(chmod(dir.c_str(), 0777) == 0);
    {
        RecoveryGuard g(dir);
        CHECK(g.snapshot().status == GuardStatus::UnsafeStorage);
    }
    CHECK(chmod(dir.c_str(), 0700) == 0);

    const std::string alias = dir + "-link";
    CHECK(symlink(dir.c_str(), alias.c_str()) == 0);
    {
        RecoveryGuard g(alias);
        CHECK(g.snapshot().status == GuardStatus::UnsafeStorage);
    }
    CHECK(unlink(alias.c_str()) == 0);
    CHECK(unlink(record.c_str()) == 0);
    CHECK(unlink(lock.c_str()) == 0);
    CHECK(rmdir(dir.c_str()) == 0);
    std::cout << "PASS: persistent AY crash-loop guard, locking and fail-closed storage\n";
}
