#include "pal4/actor_manager_effect_events.h"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

constexpr std::uint32_t kManager = 0x1000u;
constexpr std::uint32_t kPayload = 0x5000u;
constexpr std::uint32_t kBuffer = 0x6001u;

struct Runtime final : pal4::IActorManagerEffectEventRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> trace;
    std::uint32_t payload_buffer = 0u;
    std::map<std::uint32_t, std::uint32_t> string_lengths;

    std::uint32_t ReadWord(std::uint32_t address) override {
        trace.push_back("read:" + std::to_string(address));
        return words[address];
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        trace.push_back("read-byte:" + std::to_string(address));
        return bytes[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        trace.push_back("write:" + std::to_string(address) + ":" +
                        std::to_string(value));
        words[address] = value;
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        trace.push_back("write-byte:" + std::to_string(address) + ":" +
                        std::to_string(value));
        bytes[address] = value;
    }
    void Free(std::uint32_t address) override {
        trace.push_back("free:" + std::to_string(address));
    }
    std::uint32_t CombatSystem() override {
        trace.push_back("combat-system");
        return 0x3000u;
    }
    std::uint32_t EventRouter() override {
        trace.push_back("router");
        return 0x4000u;
    }
    void DispatchWord(std::uint32_t router, std::uint32_t key,
                      std::uint32_t& value, std::uint32_t mode) override {
        Check(router == 0x4000u && mode == 1u,
              "combat start dispatch arguments");
        trace.push_back("dispatch-word:" + std::to_string(key) + ":" +
                        std::to_string(value));
    }
    void DispatchAddress(std::uint32_t router, std::uint32_t key,
                         std::uint32_t payload,
                         std::uint32_t mode) override {
        Check(router == 0x4000u && mode == 1u && payload == kPayload,
              "effect end dispatch arguments");
        trace.push_back("dispatch-address:" + std::to_string(key));
        words[kPayload + 12u] = payload_buffer;
    }
    std::uint32_t TemporaryPayload() override {
        trace.push_back("temporary-payload");
        return kPayload;
    }
    std::uint32_t StringLength(std::uint32_t text) override {
        trace.push_back("string-length:" + std::to_string(text));
        return string_lengths[text];
    }
    void SetErrorFlag(std::uint32_t record,
                      std::uint8_t value) override {
        trace.push_back("set-error:" + std::to_string(record) + ":" +
                        std::to_string(value));
        words[record + 4u] = 0u;
        words[record + 8u] = 0u;
        words[record + 12u] = 0u;
    }
    void ReallocateString(std::uint32_t record,
                          std::uint32_t length) override {
        trace.push_back("reallocate:" + std::to_string(length));
        words[record + 4u] = kBuffer;
        words[record + 12u] = length;
    }
    void CopyBytes(std::uint32_t destination,
                   std::uint32_t source,
                   std::uint32_t length) override {
        trace.push_back("copy:" + std::to_string(destination) + ":" +
                        std::to_string(source) + ":" +
                        std::to_string(length));
    }
    std::uint32_t ResetArgumentPair(std::uint32_t& first,
                                    std::uint32_t& second) override {
        trace.push_back("reset-arguments:" + std::to_string(first) + ":" +
                        std::to_string(second));
        first = 0xffffffffu;
        second = 0u;
        return 0x77u;
    }
};

void MatchedPair() {
    Runtime runtime;
    runtime.words[kManager + 384u] = 0x11u;
    runtime.words[kManager + 388u] = 0x22u;
    runtime.words[pal4::ActorManagerEffectEvents::kCombatStartKey] = 0xa000u;
    runtime.words[pal4::ActorManagerEffectEvents::kEffectEndKey] = 0xa004u;
    Check(pal4::ActorManagerEffectEvents::HandleEffectEnd(
              kManager, 0x11u, 0x22u, 0u, runtime) == 0x77u &&
              runtime.words[0x3000u + 508u] == 2u &&
              std::find(runtime.trace.begin(), runtime.trace.end(),
                        "dispatch-word:40960:0") <
                  std::find(runtime.trace.begin(), runtime.trace.end(),
                            "temporary-payload") &&
              std::find(runtime.trace.begin(), runtime.trace.end(),
                        "dispatch-address:40964") <
                  std::find(runtime.trace.begin(), runtime.trace.end(),
                            "reset-arguments:17:34") &&
              runtime.trace.back() == "reset-arguments:17:34" &&
              runtime.words[kPayload] == 0xffffffffu &&
              runtime.words[kPayload + 12u] == 0u,
          "matching null third argument starts combat before effect-end event");
}

void BufferCleanup() {
    for (const auto references : {0u, 3u, 255u}) {
        Runtime runtime;
        runtime.words[kManager + 384u] = 0x99u;
        runtime.words[pal4::ActorManagerEffectEvents::kEffectEndKey] = 0xa004u;
        runtime.payload_buffer = kBuffer;
        runtime.bytes[kBuffer - 1u] =
            static_cast<std::uint8_t>(references);
        Check(pal4::ActorManagerEffectEvents::HandleEffectEnd(
                  kManager, 0x11u, 0x22u, 7u, runtime) == 0x77u &&
                  std::find(runtime.trace.begin(), runtime.trace.end(),
                            "dispatch-word:40960:0") == runtime.trace.end() &&
                  runtime.words[kPayload + 12u] == 0u &&
                  runtime.words[kPayload + 16u] == 0u &&
                  runtime.words[kPayload + 20u] == 0u,
              "effect-end cleanup clears all string-buffer fields");
        if (references == 3u) {
            Check(runtime.bytes[kBuffer - 1u] == 2u &&
                      std::find(runtime.trace.begin(), runtime.trace.end(),
                                "free:24576") == runtime.trace.end(),
                  "shared buffer decrements its reference count");
        } else {
            Check(std::find(runtime.trace.begin(), runtime.trace.end(),
                            "free:24576") != runtime.trace.end(),
                  "zero and saturated buffer counts free the allocation");
        }
    }

    Runtime third_nonzero;
    third_nonzero.words[kManager + 384u] = 0x11u;
    third_nonzero.words[kManager + 388u] = 0x22u;
    third_nonzero.words[pal4::ActorManagerEffectEvents::kEffectEndKey] =
        0xa004u;
    Check(pal4::ActorManagerEffectEvents::HandleEffectEnd(
              kManager, 0x11u, 0x22u, 1u, third_nonzero) == 0x77u &&
              third_nonzero.words[0x3000u + 508u] == 0u,
          "non-null third argument suppresses combat start after pair match");
}

void EffectMessage() {
    Runtime runtime;
    constexpr std::uint32_t kText = 0x7000u;
    runtime.string_lengths[kText] = 3u;
    runtime.payload_buffer = kBuffer;
    runtime.bytes[kBuffer - 1u] = 3u;
    runtime.words[pal4::ActorManagerEffectEvents::kEffectMessageKey] =
        0xa008u;
    Check(pal4::ActorManagerEffectEvents::HandleEffectMessage(
              0x55u, 0x66u, kText, runtime) == 0x77u &&
              runtime.words[kPayload] == 0xffffffffu &&
              runtime.words[kPayload + 4u] == 0u &&
              runtime.words[kPayload + 12u] == 0u &&
              runtime.bytes[kBuffer - 1u] == 2u &&
              std::find(runtime.trace.begin(), runtime.trace.end(),
                        "copy:24577:28672:3") <
                  std::find(runtime.trace.begin(), runtime.trace.end(),
                            "dispatch-address:40968") &&
              runtime.trace.back() == "reset-arguments:85:102",
          "effect message builds text payload before event and cleans it after");
}

}  // namespace

int main() {
    try {
        MatchedPair();
        BufferCleanup();
        EffectMessage();
        std::cout << "actor_manager_effect_events_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
