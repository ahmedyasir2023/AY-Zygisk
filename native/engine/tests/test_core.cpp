// Copyright (c) 2026 Eng. Ahmed Yasir — MIT
#include "../core.hpp"

#include <array>
#include <cstdlib>
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

static void check(bool result, const char* expression, int line) {
    if (!result) {
        std::cerr << "FAIL line " << line << ": " << expression << "\n";
        std::abort();
    }
}
#define CHECK(expression) check(static_cast<bool>(expression), #expression, __LINE__)

static void verify_controller() {
    Controller c(2);
    CHECK(c.snapshot().state == State::Dormant);
    CHECK(!c.transition(Event::BackendAttached));
    CHECK(!c.snapshot().backend_active);

    CHECK(c.transition(Event::BeginPreflight));
    CHECK(!c.transition(Event::BackendAttached));
    CHECK(c.transition(Event::PreflightPassed));
    CHECK(c.transition(Event::BackendAttached));
    CHECK(c.snapshot().backend_active);
    CHECK(c.transition(Event::BackendLost));
    CHECK(c.snapshot().state == State::Degraded);
    CHECK(c.snapshot().consecutive_failures == 1);
    CHECK(!c.snapshot().backend_active);

    CHECK(c.transition(Event::BeginPreflight));
    CHECK(c.transition(Event::PreflightFailed));
    CHECK(c.snapshot().state == State::Disabled);
    CHECK(c.snapshot().consecutive_failures == 2);
    CHECK(!c.transition(Event::BeginPreflight));
    CHECK(!c.transition(Event::Stop));  // No silent reset of a disabled engine.
    CHECK(c.transition(Event::ResetByOperator));
    CHECK(c.snapshot().state == State::Dormant);
    CHECK(c.snapshot().consecutive_failures == 0);

    CHECK(c.transition(Event::BeginPreflight));
    CHECK(c.transition(Event::PreflightPassed));
    CHECK(c.transition(Event::BackendAttached));
    CHECK(c.transition(Event::BackendHealthy));
    CHECK(c.snapshot().consecutive_failures == 0);
    CHECK(c.transition(Event::Stop));
    CHECK(c.snapshot().state == State::Dormant);
    CHECK(!c.snapshot().backend_active);

    // A zero error budget is disallowed; clamp to one failure.
    Controller strict(0);
    CHECK(strict.transition(Event::BeginPreflight));
    CHECK(strict.transition(Event::PreflightFailed));
    CHECK(strict.snapshot().state == State::Disabled);
}

static void verify_protocol() {
    FrameBytes bytes{};
    FrameHeader header{MessageKind::Hello, 0};
    CHECK(encode_header(header, &bytes));
    CHECK(bytes[0] == 'A' && bytes[1] == 'Y' && bytes[2] == 'Z');
    CHECK(bytes[4] == 1 && bytes[5] == 1);
    FrameHeader decoded{MessageKind::Shutdown, 99};
    CHECK(decode_header(bytes.data(), bytes.size(), &decoded));
    CHECK(decoded.kind == MessageKind::Hello);
    CHECK(decoded.payload_length == 0);

    header = {MessageKind::Inventory, kMaxPayloadSize};
    CHECK(encode_header(header, &bytes));
    CHECK(decode_header(bytes.data(), bytes.size(), &decoded));
    CHECK(decoded.payload_length == kMaxPayloadSize);
    CHECK(!encode_header({MessageKind::Inventory, kMaxPayloadSize + 1}, &bytes));
    CHECK(!encode_header({MessageKind::Hello, 1}, &bytes));
    CHECK(!encode_header({static_cast<MessageKind>(255), 0}, &bytes));
    CHECK(!encode_header(header, nullptr));
    CHECK(!decode_header(nullptr, kFrameHeaderSize, &decoded));
    CHECK(!decode_header(bytes.data(), kFrameHeaderSize - 1, &decoded));
    CHECK(!decode_header(bytes.data(), bytes.size(), nullptr));

    auto corrupted = bytes;
    corrupted[0] = 'Q';
    CHECK(!decode_header(corrupted.data(), corrupted.size(), &decoded));
    corrupted = bytes;
    corrupted[4] = 2;
    CHECK(!decode_header(corrupted.data(), corrupted.size(), &decoded));
    corrupted = bytes;
    corrupted[7] = 1;
    CHECK(!decode_header(corrupted.data(), corrupted.size(), &decoded));
    corrupted = bytes;
    corrupted[8] = 0xff;
    corrupted[9] = 0xff;
    CHECK(!decode_header(corrupted.data(), corrupted.size(), &decoded));

    // Fuzz protocol decoding with arbitrary 12-byte frames. It must never
    // access outside the buffer or return non-enumerated message kinds.
    std::mt19937 rng(0x41595a31u);
    for (std::size_t i = 0; i < 100000; ++i) {
        FrameBytes noise{};
        for (auto& b : noise) b = static_cast<std::uint8_t>(rng());
        FrameHeader candidate{MessageKind::Hello, 0};
        if (decode_header(noise.data(), noise.size(), &candidate)) {
            FrameBytes round_trip{};
            CHECK(encode_header(candidate, &round_trip));
            CHECK(round_trip == noise);
        }
    }
}

int main() {
    verify_controller();
    verify_protocol();
    std::cout << "PASS: original AY engine control plane and IPC frame tests\n";
}
