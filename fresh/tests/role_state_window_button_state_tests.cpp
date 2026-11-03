#include "pal4/role_state_window_button_state.h"

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

struct Runtime final : pal4::IRoleStateWindowButtonStateRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    std::uint32_t allocation = 0x6000u;
    std::uint32_t delete_count = 0u;

    std::uint32_t Allocate(std::uint32_t bytes) override {
        Check(bytes == 0xcu, "button state allocates 12 bytes");
        calls.emplace_back("allocate");
        return allocation;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.push_back("read:" + std::to_string(address));
        return words[address];
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
        calls.push_back("write:" + std::to_string(address));
    }
    std::uint32_t DeleteObject(std::uint32_t object,
                               std::uint32_t flags) override {
        Check(object == 0x6000u && flags == 1u,
              "zero-reference object deleted with flag one");
        ++delete_count;
        calls.emplace_back("delete");
        return 0x7777u;
    }
};

void TestConstructAndCreate() {
    Runtime runtime;
    Check(pal4::RoleStateWindowButtonState::Construct(
              0x6000u, 0x1111u, 0x2222u, runtime) == 0x6000u &&
              runtime.words[0x6000u] == 0x843ac8u &&
              runtime.words[0x6004u] == 0x1111u &&
              runtime.words[0x6008u] == 0x2222u &&
              runtime.calls == std::vector<std::string>{
                  "write:24580", "write:24584", "write:24576"},
          "in-place constructor writes payload before vtable");

    Runtime creator;
    Check(pal4::RoleStateWindowButtonState::Create(
              0x5000u, 0x3333u, 0x4444u, creator) == 0x5000u &&
              creator.words[0x5000u] == 0x6000u &&
              creator.words[0x6000u] == 0x843ac8u &&
              creator.words[0x6004u] == 0x3333u &&
              creator.words[0x6008u] == 0x4444u &&
              creator.calls == std::vector<std::string>{
                  "allocate", "write:24580", "write:24584",
                  "write:24576", "write:20480"},
          "factory allocates, constructs and stores pointer in slot");

    Runtime unavailable;
    unavailable.allocation = 0u;
    unavailable.words[0x5000u] = 0xdeadbeefu;
    Check(pal4::RoleStateWindowButtonState::Create(
              0x5000u, 1u, 2u, unavailable) == 0x5000u &&
              unavailable.words[0x5000u] == 0u &&
              unavailable.calls ==
                  std::vector<std::string>{"allocate", "write:20480"},
          "allocation failure clears slot and returns slot address");
}

void TestReleaseBranches() {
    Runtime empty;
    Check(!pal4::RoleStateWindowButtonState::Release(
              0x5000u, empty).has_value() &&
              empty.calls == std::vector<std::string>{"read:20480"},
          "null slot leaves original return register unspecified");

    Runtime shared;
    shared.words[0x5000u] = 0x6000u;
    shared.words[0x6004u] = 3u;
    Check(pal4::RoleStateWindowButtonState::Release(
              0x5000u, shared) == 2u &&
              shared.words[0x6004u] == 2u &&
              shared.words[0x5000u] == 0u &&
              shared.delete_count == 0u,
          "nonfinal reference decremented and slot cleared");

    Runtime final;
    final.words[0x5000u] = 0x6000u;
    final.words[0x6004u] = 1u;
    Check(pal4::RoleStateWindowButtonState::Release(
              0x5000u, final) == 0x7777u &&
              final.words[0x6004u] == 0u &&
              final.words[0x5000u] == 0u &&
              final.delete_count == 1u &&
              final.calls[final.calls.size() - 2u] == "delete",
          "last reference calls virtual deletion before clearing slot");

    Runtime underflow;
    underflow.words[0x5000u] = 0x6000u;
    underflow.words[0x6004u] = 0u;
    Check(pal4::RoleStateWindowButtonState::Release(
              0x5000u, underflow) == 0xffffffffu &&
              underflow.delete_count == 0u,
          "original unsigned reference decrement wraps at zero");
}

}  // namespace

int main() {
    TestConstructAndCreate();
    TestReleaseBranches();
}
