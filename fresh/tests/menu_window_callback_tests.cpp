#include "pal4/menu_window_callback.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IMenuWindowCallbackRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> writes;
    std::uint32_t allocation = 0x4000u;
    std::uint32_t allocation_bytes = 0u;
    std::uint32_t read_count = 0u;

    std::uint32_t ReadWord(std::uint32_t address) override {
        ++read_count;
        return words.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        writes.emplace_back(address, value);
        words[address] = value;
    }
    std::uint32_t Allocate(std::uint32_t bytes) override {
        allocation_bytes = bytes;
        return allocation;
    }
};

void CheckConstruct(bool button) {
    Runtime runtime;
    const auto result = button
        ? pal4::MenuWindowCallback::ConstructButton(
              0x2000u, 0x11u, 0x22u, 0x33u, runtime)
        : pal4::MenuWindowCallback::ConstructAction(
              0x2000u, 0x11u, 0x22u, 0x33u, runtime);
    Check(result == 0x2000u &&
          runtime.writes ==
              (std::vector<std::pair<std::uint32_t, std::uint32_t>>{
                  {0x2008u, 0x11u}, {0x200cu, 0x22u},
                  {0x2010u, 0x33u},
                  {0x2000u, button
                      ? pal4::MenuWindowCallback::kButtonVtable
                      : pal4::MenuWindowCallback::kActionVtable}}),
          "constructor layout and vtable-last order");
}

void CheckClone(bool allocate) {
    Runtime runtime;
    runtime.words[0x2008u] = 0x11u;
    runtime.words[0x200cu] = 0x22u;
    runtime.words[0x2010u] = 0x33u;
    if (!allocate) runtime.allocation = 0u;
    Check(pal4::MenuWindowCallback::CloneAction(
              0x2000u, runtime) == (allocate ? 0x4000u : 0u) &&
          runtime.allocation_bytes == 24u,
          "clone allocation and return");
    if (!allocate) {
        Check(runtime.writes.empty() && runtime.read_count == 0u,
              "failed allocation leaves source unread");
        return;
    }
    Check(runtime.writes ==
              (std::vector<std::pair<std::uint32_t, std::uint32_t>>{
                  {0x4000u, pal4::MenuWindowCallback::kBaseVtable},
                  {0x4008u, 0x11u}, {0x400cu, 0x22u},
                  {0x4010u, 0x33u},
                  {0x4000u, pal4::MenuWindowCallback::kActionVtable}}) &&
          runtime.read_count == 3u,
          "clone copies fields between base and action vtables");
}

}  // namespace

int main() {
    try {
        CheckConstruct(false);
        CheckConstruct(true);
        CheckClone(false);
        CheckClone(true);
        std::cout << "menu_window_callback_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
