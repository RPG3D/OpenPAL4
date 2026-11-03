#include "pal4/game_data_virtual_dispatch.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <stdexcept>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IGameDataVirtualDispatchRuntime {
    std::uint32_t target = 0;
    std::uint32_t slot = 0;
    std::uint32_t argument = 0;
    std::uint32_t Invoke(std::uint32_t value, std::uint32_t offset,
                         std::uint32_t input) override {
        target = value;
        slot = offset;
        argument = input;
        return 0xdead0000u + offset;
    }
};

}  // namespace

int main() {
    try {
        Runtime runtime;
        using Dispatcher = pal4::GameDataVirtualDispatch;
        const std::array calls{
            Dispatcher::Slot20, Dispatcher::Slot44,
            Dispatcher::Slot0, Dispatcher::Slot24};
        constexpr std::array<std::uint32_t, 4> slots{20, 44, 0, 24};
        for (std::size_t i = 0; i < calls.size(); ++i) {
            Check(calls[i](0x1111, 0x2222, runtime) ==
                      0xdead0000u + slots[i] &&
                      runtime.target == 0x2222 && runtime.slot == slots[i] &&
                      runtime.argument == 0x1111,
                  "virtual wrapper forwards exact target, slot and argument");
        }
        Check(Dispatcher::RawName(0x3000) == 0x3008 &&
                  Dispatcher::RawName(0xfffffffcu) == 4,
              "raw type name is eight bytes from type_info with x86 wrap");
        std::cout << "game_data_virtual_dispatch_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
