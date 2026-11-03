#include "pal4/game_manager_component_callback.h"

#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IGameManagerComponentCallbackRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::uint32_t> writes;
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        writes.push_back(address);
        words[address] = value;
    }
};

}  // namespace

int main() {
    try {
        Runtime runtime;
        Check(pal4::GameManagerComponentCallback::Construct(
                  0x1000u, runtime) == 0x1000u &&
              runtime.words.at(0x1000u) == 0x849398u &&
              runtime.words.at(0x1004u) == 0u &&
              runtime.writes ==
                  (std::vector<std::uint32_t>{0x1004u, 0x1000u}),
              "game-manager callback constructor layout");
        std::cout << "game_manager_component_callback_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
