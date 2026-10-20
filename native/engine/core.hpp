// Copyright (c) 2026 Eng. Ahmed Yasir — MIT
// Original AY Zygisk engine control-plane primitives.
// This component NEVER attaches to Zygote or injects a library.
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace ay::engine {

enum class State : std::uint8_t {
    Dormant,
    Preflight,
    Ready,
    Attached,
    Degraded,
    Disabled
};

enum class Event : std::uint8_t {
    BeginPreflight,
    PreflightPassed,
    PreflightFailed,
    BackendAttached,
    BackendHealthy,
    BackendLost,
    Stop,
    ResetByOperator
};

struct Snapshot {
    State state;
    unsigned consecutive_failures;
    bool backend_active;
};

// In-memory state machine for a future independent backend supervisor.
// Persistent crash-loop tracking and Android process attachment are separate
// requirements and must be implemented and verified before runtime activation.
class Controller final {
public:
    explicit Controller(unsigned max_failures = 3) noexcept;
    bool transition(Event event) noexcept;
    Snapshot snapshot() const noexcept;

private:
    void record_failure() noexcept;
    State state_ = State::Dormant;
    unsigned failures_ = 0;
    unsigned max_failures_ = 3;
};

enum class MessageKind : std::uint8_t {
    Hello = 1,
    Heartbeat = 2,
    Inventory = 3,
    Shutdown = 4
};

struct FrameHeader {
    MessageKind kind;
    std::uint32_t payload_length;
};

// Wire contract: 12-byte, little-endian header.
// Bytes: ASCII 'A','Y','Z','1', version=1, kind, reserved=0, length (LE u32).
// This is NOT an authenticated IPC implementation. Future Android transports
// must verify the peer identity before processing ANY command.
inline constexpr std::size_t kFrameHeaderSize = 12;
inline constexpr std::uint32_t kMaxPayloadSize = 8192;
using FrameBytes = std::array<std::uint8_t, kFrameHeaderSize>;

bool encode_header(const FrameHeader& header, FrameBytes* output) noexcept;
bool decode_header(const std::uint8_t* bytes, std::size_t size,
                   FrameHeader* output) noexcept;
bool is_control_message(MessageKind kind) noexcept;

}  // namespace ay::engine
