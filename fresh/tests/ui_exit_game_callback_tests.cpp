#include "pal4/ui_exit_game_callback.h"

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

struct Runtime final : pal4::IUiExitGameCallbackRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    std::uint32_t next = 0x5000u;
    std::uint32_t fail_write = 0u;

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        calls.push_back("write " + std::to_string(address) + "=" +
                        std::to_string(value));
        if (address == fail_write)
            throw std::runtime_error("field write");
        words[address] = value;
    }
    std::uint32_t AllocateMemory(std::uint32_t size) override {
        calls.push_back("allocate " + std::to_string(size));
        return next;
    }
};

void CheckConstructAndClone() {
    Runtime r;
    Check(pal4::UiExitGameCallback::Construct(
              0x4000u, 0x4bfe10u, 0x1111u, 0x2222u, r) == 0x4000u &&
              r.words.at(0x4000u) ==
                  pal4::UiExitGameCallback::kVtable &&
              r.words.at(0x4008u) == 0x4bfe10u &&
              r.words.at(0x400cu) == 0x1111u &&
              r.words.at(0x4010u) == 0x2222u,
          "constructor stores callback fields and vtable");
    Check(r.calls.size() == 4u &&
              r.calls[0].starts_with("write 16392=") &&
              r.calls[1].starts_with("write 16396=") &&
              r.calls[2].starts_with("write 16400=") &&
              r.calls[3].starts_with("write 16384="),
          "constructor writes vtable after fields");
    r.calls.clear();
    Check(pal4::UiExitGameCallback::Clone(0x4000u, r) == 0x5000u &&
              r.words.at(0x5000u) ==
                  pal4::UiExitGameCallback::kVtable &&
              r.words.at(0x5008u) == 0x4bfe10u &&
              r.words.at(0x500cu) == 0x1111u &&
              r.words.at(0x5010u) == 0x2222u,
          "clone copies three fields and installs final vtable");
    Check(r.calls.size() == 6u && r.calls[0] == "allocate 24" &&
              r.calls[1] == "write 20480=8664320" &&
              r.calls[5] == "write 20480=8668224",
          "clone writes base vtable before fields, final vtable last");
}

void CheckFailurePaths() {
    Runtime missing;
    missing.next = 0u;
    Check(pal4::UiExitGameCallback::Clone(0x4000u, missing) == 0u &&
              missing.calls == std::vector<std::string>{"allocate 24"},
          "allocation failure has no source reads or writes");

    Runtime partial;
    partial.words[0x4008u] = 1u;
    partial.words[0x400cu] = 2u;
    partial.words[0x4010u] = 3u;
    partial.fail_write = 0x500cu;
    bool threw = false;
    try {
        (void)pal4::UiExitGameCallback::Clone(0x4000u, partial);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Check(threw && partial.words.at(0x5000u) ==
                       pal4::UiExitGameCallback::kBaseVtable &&
              partial.words.at(0x5008u) == 1u &&
              partial.words.find(0x5010u) == partial.words.end(),
          "failed field write leaves partial clone before final vtable");
}

}  // namespace

int main() {
    try {
        CheckConstructAndClone();
        CheckFailurePaths();
        std::cout << "ui_exit_game_callback_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
