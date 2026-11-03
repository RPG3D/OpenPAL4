#include "pal4/combat_action_console_window_factory.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::ICombatActionConsoleWindowFactoryRuntime {
    std::vector<std::string> calls;
    std::uint32_t allocation = 0x5000u;
    bool throw_construct = false;

    std::uint32_t Allocate(std::uint32_t size) override {
        Check(size == 0x1e8u, "frame allocation size");
        calls.emplace_back("allocate");
        return allocation;
    }
    std::uint32_t Free(std::uint32_t address) override {
        Check(address == 0x5000u, "frame free address");
        calls.emplace_back("free");
        return 0u;
    }
    std::uint32_t ConstructFrame(std::uint32_t object,
                                 std::uint32_t name) override {
        Check(object == 0x5000u && name == 0x6000u,
              "frame constructor arguments");
        calls.emplace_back("construct");
        if (throw_construct) throw std::runtime_error("construct");
        return 0x7000u;
    }
};

}  // namespace

int main() {
    try {
        Runtime successful;
        Check(pal4::CombatActionConsoleWindowFactory::Create(
                  0x6000u, successful) == 0x7000u &&
                  successful.calls ==
                      std::vector<std::string>{"allocate", "construct"},
              "constructor return and call order");

        Runtime failed_allocation;
        failed_allocation.allocation = 0u;
        Check(pal4::CombatActionConsoleWindowFactory::Create(
                  0x6000u, failed_allocation) == 0u &&
                  failed_allocation.calls ==
                      std::vector<std::string>{"allocate"},
              "allocation failure skips constructor");

        Runtime failed_constructor;
        failed_constructor.throw_construct = true;
        bool caught = false;
        try {
            (void)pal4::CombatActionConsoleWindowFactory::Create(
                0x6000u, failed_constructor);
        } catch (const std::runtime_error&) {
            caught = true;
        }
        Check(caught && failed_constructor.calls ==
                  std::vector<std::string>{"allocate", "construct", "free"},
              "constructor exception frees allocated frame");
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
