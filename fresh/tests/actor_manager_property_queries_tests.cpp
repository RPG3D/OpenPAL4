#include "pal4/actor_manager_property_queries.h"

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

constexpr std::uint32_t actor = 0x1000u;
constexpr std::uint32_t output = 0x2081u;

struct Runtime final : pal4::IActorManagerPropertyQueryRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> trace;
    std::int32_t state = 0;
    std::uint32_t error_length = 0u;
    bool allocation_succeeds = true;

    std::int32_t ActorStateSlot20(std::uint32_t address) override {
        Check(address == actor, "state query actor");
        trace.emplace_back("state");
        return state;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        trace.emplace_back("read-word:" + std::to_string(address));
        return words.at(address);
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        trace.emplace_back("read-byte:" + std::to_string(address));
        return bytes.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
        trace.emplace_back("write-word:" + std::to_string(address));
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        bytes[address] = value;
        trace.emplace_back("write-byte:" + std::to_string(address));
    }
    void AssignText(std::uint32_t destination,
                    std::uint32_t source, std::uint32_t start,
                    std::uint32_t count) override {
        Check(destination == output && start == 0u &&
                  count == 0xffffffffu,
              "full text assignment arguments");
        trace.emplace_back("assign:" + std::to_string(source));
    }
    std::uint32_t StringLength(std::uint32_t address) override {
        Check(address == 0x8dc780u, "empty error-string source");
        trace.emplace_back("strlen");
        return error_length;
    }
    bool AllocateText(std::uint32_t address, std::uint32_t length,
                      std::uint8_t mode) override {
        Check(address == output && length == error_length && mode == 1u,
              "fallback text allocation arguments");
        trace.emplace_back("allocate");
        if (allocation_succeeds) words[output + 4u] = 0x3000u;
        return allocation_succeeds;
    }
    void Copy(std::uint32_t destination, std::uint32_t source,
              std::uint32_t length) override {
        Check(destination == 0x3000u && source == 0x8dc780u &&
                  length == error_length,
              "fallback string copy arguments");
        trace.emplace_back("copy");
    }
};

void DescriptorAndName() {
    Check(pal4::ActorManagerPropertyQueries::FixedDescriptor() == 0x844a14u,
          "fixed descriptor address");
    for (const auto state : {-2, -1, 0, 1, 5, 6, 7}) {
        Runtime runtime;
        runtime.state = state;
        const auto expected = state >= 6 ? 0x8f6140u :
            0x844a08u + static_cast<std::uint32_t>(state) * 12u;
        Check(pal4::ActorManagerPropertyQueries::StateDescriptor(
                  actor, runtime) == expected &&
                  runtime.trace == std::vector<std::string>{"state"},
              "signed state maps to 12-byte descriptor table or fallback");
    }
    for (const auto pointer : {0u, 0x4567u}) {
        Runtime runtime;
        runtime.words[actor + 988u] = pointer;
        Check(pal4::ActorManagerPropertyQueries::NameOrEmpty(
                  actor, runtime) == (pointer ? pointer : 0x841964u) &&
                  runtime.trace == std::vector<std::string>{
                      "read-word:5084"},
              "name pointer falls back to shared empty text");
    }
}

void PlayerDescriptors() {
    Check(pal4::ActorManagerPlayerPropertyQueries::FixedDescriptor() ==
              0x8449e4u,
          "player fixed descriptor address");
    for (const auto state : {-2, -1, 0, 1, 2, 3, 4}) {
        Runtime runtime;
        runtime.state = state;
        const auto expected = state >= 3 ? 0x8f94a0u :
            0x8449d8u + static_cast<std::uint32_t>(state) * 12u;
        Check(pal4::ActorManagerPlayerPropertyQueries::StateDescriptor(
                  actor, runtime) == expected &&
                  runtime.trace == std::vector<std::string>{"state"},
              "player signed state maps to descriptor or fallback");
    }
}

void EmbeddedTextVariants() {
    for (const auto selector : {0, 1}) {
        Runtime runtime;
        const auto source = actor + (selector == 0 ? 1064u : 1096u);
        runtime.bytes[source] = selector == 0 ? 0x42u : 0x73u;
        Check(pal4::ActorManagerPropertyQueries::CopyTextVariant(
                  actor, output, selector, runtime) == output &&
                  runtime.bytes.at(output) == runtime.bytes.at(source) &&
                  runtime.words.at(output + 4u) == 0u &&
                  runtime.words.at(output + 8u) == 0u &&
                  runtime.words.at(output + 12u) == 0u &&
                  runtime.trace == std::vector<std::string>{
                      "read-byte:" + std::to_string(source),
                      "write-byte:8321", "write-word:8325",
                      "write-word:8329", "write-word:8333",
                      "assign:" + std::to_string(source)},
              "selected embedded text initializes output before assign");
    }
}

void FallbackText() {
    Runtime runtime;
    runtime.error_length = 3u;
    Check(pal4::ActorManagerPropertyQueries::CopyTextVariant(
              actor, output, -1, runtime) == output &&
              runtime.bytes.at(output) == 0x81u &&
              runtime.words.at(output + 8u) == 3u &&
              runtime.bytes.at(0x3003u) == 0u &&
              runtime.trace == std::vector<std::string>{
                  "write-byte:8321", "write-word:8325",
                  "write-word:8329", "write-word:8333", "strlen",
                  "allocate", "read-word:8325", "copy",
                  "write-word:8329", "write-byte:12291"},
          "other selectors copy fallback text and terminate it");

    Runtime empty;
    Check(pal4::ActorManagerPropertyQueries::CopyTextVariant(
              actor, output, 2, empty) == output &&
              empty.words.at(output + 8u) == 0u &&
              empty.bytes.at(0x3000u) == 0u,
          "actual zero-length fallback still terminates allocated text");

    Runtime failed;
    failed.allocation_succeeds = false;
    Check(pal4::ActorManagerPropertyQueries::CopyTextVariant(
              actor, output, 3, failed) == output &&
              failed.words.at(output + 4u) == 0u &&
              failed.words.at(output + 8u) == 0u &&
              failed.trace.back() == "allocate",
          "failed fallback allocation leaves empty output record");
}

void PlayerTextVariants() {
    for (const auto selector : {0, 1}) {
        Runtime runtime;
        const auto source = actor + (selector == 0 ? 1208u : 1240u);
        runtime.bytes[source] = selector == 0 ? 0x21u : 0x62u;
        Check(pal4::ActorManagerPlayerPropertyQueries::CopyTextVariant(
                  actor, output, selector, runtime) == output &&
                  runtime.bytes.at(output) == runtime.bytes.at(source) &&
                  runtime.words.at(output + 4u) == 0u &&
                  runtime.words.at(output + 8u) == 0u &&
                  runtime.words.at(output + 12u) == 0u &&
                  runtime.trace == std::vector<std::string>{
                      "read-byte:" + std::to_string(source),
                      "write-byte:8321", "write-word:8325",
                      "write-word:8329", "write-word:8333",
                      "assign:" + std::to_string(source)},
              "player selector copies the corresponding 16-byte text record");
    }

    for (const auto selector : {-1, 2, 100}) {
        Runtime runtime;
        runtime.error_length = 4u;
        Check(pal4::ActorManagerPlayerPropertyQueries::CopyTextVariant(
                  actor, output, selector, runtime) == output &&
                  runtime.bytes.at(output) == 0x81u &&
                  runtime.words.at(output + 8u) == 4u &&
                  runtime.bytes.at(0x3004u) == 0u &&
                  runtime.trace.back() == "write-byte:12292",
              "other player selectors construct terminated fallback text");
    }

    Runtime failed;
    failed.allocation_succeeds = false;
    Check(pal4::ActorManagerPlayerPropertyQueries::CopyTextVariant(
              actor, output, 3, failed) == output &&
              failed.words.at(output + 4u) == 0u &&
              failed.words.at(output + 8u) == 0u &&
              failed.trace.back() == "allocate",
          "player fallback allocation failure leaves an empty text record");
}

}  // namespace

int main() {
    try {
        DescriptorAndName();
        PlayerDescriptors();
        EmbeddedTextVariants();
        FallbackText();
        PlayerTextVariants();
        std::cout << "actor_manager_property_queries_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
