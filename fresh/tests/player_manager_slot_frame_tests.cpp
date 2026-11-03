#include "pal4/player_manager_slot_frame.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::PlayerManagerSlotFrameRuntimeStub {
    std::uint32_t active{};
    std::vector<std::string> events;

    std::uint32_t ReadDword(std::uint32_t address) override {
        Check(address == 0x51f0u, "active flag at +0x1f0");
        events.emplace_back("read-active");
        return active;
    }
    void UpdatePosition(float elapsed, std::uint32_t mode) override {
        Check(elapsed == 0.25f && mode == 0x77u,
              "inherited position update arguments");
        events.emplace_back("position");
    }
    std::uint32_t UpdateAnimationList(std::uint32_t list,
                                      float elapsed) override {
        Check(list == 0x5aa8u && elapsed == 0.25f,
              "animation list at +0xaa8 and elapsed value");
        events.emplace_back("animation");
        return 0x9876u;
    }
};

}  // namespace

int main() {
    for (const auto active : {0u, 1u}) {
        Runtime runtime;
        runtime.active = active;
        Check(pal4::PlayerManagerSlotFrame::Update(
                  0x5000u, 0.25f, 0x77u, runtime) == 0x9876u,
              "frame update forwards animation-list result");
        Check(runtime.events == (active == 0u
                  ? std::vector<std::string>{"read-active", "animation"}
                  : std::vector<std::string>{"read-active", "position",
                                             "animation"}),
              "position update is gated; animation always runs last");
    }
    std::cout << "2 player-manager frame paths passed\n";
}
