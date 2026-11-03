#include "pal4/actor_manager_name_property.h"

#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IActorManagerNamePropertyRuntime {
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> trace;
    bool allocation_succeeds = true;
    bool reset_on_first_detach = false;
    unsigned detaches = 0u;

    [[noreturn]] void ThrowRange() override {
        throw std::out_of_range("name property range");
    }
    std::uint8_t DetachAndReset(pal4::StringBufferState& text) override {
        trace.emplace_back("detach");
        if (++detaches == 1u && reset_on_first_detach) text = {};
        return detaches == 1u ? 0x71u : 0x72u;
    }
    bool AllocateText(pal4::StringBufferState& text, std::uint32_t length,
                      std::uint8_t mode) override {
        trace.emplace_back("allocate:" + std::to_string(length) + ":" +
                           std::to_string(mode));
        if (allocation_succeeds) {
            text.data = 0x3000u;
            text.capacity = length;
        }
        return allocation_succeeds;
    }
    void SetTextSize(pal4::StringBufferState& text,
                     std::uint32_t length) override {
        text.length = length;
        trace.emplace_back("size:" + std::to_string(length));
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        return bytes.at(address);
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        bytes[address] = value;
        trace.emplace_back("byte:" + std::to_string(address));
    }
    void Copy(std::uint32_t destination, std::uint32_t source,
              std::uint32_t count) override {
        trace.emplace_back("copy:" + std::to_string(destination) + ":" +
                           std::to_string(source) + ":" +
                           std::to_string(count));
        for (std::uint32_t i = 0; i < count; ++i)
            bytes[destination + i] = bytes.at(source + i);
    }
    void Free(std::uint32_t address) override {
        trace.emplace_back("free:" + std::to_string(address));
    }
};

void AddressAndSharedBuffer() {
    Check(pal4::ActorManagerNameProperty::Address(0x1000u) == 0x132cu,
          "getter returns actor plus 812");
    Runtime runtime;
    pal4::StringBufferState destination{0x1000u, 5u, 7u};
    const pal4::StringBufferState source{0x2081u, 3u, 31u};
    runtime.bytes[0x0fffu] = 2u;
    runtime.bytes[0x2080u] = 3u;
    const auto result = pal4::ActorManagerNameProperty::Assign(
        destination, source, runtime);
    Check(result == 0x81u && destination.data == source.data &&
              destination.length == source.length &&
              destination.capacity == source.capacity &&
              runtime.bytes.at(0x0fffu) == 1u &&
              runtime.bytes.at(0x2080u) == 4u &&
              runtime.trace == std::vector<std::string>{
                  "byte:4095", "byte:8320"},
          "share branch decrements old and increments source reference");

    Runtime unique_old;
    destination = {0x1000u, 5u, 7u};
    unique_old.bytes[0x0fffu] = 0xffu;
    unique_old.bytes[0x2080u] = 1u;
    (void)pal4::ActorManagerNameProperty::Assign(
        destination, source, unique_old);
    Check(unique_old.trace.front() == "free:4095" &&
              unique_old.bytes.at(0x2080u) == 2u,
          "unique old buffer is freed before sharing source");
}

void CopyAndEmptyBuffer() {
    Runtime runtime;
    pal4::StringBufferState destination{0x1000u, 5u, 7u};
    const pal4::StringBufferState source{0x2081u, 3u, 31u};
    runtime.bytes[0x2080u] = 0xfeu;
    runtime.bytes[0x2081u] = 'A';
    runtime.bytes[0x2082u] = 'B';
    runtime.bytes[0x2083u] = 'C';
    Check(pal4::ActorManagerNameProperty::Assign(
              destination, source, runtime) == 3u &&
              destination.data == 0x3000u && destination.length == 3u &&
              runtime.bytes.at(0x3000u) == 'A' &&
              runtime.bytes.at(0x3002u) == 'C' &&
              runtime.bytes.at(0x3003u) == 0u &&
              runtime.trace == std::vector<std::string>{
                  "allocate:3:1", "copy:12288:8321:3", "byte:12291"},
          "non-shareable source is copied and terminated");

    Runtime empty_runtime;
    destination = {};
    const pal4::StringBufferState empty{};
    Check(pal4::ActorManagerNameProperty::Assign(
              destination, empty, empty_runtime) == 0u &&
              empty_runtime.trace == std::vector<std::string>{
                  "allocate:0:1", "copy:12288:8657252:0", "byte:12288"},
          "empty source uses shared empty-text address");

    Runtime failed;
    failed.allocation_succeeds = false;
    destination = {0x1000u, 2u, 4u};
    Check(pal4::ActorManagerNameProperty::Assign(
              destination, empty, failed) == 0u &&
              destination.data == 0x1000u && destination.length == 2u &&
              failed.trace == std::vector<std::string>{"allocate:0:1"},
          "failed copy allocation preserves destination state");
}

void SelfAssignment() {
    Runtime unchanged;
    pal4::StringBufferState text{0x1000u, 3u, 7u};
    Check(pal4::ActorManagerNameProperty::Assign(
              text, text, unchanged) == 0x72u &&
              unchanged.trace == std::vector<std::string>{
                  "detach", "detach"},
          "self assignment calls detach twice when length remains intact");

    Runtime detached;
    detached.reset_on_first_detach = true;
    text = {0x1000u, 3u, 7u};
    Check(pal4::ActorManagerNameProperty::Assign(
              text, text, detached) == 0x72u &&
              text.length == 3u &&
              detached.trace == std::vector<std::string>{
                  "detach", "copy:3:0:0", "allocate:3:0", "size:3",
                  "detach"},
          "self assignment follows post-detach length arithmetic");
}

}  // namespace

int main() {
    try {
        AddressAndSharedBuffer();
        CopyAndEmptyBuffer();
        SelfAssignment();
        std::cout << "actor_manager_name_property_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
