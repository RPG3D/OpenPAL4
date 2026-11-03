#include "pal4/game_input_dispatch.h"

#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <vector>

namespace {

class Runtime final : public pal4::IGameInputCallbackRuntime {
public:
    std::uint32_t Invoke(const std::uint32_t address) override {
        called.push_back(address);
        return address + 1u;
    }
    std::vector<std::uint32_t> called;
};

}  // namespace

int main() {
    std::array<std::uint32_t, 12> config{};
    config[8] = 0x1234u;
    config[11] = 0x5678u;
    pal4::GameInputDispatch dispatch(config);
    Runtime runtime;
    if (dispatch.HandleKeyboard(runtime) != 2u ||
        dispatch.HandleMouse(runtime) != 2u || !runtime.called.empty()) {
        std::cerr << "disabled input handler invoked a callback\n";
        return EXIT_FAILURE;
    }
    config[7] = 3u;
    if (dispatch.HandleKeyboard(runtime) != 0x1235u ||
        dispatch.HandleMouse(runtime) != 2u ||
        runtime.called != std::vector<std::uint32_t>{0x1234u}) {
        std::cerr << "keyboard gate or callback return differs\n";
        return EXIT_FAILURE;
    }
    config[10] = 1u;
    if (dispatch.HandleMouse(runtime) != 0x5679u ||
        runtime.called !=
            std::vector<std::uint32_t>{0x1234u, 0x5678u}) {
        std::cerr << "mouse gate or callback return differs\n";
        return EXIT_FAILURE;
    }
    std::cout << "game input dispatch tests passed\n";
}
