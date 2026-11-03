#include "pal4/property_window_callback_clone.h"

#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IPropertyWindowCallbackCloneRuntime {
    std::map<std::uint32_t, std::uint32_t> words{
        {0x1008u, 7u}, {0x100cu, 8u}, {0x1010u, 9u}};
    std::vector<std::string> calls;
    std::uint32_t allocation = 0x2000u;

    std::uint32_t Allocate(std::uint32_t bytes) override {
        Check(bytes == 0x18u, "24-byte allocation");
        calls.emplace_back("allocate");
        return allocation;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.push_back("read:" + std::to_string(address));
        return words.at(address);
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        calls.push_back("write:" + std::to_string(address) + ":" +
                        std::to_string(value));
        words[address] = value;
    }
};

void CheckClone(Runtime& runtime, std::uint32_t base,
                std::uint32_t final) {
    Check(runtime.words[0x2000u] == final &&
              runtime.words[0x2008u] == 7u &&
              runtime.words[0x200cu] == 8u &&
              runtime.words[0x2010u] == 9u,
          "cloned callback fields");
    Check(runtime.calls == std::vector<std::string>{
              "allocate", "write:8192:" + std::to_string(base),
              "read:4104", "write:8200:7",
              "read:4108", "write:8204:8",
              "read:4112", "write:8208:9",
              "write:8192:" + std::to_string(final)},
          "base vtable, field copy, final vtable order");
}

}  // namespace

int main() {
    Runtime use;
    Check(pal4::PropertyWindowCallbackClone::CloneUseItem(0x1000u, use) ==
              0x2000u, "use callback address");
    CheckClone(use, 0x843500u, 0x843a20u);

    Runtime description;
    Check(pal4::PropertyWindowCallbackClone::CloneDescription(
              0x1000u, description) == 0x2000u,
          "description callback address");
    CheckClone(description, 0x8435d0u, 0x843a6cu);

    Runtime no_memory;
    no_memory.allocation = 0u;
    Check(pal4::PropertyWindowCallbackClone::CloneUseItem(
              0x1000u, no_memory) == 0u &&
              no_memory.calls == std::vector<std::string>{"allocate"},
          "allocation failure does not read or write");
}
