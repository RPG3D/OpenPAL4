#include "pal4/ui_exit_game.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IUiExitGameRuntime {
    std::vector<std::string> calls;
    bool throw_exit = false;

    std::uint32_t GameInstance() override {
        calls.emplace_back("instance");
        return 0x7000u;
    }
    void HandleExitCommand(std::uint32_t game) override {
        Check(game == 0x7000u, "exit handler receives PALGameIV");
        calls.emplace_back("exit");
        if (throw_exit) throw std::runtime_error("exit failed");
    }
};

}  // namespace

int main() {
    try {
        Runtime normal;
        Check(pal4::UiExitGameHandler::Handle(0x12345678u, normal) == 1u &&
                  normal.calls ==
                      std::vector<std::string>{"instance", "exit"},
              "callback ignores argument and returns one after exit");

        Runtime throwing;
        throwing.throw_exit = true;
        bool caught = false;
        try {
            (void)pal4::UiExitGameHandler::Handle(0u, throwing);
        } catch (const std::runtime_error& error) {
            caught = std::string(error.what()) == "exit failed";
        }
        Check(caught && throwing.calls ==
                            std::vector<std::string>{"instance", "exit"},
              "exit failure propagates before fixed return");
        std::cout << "ui_exit_game_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
