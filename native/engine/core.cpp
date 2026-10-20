// Copyright (c) 2026 Eng. Ahmed Yasir — MIT
#include "core.hpp"

#include <limits>

namespace ay::engine {

Controller::Controller(unsigned max_failures) noexcept
    : max_failures_(max_failures == 0 ? 1 : max_failures) {}

void Controller::record_failure() noexcept {
    if (failures_ < std::numeric_limits<unsigned>::max()) {
        ++failures_;
    }
    state_ = failures_ >= max_failures_ ? State::Disabled : State::Degraded;
}

bool Controller::transition(Event event) noexcept {
    if (event == Event::ResetByOperator) {
        if (state_ != State::Disabled) return false;
        failures_ = 0;
        state_ = State::Dormant;
        return true;
    }
    if (state_ == State::Disabled) return false;

    if (event == Event::Stop) {
        state_ = State::Dormant;
        return true;
    }
    switch (state_) {
        case State::Dormant:
        case State::Degraded:
            if (event == Event::BeginPreflight) {
                state_ = State::Preflight;
                return true;
            }
            return false;
        case State::Preflight:
            if (event == Event::PreflightPassed) {
                state_ = State::Ready;
                return true;
            }
            if (event == Event::PreflightFailed) {
                record_failure();
                return true;
            }
            return false;
        case State::Ready:
            if (event == Event::BackendAttached) {
                state_ = State::Attached;
                return true;
            }
            if (event == Event::PreflightFailed) {
                record_failure();
                return true;
            }
            return false;
        case State::Attached:
            if (event == Event::BackendHealthy) {
                failures_ = 0;
                return true;
            }
            if (event == Event::BackendLost) {
                record_failure();
                return true;
            }
            return false;
        case State::Disabled:
            return false;
    }
    return false;
}

Snapshot Controller::snapshot() const noexcept {
    return {state_, failures_, state_ == State::Attached};
}

bool is_control_message(MessageKind kind) noexcept {
    switch (kind) {
        case MessageKind::Hello:
        case MessageKind::Heartbeat:
        case MessageKind::Inventory:
        case MessageKind::Shutdown:
            return true;
    }
    return false;
}

static bool valid_header(const FrameHeader& header) noexcept {
    if (!is_control_message(header.kind) ||
        header.payload_length > kMaxPayloadSize) {
        return false;
    }
    if ((header.kind == MessageKind::Hello ||
         header.kind == MessageKind::Heartbeat ||
         header.kind == MessageKind::Shutdown) &&
        header.payload_length != 0) {
        return false;
    }
    return true;
}

bool encode_header(const FrameHeader& header, FrameBytes* output) noexcept {
    if (output == nullptr || !valid_header(header)) return false;
    const auto length = header.payload_length;
    *output = {'A', 'Y', 'Z', '1', 1,
               static_cast<std::uint8_t>(header.kind), 0, 0,
               static_cast<std::uint8_t>(length & 0xffu),
               static_cast<std::uint8_t>((length >> 8u) & 0xffu),
               static_cast<std::uint8_t>((length >> 16u) & 0xffu),
               static_cast<std::uint8_t>((length >> 24u) & 0xffu)};
    return true;
}

bool decode_header(const std::uint8_t* bytes, std::size_t size,
                   FrameHeader* output) noexcept {
    if (bytes == nullptr || output == nullptr || size < kFrameHeaderSize) {
        return false;
    }
    if (bytes[0] != 'A' || bytes[1] != 'Y' || bytes[2] != 'Z' ||
        bytes[3] != '1' || bytes[4] != 1 || bytes[6] != 0 ||
        bytes[7] != 0) {
        return false;
    }
    const auto length =
        static_cast<std::uint32_t>(bytes[8]) |
        (static_cast<std::uint32_t>(bytes[9]) << 8u) |
        (static_cast<std::uint32_t>(bytes[10]) << 16u) |
        (static_cast<std::uint32_t>(bytes[11]) << 24u);
    const FrameHeader parsed{static_cast<MessageKind>(bytes[5]), length};
    if (!valid_header(parsed)) return false;
    *output = parsed;
    return true;
}

}  // namespace ay::engine
