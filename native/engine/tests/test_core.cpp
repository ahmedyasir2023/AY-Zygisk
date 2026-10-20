// Copyright (c) 2026 Eng. Ahmed Yasir — MIT
#include "../core.hpp"

#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <random>

using ay::engine::Controller;
using ay::engine::Event;
using ay::engine::FrameBytes;
using ay::engine::FrameHeader;
using ay::engine::MessageKind;
using ay::engine::State;
using ay::engine::decode_header;
using ay::engine::encode_header;
using ay::engine::kFrameHeaderSize;
using ay::engine::kMaxPayloadSize;

static void verify_controller() {
    Controller c(2);
    assert(c.snapshot().state == State::Dormant);
    assert(!c.transition(Event::BackendAttached));
    assert(!c.snapshot().backend_active);

    assert(c.transition(Event::BeginPreflight));
    assert(!c.transition(Event::BackendAttached));
    assert(c.transition(Event::PreflightPassed));
    assert(c.transition(Event::BackendAttached));
    assert(c.snapshot().backend_active);
    assert(c.transition(Event::BackendLost));
    assert(c.snapshot().state == State::Degraded);
    assert(c.snapshot().consecutive_failures == 1);
    assert(!c.snapshot().backend_active);

    assert(c.transition(Event::BeginPreflight));
    assert(c.transition(Event::PreflightFailed));
    assert(c.snapshot().state == State::Disabled);
    assert(c.snapshot().consecutive_failures == 2);
    assert(!c.transition(Event::BeginPreflight));
    assert(!c.transition(Event::Stop));  // No silent reset of a disabled engine.
    assert(c.transition(Event::ResetByOperator));
    assert(c.snapshot().state == State::Dormant);
    assert(c.snapshot().consecutive_failures == 0);

    assert(c.transition(Event::BeginPreflight));
    assert(c.transition(Event::PreflightPassed));
    assert(c.transition(Event::BackendAttached));
    assert(c.transition(Event::BackendHealthy));
    assert(c.snapshot().consecutive_failures == 0);
    assert(c.transition(Event::Stop));
    assert(c.snapshot().state == State::Dormant);
    assert(!c.snapshot().backend_active);

    // A zero error budget is disallowed; clamp to one failure.
    Controller strict(0);
    assert(strict.transition(Event::BeginPreflight));
    assert(strict.transition(Event::PreflightFailed));
    assert(strict.snapshot().state == State::Disabled);
}

static void verify_protocol() {
    FrameBytes bytes{};
    FrameHeader header{MessageKind::Hello, 0};
    assert(encode_header(header, &bytes));
    assert(bytes[0] == 'A' && bytes[1] == 'Y' && bytes[2] == 'Z');
    assert(bytes[4] == 1 && bytes[5] == 1);
    FrameHeader decoded{MessageKind::Shutdown, 99};
    assert(decode_header(bytes.data(), bytes.size(), &decoded));
    assert(decoded.kind == MessageKind::Hello);
    assert(decoded.payload_length == 0);

    header = {MessageKind::Inventory, kMaxPayloadSize};
    assert(encode_header(header, &bytes));
    assert(decode_header(bytes.data(), bytes.size(), &decoded));
    assert(decoded.payload_length == kMaxPayloadSize);
    assert(!encode_header({MessageKind::Inventory, kMaxPayloadSize + 1}, &bytes));
    assert(!encode_header({MessageKind::Hello, 1}, &bytes));
    assert(!encode_header({static_cast<MessageKind>(255), 0}, &bytes));
    assert(!encode_header(header, nullptr));
    assert(!decode_header(nullptr, kFrameHeaderSize, &decoded));
    assert(!decode_header(bytes.data(), kFrameHeaderSize - 1, &decoded));
    assert(!decode_header(bytes.data(), bytes.size(), nullptr));

    auto corrupted = bytes;
    corrupted[0] = 'Q';
    assert(!decode_header(corrupted.data(), corrupted.size(), &decoded));
    corrupted = bytes;
    corrupted[4] = 2;
    assert(!decode_header(corrupted.data(), corrupted.size(), &decoded));
    corrupted = bytes;
    corrupted[7] = 1;
    assert(!decode_header(corrupted.data(), corrupted.size(), &decoded));
    corrupted = bytes;
    corrupted[8] = 0xff;
    corrupted[9] = 0xff;
    assert(!decode_header(corrupted.data(), corrupted.size(), &decoded));

    // Fuzz protocol decoding with arbitrary 12-byte frames. It must never
    // access outside the buffer or return non-enumerated message kinds.
    std::mt19937 rng(0x41595a31u);
    for (std::size_t i = 0; i < 100000; ++i) {
        FrameBytes noise{};
        for (auto& b : noise) b = static_cast<std::uint8_t>(rng());
        FrameHeader candidate{MessageKind::Hello, 0};
        if (decode_header(noise.data(), noise.size(), &candidate)) {
            FrameBytes round_trip{};
            assert(encode_header(candidate, &round_trip));
            assert(round_trip == noise);
        }
    }
}

int main() {
    verify_controller();
    verify_protocol();
    std::cout << "PASS: original AY engine control plane and IPC frame tests\n";
}
