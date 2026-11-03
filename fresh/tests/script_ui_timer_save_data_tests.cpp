#include "pal4/script_ui_timer_save_data.h"

#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

struct Runtime final : pal4::IScriptUiTimerSaveDataRuntime {
    std::vector<std::string> calls;

    std::uint32_t Game() override {
        calls.emplace_back("game");
        return 0x4000u;
    }
    std::uint32_t GameDataEntry(std::uint32_t game,
                                std::uint32_t index) override {
        if (game != 0x4000u || index != 70u)
            throw std::runtime_error("UI timer save-data entry operands");
        calls.emplace_back("entry");
        return 0xabcdef01u;
    }
};

}  // namespace

int main() {
    try {
        Runtime runtime;
        if (pal4::ScriptUiTimerSaveData::Get(runtime) != 0xabcdef01u ||
            runtime.calls != std::vector<std::string>{"game", "entry"})
            throw std::runtime_error("entry 70 return or call order");
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
