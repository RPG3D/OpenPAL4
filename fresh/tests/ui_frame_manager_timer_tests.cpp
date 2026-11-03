#include "pal4/ui_frame_manager_singleton.h"

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

struct Runtime final : pal4::IUiFrameManagerTimerRuntime {
    std::map<std::uint32_t, std::uint8_t> references;
    std::vector<std::string> events;
    pal4::UiFrameTimerText captured{};
    std::uint32_t frame = 0x7000u;
    bool allocate = true;
    bool release_in_timer = false;

    std::uint32_t FindFrame(std::uint32_t manager,
                            const char* name) override {
        Check(manager == 0x5000u && std::string(name) == "portrait",
              "timer looks up the portrait frame");
        events.emplace_back("find");
        return frame;
    }
    std::uint32_t EmptyText() override {
        events.emplace_back("empty");
        return 0xe000u;
    }
    std::uint8_t ReadReference(std::uint32_t data) override {
        events.emplace_back("read ref");
        return references[data];
    }
    void WriteReference(std::uint32_t data,
                        std::uint8_t value) override {
        events.emplace_back("write ref");
        references[data] = value;
    }
    void SetErrorFlag(pal4::UiFrameTimerText& text,
                      std::uint8_t flag) override {
        Check(flag == 1u, "shared text error flag");
        events.emplace_back("flag");
        text.control = (text.control & ~0xffu) | flag;
    }
    bool AllocateText(pal4::UiFrameTimerText& text,
                      std::uint32_t length,
                      std::uint8_t flag) override {
        Check(flag == 1u, "copy text allocation flag");
        events.emplace_back("allocate");
        if (allocate) {
            text.data = 0xa000u;
            text.capacity = length + 1u;
        }
        return allocate;
    }
    void CopyText(std::uint32_t destination,
                  std::uint32_t source,
                  std::uint32_t length) override {
        Check(destination == 0xa000u &&
                  (source == 0x9000u || source == 0xe000u) &&
                  length <= 5u,
              "copied text source, destination and count");
        events.emplace_back("copy");
    }
    void SetTextSize(pal4::UiFrameTimerText& text,
                     std::uint32_t length) override {
        events.emplace_back("size");
        text.length = length;
    }
    std::uint32_t InitializeTimer(
        std::uint32_t receiver, std::uint32_t period,
        const pal4::UiFrameTimerText& text) override {
        Check(receiver == 0x7000u && period == 0x12345678u,
              "timer initializer receives frame and full period");
        events.emplace_back("timer");
        captured = text;
        if (release_in_timer && text.data != 0u)
            references[text.data] -= 1u;
        return 0xd000u;
    }
    std::uint32_t Free(std::uint32_t address) override {
        Check(address == 0x8fffu, "reference byte address freed");
        events.emplace_back("free");
        return 0xbeefu;
    }
};

std::uint32_t Run(Runtime& runtime, std::uint32_t data,
                  std::uint32_t length, std::uint32_t capacity = 6u) {
    return pal4::UiFrameManagerSingleton::InitializePortraitTimer(
        0x5000u, 0x12345678u, 0x7bu,
        data, length, capacity, runtime);
}

void CheckMissingFrame() {
    Runtime no_frame;
    no_frame.frame = 0u;
    no_frame.references[0x9000u] = 3u;
    Check(Run(no_frame, 0x9000u, 4u) == 0x8fffu &&
              no_frame.references[0x9000u] == 2u &&
              no_frame.events == std::vector<std::string>{
                  "find", "read ref", "write ref"},
          "missing portrait still releases original text once");

    Runtime null_text;
    null_text.frame = 0u;
    Check(Run(null_text, 0u, 4u) == 0u &&
              null_text.events == std::vector<std::string>{"find"},
          "missing frame with null text returns null");
}

void CheckSharedAndCopied() {
    Runtime shared;
    shared.references[0x9000u] = 3u;
    shared.release_in_timer = true;
    Check(Run(shared, 0x9000u, 4u) == 0x8fffu &&
              shared.captured.data == 0x9000u &&
              shared.captured.length == 4u &&
              shared.captured.capacity == 6u &&
              shared.captured.control == 1u &&
              shared.references[0x9000u] == 2u &&
              shared.events == std::vector<std::string>{
                  "find", "read ref", "flag", "read ref", "write ref",
                  "timer", "read ref", "write ref"},
          "shareable text is retained for the callee and released by caller");

    Runtime copied;
    copied.references[0x9000u] = 0xfeu;
    Check(Run(copied, 0x9000u, 5u) == 0x8fffu &&
              copied.captured.data == 0xa000u &&
              copied.captured.length == 5u &&
              copied.references[0x9000u] == 0xfdu &&
              copied.events == std::vector<std::string>{
                  "find", "read ref", "allocate", "copy", "size",
                  "timer", "read ref", "write ref"},
          "0xfe reference sentinel forces a copy");
}

void CheckEmptyAllocationAndFree() {
    Runtime zero_length;
    zero_length.references[0x9000u] = 1u;
    Check(Run(zero_length, 0x9000u, 0u) == 0x8fffu &&
              zero_length.captured.data == 0xa000u &&
              zero_length.captured.length == 0u &&
              zero_length.events == std::vector<std::string>{
                  "find", "allocate", "copy", "size", "timer",
                  "read ref", "write ref"},
          "zero length uses the allocation path");

    Runtime no_allocation;
    no_allocation.allocate = false;
    no_allocation.references[0x9000u] = 0u;
    Check(Run(no_allocation, 0x9000u, 0u) == 0xbeefu &&
              no_allocation.captured.data == 0u &&
              no_allocation.captured.control == 0x7bu &&
              no_allocation.events == std::vector<std::string>{
                  "find", "allocate", "timer", "read ref", "free"},
          "allocation failure still initializes timer; zero ref frees input");

    Runtime null_source;
    null_source.references[0xe000u] = 0xfeu;
    Check(Run(null_source, 0u, 4u) == 0xd000u &&
              null_source.captured.data == 0xa000u &&
              null_source.events == std::vector<std::string>{
                  "find", "empty", "read ref", "allocate", "empty",
                  "copy", "size", "timer"},
          "null source uses the empty text backing and returns timer result");

    Runtime sentinel;
    sentinel.frame = 0u;
    sentinel.references[0x9000u] = 0xffu;
    Check(Run(sentinel, 0x9000u, 1u) == 0xbeefu &&
              sentinel.events == std::vector<std::string>{
                  "find", "read ref", "free"},
          "0xff reference sentinel frees source on the missing-frame path");
}

}  // namespace

int main() {
    try {
        CheckMissingFrame();
        CheckSharedAndCopied();
        CheckEmptyAllocationAndFree();
        std::cout << "ui_frame_manager_timer_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
