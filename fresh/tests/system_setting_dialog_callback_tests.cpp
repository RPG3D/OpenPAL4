#include "pal4/system_setting_dialog_callback.h"

#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::ISystemSettingDialogCallbackRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    std::uint32_t allocation = 0x6000u;

    std::uint32_t Allocate(const std::uint32_t bytes) override {
        Check(bytes == 24u, "clone allocation size");
        calls.emplace_back("allocate");
        return allocation;
    }
    std::uint32_t ReadWord(const std::uint32_t address) override {
        calls.emplace_back("read:" + std::to_string(address));
        return words.at(address);
    }
    void WriteWord(const std::uint32_t address,
                   const std::uint32_t value) override {
        calls.emplace_back("write:" + std::to_string(address));
        words[address] = value;
    }
};

void TestConstruct() {
    Runtime runtime;
    runtime.words[0x5004u] = 0xaaaau;
    runtime.words[0x5014u] = 0xbbbbu;
    Check(pal4::SystemSettingDialogCallback::Construct(
              0x5000u, 0x481630u, 0u, 0x8000u, runtime) == 0x5000u,
          "constructor returns same object");
    Check(runtime.words.at(0x5000u) == 0x843c08u &&
              runtime.words.at(0x5008u) == 0x481630u &&
              runtime.words.at(0x500cu) == 0u &&
              runtime.words.at(0x5010u) == 0x8000u,
          "constructor payload and vtable");
    Check(runtime.words.at(0x5004u) == 0xaaaau &&
              runtime.words.at(0x5014u) == 0xbbbbu &&
              runtime.calls == std::vector<std::string>{
                  "write:20488", "write:20492", "write:20496",
                  "write:20480"},
          "constructor preserves other fields and writes vtable last");
}

void TestClone() {
    Runtime runtime;
    runtime.words[0x5008u] = 0x1111u;
    runtime.words[0x500cu] = 0x2222u;
    runtime.words[0x5010u] = 0x3333u;
    runtime.words[0x6004u] = 0xaaaau;
    runtime.words[0x6014u] = 0xbbbbu;
    Check(pal4::SystemSettingDialogCallback::Clone(0x5000u, runtime) ==
              0x6000u,
          "clone returns allocation");
    Check(runtime.words.at(0x6000u) == 0x843c08u &&
              runtime.words.at(0x6008u) == 0x1111u &&
              runtime.words.at(0x600cu) == 0x2222u &&
              runtime.words.at(0x6010u) == 0x3333u &&
              runtime.words.at(0x6004u) == 0xaaaau &&
              runtime.words.at(0x6014u) == 0xbbbbu,
          "clone copies only payload fields");
    Check(runtime.calls == std::vector<std::string>{
              "allocate", "write:24576", "read:20488", "write:24584",
              "read:20492", "write:24588", "read:20496", "write:24592",
              "write:24576"},
          "clone installs base vtable before copying and final vtable after");
    runtime.calls.clear();
    runtime.allocation = 0u;
    Check(pal4::SystemSettingDialogCallback::Clone(0x5000u, runtime) == 0u &&
              runtime.calls == std::vector<std::string>{"allocate"},
          "allocation failure does not read source or write destination");
}

}  // namespace

int main() {
    TestConstruct();
    TestClone();
    return EXIT_SUCCESS;
}
