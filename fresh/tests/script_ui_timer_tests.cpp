#include "pal4/script_ui_timer.h"

#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

struct Runtime final : pal4::IScriptUiTimerRuntime {
    std::vector<std::string> calls;
    pal4::UiFrameTimerText received{};
    std::uint8_t reference = 2u;
    bool allocate = true;

    std::uint32_t EmptyText() override {
        calls.emplace_back("empty");
        return 0xe000u;
    }
    std::uint8_t ReadReference(std::uint32_t data) override {
        Check(data == 0x9000u || data == 0xe000u,
              "source reference address");
        calls.emplace_back("read-ref");
        return reference;
    }
    void WriteReference(std::uint32_t data,
                        std::uint8_t value) override {
        Check(data == 0x9000u || data == 0xe000u,
              "shared source reference address");
        reference = value;
        calls.emplace_back("write-ref");
    }
    void SetErrorFlag(pal4::UiFrameTimerText& text,
                      std::uint8_t flag) override {
        Check(flag == 1u, "shared timer text flag");
        text.control = flag;
        calls.emplace_back("flag");
    }
    bool AllocateText(pal4::UiFrameTimerText& text,
                      std::uint32_t length,
                      std::uint8_t flag) override {
        Check(flag == 1u && length <= 5u, "timer text allocation");
        calls.emplace_back("allocate");
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
                  (source == 0x9000u || source == 0xe000u) && length <= 5u,
              "timer text copy operands");
        calls.emplace_back("copy");
    }
    void SetTextSize(pal4::UiFrameTimerText& text,
                     std::uint32_t length) override {
        text.length = length;
        calls.emplace_back("size");
    }
    std::uint32_t FrameManager() override {
        calls.emplace_back("manager");
        return 0x5000u;
    }
    std::uint32_t InitializeTimerFrame(
        std::uint32_t manager, std::uint32_t period,
        const pal4::UiFrameTimerText& text) override {
        Check(manager == 0x5000u && period == 0x12345678u,
              "timer manager and full period");
        received = text;
        calls.emplace_back("timer");
        return 0xb000u;
    }
};

std::uint32_t Run(Runtime& runtime, std::uint32_t data,
                  std::uint32_t length, std::uint32_t capacity = 8u) {
    return pal4::ScriptUiTimer::Start(
        0x12345678u, {0x1234567bu, data, length, capacity}, runtime);
}

void SharedText() {
    for (auto initial : {0u, 2u, 0xfdu}) {
        Runtime runtime;
        runtime.reference = static_cast<std::uint8_t>(initial);
        Check(Run(runtime, 0x9000u, 4u) == 0xb000u &&
                  runtime.reference == initial + 1u &&
                  runtime.received.control == 1u &&
                  runtime.received.data == 0x9000u &&
                  runtime.received.length == 4u &&
                  runtime.received.capacity == 8u,
              "shareable text forwards all fields");
        Check(runtime.calls == std::vector<std::string>{
                  "read-ref", "flag", "read-ref", "write-ref",
                  "manager", "timer"}, "reference increment precedes timer");
    }
}

void CopiedText() {
    for (auto initial : {0xfeu, 0xffu}) {
        Runtime runtime;
        runtime.reference = static_cast<std::uint8_t>(initial);
        Check(Run(runtime, 0x9000u, 5u) == 0xb000u &&
                  runtime.reference == initial &&
                  runtime.received.data == 0xa000u &&
                  runtime.received.length == 5u &&
                  runtime.received.capacity == 6u &&
                  runtime.received.control == 0x7bu,
              "reference sentinel forces copied text");
        Check(runtime.calls == std::vector<std::string>{
                  "read-ref", "allocate", "copy", "size", "manager", "timer"},
              "copy path order");
    }
}

void EmptyAndAllocationFailure() {
    Runtime empty;
    Check(Run(empty, 0x9000u, 0u) == 0xb000u &&
              empty.received.length == 0u &&
              empty.calls == std::vector<std::string>{
                  "allocate", "copy", "size", "manager", "timer"},
          "zero-length string still allocates");

    Runtime null_source;
    null_source.reference = 0xfeu;
    Check(Run(null_source, 0u, 4u) == 0xb000u &&
              null_source.calls == std::vector<std::string>{
                  "empty", "read-ref", "allocate", "empty", "copy",
                  "size", "manager", "timer"},
          "null source uses native empty text fallback");

    Runtime failed;
    failed.allocate = false;
    Check(Run(failed, 0x9000u, 0u) == 0xb000u &&
              failed.received.data == 0u && failed.received.length == 0u &&
              failed.calls == std::vector<std::string>{
                  "allocate", "manager", "timer"},
          "allocation failure still forwards empty temporary");
}

}  // namespace

int main() {
    try {
        SharedText();
        CopiedText();
        EmptyAndAllocationFailure();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
