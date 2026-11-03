#include "pal4/base_object_component.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string_view>
#include <utility>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IBaseObjectComponentRuntime {
    std::vector<std::pair<std::uint32_t, std::uint32_t>> calls;
    std::vector<char> events;
    std::uint32_t animation{0x6000u};

    void ClearAnimationFlag(std::uint32_t address,
                            std::uint32_t flag) override {
        calls.emplace_back(address, flag);
    }
    std::uint32_t FindAnimation(std::uint32_t container,
                                const char* name) override {
        Check(container == 0x7000u && std::string_view(name) == "rps",
              "component searches the rps animation");
        events.push_back('F');
        return animation;
    }
    std::uint32_t ReadDword(std::uint32_t address) override {
        Check(address == 0x60c0u,
              "component reads animation time bits at +0xc0");
        events.push_back('R');
        return 0x3f800000u;
    }
    void SetAnimationTime(std::uint32_t address,
                          std::uint32_t time_bits) override {
        Check(address == 0x5170u && time_bits == 0x3f800000u,
              "component applies stored animation time to its +0x170 field");
        events.push_back('T');
    }
    void WriteDword(std::uint32_t address,
                    std::uint32_t value) override {
        Check(address == 0x80c4u && value == 0u,
              "component clears output +0xc4 after optional time update");
        events.push_back('W');
    }
};

}  // namespace

int main() {
    pal4::BaseObjectState state{};
    Runtime runtime;
    Check(pal4::BaseObjectComponent::Initialize(
              state, 0xaaaau, 0xbbbbu, runtime) == 0u &&
              runtime.calls.empty(),
          "null component returns zero before flag operation");
    state.Write(4u, 0x5000u);
    Check(pal4::BaseObjectComponent::Initialize(
              state, 0xaaaau, 0xbbbbu, runtime) == 1u &&
              runtime.calls ==
                  std::vector<std::pair<std::uint32_t, std::uint32_t>>{
                      {0x5170u, 1u}},
          "component clears animation flag one at pointer +0x170");
    state.Write(4u, 0u);
    Check(pal4::BaseObjectComponent::Process(
              state, 0x7000u, 0x8000u, runtime) == 0u &&
              runtime.events.empty(),
          "null component skips animation and output operations");
    state.Write(4u, 0x5000u);
    Check(pal4::BaseObjectComponent::Process(
              state, 0x7000u, 0x8000u, runtime) == 1u &&
              runtime.events == std::vector<char>{'F', 'R', 'T', 'W'},
          "found animation sets time before clearing output word");
    runtime.events.clear();
    runtime.animation = 0u;
    Check(pal4::BaseObjectComponent::Process(
              state, 0x7000u, 0x8000u, runtime) == 1u &&
              runtime.events == std::vector<char>{'F', 'W'},
          "missing animation still clears output and returns one");
}
