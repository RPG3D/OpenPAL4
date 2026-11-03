#include "pal4/hint_window.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IHintWindowRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::unordered_map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> calls;
    std::vector<std::string> found_names;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> writes;
    std::string layout;
    std::string copied;
    std::uint32_t text_destination = 0u;
    std::uint32_t text_length = 0u;
    std::uint32_t freed = 0u;
    std::uint32_t destroyed = 0u;
    std::uint32_t subscribed_window = 0u;
    std::uint32_t subscribed_event = 0u;
    std::uint32_t subscribed_handler = 0u;
    std::uint32_t released = 0u;
    std::uint32_t allocation = 0xa000u;
    std::uint32_t fail_find = 0u;
    bool text_allocation = true;

    void LoadLayout(std::uint32_t window,
                    std::string_view name) override {
        Check(window == 0x1000u, "hint layout owner");
        calls.emplace_back("layout");
        layout = name;
        words[0x1024u] = 0x8000u;
        words[0x1034u] = 0x9000u;
        words[0x1038u] = 0u;
        words[0x103cu] = 31u;
        bytes[0x8fffu] = 0u;
    }
    std::uint32_t DestroyBase(std::uint32_t window) override {
        destroyed = window;
        calls.emplace_back("destroy");
        return 0xdeadu;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
        writes.emplace_back(address, value);
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        return bytes.at(address);
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        bytes[address] = value;
        calls.emplace_back("byte");
    }
    void SetErrorFlag(std::uint32_t record,
                      std::uint8_t flag) override {
        Check(record == 0x1030u && flag == 0u,
              "initial text error flag");
        calls.emplace_back("error");
    }
    bool AllocateText(std::uint32_t record,
                      std::uint32_t length,
                      std::uint8_t mode) override {
        Check(record == 0x1030u && length == 15u && mode == 1u,
              "clock text allocation arguments");
        calls.emplace_back("text_alloc");
        return text_allocation;
    }
    void CopyText(std::uint32_t destination,
                  std::string_view text) override {
        calls.emplace_back("copy");
        text_destination = destination;
        copied = text;
    }
    void SetTextSize(std::uint32_t record,
                     std::uint32_t length) override {
        Check(record == 0x1030u, "text size record");
        calls.emplace_back("size");
        text_length = length;
    }
    std::string GetNamePrefix(std::uint32_t field) override {
        Check(field == 0x1018u, "hint name source");
        return layout;
    }
    std::uint32_t FindWindow(std::string_view name) override {
        found_names.emplace_back(name);
        if (found_names.size() == fail_find)
            throw std::runtime_error("find failed");
        return 0x8100u +
               static_cast<std::uint32_t>(found_names.size()) * 0x100u;
    }
    void SetAutoBreak(std::uint32_t window,
                      bool enabled) override {
        Check(window == 0x8300u && !enabled,
              "edit box auto break disabled");
        calls.emplace_back("auto_break");
    }
    std::uint32_t Allocate(std::uint32_t bytes) override {
        Check(bytes == 12u, "z-order handler size");
        calls.emplace_back("handler_alloc");
        return allocation;
    }
    std::uint32_t Subscribe(std::uint32_t window,
                            std::uint32_t event,
                            std::uint32_t handler) override {
        calls.emplace_back("subscribe");
        subscribed_window = window;
        subscribed_event = event;
        subscribed_handler = handler;
        return 0xb000u;
    }
    void ReleaseConnection(std::uint32_t connection) override {
        calls.emplace_back("release");
        released = connection;
    }
    void Free(std::uint32_t address) override {
        calls.emplace_back("free");
        freed = address;
    }
};

void CheckConstruction() {
    Runtime runtime;
    using Hint = pal4::HintWindow;
    Check(Hint::Construct(0x1000u, "HintLayout", runtime) == 0x1000u &&
              runtime.layout == "HintLayout" &&
              runtime.words.at(0x1000u) == Hint::kVtable &&
              runtime.bytes.at(0x1040u) == 1u &&
              runtime.text_destination == 0x9000u &&
              runtime.copied == "HintWindowClock" &&
              runtime.text_length == 15u,
          "hint construction initializes layout, clock text and enabled flag");
    Check(runtime.found_names ==
              (std::vector<std::string>{"HintLayoutImgFrame",
                                        "HintLayoutEditMessage"}) &&
              runtime.words.at(0x102cu) == 0x8200u &&
              runtime.words.at(0x1028u) == 0x8300u &&
              runtime.words.at(0xa000u) == Hint::kZOrderHandlerVtable &&
              runtime.words.at(0xa004u) == Hint::kOnZOrderChanged &&
              runtime.words.at(0xa008u) == 0x1000u &&
              runtime.subscribed_window == 0x8000u &&
              runtime.subscribed_event == Hint::kZOrderChangedEvent &&
              runtime.subscribed_handler == 0xa000u &&
              runtime.released == 0xb000u,
          "controls and z-order handler use verified offsets and event");
    Check(runtime.calls ==
              (std::vector<std::string>{
                  "layout", "error", "text_alloc", "copy", "size",
                  "byte", "auto_break", "handler_alloc", "subscribe",
                  "release"}),
          "text and CEGUI operations follow constructor order");
}

void CheckAllocationFailuresAndCleanup() {
    Runtime runtime;
    runtime.text_allocation = false;
    runtime.allocation = 0u;
    (void)pal4::HintWindow::Construct(0x1000u, "Hint", runtime);
    Check(runtime.copied.empty() && runtime.text_length == 0u &&
              runtime.subscribed_handler == 0u &&
              runtime.words.at(0x1028u) == 0x8300u,
          "text and callback allocation failures preserve control setup");

    Runtime failed_lookup;
    failed_lookup.fail_find = 2u;
    bool caught = false;
    try {
        (void)pal4::HintWindow::Construct(0x1000u, "Hint", failed_lookup);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && failed_lookup.found_names.size() == 2u &&
              failed_lookup.freed == 0x8fffu &&
              failed_lookup.destroyed == 0x1000u &&
              failed_lookup.words.at(0x1034u) == 0u,
          "failed control lookup releases clock buffer and base dialog");
}

void CheckDestruction() {
    for (const auto reference : {0u, 0xffu, 1u, 0xfeu}) {
        Runtime runtime;
        runtime.words[0x1034u] = 0x9000u;
        runtime.words[0x1038u] = 15u;
        runtime.words[0x103cu] = 31u;
        runtime.bytes[0x8fffu] = static_cast<std::uint8_t>(reference);
        Check(pal4::HintWindow::Destruct(0x1000u, runtime) ==
                  0xdeadu &&
                  runtime.words.at(0x1000u) ==
                      pal4::HintWindow::kVtable &&
                  runtime.words.at(0x1034u) == 0u &&
                  runtime.words.at(0x1038u) == 0u &&
                  runtime.words.at(0x103cu) == 0u &&
                  runtime.destroyed == 0x1000u,
              "destructor resets all clock string fields and returns base result");
        if (reference == 0u || reference == 0xffu)
            Check(runtime.freed == 0x8fffu &&
                      runtime.calls ==
                          (std::vector<std::string>{"free", "destroy"}),
                  "exclusive or sentinel storage is freed before base");
        else
            Check(runtime.freed == 0u &&
                      runtime.bytes.at(0x8fffu) == reference - 1u &&
                      runtime.calls ==
                          (std::vector<std::string>{"byte", "destroy"}),
                  "shared storage decrements its reference before base");
    }

    Runtime empty;
    empty.words[0x1034u] = 0u;
    empty.words[0x1038u] = 0u;
    empty.words[0x103cu] = 0u;
    (void)pal4::HintWindow::Destruct(0x1000u, empty);
    Check(empty.freed == 0u &&
              empty.calls == (std::vector<std::string>{"destroy"}),
          "empty clock record needs no release");
}

}  // namespace

int main() {
    try {
        CheckConstruction();
        CheckAllocationFailuresAndCleanup();
        CheckDestruction();
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
