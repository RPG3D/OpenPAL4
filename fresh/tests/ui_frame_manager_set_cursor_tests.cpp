#include "pal4/ui_frame_manager_singleton.h"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IUiFrameManagerSetCursorRuntime {
    std::vector<std::string> events;
    pal4::StringBufferState current{};
    std::uint32_t candidate = 0x7000u;
    std::int32_t comparison = 0;
    std::uint8_t reference = 0u;
    std::uint32_t cursor = 0xabcdu;
    std::string stored;
    std::uint8_t cleanup_byte = 0x63u;

    std::uint8_t SetErrorFlag(pal4::StringBufferState& text,
                              std::uint8_t flag) override {
        events.emplace_back(flag == 0u ? "init" : "cleanup");
        if (&text != &current) {
            if (flag == 0u) text = {};
        } else {
            text = {};
        }
        return cleanup_byte;
    }
    void AssignCString(pal4::StringBufferState& text,
                       const char* value) override {
        Check(std::string(value) == stored, "query string");
        events.emplace_back("assign");
        text.data = 0x1111u;
        text.length = static_cast<std::uint32_t>(std::strlen(value));
    }
    std::uint32_t TreeEnd(std::uint32_t manager) override {
        Check(manager == 0x5000u, "tree owner");
        events.emplace_back("end");
        return 0xdeadbeefu;
    }
    std::uint32_t FindNode(std::uint32_t manager,
                           const pal4::StringBufferState& name) override {
        Check(manager == 0x5000u && name.data == 0x1111u,
              "find query");
        events.emplace_back("find");
        return candidate;
    }
    std::int32_t CompareNodeName(
        const pal4::StringBufferState& name,
        std::uint32_t node_name_address) override {
        Check(name.data == 0x1111u &&
                  node_name_address == candidate + 12u,
              "compare node field");
        events.emplace_back("compare");
        return comparison;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        Check(address == candidate + 28u, "cursor node offset");
        events.emplace_back("read-cursor");
        return cursor;
    }
    void SetCursor(std::uint32_t value) override {
        Check(value == cursor, "Win32 cursor handle");
        events.emplace_back("set-cursor");
    }
    pal4::StringBufferState& CurrentName(
        std::uint32_t manager) override {
        Check(manager == 0x5000u, "current name owner");
        events.emplace_back("current");
        return current;
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        Check(address == current.data - 1u, "reference header");
        events.emplace_back("read-ref");
        return reference;
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        Check(address == current.data - 1u &&
                  value == static_cast<std::uint8_t>(reference - 1u),
              "reference decrement");
        events.emplace_back("write-ref");
        reference = value;
    }
    void Reallocate(pal4::StringBufferState& text,
                    std::uint32_t length) override {
        Check(&text == &current && length == stored.size(),
              "reallocate current name");
        events.emplace_back("reallocate");
        current.data = 0x8805u;
        current.capacity = length;
    }
    void CopyCString(pal4::StringBufferState& text,
                     const char* value,
                     std::uint32_t length) override {
        Check(&text == &current && length == stored.size() &&
                  std::string(value) == stored,
              "copy selected name");
        events.emplace_back("copy");
        current.length = length;
    }
};

void CheckNoMatch(bool candidate_missing) {
    Runtime runtime;
    runtime.stored = "talk.ani";
    runtime.candidate = candidate_missing ? 0xdeadbeefu : 0x7000u;
    runtime.comparison = -1;
    Check(pal4::UiFrameManagerSingleton::SetCursor(
              0x5000u, "talk.ani", runtime) == 0x63u,
          "missing cursor cleanup byte");
    auto expected = std::vector<std::string>{
        "init", "assign", "find", "end"};
    if (!candidate_missing) expected.emplace_back("compare");
    expected.emplace_back("cleanup");
    Check(runtime.events == expected, "no match skips Win32 SetCursor");
}

void CheckEmpty(bool shared) {
    Runtime runtime;
    runtime.stored = "";
    runtime.current.data = 0x2001u;
    runtime.reference = shared ? 2u : 0u;
    Check(pal4::UiFrameManagerSingleton::SetCursor(
              0x5000u, "", runtime) == 0x63u,
          "empty name returns current cleanup byte");
    Check(runtime.current.data == 0u &&
              runtime.reference == (shared ? 1u : 0u),
          "empty name current record cleared");
    const auto& events = runtime.events;
    Check(events[events.size() - 1u] ==
              (shared ? "init" : "cleanup") &&
              events[events.size() - 2u] ==
                  (shared ? "write-ref" : "read-ref"),
          "empty name reference branch");
}

void CheckReuseOrReallocate(std::uint8_t reference,
                            std::uint32_t capacity,
                            bool expect_reallocate) {
    Runtime runtime;
    runtime.stored = "talk.ani";
    runtime.current = {0x2001u, 4u, capacity};
    runtime.reference = reference;
    const auto expected_address =
        expect_reallocate ? 0x8805u : 0x2001u;
    Check(pal4::UiFrameManagerSingleton::SetCursor(
              0x5000u, "talk.ani", runtime) ==
              static_cast<std::uint8_t>(expected_address),
          "selected name returns data pointer low byte");
    Check(runtime.current.data == expected_address &&
              runtime.current.length == 8u,
          "selected name copied");
    const auto reallocated =
        std::count(runtime.events.begin(), runtime.events.end(),
                   "reallocate");
    Check((reallocated != 0) == expect_reallocate,
          "capacity and shared reference determine reallocation");
    Check(runtime.events[runtime.events.size() - 2u] ==
              (expect_reallocate ? "reallocate" : "read-ref") ||
              (expect_reallocate &&
               runtime.events[runtime.events.size() - 2u] == "cleanup"),
          "copy occurs after allocation decision");
}

}  // namespace

int main() {
    try {
        CheckNoMatch(true);
        CheckNoMatch(false);
        CheckEmpty(true);
        CheckEmpty(false);
        CheckReuseOrReallocate(0u, 16u, false);
        CheckReuseOrReallocate(0u, 4u, true);
        CheckReuseOrReallocate(2u, 16u, true);
        std::cout << "ui_frame_manager_set_cursor_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
