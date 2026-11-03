#include "pal4/game_manager_render_parameters.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class Runtime final : public pal4::IGameManagerRenderParameterRuntime {
public:
    void SyncCamera(const std::uint32_t camera_record,
                    const std::uint32_t first,
                    const std::uint32_t second) override {
        Check(camera_record == 0x7000u && first == 0x11u && second == 0x22u,
              "camera sync receives record at +16 and original arguments");
        events.push_back("camera");
    }
    std::uint32_t InvokeHandler(const std::uint32_t handler,
                                const std::uint32_t slot_offset,
                                const std::uint32_t first,
                                const std::uint32_t second) override {
        Check(handler == 0x8000u && slot_offset == 0x38u &&
                  first == 0x11u && second == 0x22u,
              "handler uses +12 pointer and virtual slot +56");
        events.push_back("handler");
        return 0xfeedu;
    }
    std::vector<std::string> events;
};

pal4::ScreenEffectDispatcherState InitialState() {
    pal4::ScreenEffectDispatcherState state{};
    state.enabled = 1u;
    state.active = 2;
    state.current_key = 17;
    state.current_handler = 0x8000u;
    state.frame_state = 0x7000u;
    return state;
}

void GuardReturnsAndOrdering() {
    auto state = InitialState();
    Runtime runtime;
    state.enabled = 0u;
    Check(pal4::GameManagerRenderParameters::Update(
              state, 0x11u, 0x22u, 0xabcdu, runtime) == 0xabcdu &&
              runtime.events.empty(),
          "disabled state preserves incoming EAX without callbacks");
    state.enabled = 1u;
    state.active = -3;
    Check(pal4::GameManagerRenderParameters::Update(
              state, 0x11u, 0x22u, 0xabcdu, runtime) == 0xfffffffdu &&
              runtime.events.empty(),
          "nonpositive signed activity returns current count");
    state.active = 0;
    Check(pal4::GameManagerRenderParameters::Update(
              state, 0x11u, 0x22u, 0xabcdu, runtime) == 0u &&
              runtime.events.empty(),
          "zero activity skips camera sync");
    state.active = 2;
    state.current_key = 0;
    Check(pal4::GameManagerRenderParameters::Update(
              state, 0x11u, 0x22u, 0xabcdu, runtime) == 0u &&
              runtime.events == std::vector<std::string>{"camera"},
          "zero key returns after camera sync");
    state.current_key = 17;
    runtime.events.clear();
    Check(pal4::GameManagerRenderParameters::Update(
              state, 0x11u, 0x22u, 0xabcdu, runtime) == 0xfeedu &&
              runtime.events == std::vector<std::string>({
                  "camera", "handler"}),
          "active path syncs camera before handler forwarding");
}

void LinkableStub() {
    const auto state = InitialState();
    pal4::GameManagerRenderParameterRuntimeStub stub;
    Check(pal4::GameManagerRenderParameters::Update(
              state, 0x11u, 0x22u, 0u, stub) == 0u,
          "empty runtime links and returns handler stub result");
}

}  // namespace

int main() {
    GuardReturnsAndOrdering();
    LinkableStub();
    std::cout << "2 game manager render parameter test cases passed\n";
}
