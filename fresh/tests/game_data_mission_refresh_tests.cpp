#include "pal4/game_data_mission_table.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

struct Runtime final : pal4::IGameDataMissionRefreshRuntime {
    std::vector<std::string> calls;
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        calls.push_back("write " + std::to_string(address) + " " +
                        std::to_string(value));
    }
    std::uint32_t Refresh(std::uint32_t table) override {
        calls.push_back("refresh " + std::to_string(table));
        return 0xdead;
    }
};

}  // namespace

int main() {
    try {
        Runtime runtime;
        if (pal4::GameDataMissionTable::MarkAndRefresh(0x1000, runtime) !=
                0xdead ||
            runtime.calls !=
                std::vector<std::string>{"write 4184 1", "refresh 4096"})
            throw std::runtime_error(
                "Mission refresh must set byte +88 before forwarding result");
        std::cout << "game_data_mission_refresh_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
