#include "pal4/smith_window_dialog_callback.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::ISmithWindowDialogCallbackRuntime {
    static constexpr std::uint32_t source = 0x1000u;
    static constexpr std::uint32_t clone = 0x2000u;
    std::uint32_t allocation = clone;
    std::uint32_t allocation_size = 0u;
    std::uint32_t freed = 0u;
    std::uint32_t fail_write_at = 0u;
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;

    std::uint32_t Allocate(std::uint32_t bytes) override {
        allocation_size = bytes;
        calls.emplace_back("allocate");
        return allocation;
    }
    void Free(std::uint32_t address) override {
        freed = address;
        calls.emplace_back("free");
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read:" + std::to_string(address));
        return words.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        calls.emplace_back("write:" + std::to_string(address));
        if (address == fail_write_at) throw std::runtime_error("write failed");
        words[address] = value;
    }
};

void CheckInitialize() {
    Runtime runtime;
    Check(pal4::SmithWindowDialogCallback::Initialize(
              Runtime::source, 0x1111u, 0x2222u, 0x3333u, runtime) ==
              Runtime::source,
          "initialize returns object");
    Check(runtime.words.at(Runtime::source + 8u) == 0x1111u &&
              runtime.words.at(Runtime::source + 0xcu) == 0x2222u &&
              runtime.words.at(Runtime::source + 0x10u) == 0x3333u &&
              runtime.words.at(Runtime::source) ==
                  pal4::SmithWindowDialogCallback::kVtable,
          "initialize stores callback fields and vtable");
    Check(runtime.calls == (std::vector<std::string>{
              "write:4104", "write:4108", "write:4112", "write:4096"}),
          "initialize write order");
}

void CheckClone() {
    Runtime runtime;
    runtime.words[Runtime::source + 8u] = 0x1111u;
    runtime.words[Runtime::source + 0xcu] = 0x2222u;
    runtime.words[Runtime::source + 0x10u] = 0x3333u;
    Check(pal4::SmithWindowDialogCallback::Clone(
              Runtime::source, runtime) == Runtime::clone,
          "clone returns allocation");
    Check(runtime.allocation_size == 0x18u && runtime.freed == 0u &&
              runtime.words.at(Runtime::clone) ==
                  pal4::SmithWindowDialogCallback::kVtable &&
              runtime.words.at(Runtime::clone + 8u) == 0x1111u &&
              runtime.words.at(Runtime::clone + 0xcu) == 0x2222u &&
              runtime.words.at(Runtime::clone + 0x10u) == 0x3333u,
          "clone size and copied fields");
    Check(runtime.calls == (std::vector<std::string>{
              "allocate", "write:8192", "read:4104", "write:8200",
              "read:4108", "write:8204", "read:4112", "write:8208",
              "write:8192"}),
          "clone installs base vtable before copy and derived vtable last");
}

void CheckAllocationFailure() {
    Runtime runtime;
    runtime.allocation = 0u;
    Check(pal4::SmithWindowDialogCallback::Clone(
              Runtime::source, runtime) == 0u &&
              runtime.allocation_size == 0x18u && runtime.freed == 0u &&
              runtime.calls == (std::vector<std::string>{"allocate"}),
          "failed allocation has no other effects");
}

void CheckExceptionalCleanup() {
    Runtime runtime;
    runtime.words[Runtime::source + 8u] = 1u;
    runtime.fail_write_at = Runtime::clone + 8u;
    bool caught = false;
    try {
        (void)pal4::SmithWindowDialogCallback::Clone(
            Runtime::source, runtime);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && runtime.freed == Runtime::clone &&
              runtime.words.at(Runtime::clone) ==
                  pal4::SmithWindowDialogCallback::kBaseVtable &&
              runtime.calls.back() == "free",
          "exception releases partially initialized clone");
}

}  // namespace

int main() {
    try {
        CheckInitialize();
        CheckClone();
        CheckAllocationFailure();
        CheckExceptionalCleanup();
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
